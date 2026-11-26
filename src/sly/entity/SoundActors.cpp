// openTIT - Construction of the sound actors (SoundActor, see ActorEntities.h).
#include "sly/entity/ActorEntities.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "sly/entity/ActorDefs.h"
#include "sly/entity/ActorHooks.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/objects/EntityPlacements.h"

#include <cfloat>
#include <cmath>

namespace sly {
namespace entity {

namespace {
// Float variable of the anim script that times the current slot.
constexpr uint32_t kVar_SlotTimer = 0xBBBC16FFu;
} // namespace

// @func 0x001ADF30 SoundActorDef::Spawn
// The instance starts with the definition's slot durations (the placement
// may override them) and the sound actor type bit.
Entity* SoundActorDef::Spawn(EntityPlacement* placement)
{
    objects::SoundActorInstanceDesc desc(InstanceName(placement), this);
    desc.typeBits[1] |= 0x80000u;
    for (int i = 0; i < 7; ++i)
        desc.slotTimes[i] = slotSounds[i].duration;
    placement->FillInstanceDesc(desc);
    auto* actor = new SoundActor(desc);
    actor->OnSpawn();
    return actor;
}

// @func 0x001ADD08 SoundActor::SoundActor
// A slot time at -FLT_MAX (unset) takes the definition's duration. The
// sequence resumes at the definition's start slot on the first update.
SoundActor::SoundActor(const objects::SoundActorInstanceDesc& desc) : ActorEntity(desc)
{
    savedTime = desc.field_EC;
    sequenceFlags &= ~(kSeq_ResumePending | kSeq_Scaled);
    const auto* soundDef = static_cast<const SoundActorDef*>(desc.def);
    for (int i = 0; i < 7; ++i) {
        SoundSlot& slot = sounds[i];
        slot.state = nullptr;
        slot.sound = audio::CreateSoundName(soundDef->slotSounds[i].name, soundDef->soundBank);
        slot.voice = 0;
        const float time = desc.slotTimes[i];
        slot.duration = time > -FLT_MAX ? time : soundDef->slotSounds[i].duration;
    }
    ResolveSlotStates();
    for (int i = 0; i < 7; ++i) {
        if (kSlotNames[i] == soundDef->startSlot) {
            savedSlot = i;
            break;
        }
    }
    sequenceFlags |= kSeq_ResumePending;
    if (soundDef->soundFlags & 0x80)
        sequenceFlags |= kSeq_Held;
    else
        sequenceFlags &= ~kSeq_Held;
}

// @func 0x001ADB14 SoundActor::ResolveSlotStates
void SoundActor::ResolveSlotStates()
{
    if (!def || !animScript)
        return;
    if (ape::ApeAnimScriptVar* timer = animScript->FindVar(kVar_SlotTimer)) {
        if (timer->GetType() == ape::AnimScriptVarType::Float)
            slotTimer = static_cast<ape::ApeAnimScriptFloatVar*>(timer);
    }
    ape::ApeAnimScript* script = animScript->GetScript();
    for (int i = 0; i < 7; ++i)
        sounds[i].state = script ? script->FindState(kSlotNames[i]) : nullptr;
}

// Body of part `index` of an actor, null when the actor has no such part.
static void* PartBody(const ActorEntity& actor, uint32_t index)
{
    if (!actor.parts || index >= actor.parts->Count())
        return nullptr;
    return (*actor.parts)[index].body;
}

// -- SoundActorB16678 (the switch) ---------------------------------------------------

// @func 0x001BEFE8 SoundActorB16678Def::Spawn
Entity* SoundActorB16678Def::Spawn(EntityPlacement* placement)
{
    objects::SoundActorB16678InstanceDesc desc(InstanceName(placement), this);
    desc.typeBits[1] |= 0x80000u;
    for (int i = 0; i < 7; ++i)
        desc.slotTimes[i] = slotSounds[i].duration;
    desc.switchState = field_190;
    desc.field_F4 = field_194;
    desc.field_F8[0] = field_198;
    desc.field_F8[1] = field_199;
    desc.typeBits[1] |= 0x100000u;
    placement->FillInstanceDesc(desc);
    auto* actor = new SoundActorB16678(desc);
    actor->OnSpawn();
    return actor;
}

SoundActorB16678::SoundActorB16678(const objects::SoundActorB16678InstanceDesc& desc)
    : SoundActor(desc), position(int32_t(desc.switchState)), flipPlayer(-1), resetTime(desc.field_F4), delay(0.0f)
{
    switchFlags &= 0x07;
    if (desc.field_F8[0])
        switchFlags |= 0x80;   // locks after one flip
    if (desc.field_F8[1])
        switchFlags |= 0x10;
}

// -- SoundActorB168E0 ------------------------------------------------------------------

// @func 0x001C0A10 SoundActorB168E0Def::Spawn
// NOTE: without a definition the console would default flag bit 30 to set.
Entity* SoundActorB168E0Def::Spawn(EntityPlacement* placement)
{
    objects::SoundActorB168E0InstanceDesc desc(InstanceName(placement), this);
    desc.typeBits[1] |= 0x80000u;
    for (int i = 0; i < 7; ++i)
        desc.slotTimes[i] = slotSounds[i].duration;
    desc.field_F0 = field_198;
    desc.flags_F4 = (desc.flags_F4 & 0x3FFFFFFFu) | (flags_19C & 0xC0000000u);
    desc.typeBits[1] |= 0x200000u;
    placement->FillInstanceDesc(desc);
    auto* actor = new SoundActorB168E0(desc);
    actor->OnSpawn();
    return actor;
}

// @func 0x001C08AC SoundActorB168E0::SoundActorB168E0
SoundActorB168E0::SoundActorB168E0(const objects::SoundActorB168E0InstanceDesc& desc)
    : SoundActor(desc), field_238(desc.field_F0)
{
    flags240 = 0;
    if (desc.flags_F4 & 0x80000000u)
        flags240 |= 0x40;
    if (desc.flags_F4 & 0x40000000u)
        flags240 |= 0x20;
    const auto* switchDef = static_cast<const SoundActorB168E0Def*>(desc.def);
    partBody = PartBody(*this, uint32_t(switchDef->field_190));
    if (switchDef->field_194)
        flags240 |= 0x04;
}

// -- SoundActorB16B48 ------------------------------------------------------------------

// @func 0x001C2E20 SoundActorB16B48Def::Spawn
Entity* SoundActorB16B48Def::Spawn(EntityPlacement* placement)
{
    objects::SoundActorB16B48InstanceDesc desc(InstanceName(placement), this);
    desc.typeBits[1] |= 0x80000u;
    for (int i = 0; i < 7; ++i)
        desc.slotTimes[i] = slotSounds[i].duration;
    desc.field_F0 = field_1B8;
    desc.typeBits[1] |= 0x1000000u;
    placement->FillInstanceDesc(desc);
    auto* actor = new SoundActorB16B48(desc);
    actor->OnSpawn();
    return actor;
}

// @func 0x001C2BE0 SoundActorB16B48::SoundActorB16B48
// Light parts without a body are skipped.
SoundActorB16B48::SoundActorB16B48(const objects::SoundActorB16B48InstanceDesc& desc)
    : SoundActor(desc), field_258(desc.field_F0)
{
    lightFlags &= 0x1F;
    const auto* lightDef = static_cast<const SoundActorB16B48Def*>(desc.def);
    if (!lightDef)
        return;
    for (int32_t part : lightDef->bodyParts) {
        if (void* body = PartBody(*this, uint32_t(part)))
            lightBodies.PushBack(body);
    }
    if (lightDef->lightNames.Count() != 0)
        lightFlags |= 0x40;
}

// -- SoundActorB17728 ------------------------------------------------------------------

// @func 0x001D40A8 SoundActorB17728Def::Spawn
Entity* SoundActorB17728Def::Spawn(EntityPlacement* placement)
{
    objects::SoundActorB17728InstanceDesc desc(InstanceName(placement), this);
    desc.typeBits[1] |= 0x80000u;
    for (int i = 0; i < 7; ++i)
        desc.slotTimes[i] = slotSounds[i].duration;
    desc.field_F0 = field_190;
    desc.field_F4 = field_194;
    desc.typeBits[1] |= 0x4000000u;
    placement->FillInstanceDesc(desc);
    auto* actor = new SoundActorB17728(desc);
    actor->OnSpawn();
    return actor;
}

SoundActorB17728::SoundActorB17728(const objects::SoundActorB17728InstanceDesc& desc)
    : SoundActor(desc), attachedName(desc.field_F0), rangeSq(desc.field_F4 * desc.field_F4)
{
    lookupFlags &= 0x7F;
    holdWrap = false;
}

// -- SoundActorB17270 (the gadget turret) ----------------------------------------------

// @func 0x001CB448 SoundActorB17270Def::Spawn
Entity* SoundActorB17270Def::Spawn(EntityPlacement* placement)
{
    objects::SoundActorB17270InstanceDesc desc(InstanceName(placement), this);
    desc.typeBits[1] |= 0x80000u;
    for (int i = 0; i < 7; ++i)
        desc.slotTimes[i] = slotSounds[i].duration;
    desc.field_F0 = field_1B4;
    for (int i = 0; i < 7; ++i)
        desc.field_F4[i] = field_1B8[i];
    desc.field_110 = field_1E0;
    desc.field_114[0] = field_1E4;
    desc.field_114[1] = field_1E5;
    desc.typeBits[1] |= 0x2000000u;
    placement->FillInstanceDesc(desc);
    auto* actor = new SoundActorB17270(desc);
    actor->OnSpawn();
    return actor;
}

// @func 0x001CB000 SoundActorB17270::SoundActorB17270
SoundActorB17270::SoundActorB17270(const objects::SoundActorB17270InstanceDesc& desc)
    : SoundActor(desc), field_2F4(desc.field_F0), field_2F8(desc.field_F4[0]), field_2FC(desc.field_F4[1]),
      field_31C(desc.field_F4[2]), field_320(desc.field_F4[3]), field_324(desc.field_F4[4]),
      shotCount(desc.field_110)
{
    static const ape::Matrix34 kIdentity = {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f},
                                            {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}};
    // NOTE: console global 0x01164B70, assumed +Z.
    const ape::Vec3 forward = {0.0f, 0.0f, 1.0f, 0.0f};
    aimFrame = kIdentity;
    baseTransform = kIdentity;
    restAim = wantedAim = currentAim = forward;
    rangeSq = desc.field_F4[5] * desc.field_F4[5];
    rangeSq2 = desc.field_F4[6] >= 0.0f ? desc.field_F4[6] * desc.field_F4[6] : -1.0f;
    turretFlags = uint8_t((turretFlags & 0x3F) | (desc.field_114[0] ? 0x80 : 0) | (desc.field_114[1] ? 0x40 : 0));
    field_355 = false;
    engaged = false;
    flags357 = uint8_t((flags357 & 0x7F) | 0x40);
    if (!def)
        return;
    const auto* turretDef = static_cast<const SoundActorB17270Def*>(def);
    aimBoneIndex = FindBone(turretDef->aimPartName);
    if (aimBoneIndex == -1)
        aimBoneObject = EntityFindObject(this, turretDef->aimPartName);
    FindAimParts();
    UpdateAimFrames();
    ape::Vec3 aim = aimFrame.zAxis;
    if (turretFlags & 0x80)
        aim.y = 0.0f;
    const float lengthSq = aim.z * aim.z + aim.x * aim.x + aim.y * aim.y;
    if (lengthSq >= 1e-05f) {
        const float inv = 1.0f / std::sqrt(lengthSq);
        aim.x *= inv;
        aim.y *= inv;
        aim.z *= inv;
    }
    wantedAim = currentAim = restAim = {aim.x, aim.y, aim.z, 0.0f};
    weapon = EntitySkin(this);
    component03 = EntityComponent03(this);
    if (weapon) {
        SkinSetOwnerGroup(weapon, GetRenderId());
        SkinSetShotCallback(weapon, &SoundActorB17270::OnWeaponShot);
        const float interval = SkinRate(weapon);
        if (shotCount > 1 && interval > 0.0f)
            shotDelay = float(shotCount - 1) * interval;
    }
}

// @func 0x001CF5E0 SoundActorB17270::OnWeaponShot
// NOTE: the console does not check that the owner is a turret (type bit
// 0x2000000 of the second type word) before writing the flag.
void SoundActorB17270::OnWeaponShot(Entity* owner)
{
    if (owner && (owner->typeBits[1] & 0x2000000u))
        static_cast<SoundActorB17270*>(owner)->flags357 |= 0x40;
}

} // namespace entity
} // namespace sly
