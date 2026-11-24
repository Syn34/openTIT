// openTIT - Tuning variables.
#include "ape/tuning/ApeTuning.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"

#include <cstring>

namespace ape {

namespace {
// Fixed-capacity registry in registration order. Constant-initialised so it
// is usable from static constructors in any translation unit.
constexpr int kMaxTuningVars = 2800;
TuningVar* g_tuningVars[kMaxTuningVars];
int g_tuningVarCount;

const uint32_t kHash_Tuning = 0x942972B5;  // HashNameNoCase("Tuning")
const uint32_t kHash_Var    = 0x00014B07;  // HashNameNoCase("Var")
const uint32_t kHash_Name   = 0x0024728B;  // HashNameNoCase("name")
const uint32_t kHash_Value  = 0x04DA9951;  // HashNameNoCase("value")
}

// @func 0x004FBFA8 TuningVar::TuningVar
TuningVar::TuningVar(const char* name)
{
    StrCopy(m_name, name);
    m_nameHash = HashNameNoCase(name);
    if (g_tuningVarCount < kMaxTuningVars)
        g_tuningVars[g_tuningVarCount++] = this;
}

// @func 0x004FC028 TuningVar::~TuningVar
// @func 0x004FC0CC TuningVar::~TuningVar (deleting)
// @func 0x004FC1A0 TuningVar::~TuningVar (base-object copy used by subclasses)
TuningVar::~TuningVar()
{
    for (int i = 0; i < g_tuningVarCount; ++i) {
        if (g_tuningVars[i] != this)
            continue;
        // Keep registration order: close the gap.
        for (int k = i; k < g_tuningVarCount - 1; ++k)
            g_tuningVars[k] = g_tuningVars[k + 1];
        g_tuningVars[--g_tuningVarCount] = nullptr;
        return;
    }
}

// Default setters ignore values of a type the variable cannot represent.
// @func 0x00735F90 TuningVar::SetBool
void TuningVar::SetBool(bool) {}
// @func 0x00735F94 TuningVar::SetInt
void TuningVar::SetInt(int32_t) {}
// @func 0x00735F98 TuningVar::SetFloat
void TuningVar::SetFloat(float) {}
// @func 0x00735F9C TuningVar::SetVec2
void TuningVar::SetVec2(const float*) {}
// @func 0x00735FA0 TuningVar::SetVec3
void TuningVar::SetVec3(const float*) {}
// @func 0x00735FA4 TuningVar::SetColor
void TuningVar::SetColor(const xml::Color8*) {}
// @func 0x00735FA8 TuningVar::SetString
void TuningVar::SetString(const char*) {}
// @func 0x00735FAC TuningVar::SetHash
void TuningVar::SetHash(uint32_t) {}

// -- Bool --------------------------------------------------------------------
// @func 0x00732D98 TuningBool::~TuningBool
// @func 0x00736050 TuningBool::~TuningBool (deleting)
TuningBool::~TuningBool() = default;
// @func 0x00735FFC TuningBool::GetType
TuningVar::Type TuningBool::GetType() const { return kBool; }
// @func 0x00736004 TuningBool::SetBool
void TuningBool::SetBool(bool value) { m_value = value; }
// @func 0x0073600C TuningBool::SetInt
void TuningBool::SetInt(int32_t value) { m_value = value != 0; }
// @func 0x00736028 TuningBool::SetFloat
void TuningBool::SetFloat(float value) { m_value = value != 0.0f; }

// -- Int ---------------------------------------------------------------------
// @func 0x00732EC8 TuningInt::~TuningInt
// @func 0x00736114 TuningInt::~TuningInt (deleting)
TuningInt::~TuningInt() = default;
// @func 0x007360F4 TuningInt::GetType
TuningVar::Type TuningInt::GetType() const { return kInt; }
// @func 0x007360FC TuningInt::SetInt
void TuningInt::SetInt(int32_t value) { m_value = value; }
// @func 0x00736104 TuningInt::SetFloat
void TuningInt::SetFloat(float value) { m_value = (int32_t)value; }

// -- Float -------------------------------------------------------------------
// @func 0x00732E08 TuningFloat::~TuningFloat
// @func 0x007361EC TuningFloat::~TuningFloat (deleting)
TuningFloat::~TuningFloat() = default;
// @func 0x007361B8 TuningFloat::GetType
TuningVar::Type TuningFloat::GetType() const { return kFloat; }
// @func 0x007361C0 TuningFloat::SetInt
void TuningFloat::SetInt(int32_t value) { m_value = (float)value; }
// @func 0x007361E4 TuningFloat::SetFloat
void TuningFloat::SetFloat(float value) { m_value = value; }

// -- Vec2 --------------------------------------------------------------------
// @func 0x007489F0 TuningVec2::~TuningVec2
// @func 0x0074900C TuningVec2::~TuningVec2 (deleting)
TuningVec2::~TuningVec2() = default;
// @func 0x00748FF0 TuningVec2::GetType
TuningVar::Type TuningVec2::GetType() const { return kVec2; }
// @func 0x00748FF8 TuningVec2::SetVec2
void TuningVec2::SetVec2(const float value[2])
{
    m_value[0] = value[0];
    m_value[1] = value[1];
}

// -- Vec3 --------------------------------------------------------------------
// @func 0x00732E38 TuningVec3::~TuningVec3
// @func 0x007362C8 TuningVec3::~TuningVec3 (deleting)
TuningVec3::~TuningVec3() = default;
// @func 0x007362A4 TuningVec3::GetType
TuningVar::Type TuningVec3::GetType() const { return kVec3; }
// @func 0x007362AC TuningVec3::SetVec3
void TuningVec3::SetVec3(const float value[3])
{
    m_value[0] = value[0];
    m_value[1] = value[1];
    m_value[2] = value[2];
}

// -- Color -------------------------------------------------------------------
// @func 0x00732E68 TuningColor::~TuningColor
// @func 0x007363C4 TuningColor::~TuningColor (deleting)
TuningColor::~TuningColor() = default;
// @func 0x00736390 TuningColor::GetType
TuningVar::Type TuningColor::GetType() const { return kColor; }
// @func 0x00736398 TuningColor::SetColor
void TuningColor::SetColor(const xml::Color8* value) { m_value = *value; }

// -- String ------------------------------------------------------------------
// @func 0x0073644C TuningString::TuningString
TuningString::TuningString(const char* name, const char* value) : TuningVar(name)
{
    m_value[0] = '\0';
    StrNCopy(m_value, value, sizeof(m_value) - 1);
    m_value[sizeof(m_value) - 1] = '\0';
}
// @func 0x00732EF8 TuningString::~TuningString
// @func 0x007364B4 TuningString::~TuningString (deleting)
TuningString::~TuningString() = default;
// @func 0x007364AC TuningString::GetType
TuningVar::Type TuningString::GetType() const { return kString; }
// @func 0x0073641C TuningString::SetString
void TuningString::SetString(const char* value)
{
    StrNCopy(m_value, value, sizeof(m_value) - 1);
}

// -- Hash --------------------------------------------------------------------
// @func 0x00732E98 TuningHash::~TuningHash
// @func 0x007365A0 TuningHash::~TuningHash (deleting)
TuningHash::~TuningHash() = default;
// @func 0x00736558 TuningHash::GetType
TuningVar::Type TuningHash::GetType() const { return kHash; }
// @func 0x00736560 TuningHash::SetString
void TuningHash::SetString(const char* value) { m_value = HashNameNoCase(value); }
// @func 0x00736598 TuningHash::SetHash
void TuningHash::SetHash(uint32_t value) { m_value = value; }

// -- registry ----------------------------------------------------------------
int TuningVarCount()
{
    return g_tuningVarCount;
}

TuningVar* TuningVarAt(int index)
{
    return (index >= 0 && index < g_tuningVarCount) ? g_tuningVars[index] : nullptr;
}

TuningVar* FindTuningVar(uint32_t nameHash)
{
    for (int i = 0; i < g_tuningVarCount; ++i)
        if (g_tuningVars[i]->GetNameHash() == nameHash)
            return g_tuningVars[i];
    return nullptr;
}

namespace {
// Reads the "value" attribute of one <Var> into `var` using the getter that
// matches the variable's type. Returns false if the attribute is unusable.
bool ApplyValue(TuningVar& var, xml::XmlElement& elem, bool compareStrings)
{
    switch (var.GetType()) {
    case TuningVar::kHash: {
        uint32_t hash;
        if (elem.GetHash(kHash_Value, &hash, 0) != xml::kQuery_Ok)
            return false;
        var.SetHash(hash);
        return true;
    }
    case TuningVar::kString: {
        const char* text = elem.GetString("value");
        if (!text)
            return false;
        if (compareStrings)
            (void)StrCmp(static_cast<TuningString&>(var).Get(), text);
        var.SetString(text);
        return true;
    }
    case TuningVar::kColor: {
        static const xml::Color8 kDefault = { 0, 0, 0, 0 };
        xml::Color8 color;
        if (elem.GetColor(kHash_Value, &color, &kDefault) != xml::kQuery_Ok)
            return false;
        var.SetColor(&color);
        return true;
    }
    case TuningVar::kVec3: {
        static const float kDefault[3] = { 0.0f, 0.0f, 0.0f };
        float v[3];
        if (elem.GetVec3(kHash_Value, v, kDefault) != xml::kQuery_Ok)
            return false;
        var.SetVec3(v);
        return true;
    }
    case TuningVar::kVec2: {
        static const float kDefault[2] = { 0.0f, 0.0f };
        float v[2];
        if (elem.GetVec2(kHash_Value, v, kDefault) != xml::kQuery_Ok)
            return false;
        var.SetVec2(v);
        return true;
    }
    case TuningVar::kFloat: {
        float f;
        if (elem.GetFloat(kHash_Value, &f, 0.0f) != xml::kQuery_Ok)
            return false;
        var.SetFloat(f);
        return true;
    }
    case TuningVar::kInt: {
        int32_t i;
        if (elem.GetInt(kHash_Value, &i, 0) != xml::kQuery_Ok)
            return false;
        var.SetInt(i);
        return true;
    }
    case TuningVar::kBool: {
        bool b;
        if (elem.GetBool(kHash_Value, &b, false) != xml::kQuery_Ok)
            return false;
        var.SetBool(b);
        return true;
    }
    }
    return false;
}
}

// @func 0x004FC250 LoadTuningFile
bool LoadTuningFile(const char* path, bool compareStrings)
{
    xml::XmlDocument doc(path);
    if (!doc.LoadFile(0))
        return false;
    xml::XmlElement* root = doc.FirstChildElement(kHash_Tuning);
    if (!root)
        return false;

    for (xml::XmlElement* elem = root->FirstChildElement(kHash_Var); elem;
         elem = elem->NextSiblingElement(kHash_Var)) {
        uint32_t nameHash;
        if (elem->GetHash(kHash_Name, &nameHash, 0) != xml::kQuery_Ok)
            continue;
        TuningVar* var = FindTuningVar(nameHash);
        if (!var || !ApplyValue(*var, *elem, compareStrings)) {
            // The shipping build only looked the name up here (its warning
            // print is compiled out).
            (void)elem->GetString(kHash_Name);
        }
    }
    return true;
}

} // namespace ape
