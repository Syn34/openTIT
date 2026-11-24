// openTIT - Built-in anim script variable types.
//
// Each variable class doubles as its own prototype: the registry keeps one
// instance per XML element name ("int", "float", "hash", "object", ...) and
// Create() builds the variables found in script data. Common attributes are
// "name" (the variable's name hash) and "val"/"Value" (the initial value).
#pragma once

#include "ape/anim/ApeAnimScript.h"
#include "ape/math/ApeMath.h"

namespace ape {

// Name hash value ("val" as a hash).
class ApeAnimScriptHashVar : public ApeAnimScriptVar {
public:
    ApeAnimScriptHashVar() = default;
    ~ApeAnimScriptHashVar() override;

    uint32_t GetHash() const override { return m_value; }
    AnimScriptVarType GetType() const override { return AnimScriptVarType::Hash; }
    ApeAnimScriptVar* Clone() const override;
    char* ToString(char* buffer) const override;
    int Unknown2C() const override;
    uint32_t GetSize() const override { return 0x18; }
    ApeAnimScriptVar* Create(xml::XmlElement* element) const override;

    void SetHash(uint32_t value) { m_value = value; }

private:
    uint32_t m_value = 0;
};

// 32-bit integer ("str" as a name hash, else "val" as an int).
class ApeAnimScriptIntVar : public ApeAnimScriptVar {
public:
    ApeAnimScriptIntVar() = default;
    ~ApeAnimScriptIntVar() override;

    bool IsNumber() const override { return true; }
    float GetFloat() const override { return (float)m_value; }
    int32_t GetInt() const override { return m_value; }
    AnimScriptVarType GetType() const override { return AnimScriptVarType::Int; }
    ApeAnimScriptVar* Clone() const override;
    char* ToString(char* buffer) const override;
    int Unknown2C() const override;
    uint32_t GetSize() const override { return 0x18; }
    ApeAnimScriptVar* Create(xml::XmlElement* element) const override;

    void SetInt(int32_t value) { m_value = value; }

private:
    int32_t m_value = 0;
};

class ApeAnimScriptFloatVar : public ApeAnimScriptVar {
public:
    ApeAnimScriptFloatVar() = default;
    ~ApeAnimScriptFloatVar() override;

    bool IsNumber() const override { return true; }
    float GetFloat() const override { return m_value; }
    int32_t GetInt() const override { return (int32_t)m_value; }
    AnimScriptVarType GetType() const override { return AnimScriptVarType::Float; }
    ApeAnimScriptVar* Clone() const override;
    char* ToString(char* buffer) const override;
    int Unknown2C() const override;
    uint32_t GetSize() const override { return 0x18; }
    ApeAnimScriptVar* Create(xml::XmlElement* element) const override;

    void SetFloat(float value) { m_value = value; }

private:
    float m_value = 0.0f;
};

// Reference to a world object. The object is looked up by name when an
// instance starts (or on first use through ApeAnimScriptInstance::ResolveObject).
class ApeAnimScriptObjectVar : public ApeAnimScriptVar {
public:
    ApeAnimScriptObjectVar() = default;
    ~ApeAnimScriptObjectVar() override;

    bool IsReference() const override { return true; }
    AnimScriptVarType GetType() const override { return AnimScriptVarType::Object; }
    ApeAnimScriptVar* Clone() const override;
    char* ToString(char* buffer) const override;
    int Unknown2C() const override;
    void OnOwnerDestroyed() override;
    uint32_t GetSize() const override { return 0x28; }
    ApeAnimScriptVar* Create(xml::XmlElement* element) const override;

    uint32_t GetObjectName() const { return m_objectName; }
    uint32_t GetDefHash() const { return m_defHash; }
    ApeRefCounted* GetObject() const { return m_object; }
    void SetObject(ApeRefCounted* object) { AssignRef(m_object, object); }
    // Takes over the referenced object and its names from `other`.
    void CopyReference(const ApeAnimScriptObjectVar& other);
    void SetNotifyObject(bool notify) { m_notifyObject = notify; }
    uint32_t GetField24() const { return m_field24; }
    void SetField24(uint32_t value) { m_field24 = value; }

private:
    uint32_t m_objectName = 0;      // "val" (hash) or int 0x38939B8F
    uint32_t m_defHash = 0;         // "def" (hash) or "defHash" (int)
    ApeRefCounted* m_object = nullptr;
    // NOTE: attribute 0x80B914A7 (default true). When set, the object is told
    // when the owning instance dies.
    bool m_notifyObject = true;
    bool m_field21 = false;
    uint32_t m_field24 = 0;          // NOTE: set by code outside this module
};

class ApeAnimScriptVec3Var : public ApeAnimScriptVar {
public:
    ApeAnimScriptVec3Var();
    ~ApeAnimScriptVec3Var() override;

    AnimScriptVarType GetType() const override { return AnimScriptVarType::Vec3; }
    ApeAnimScriptVar* Clone() const override;
    int Unknown2C() const override;
    uint32_t GetSize() const override { return 0x30; }
    ApeAnimScriptVar* Create(xml::XmlElement* element) const override;

    const Vec3& GetVec3() const { return m_value; }
    void SetVec3(const Vec3& value) { m_value = value; }

private:
    Vec3 m_value;
};

class ApeAnimScriptMatrixVar : public ApeAnimScriptVar {
public:
    ApeAnimScriptMatrixVar();
    ~ApeAnimScriptMatrixVar() override;

