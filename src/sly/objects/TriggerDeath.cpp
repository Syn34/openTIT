// openTIT - Death trigger volumes.
#include "sly/objects/TriggerDeath.h"

#include "ape/anim/ApeAnimScript.h"
#include "sly/game/AudioSystem.h"
#include "sly/objects/TriggerTuning.h"
#include "sly/objects/TriggerTypes.h"

namespace sly {
namespace objects {

namespace {
constexpr uint32_t kAttr_ScriptLifetimeName = 0xF89FDD94;   // "ScriptLifetime"
// Sounds of the fall kind. NOTE: names unknown.
constexpr uint32_t kFallSound = 0xA7012382;
constexpr uint32_t kFallSound2 = 0x734A2E83;
// Collision filter words of the volume: the fall kind collides with one more layer.
constexpr uint32_t kDeathFilter = 0x14;
constexpr uint32_t kFallDeathFilter = 0x18;

bool ReadBool(ape::xml::XmlElement* elem, uint32_t attr, bool fallback)
{
    bool value = fallback;
    elem->GetBool(attr, &value, fallback);
    return value;
}
} // namespace

// ---------------------------------------------------------------------------
// Definition

DeathTriggerDef::DeathTriggerDef(const TriggerDefDesc& desc, uint32_t type, uint32_t script,
                                 int state, float amount, bool f0, bool f1)
    : TriggerDef(desc), deathType(type), scriptName(script), physicsState(state), damage(amount),
      flag0(f0), flag1(f1)
{
    this->script = ape::ApeAnimScriptLibrary::FindScriptAnywhere(scriptName, 0, true);
}

// @func 0x0021B898 CreateDeathTriggerDef
EntityDef* CreateDeathTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                 const EntityDefDesc& loader, const EntityDef* parent)
{
    TriggerDefDesc desc(kDefClass_Trigger | kTrigType_Death, loader, "TriggerDeath_");
    desc.Parse(root, elem, loader, parent);

    uint32_t stateHash = 0;
    elem->GetHash(kAttr_PhysicsState, &stateHash, 0);
    const int state = PhysicsStateIndex(stateHash);
    float damage = 0.0f;
    elem->GetFloat(kAttr_DamageAmount, &damage, 0.0f);
    uint32_t deathType = kDefaultDeathType;
    elem->GetHash(kAttr_DeathType, &deathType, 0);
    uint32_t scriptName = 0;
    elem->GetHash(kAttr_ScriptLifetimeName, &scriptName, 0);
    const bool flag0 = ReadBool(elem, kAttr_DeathFlag1, false);
    const bool flag1 = ReadBool(elem, kAttr_DeathFlag4, false);
    return new DeathTriggerDef(desc, deathType, scriptName, state, damage, flag0, flag1);
}

DeathTriggerInstanceDesc::DeathTriggerInstanceDesc(const char* instanceName, DeathTriggerDef* ownerDef)
    : TriggerInstanceDesc(instanceName, ownerDef)
{
    // NOTE: the console leaves these to the placement; a plain trigger
    // placement would hand over uninitialised values. The definition's are
    // used here instead.
    if (!ownerDef)
        return;
    deathType = ownerDef->deathType;
    physicsState = ownerDef->physicsState;
    damage = ownerDef->damage;
    flags[1] = ownerDef->flag0;
    flags[4] = ownerDef->flag1;
}

// @func 0x0021BD90 DeathTriggerDef::Create
ObjEntity* DeathTriggerDef::Create(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->flags & kTrigType_Death) != 0))
        return nullptr;
    DeathTriggerInstanceDesc desc(GetInstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    DeathTrigger* trigger = new DeathTrigger(desc);
    trigger->OnCreated();
    return trigger;
}

// ---------------------------------------------------------------------------
// Placement

DeathTriggerPlacement::DeathTriggerPlacement(const char* placementName)
    : TriggerPlacement(placementName, nullptr, nullptr, true, false, true, false, false, false)
{
    flags |= kTrigType_Death;
}

