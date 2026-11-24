// openTIT - The game's own anim script variable types.
#include "sly/levels/script/ScriptVars.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"

namespace sly {
namespace levels {

using ape::AnimHash;
using ape::ApeAnimScriptVar;
using ape::xml::XmlElement;

namespace {
constexpr uint32_t kAttrName = AnimHash("name");
constexpr uint32_t kAttrVal = AnimHash("val");

// Element names the prototypes are registered under.
constexpr uint32_t kElemSound = 0x08D5E6EBu;   // NOTE: name unknown (sound handle variable)
constexpr uint32_t kElemString = AnimHash("String");
constexpr uint32_t kElemTrigger = AnimHash("Trigger");
constexpr uint32_t kElemSpawner = AnimHash("Spawner");
constexpr uint32_t kElemSpawnerGroup = AnimHash("SpawnerGroup");
constexpr uint32_t kElemLaser = AnimHash("Laser");
constexpr uint32_t kElemLaserField = AnimHash("LaserField");
constexpr uint32_t kElemLaserGroup = AnimHash("LaserGroup");

// The handle variables take only their name from script data.
uint32_t ReadNameHash(XmlElement* element)
{
    const char* name = element->GetString("name");
    return name ? ape::HashNameNoCase(name) : 0;
}
} // namespace

// ---------------------------------------------------------------------------
// Sound

// @func 0x000A7610 ScriptSoundVar::ScriptSoundVar
ScriptSoundVar::ScriptSoundVar() = default;

// @func 0x000A7644 ScriptSoundVar::~ScriptSoundVar
// @func 0x000A76C4 ScriptSoundVar::~ScriptSoundVar (deleting)
ScriptSoundVar::~ScriptSoundVar()
{
    if (m_voice != 0)
        audio::StopVoice(m_voice, true);
}

// @func 0x000A775C ScriptSoundVar::Clone
ApeAnimScriptVar* ScriptSoundVar::Clone() const
{
    auto* copy = new ScriptSoundVar();
    copy->m_voice = m_voice;
    copy->m_nameHash = m_nameHash;
    return copy;
}

// @func 0x000A77F8 ScriptSoundVar::Create
ApeAnimScriptVar* ScriptSoundVar::Create(XmlElement* element) const
{
    auto* var = new ScriptSoundVar();
    var->m_nameHash = ReadNameHash(element);
    return var;
}

// ---------------------------------------------------------------------------
// String

// @func 0x000A78B4 ScriptStringVar::ScriptStringVar
ScriptStringVar::ScriptStringVar() = default;

// @func 0x000A78D4 ScriptStringVar::SetString
void ScriptStringVar::SetString(const char* text)
{
    if (text && text[0] != '\0')
        ape::StrCopyBounded(m_text, text, sizeof(m_text));
}

// @func 0x000A7928 ScriptStringVar::Clone
ApeAnimScriptVar* ScriptStringVar::Clone() const
{
    auto* copy = new ScriptStringVar();
    copy->SetString(m_text);
    copy->m_nameHash = m_nameHash;
    return copy;
}

// @func 0x000A79D0 ScriptStringVar::Create
ApeAnimScriptVar* ScriptStringVar::Create(XmlElement* element) const
{
    auto* var = new ScriptStringVar();
    element->GetHash(kAttrName, &var->m_nameHash, 0);
    var->SetString(element->GetString(kAttrVal));
    return var;
}

// ---------------------------------------------------------------------------
// Handle variables

// @func 0x000A7AAC ScriptHandleVar::ScriptHandleVar (Trigger)
// @func 0x000A7C84 ScriptHandleVar::ScriptHandleVar (Spawner)
// @func 0x000A7E5C ScriptHandleVar::ScriptHandleVar (SpawnerGroup)
// @func 0x000A8034 ScriptHandleVar::ScriptHandleVar (Laser)
// @func 0x000A820C ScriptHandleVar::ScriptHandleVar (LaserField)
// @func 0x000A83E4 ScriptHandleVar::ScriptHandleVar (LaserGroup)
template <class T, int kType>
ScriptHandleVar<T, kType>::ScriptHandleVar() = default;

// @func 0x000A7AD0 ScriptHandleVar::~ScriptHandleVar (Trigger)
// @func 0x000A7B00 ScriptHandleVar::~ScriptHandleVar (Trigger, deleting)
// @func 0x000A7CA8 ScriptHandleVar::~ScriptHandleVar (Spawner)
// @func 0x000A7CD8 ScriptHandleVar::~ScriptHandleVar (Spawner, deleting)
// @func 0x000A7E80 ScriptHandleVar::~ScriptHandleVar (SpawnerGroup)
// @func 0x000A7EB0 ScriptHandleVar::~ScriptHandleVar (SpawnerGroup, deleting)
// @func 0x000A8058 ScriptHandleVar::~ScriptHandleVar (Laser)
// @func 0x000A8088 ScriptHandleVar::~ScriptHandleVar (Laser, deleting)
// @func 0x000A8230 ScriptHandleVar::~ScriptHandleVar (LaserField)
// @func 0x000A8260 ScriptHandleVar::~ScriptHandleVar (LaserField, deleting)
// @func 0x000A8408 ScriptHandleVar::~ScriptHandleVar (LaserGroup)
// @func 0x000A8438 ScriptHandleVar::~ScriptHandleVar (LaserGroup, deleting)
template <class T, int kType>
ScriptHandleVar<T, kType>::~ScriptHandleVar() = default;

// @func 0x000A7B58 ScriptHandleVar::Clone (Trigger)
// @func 0x000A7D30 ScriptHandleVar::Clone (Spawner)
// @func 0x000A7F08 ScriptHandleVar::Clone (SpawnerGroup)
// @func 0x000A80E0 ScriptHandleVar::Clone (Laser)
// @func 0x000A82B8 ScriptHandleVar::Clone (LaserField)
// @func 0x000A8490 ScriptHandleVar::Clone (LaserGroup)
template <class T, int kType>
ApeAnimScriptVar* ScriptHandleVar<T, kType>::Clone() const
{
    auto* copy = new ScriptHandleVar();
    copy->m_value = m_value;
    copy->m_nameHash = m_nameHash;
    return copy;
}

// @func 0x000A7BD8 ScriptHandleVar::Create (Trigger)
// @func 0x000A7DB0 ScriptHandleVar::Create (Spawner)
// @func 0x000A7F88 ScriptHandleVar::Create (SpawnerGroup)
// @func 0x000A8160 ScriptHandleVar::Create (Laser)
// @func 0x000A8338 ScriptHandleVar::Create (LaserField)
// @func 0x000A8510 ScriptHandleVar::Create (LaserGroup)
template <class T, int kType>
ApeAnimScriptVar* ScriptHandleVar<T, kType>::Create(XmlElement* element) const
{
    auto* var = new ScriptHandleVar();
    var->m_nameHash = ReadNameHash(element);
    return var;
}

template class ScriptHandleVar<WorldObject*, kVarType_Trigger>;
template class ScriptHandleVar<Spawner*, kVarType_Spawner>;
template class ScriptHandleVar<SpawnGroup*, kVarType_SpawnGroup>;
template class ScriptHandleVar<Laser*, kVarType_Laser>;
template class ScriptHandleVar<LaserField*, kVarType_LaserField>;
template class ScriptHandleVar<LaserGroup*, kVarType_LaserGroup>;

// ---------------------------------------------------------------------------

void RegisterScriptVars()
{
    auto& prototypes = ape::VarPrototypes();
    prototypes.Add(kElemSound, new ScriptSoundVar());
    prototypes.Add(kElemString, new ScriptStringVar());
    prototypes.Add(kElemTrigger, new ScriptTriggerVar());
    prototypes.Add(kElemSpawner, new ScriptSpawnerVar());
    prototypes.Add(kElemSpawnerGroup, new ScriptSpawnGroupVar());
    prototypes.Add(kElemLaser, new ScriptLaserVar());
    prototypes.Add(kElemLaserField, new ScriptLaserFieldVar());
    prototypes.Add(kElemLaserGroup, new ScriptLaserGroupVar());
}

} // namespace levels
} // namespace sly
