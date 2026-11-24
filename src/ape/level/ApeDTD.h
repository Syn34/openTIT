// openTIT - ApeDTD: data type definitions (attribute schemas).
//
// A DTD describes the attributes an XML-driven data record may carry: each
// attribute has a name, a value type, a default value and the column (slot)
// it occupies in the record's value array. The knowledge (AI) system, the
// anim script system and the setting system build one DTD per record type
// and then read records against it.
//
// Vector and colour attributes span several consecutive columns, every other
// type occupies one.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ape {

enum DTDAttributeType : int32_t {
    kDTD_Int     = 0,
    kDTD_Int1    = 1,   // NOTE: read exactly like kDTD_Int; exact meaning unknown
    kDTD_Hash    = 2,   // name hash of a string value
    kDTD_Float   = 3,
    kDTD_Bool    = 4,
    kDTD_String  = 5,
    kDTD_Vector2 = 6,
    kDTD_Vector3 = 7,
    kDTD_RGBA    = 8,
    kDTD_Custom  = 9,
};

// Number of record columns an attribute of the given type occupies.
int DTDColumnCount(int32_t type);

union DTDValue {
    int32_t i;
    uint32_t u;
    float f;
    bool b;
    const char* str;
};

struct DTDAttribute {
    char name[0x33] = {};          // up to 50 characters
    int32_t type = kDTD_Int;
    int32_t field_3C = 0;          // NOTE: callers pass 0 or 1; meaning unknown
    int32_t column = 0;
    // Names of the values of an enumerated attribute; at most enumCapacity.
    std::vector<std::string> enumNames;
    uint32_t enumCapacity = 0;
    DTDValue defaultValue{};       // for kDTD_String points into defaultString
    std::string defaultString;     // default rendered as text (empty if none)
    bool hasDefaultString = false;
    uint32_t nameHash = 0;
    uint8_t field_58 = 0;
    uint32_t field_5C = 0;

    // Reserves room for 'count' enumeration value names (drops any old ones).
    void AllocEnumNames(uint32_t count);
    // Appends a value name (truncated to 99 characters) if there is room.
    void AddEnumName(const char* name);
};

// A nested element type allowed inside records of this DTD.
struct DTDChild {
    char name[0x33] = {};
    int32_t type = 0;              // NOTE: 3 is kept in a list of its own; meaning unknown
};

class ApeDTD {
public:
    // A zero attribute capacity selects 2. Child lists are only created up
    // front when childCapacity is non-zero.
    ApeDTD(const char* name, uint32_t attributeCapacity, uint32_t childCapacity);
    virtual ~ApeDTD();
    ApeDTD(const ApeDTD&) = delete;
    ApeDTD& operator=(const ApeDTD&) = delete;

    // Adds an attribute. column -1 means "next free column". When given,
    // 'defaultValue' points at a value of the attribute's type (an int32,
    // uint32, float, bool or const char*).
    DTDAttribute* AddAttribute(const char* name, int32_t type, int32_t field3C, int32_t column = -1,
                               const void* defaultValue = nullptr);
    // Adds an allowed child element.
    bool AddChild(const char* name, int32_t type);

    const char* GetName() const { return m_name; }
    // Prefix of the names generated for records of this DTD (+0x2C); also
    // sets flag 0x80. Used by the setting definitions ("SLaserFixed_", ...).
    void SetNamePrefix(const char* prefix)
    {
        m_namePrefix = prefix;
        m_flags |= 0x80;
    }
    const char* GetNamePrefix() const { return m_namePrefix; }
    uint32_t GetElementCount() const { return m_elementCount; }
    int32_t GetColumnCount() const { return m_columnCount; }
    const std::vector<std::unique_ptr<DTDAttribute>>& GetAttributes() const { return m_attributes; }
    const std::vector<DTDChild>* GetChildren() const { return m_children.get(); }
    const std::vector<DTDChild>* GetType3Children() const { return m_type3Children.get(); }

private:
    uint32_t m_elementCount = 0;                         // +0x04 children added
    std::vector<std::unique_ptr<DTDAttribute>> m_attributes; // +0x08
    std::unique_ptr<std::vector<DTDChild>> m_children;       // +0x1C
    std::unique_ptr<std::vector<DTDChild>> m_type3Children;  // +0x20
    const char* m_name = nullptr;                        // +0x24 not copied
    uint8_t m_flags = 0;                                 // +0x28 top two bits cleared by the ctor
    const char* m_namePrefix = nullptr;                  // +0x2C
    int32_t m_columnCount = 0;                           // +0x30
};

} // namespace ape