// @func 0x0021C074 ReadDeathTriggerPlacement
static void ReadDeathTriggerPlacement(ape::xml::XmlElement* elem, DeathTriggerPlacement& placement)
{
    ParseTriggerPlacementFlags(elem, placement);
    uint32_t stateHash = 0;
    elem->GetHash(kAttr_PhysicsState, &stateHash, 0);
    const int state = PhysicsStateIndex(stateHash);
    if (state >= 0)
        placement.physicsState = state;
    elem->GetHash(kAttr_DeathType, &placement.deathType, placement.deathType);
    elem->GetFloat(kAttr_DamageAmount, &placement.damage, placement.damage);
    static constexpr uint32_t kFlagAttrs[5] = {
        kAttr_DeathFlag0, kAttr_DeathFlag1, kAttr_DeathFlag2, kAttr_DeathFlag3, kAttr_DeathFlag4,
    };
    for (int i = 0; i < 5; ++i)
        placement.deathFlags[i] = ReadBool(elem, kFlagAttrs[i], placement.deathFlags[i]);
}

// @func 0x0021C380 ParseDeathTriggerPlacement
EntityPlacement* ParseDeathTriggerPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                            const char* name)
{
    (void)root;
    DeathTriggerPlacement* placement = new DeathTriggerPlacement(name);
    ReadDeathTriggerPlacement(elem, *placement);
    return placement;
}

// @func 0x0021C47C DeathTriggerPlacement::FillInstanceDesc
void DeathTriggerPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    TriggerPlacement::FillInstanceDesc(baseDesc);
    DeathTriggerInstanceDesc& desc = static_cast<DeathTriggerInstanceDesc&>(baseDesc);
    desc.deathType = deathType;
    desc.physicsState = physicsState;
    desc.damage = damage;
    for (int i = 0; i < 5; ++i)
        desc.flags[i] = deathFlags[i];
}

// ---------------------------------------------------------------------------
// Entity

// @func 0x0021BBE4 DeathTrigger::DeathTrigger
DeathTrigger::DeathTrigger(const DeathTriggerInstanceDesc& desc)
    : TriggerBase(desc, 0x40, true), m_physicsState(desc.physicsState), m_damage(desc.damage)
{
    for (int i = 0; i < 5; ++i)
        m_flags[i] = desc.flags[i];
    for (uint32_t i = 0; i < 6; ++i) {
        if (kDeathTypes[i] == desc.deathType) {
            m_deathKind = i;
            break;
        }
    }
    if (m_deathKind == 0) {
        m_fallSound = FindSoundName(kFallSound, 0);
        m_fallSound2 = FindSoundName(kFallSound2, 0);
        VolumeSetCollisionFilter(m_volume, kFallDeathFilter);
    } else {
        VolumeSetCollisionFilter(m_volume, kDeathFilter);
    }
    // The definition's script runs for as long as the trigger lives.
    if (def) {
        if (void* script = static_cast<DeathTriggerDef*>(def)->script) {
            m_script = EntityStartScript(this, script);
            if (m_script)
                ScriptInstanceSetPaused(m_script, false);
        }
    }
}

// @func 0x0021C5B0 DeathTrigger::~DeathTrigger
// @func 0x0021C72C DeathTrigger::~DeathTrigger (deleting)
DeathTrigger::~DeathTrigger()
{
    audio::SoundNameRelease(m_fallSound);
    audio::SoundNameRelease(m_fallSound2);
    if (m_script)
        ScriptInstanceSetPaused(m_script, true);
}

// @func 0x0021C774 DeathTrigger::Update
// Occupants that left the required physics state stop counting as inside.
void DeathTrigger::Update(float dt)
{
    if (m_physicsState != -1) {
        size_t i = 0;
        while (i < m_occupants.size()) {
            ObjEntity* occupant = m_occupants[i];
            const int state = CharacterPhysicsState(occupant);
            if (state == -1 || state == m_physicsState)
                ++i;
            else
                OnExit(occupant);
        }
    }
    TriggerBase::Update(dt);
}

// @func 0x0021D300 DeathTrigger::SetEnabled
void DeathTrigger::SetEnabled(bool enabled)
{
    ChangeEnabled(enabled);
}

// @func 0x0021D2C8 DeathTrigger::IsDebugDisplayed
bool DeathTrigger::IsDebugDisplayed() const
{
    return TriggerDebugSwitch(g_debugDisplayTriggerDeath.Get());
}

} // namespace objects
} // namespace sly
