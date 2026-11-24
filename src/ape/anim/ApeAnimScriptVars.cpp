// openTIT - Built-in anim script variable types.
#include "ape/anim/ApeAnimScriptVars.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"

#include <cstdio>
#include <cstring>

namespace ape {

namespace {

constexpr uint32_t kAttrName = AnimHash("name");
constexpr uint32_t kAttrVal = AnimHash("val");
constexpr uint32_t kAttrValue = AnimHash("Value");
constexpr uint32_t kAttrStr = AnimHash("str");
constexpr uint32_t kAttrDef = AnimHash("def");
constexpr uint32_t kAttrDefHash = AnimHash("defHash");
// NOTE: names of these two attributes are unknown.
constexpr uint32_t kAttrObjectNameInt = 0x38939B8Fu;
constexpr uint32_t kAttrNotifyObject = 0x80B914A7u;

const Vec3 kZeroVec3 = {0.0f, 0.0f, 0.0f, 0.0f};
const Matrix34 kIdentity = {{1.0f, 0.0f, 0.0f, 0.0f},
                            {0.0f, 1.0f, 0.0f, 0.0f},
                            {0.0f, 0.0f, 1.0f, 0.0f},
                            {0.0f, 0.0f, 0.0f, 0.0f}};

AnimScriptVarHooks g_varHooks;

} // namespace

AnimScriptVarHooks& GetAnimScriptVarHooks()
{
    return g_varHooks;
}

// ---------------------------------------------------------------------------
// Hash

// @func 0x004BE9BC ApeAnimScriptHashVar::~ApeAnimScriptHashVar
// @func 0x004BE9CC ApeAnimScriptHashVar::~ApeAnimScriptHashVar (deleting)
ApeAnimScriptHashVar::~ApeAnimScriptHashVar() = default;

// @func 0x004BEA10 ApeAnimScriptHashVar::Clone
ApeAnimScriptVar* ApeAnimScriptHashVar::Clone() const
{
    auto* copy = new ApeAnimScriptHashVar();
    copy->m_value = GetHash();
    copy->m_nameHash = m_nameHash;
    return copy;
}

char* ApeAnimScriptHashVar::ToString(char* buffer) const
{
    std::sprintf(buffer, "0x%08x", m_value);
    return buffer;
}

// @func 0x004BEAAC ApeAnimScriptHashVar::Unknown2C
int ApeAnimScriptHashVar::Unknown2C() const
{
    return 0;
}

// @func 0x004BEAB4 ApeAnimScriptHashVar::Create
ApeAnimScriptVar* ApeAnimScriptHashVar::Create(xml::XmlElement* element) const
{
    auto* var = new ApeAnimScriptHashVar();
    element->GetHash(kAttrName, &var->m_nameHash, 0);
    element->GetHash(kAttrVal, &var->m_value, var->m_value);
    return var;
}

// ---------------------------------------------------------------------------
// Int

// @func 0x004BEB6C ApeAnimScriptIntVar::~ApeAnimScriptIntVar
// @func 0x004BEB7C ApeAnimScriptIntVar::~ApeAnimScriptIntVar (deleting)
ApeAnimScriptIntVar::~ApeAnimScriptIntVar() = default;

// @func 0x004BEBC0 ApeAnimScriptIntVar::Clone
ApeAnimScriptVar* ApeAnimScriptIntVar::Clone() const
{
    auto* copy = new ApeAnimScriptIntVar();
    copy->m_value = GetInt();
    copy->m_nameHash = m_nameHash;
    return copy;
}

char* ApeAnimScriptIntVar::ToString(char* buffer) const
{
    std::sprintf(buffer, "%d", m_value);
    return buffer;
}

// @func 0x004BEC5C ApeAnimScriptIntVar::Unknown2C
int ApeAnimScriptIntVar::Unknown2C() const
{
    return 0;
}

// @func 0x004BEC64 ApeAnimScriptIntVar::Create
ApeAnimScriptVar* ApeAnimScriptIntVar::Create(xml::XmlElement* element) const
{
    auto* var = new ApeAnimScriptIntVar();
    element->GetHash(kAttrName, &var->m_nameHash, 0);
    // The value may be given as a name ("str", stored as its hash) or a number.
    uint32_t hashValue = 0;
    int32_t intValue = 0;
    if (element->GetHash(kAttrStr, &hashValue, 0) == xml::kQuery_Ok)
        var->m_value = (int32_t)hashValue;
    else if (element->GetInt(kAttrVal, &intValue, 0) == xml::kQuery_Ok)
        var->m_value = intValue;
    return var;
}

// ---------------------------------------------------------------------------
// Float

// @func 0x004BED64 ApeAnimScriptFloatVar::~ApeAnimScriptFloatVar
// @func 0x004BED74 ApeAnimScriptFloatVar::~ApeAnimScriptFloatVar (deleting)
ApeAnimScriptFloatVar::~ApeAnimScriptFloatVar() = default;

// @func 0x004BEDB8 ApeAnimScriptFloatVar::Clone
ApeAnimScriptVar* ApeAnimScriptFloatVar::Clone() const
{
    auto* copy = new ApeAnimScriptFloatVar();
    copy->m_value = GetFloat();
    copy->m_nameHash = m_nameHash;
    return copy;
}

char* ApeAnimScriptFloatVar::ToString(char* buffer) const
{
    std::sprintf(buffer, "%.5f", m_value);
    return buffer;
}

// @func 0x004BEE54 ApeAnimScriptFloatVar::Unknown2C
int ApeAnimScriptFloatVar::Unknown2C() const
{
    return 0;
}

// @func 0x004BEE5C ApeAnimScriptFloatVar::Create
ApeAnimScriptVar* ApeAnimScriptFloatVar::Create(xml::XmlElement* element) const
{
    auto* var = new ApeAnimScriptFloatVar();
    element->GetHash(kAttrName, &var->m_nameHash, 0);
    element->GetFloat(kAttrVal, &var->m_value, var->m_value);
    element->GetFloat(kAttrValue, &var->m_value, var->m_value);
    return var;
}

// ---------------------------------------------------------------------------
// Object

// @func 0x004BEF40 ApeAnimScriptObjectVar::~ApeAnimScriptObjectVar
// @func 0x004BEFAC ApeAnimScriptObjectVar::~ApeAnimScriptObjectVar (deleting)
ApeAnimScriptObjectVar::~ApeAnimScriptObjectVar()
{
    if (m_object)
        m_object->Release();
}

// @func 0x004BF034 ApeAnimScriptObjectVar::Clone
ApeAnimScriptVar* ApeAnimScriptObjectVar::Clone() const
{
    auto* copy = new ApeAnimScriptObjectVar();
    copy->m_objectName = m_objectName;
    copy->m_defHash = m_defHash;
    // NOTE: the pointer is copied without taking a reference; it is replaced
    // by a counted one below when a global of the same name exists.
    copy->m_object = m_object;
    copy->m_nameHash = m_nameHash;
    copy->m_notifyObject = m_notifyObject;

    AnimScriptVarMap& globals = GlobalVars();
    if (!globals.Contains(copy->m_nameHash))
        return copy;
    ApeAnimScriptVar* global = globals.Find(copy->m_nameHash);
    if (global && global->GetType() == copy->GetType()) {
        ApeRefCounted* object = static_cast<ApeAnimScriptObjectVar*>(global)->m_object;
        if (object)
            object->AddRef();
        if (copy->m_object)
            copy->m_object->Release();
        copy->m_object = object;
    }
    return copy;
}

char* ApeAnimScriptObjectVar::ToString(char* buffer) const
{
    std::strcpy(buffer, "- empty -");
    if (m_object && g_varHooks.getObjectName)
        std::sprintf(buffer, "%s", g_varHooks.getObjectName(m_object));
    return buffer;
}

// @func 0x004BF2E8 ApeAnimScriptObjectVar::Unknown2C
int ApeAnimScriptObjectVar::Unknown2C() const
{
    return 0;
}

void ApeAnimScriptObjectVar::OnOwnerDestroyed()
{
    if (m_object && m_notifyObject && g_varHooks.onObjectReleasedByScript)
        g_varHooks.onObjectReleasedByScript(m_object);
}

// @func 0x004BF2F0 ApeAnimScriptObjectVar::Create
ApeAnimScriptVar* ApeAnimScriptObjectVar::Create(xml::XmlElement* element) const
{
    auto* var = new ApeAnimScriptObjectVar();
    element->GetHash(kAttrName, &var->m_nameHash, 0);
    if (element->GetHash(kAttrVal, &var->m_objectName, 0) != xml::kQuery_Ok) {
        int32_t value = 0;
        element->GetInt(kAttrObjectNameInt, &value, 0);
        var->m_objectName = (uint32_t)value;
    }
    if (element->GetHash(kAttrDef, &var->m_defHash, 0) != xml::kQuery_Ok) {
        int32_t value = 0;
        element->GetInt(kAttrDefHash, &value, 0);
        var->m_defHash = (uint32_t)value;
    }
    element->GetBool(kAttrNotifyObject, &var->m_notifyObject, true);
    return var;
}

void ApeAnimScriptObjectVar::CopyReference(const ApeAnimScriptObjectVar& other)
{
    m_field24 = other.m_field24;
    m_objectName = other.m_objectName;
    SetObject(other.m_object);
    m_defHash = other.m_defHash;
}

// ---------------------------------------------------------------------------
// Vec3

ApeAnimScriptVec3Var::ApeAnimScriptVec3Var() : m_value(kZeroVec3) {}

// @func 0x004BF454 ApeAnimScriptVec3Var::~ApeAnimScriptVec3Var
// @func 0x004BF464 ApeAnimScriptVec3Var::~ApeAnimScriptVec3Var (deleting)
ApeAnimScriptVec3Var::~ApeAnimScriptVec3Var() = default;

// @func 0x004BF4A8 ApeAnimScriptVec3Var::Clone
ApeAnimScriptVar* ApeAnimScriptVec3Var::Clone() const
{
    auto* copy = new ApeAnimScriptVec3Var();
    copy->m_value.x = m_value.x;
    copy->m_value.y = m_value.y;
    copy->m_value.z = m_value.z;
    copy->m_nameHash = m_nameHash;
    return copy;
}

// @func 0x004BF54C ApeAnimScriptVec3Var::Unknown2C
int ApeAnimScriptVec3Var::Unknown2C() const
{
    return 0;
}

// @func 0x004BF554 ApeAnimScriptVec3Var::Create
ApeAnimScriptVar* ApeAnimScriptVec3Var::Create(xml::XmlElement* element) const
{
    auto* var = new ApeAnimScriptVec3Var();
    // NOTE: the value is only read when the name was found.
    if (element->GetHash(kAttrName, &var->m_nameHash, 0) == xml::kQuery_Ok) {
        float value[3] = {var->m_value.x, var->m_value.y, var->m_value.z};
        element->GetVec3(kAttrVal, value, value);
        element->GetVec3(kAttrValue, value, value);
        var->m_value.x = value[0];
        var->m_value.y = value[1];
        var->m_value.z = value[2];
    }
    return var;
}

// ---------------------------------------------------------------------------
// Matrix

ApeAnimScriptMatrixVar::ApeAnimScriptMatrixVar() : m_value(kIdentity) {}

// @func 0x004BF650 ApeAnimScriptMatrixVar::~ApeAnimScriptMatrixVar
// @func 0x004BF660 ApeAnimScriptMatrixVar::~ApeAnimScriptMatrixVar (deleting)
ApeAnimScriptMatrixVar::~ApeAnimScriptMatrixVar() = default;

// @func 0x004BF6A4 ApeAnimScriptMatrixVar::Clone
ApeAnimScriptVar* ApeAnimScriptMatrixVar::Clone() const
{
    auto* copy = new ApeAnimScriptMatrixVar();
    Matrix34Copy(copy->m_value, m_value);
    copy->m_nameHash = m_nameHash;
    return copy;
}

// @func 0x004BF7D8 ApeAnimScriptMatrixVar::Unknown2C
int ApeAnimScriptMatrixVar::Unknown2C() const
{
    return 0;
}

// @func 0x004BF7E0 ApeAnimScriptMatrixVar::Create
ApeAnimScriptVar* ApeAnimScriptMatrixVar::Create(xml::XmlElement* element) const
{
    // Matrices start as identity; only the name comes from data.
    auto* var = new ApeAnimScriptMatrixVar();
    element->GetHash(kAttrName, &var->m_nameHash, 0);
    return var;
}

// ---------------------------------------------------------------------------
// Controller

// @func 0x004BF8E4 ApeAnimScriptControllerVar::~ApeAnimScriptControllerVar
// @func 0x004BF8F4 ApeAnimScriptControllerVar::~ApeAnimScriptControllerVar (deleting)
ApeAnimScriptControllerVar::~ApeAnimScriptControllerVar() = default;

// @func 0x004BF938 ApeAnimScriptControllerVar::Clone
ApeAnimScriptVar* ApeAnimScriptControllerVar::Clone() const
{
    auto* copy = new ApeAnimScriptControllerVar();
    copy->m_controller = m_controller;
    copy->m_field14 = m_field14;
    copy->m_nameHash = m_nameHash;
    return copy;
}

// @func 0x004BF9C4 ApeAnimScriptControllerVar::Create
ApeAnimScriptVar* ApeAnimScriptControllerVar::Create(xml::XmlElement* element) const
{
    auto* var = new ApeAnimScriptControllerVar();
    element->GetHash(kAttrName, &var->m_nameHash, 0);
    element->GetInt(kAttrVal, &var->m_controller, 0);
    return var;
}

// ---------------------------------------------------------------------------
// GUI

// @func 0x004BFA84 ApeAnimScriptGuiVar::ApeAnimScriptGuiVar
ApeAnimScriptGuiVar::ApeAnimScriptGuiVar(xml::XmlElement* element)
{
    element->GetHash(kAttrName, &m_nameHash, 0);
    const char* path = element->GetString("val");
    if (!path)
        return;
    // Split "a.b.c" into hashed node names.
    char token[200];
    const char* start = path;
    while (const char* dot = std::strchr(start, '.')) {
        const size_t length = (size_t)(dot - start);
        StrNCopy(token, start, length);
        token[length] = '\0';
        m_path.push_back(HashNameNoCase(token));
        start = dot + 1;
    }
    if (StrLen(start) > 0) {
        StrCopy(token, start);
        m_path.push_back(HashNameNoCase(token));
    }
}

// @func 0x004BFCC0 ApeAnimScriptGuiVar::~ApeAnimScriptGuiVar
// @func 0x004BFD3C ApeAnimScriptGuiVar::~ApeAnimScriptGuiVar (deleting)
ApeAnimScriptGuiVar::~ApeAnimScriptGuiVar()
{
    m_node = nullptr;
}

// @func 0x004BFDDC ApeAnimScriptGuiVar::Resolve
void* ApeAnimScriptGuiVar::Resolve()
{
    if (m_node)
        return m_node;
    void* node = nullptr;
    for (uint32_t nameHash : m_path) {
        node = g_varHooks.findGuiNode ? g_varHooks.findGuiNode(nameHash, node) : nullptr;
        m_node = node;
    }
    return node;
}

// @func 0x004BFE6C ApeAnimScriptGuiVar::Clone
ApeAnimScriptVar* ApeAnimScriptGuiVar::Clone() const
{
    auto* copy = new ApeAnimScriptGuiVar();
    copy->m_path = m_path;
    copy->Resolve();
    copy->m_nameHash = m_nameHash;
    return copy;
}

// @func 0x004C0008 ApeAnimScriptGuiVar::Create
ApeAnimScriptVar* ApeAnimScriptGuiVar::Create(xml::XmlElement* element) const
{
    return new ApeAnimScriptGuiVar(element);
}

// ---------------------------------------------------------------------------
// Material

// @func 0x004C0A44 ApeAnimScriptMaterialVar::~ApeAnimScriptMaterialVar
// @func 0x004C0A54 ApeAnimScriptMaterialVar::~ApeAnimScriptMaterialVar (deleting)
ApeAnimScriptMaterialVar::~ApeAnimScriptMaterialVar() = default;

// @func 0x004C0A98 ApeAnimScriptMaterialVar::Clone
ApeAnimScriptVar* ApeAnimScriptMaterialVar::Clone() const
{
    auto* copy = new ApeAnimScriptMaterialVar();
    copy->m_material = m_material;
    copy->m_resolved = m_resolved;
    copy->m_nameHash = m_nameHash;
    return copy;
}

// @func 0x004C0B24 ApeAnimScriptMaterialVar::Create
ApeAnimScriptVar* ApeAnimScriptMaterialVar::Create(xml::XmlElement* element) const
{
    auto* var = new ApeAnimScriptMaterialVar();
    element->GetHash(kAttrName, &var->m_nameHash, 0);
    if (element->GetHash(kAttrVal, &var->m_material, 0) != xml::kQuery_Ok)
        element->GetHash(kAttrValue, &var->m_material, 0);
    return var;
}

// ---------------------------------------------------------------------------
// Time function

namespace {
constexpr float kPi = 3.14159274f;
constexpr float kTwoPi = 6.28318548f;
constexpr float kHalfPi = 1.57079637f;

float ScriptClock()
{
    return g_varHooks.getScriptClock ? g_varHooks.getScriptClock() : 0.0f;
}

// Fractional part, truncating towards zero like the console code.
float Frac(float value)
{
    return value - (float)(int64_t)value;
}

// Parabolic sine approximation for x in [-pi, pi].
float FastSine(float x)
{
    const float absX = x < 0.0f ? -x : x;
    float y = x * 1.27323949f + x * -0.405284703f * absX;
    const float absY = y < 0.0f ? -y : y;
    return (y * absY - y) * 0.225f + y;
}

// Wraps x into [-pi, pi].
float WrapAngle(float x)
{
    const float bias = x < 0.0f ? -kPi : kPi;
    x += bias;
    return (x - (float)(int64_t)(x / kTwoPi) * kTwoPi) - bias;
}
} // namespace

ApeAnimScriptTimeFunctionVar::ApeAnimScriptTimeFunctionVar()
{
    m_startClock = ScriptClock();
}

// @func 0x004C0080 ApeAnimScriptTimeFunctionVar::ApeAnimScriptTimeFunctionVar
ApeAnimScriptTimeFunctionVar::ApeAnimScriptTimeFunctionVar(xml::XmlElement* element)
{
    m_startClock = ScriptClock();
    element->GetHash(kAttrName, &m_nameHash, 0);
    uint32_t type = 0;
    if (element->GetHash(AnimHash("type"), &type, 0) == xml::kQuery_Ok) {
        switch (type) {
        case AnimHash("sin"): m_wave = Wave::Sin; break;
        case AnimHash("cos"): m_wave = Wave::Cos; break;
        case AnimHash("square"): m_wave = Wave::Square; break;
        case AnimHash("triangle"): m_wave = Wave::Triangle; break;
        case AnimHash("sawtooth"): m_wave = Wave::Sawtooth; break;
        case 0x6845C350u: m_wave = Wave::Wave5; break;
        case AnimHash("Const"): m_wave = Wave::Const; break;
        case AnimHash("lerp"): m_wave = Wave::Lerp; break;
        default: break;
        }
    }
    element->GetFloat(AnimHash("Amplitude"), &m_amplitude, 1.0f);
    element->GetFloat(AnimHash("Frequency"), &m_frequency, 0.0f);
    element->GetFloat(AnimHash("phase"), &m_phase, 0.0f);
    element->GetFloat(AnimHash("Offset"), &m_offset, 0.0f);
    element->GetFloat(kAttrValue, &m_value, 0.0f);
    element->GetFloat(AnimHash("Time"), &m_time, 1.0f);
    if ((m_time < 0.0f ? -m_time : m_time) < 1.0e-5f)
        m_time = 1.0f;   // avoid dividing by zero in Lerp
    element->GetFloat(AnimHash("Start"), &m_start, 1.0f);
    element->GetFloat(AnimHash("End"), &m_end, 0.0f);
}

ApeAnimScriptTimeFunctionVar::~ApeAnimScriptTimeFunctionVar() = default;

// @func 0x004C0420 ApeAnimScriptTimeFunctionVar::Evaluate
float ApeAnimScriptTimeFunctionVar::Evaluate(float time) const
{
    switch (m_wave) {
    case Wave::Lerp: {
        float elapsed = time - m_startClock;
        if (elapsed < 0.0f)
            elapsed = m_lastClock - m_startClock;
        const float t = elapsed / m_time;
        return m_start * (1.0f - t) + m_end * t;
    }
    case Wave::Const:
        return m_value;
    case Wave::Wave5:
    case Wave::Sawtooth: {
        const float cycle = Frac(Frac(time) * m_frequency);
        return m_amplitude * (cycle * 2.0f - 1.0f) + m_offset;
    }
    case Wave::Triangle: {
        float cycle = Frac(Frac(time) * m_frequency);
        float slope = 2.0f;
        if (cycle >= 0.5f) {
            cycle -= 1.0f;
            slope = -2.0f;
        }
        return m_amplitude * (cycle * slope * 2.0f - 1.0f) + m_offset;
    }
    case Wave::Square: {
        const float cycle = Frac(Frac(time) * m_frequency);
        const float high = cycle < 0.5f ? 1.0f : 0.0f;
        return m_amplitude * (high * 2.0f - 1.0f) + m_offset;
    }
    case Wave::Cos: {
        float x = WrapAngle(m_frequency * time + m_phase) + kHalfPi;
        if (x > kPi)
            x -= kTwoPi;
        return m_amplitude * FastSine(x) + m_offset;
    }
    case Wave::Sin:
        return m_amplitude * FastSine(WrapAngle(m_frequency * time + m_phase)) + m_offset;
    default:
        return 0.0f;
    }
}

// @func 0x004C0728 ApeAnimScriptTimeFunctionVar::GetFloat
float ApeAnimScriptTimeFunctionVar::GetFloat() const
{
    m_lastClock = ScriptClock();
    return Evaluate(m_lastClock);
}

// @func 0x004C07D0 ApeAnimScriptTimeFunctionVar::Clone
ApeAnimScriptVar* ApeAnimScriptTimeFunctionVar::Clone() const
{
    auto* copy = new ApeAnimScriptTimeFunctionVar();
    copy->m_wave = m_wave;
    copy->m_nameHash = m_nameHash;
    copy->m_amplitude = m_amplitude;
    copy->m_frequency = m_frequency;
    copy->m_offset = m_offset;
    copy->m_phase = m_phase;
    copy->m_value = m_value;
    copy->m_start = m_start;
    copy->m_end = m_end;
    copy->m_time = m_time;
    // A clone starts its own clock.
    copy->m_startClock = ScriptClock();
    copy->m_lastClock = copy->m_startClock;
    return copy;
}

// @func 0x004C09D8 ApeAnimScriptTimeFunctionVar::Create
ApeAnimScriptVar* ApeAnimScriptTimeFunctionVar::Create(xml::XmlElement* element) const
{
    return new ApeAnimScriptTimeFunctionVar(element);
}

} // namespace ape
