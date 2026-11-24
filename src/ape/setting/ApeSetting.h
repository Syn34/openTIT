// openTIT - Setting system: XML driven parameter records (0x004F32F4-0x004F7C48).
//
// A *setting definition* (SettingDef) describes one kind of record ("Sly_LaserFixed",
// "EnemySetting", ...). Its parameters are static SettingParam descriptors that the
// definition registers into an ApeDTD when the manager creates it; every parameter
// occupies one or more value columns. A *setting* is one record: a value array laid
// out by the DTD, one state per column, and child settings for the definition's child
// definitions.
//
// Settings are read from "SettingFile" XML documents (shipped inside 'SETx' chunks)
// and "tag setting" documents ('GMTx' chunks). A setting may inherit the values of an
// earlier named setting ("Inherit" attribute). Named settings are registered in the
// manager by name hash, unnamed ones in a plain list. Setting *arrays* group settings
// of the element types an array type (SettingArrayDef) lists.
//
// The game derives its own manager (SAI settings, see sly/game/SAISettings.h) that
// passes the factories of its definitions to the base constructor.
#pragma once

#include "ape/level/ApeDTD.h"

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace ape {

class ApeStringPool;

namespace xml {
class XmlDocument;
class XmlElement;
}

namespace iff {
struct IffReader;
}

namespace setting {

class Setting;
class SettingDef;

// State of one value column of a setting.
enum SettingColumnState : uint32_t {
    kColumn_Unset     = 0,
    kColumn_Default   = 1,   // filled from the parameter default
    kColumn_Read      = 2,   // read from XML
    kColumn_Inherited = 3,   // copied from the inherited setting
};

// Parameter usage value passed to ApeDTD::AddAttribute. Parameters with
// kParam_HasDefault also get a (zero) default text in the DTD.
// NOTE: the console data only uses 1 and 2; the names are guesses.
enum SettingParamUsage : int32_t {
    kParam_Optional   = 1,
    kParam_HasDefault = 2,
};

// Static parameter descriptor (0x18 bytes on the console, base vtable 0x00B0DBF8).
// The single-column form covers int, hash, float, bool and string parameters,
// which share one ApplyDefault (0x007366CC).
class SettingParam {
public:
    SettingParam(const char* name, int32_t type, int32_t usage, DTDValue defaultValue = {})
        : name(name), type(type), usage(usage), defaultValue(defaultValue)
    {
    }
    // One instantiation per value type on the console; the SAI module's integer
    // ones are tagged here.
    // @func 0x00781AD0 SettingParam::~SettingParam (copy, int)
    // @func 0x00782080 SettingParam::~SettingParam (deleting, copy, int)
    // @func 0x00781AB0 SettingParam::~SettingParam (copy, int1)
    // @func 0x007820C4 SettingParam::~SettingParam (deleting, copy, int1)
    virtual ~SettingParam() = default;
    SettingParam(const SettingParam&) = delete;
    SettingParam& operator=(const SettingParam&) = delete;

    // Writes the default into a setting's value array.
    virtual void ApplyDefault(DTDValue* values) const { values[attribute->column] = defaultValue; }

    // Value column of the parameter in the definition that registered it last.
    int32_t Column() const { return attribute ? attribute->column : 0; }

    const char* name;                  // +0x04
    int32_t type;                      // +0x08 DTDAttributeType
    int32_t usage;                     // +0x0C SettingParamUsage
    DTDAttribute* attribute = nullptr; // +0x10 set by SettingDef::AddParam
    DTDValue defaultValue;             // +0x14
};

// Helpers that build the default of a single-column parameter.
inline DTDValue SettingInt(int32_t v) { DTDValue d{}; d.i = v; return d; }
inline DTDValue SettingHash(uint32_t v) { DTDValue d{}; d.u = v; return d; }
inline DTDValue SettingFloat(float v) { DTDValue d{}; d.f = v; return d; }
inline DTDValue SettingBool(bool v) { DTDValue d{}; d.b = v; return d; }
inline DTDValue SettingString(const char* v) { DTDValue d{}; d.str = v; return d; }

// Three-float parameter (0x40 bytes, vtable 0x00B0DC5C; default at +0x30).
class SettingVec3Param : public SettingParam {
public:
    SettingVec3Param(const char* name, int32_t usage, float x, float y, float z)
        : SettingParam(name, kDTD_Vector3, usage), value{ x, y, z }
    {
    }
    // 0x00736944
    void ApplyDefault(DTDValue* values) const override
    {
        for (int i = 0; i < 3; ++i)
            values[attribute->column + i].f = value[i];
    }
    float value[3];
};

// Colour parameter (vtable 0x00B21458; default bytes at +0x1C). Each channel
// is stored as an integer column.
class SettingColorParam : public SettingParam {
public:
    SettingColorParam(const char* name, int32_t usage, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        : SettingParam(name, kDTD_RGBA, usage), rgba{ r, g, b, a }
    {
    }
    // @func 0x00781AC0 SettingColorParam::~SettingColorParam
    // @func 0x00782170 SettingColorParam::~SettingColorParam (deleting)
    ~SettingColorParam() override = default;

    // @func 0x00782108 SettingColorParam::ApplyDefault
    void ApplyDefault(DTDValue* values) const override
    {
        for (int i = 0; i < 4; ++i)
            values[attribute->column + i].u = rgba[i];
    }
    uint8_t rgba[4];
};

// Called for each <association value="..."/> of a tag setting.
using SettingAssociationFn = void (*)(Setting* setting, const char* value, bool reload);

// Setting definition (0x5C bytes, vtable 0x00B23020).
class SettingDef {
public:
    SettingDef();
    virtual ~SettingDef();
    SettingDef(const SettingDef&) = delete;
    SettingDef& operator=(const SettingDef&) = delete;

    virtual const char* GetName() const = 0;
    // HashNameNoCase(GetName()) in every known definition.
    virtual uint32_t GetTypeHash() const = 0;
    // NOTE: returns 0 by default and the constant 0xC9D7446A in the character
    // and hot spot definitions; no caller is known, so the meaning is open.
    virtual uint32_t GetGroupHash() const { return 0; }
    // Registers the parameters (AddParam) once the DTD exists.
    virtual void InitParams() = 0;
    // Reads a kDTD_Custom attribute; true when a value was stored.
    virtual bool ReadCustomAttribute(xml::XmlElement* element, DTDAttribute* attribute, Setting* setting,
                                     DTDValue* values)
    {
        return false;
    }
    // Called after reading an attribute whose DTD entry has field_58 set.
    virtual void OnAttributeRead(DTDAttribute* attribute, Setting* setting, DTDValue* values, bool reload) {}
    // Allocates an empty setting of this definition.
    virtual Setting* CreateSetting(const char* name);

    // Adds a parameter to the DTD and to the parameter list.
    void AddParam(SettingParam& param);
    // Adds a child definition. NOTE: no console caller of the child list is known;
    // this is the obvious way to fill it.
    void AddChildDef(SettingDef* child) { childDefs.push_back(child); }

    ApeDTD* GetDTD() const { return dtd; }
    uint32_t ChildCount() const { return (uint32_t)childDefs.size(); }

    bool field_04 = true;                      // +0x04 NOTE: never read here
    ApeDTD* dtd = nullptr;                     // +0x08 created by the manager
    std::vector<SettingDef*> childDefs;        // +0x0C (handle list on the console)
    std::vector<SettingParam*> params;         // +0x30 (handle list on the console)
    Setting* defaultSetting = nullptr;         // +0x54 shared setting for absent elements
    SettingAssociationFn association = nullptr; // +0x58
};

// Setting (0x24 bytes, vtable 0x00B23050).
class Setting {
public:
    Setting(SettingDef* def, const char* name);
    virtual ~Setting();
    Setting(const Setting&) = delete;
    Setting& operator=(const Setting&) = delete;

    // Fills every column with its parameter default.
    virtual void ResetToDefaults();
    // Reads the attributes of `element` (may be null) and the child settings.
    // With `reload`, existing strings are overwritten in place when they fit.
    virtual bool Read(xml::XmlElement* element, bool reload);

    // Copies the read and inherited columns of `source` (same definition)
    // into this setting, recursively for the children.
    bool InheritFrom(const Setting* source);

    const DTDValue& Value(const SettingParam& param) const { return values[param.Column()]; }

    const char* name = nullptr;      // +0x04 (string pool)
    uint32_t nameHash = 0;           // +0x08
    DTDValue* values = nullptr;      // +0x0C one per DTD column
    SettingDef* def;                 // +0x10
    Setting** children = nullptr;    // +0x14 one per child definition
    uint32_t* columnState = nullptr; // +0x18 SettingColumnState per column
    uint32_t childCount = 0;         // +0x1C
    bool isChild = false;            // +0x20 owned by a parent setting
    bool registered = false;         // +0x21 added to the manager
};

// Array type: names the definitions its elements may have (the DTD's type-3
// children). NOTE: no array type is registered by the shipped game.
class SettingArrayDef {
public:
    virtual ~SettingArrayDef();
    virtual const char* GetName() const = 0;
    virtual uint32_t GetTypeHash() const = 0;
    // Adds the element types to the DTD; called once the DTD exists.
    virtual void Init() = 0;

    uint32_t elementTypeCount = 0;   // +0x04
    ApeDTD* dtd = nullptr;           // +0x08
};

// A named array of settings (0x18 bytes on the console).
struct SettingArray {
    const char* name = nullptr;              // +0x00 (string pool)
    uint32_t nameHash = 0;                   // +0x04
    std::vector<Setting*> items;             // +0x08 buffer, +0x0C capacity, +0x10 count
    SettingArrayDef* type = nullptr;         // +0x14
};

// Context of the element readers (stack struct on the console).
struct SettingReadContext {
    void* type = nullptr;      // definition or array type; NOTE: not read by the element reader
    bool reload = false;
    Setting* result = nullptr; // setting created or updated by the reader
    bool inArray = false;      // NOTE: set for array elements, never read
};

// Factory of a definition or array type.
using SettingDefFactory = SettingDef* (*)();
using SettingArrayDefFactory = SettingArrayDef* (*)();

// Setting manager (vtable 0x00B22FC8, singleton 0x00F668D8).
class SettingManager {
public:
    SettingManager(const SettingDefFactory* defs, uint32_t defCount, const SettingArrayDefFactory* arrayTypes,
                   uint32_t arrayTypeCount, int stringPoolSize, bool allocCache, bool rejectDuplicates);
    virtual ~SettingManager();
    SettingManager(const SettingManager&) = delete;
    SettingManager& operator=(const SettingManager&) = delete;

    // Deletes every setting and setting array (definitions stay).
    void Clear();
    void ResetRootNamedCount();

    // Reads a "SettingFile" document.
    void LoadSettingFile(xml::XmlDocument& document, bool reload);
    // Reads a tag setting document.
    void LoadTagSettingFile(xml::XmlDocument& document, bool reload);

    SettingDef* FindDef(uint32_t typeHash) const;
    Setting* FindSetting(uint32_t nameHash) const;
    SettingArray* FindSettingArray(uint32_t nameHash) const;

    // Creates a setting with default values and default children.
    Setting* CreateSettingTree(const char* name, SettingDef* def);
    // Returns the definition's shared default setting, creating it on first use.
    Setting* GetDefaultSetting(SettingDef* def);
    // Creates a setting for `element`; null elements yield the shared default.
    Setting* CreateSettingFromXml(const char* name, SettingDef* def, xml::XmlElement* element);
    // Applies the "Inherit" attribute of `element` (and of its children).
    bool ApplyInherit(xml::XmlElement* element, Setting* setting);
    // Adds a setting (and its children) to the named map or the unnamed list.
    void Register(Setting* setting);
    // Deletes a setting array and the settings in it.
    void RemoveSettingArray(const char* name);

    // Element readers.
    bool ReadSettingElement(xml::XmlElement* element, SettingReadContext& ctx);
    bool ReadSettingArray(xml::XmlElement* element, SettingReadContext& ctx);
    bool ReadTagSetting(xml::XmlElement* element, SettingReadContext& ctx);
    bool ReadTagAssociations(xml::XmlElement* element, SettingReadContext& ctx);
    // Reads the attributes of a tag setting element (no child settings).
    bool ReadTagAttributes(Setting* setting, xml::XmlElement* element);

    ApeStringPool* StringPool() const { return m_strings.get(); }
    uint32_t RootNamedCount() const { return m_rootNamedCount; }

private:
    void RegisterDefs(const SettingDefFactory* factories, uint32_t count);
    void RegisterArrayTypes(const SettingArrayDefFactory* factories, uint32_t count);
    void DeleteSettingArrays();

    std::unique_ptr<ApeStringPool> m_strings;                // +0x04
    std::multimap<uint32_t, SettingDef*> m_defs;             // +0x08 by type hash
    std::multimap<uint32_t, Setting*> m_named;               // +0x20 by name hash
    std::multimap<uint32_t, SettingArrayDef*> m_arrayTypes;  // +0x38 by type hash
    std::multimap<uint32_t, SettingArray*> m_arrays;         // +0x50 by name hash
    std::vector<Setting*> m_unnamed;                         // +0x68 (handle list, 1500 nodes)
    uint32_t m_rootNamedCount = 0;                           // +0x8C named settings that are not children
    bool m_rejectDuplicates;                                 // +0x90
    std::unique_ptr<uint8_t[]> m_cache;                      // +0x94 32 x 0x30 bytes; NOTE: unused here
    uint32_t field_98 = 0;                                   // +0x98
};

// The active manager (0x00F668D8).
extern SettingManager* g_settingManager;

// IFF handlers of the 'SETF/SETH/SETD' and 'GMTF/GMTH/GMTD' chunks.
int SettingFileChunk(iff::IffReader* reader, void* arg);
int SettingFileHeaderChunk(iff::IffReader* reader, void* arg);
int SettingFileDataChunk(iff::IffReader* reader, void* arg);
int TagSettingFileChunk(iff::IffReader* reader, void* arg);
int TagSettingFileHeaderChunk(iff::IffReader* reader, void* arg);
int TagSettingFileDataChunk(iff::IffReader* reader, void* arg);

} // namespace setting
} // namespace ape