    AnimScriptVarType GetType() const override { return AnimScriptVarType::Matrix; }
    ApeAnimScriptVar* Clone() const override;
    int Unknown2C() const override;
    uint32_t GetSize() const override { return 0x60; }
    ApeAnimScriptVar* Create(xml::XmlElement* element) const override;

    const Matrix34& GetMatrix() const { return m_value; }
    void SetMatrix(const Matrix34& value) { m_value = value; }

private:
    Matrix34 m_value;
};

// Game pad selector used by the rumble command ("val" = pad index, -1 = any).
class ApeAnimScriptControllerVar : public ApeAnimScriptVar {
public:
    ApeAnimScriptControllerVar() = default;
    ~ApeAnimScriptControllerVar() override;

    AnimScriptVarType GetType() const override { return AnimScriptVarType::Controller; }
    ApeAnimScriptVar* Clone() const override;
    uint32_t GetSize() const override { return 0x1C; }
    ApeAnimScriptVar* Create(xml::XmlElement* element) const override;

    int32_t GetControllerIndex() const { return m_controller; }

private:
    uint32_t m_field14 = 0;     // NOTE: copied by Clone, never set from data
    int32_t m_controller = -1;
};

// Path to a GUI node: "val" holds '.' separated node names, resolved lazily.
class ApeAnimScriptGuiVar : public ApeAnimScriptVar {
public:
    ApeAnimScriptGuiVar() = default;
    explicit ApeAnimScriptGuiVar(xml::XmlElement* element);
    ~ApeAnimScriptGuiVar() override;

    AnimScriptVarType GetType() const override { return AnimScriptVarType::Gui; }
    ApeAnimScriptVar* Clone() const override;
    uint32_t GetSize() const override { return 0x2C; }
    ApeAnimScriptVar* Create(xml::XmlElement* element) const override;

    // Walks the path through the GUI hook and caches the result.
    void* Resolve();
    const std::vector<uint32_t>& GetPath() const { return m_path; }

private:
    std::vector<uint32_t> m_path;
    void* m_node = nullptr;
};

// Reference to a material by name ("val"/"Value").
class ApeAnimScriptMaterialVar : public ApeAnimScriptVar {
public:
    ApeAnimScriptMaterialVar() = default;
    ~ApeAnimScriptMaterialVar() override;

    AnimScriptVarType GetType() const override { return AnimScriptVarType::Material; }
    ApeAnimScriptVar* Clone() const override;
    uint32_t GetSize() const override { return 0x1C; }
    ApeAnimScriptVar* Create(xml::XmlElement* element) const override;

    uint32_t GetMaterialName() const { return m_material; }
    void* GetMaterial() const { return m_resolved; }
    void SetMaterial(void* material) { m_resolved = material; }

private:
    uint32_t m_material = 0;
    void* m_resolved = nullptr;  // looked up by the commands that use it
};

// Number that follows a waveform of the script clock.
class ApeAnimScriptTimeFunctionVar : public ApeAnimScriptVar {
public:
    enum class Wave : int32_t {
        Sin = 0,
        Cos = 1,
        Square = 2,
        Triangle = 3,
        Sawtooth = 4,
        Wave5 = 5,      // NOTE: name unknown (hash 0x6845C350); same shape as Sawtooth
        Const = 6,      // "Value"
        Lerp = 7,       // "Start" to "End" over "Time" seconds
        None = 8,
    };

    ApeAnimScriptTimeFunctionVar();
    explicit ApeAnimScriptTimeFunctionVar(xml::XmlElement* element);
    ~ApeAnimScriptTimeFunctionVar() override;

    bool IsNumber() const override { return true; }
    float GetFloat() const override;
    AnimScriptVarType GetType() const override { return AnimScriptVarType::TimeFunction; }
    ApeAnimScriptVar* Clone() const override;
    uint32_t GetSize() const override { return 0x44; }
    ApeAnimScriptVar* Create(xml::XmlElement* element) const override;

    // Value of the waveform at clock time `time`.
    float Evaluate(float time) const;

private:
    Wave m_wave = Wave::None;    // "type"
    float m_startClock = 0.0f;   // clock when the variable was made
    mutable float m_lastClock = 0.0f;
    float m_amplitude = 1.0f;    // "Amplitude"
    float m_frequency = 0.0f;    // "Frequency"
    float m_phase = 0.0f;        // "phase"
    float m_offset = 0.0f;       // "Offset"
    float m_value = 0.0f;        // "Value"
    float m_start = 1.0f;        // "Start"
    float m_end = 0.0f;          // "End"
    float m_time = 1.0f;         // "Time"
};

// Hooks for the variable types that point into other systems.
struct AnimScriptVarHooks {
    // GUI child node `nameHash` below `parent` (null parent = GUI root).
    void* (*findGuiNode)(uint32_t nameHash, void* parent) = nullptr;
    // An object variable's owning instance died (object variables with the
    // notify flag set).
    void (*onObjectReleasedByScript)(ApeRefCounted* object) = nullptr;
    // Debug name of a referenced object.
    const char* (*getObjectName)(ApeRefCounted* object) = nullptr;
    // Seconds on the game clock that drives time function variables.
    float (*getScriptClock)() = nullptr;
};
AnimScriptVarHooks& GetAnimScriptVarHooks();

} // namespace ape
