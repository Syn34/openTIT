// openTIT - Placement copies (slot 2 of the placement classes): each hands
// its fields to the instance description an entity is constructed from.
// See EntityPlacements.h for the layouts.
#include "sly/objects/EntityPlacements.h"

namespace sly {
namespace objects {

namespace {

// Placement words that keep only their top two bits (30/31) in the
// description.
uint32_t TakeTopBits(uint32_t into, uint32_t from)
{
    return (into & 0x3FFFFFFFu) | (from & 0xC0000000u);
}

// Copies x, y and z (the fourth lane stays).
void CopyXYZ(ape::Vec3& to, const ape::Vec3& from)
{
    to = {from.x, from.y, from.z, to.pad};
}

void CopyPhysical(PhysicalSettings& to, const PhysicalSettings& from)
{
    CopyXYZ(to.field_00, from.field_00);
    CopyXYZ(to.rotation, from.rotation);
    to.field_20 = from.field_20;
}

// Appends the 0x4A meaningful bytes of every override record.
void AppendOverrides(ape::ApeTArray<ScriptPropertyOverride>& to, const ape::ApeTArray<ScriptPropertyOverride>& from)
{
    for (const ScriptPropertyOverride& record : from)
        to.PushBack(record);
}

} // namespace

// @func 0x000DFBFC ScriptedEntityPlacement::FillInstanceDesc
void ScriptedEntityPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    EntityPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<ScriptedInstanceDesc&>(baseDesc);
    desc.field_B8 = field_DC;
    desc.field_BC = field_E0;
    desc.field_C0 = field_E4;
    desc.field_C4 = field_E8;
    AppendOverrides(desc.overridesA, overridesA);
    AppendOverrides(desc.overridesB, overridesB);
}

// @func 0x000E44C0 ActorPlacement::FillInstanceDesc
void ActorPlacement::FillInstanceDesc(EntityInstanceDesc& desc) const
{
    ScriptedEntityPlacement::FillInstanceDesc(desc);
}

// @func 0x000EB080 PhysicalEntityPlacement::FillInstanceDesc
void PhysicalEntityPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ScriptedEntityPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<PhysicalInstanceDesc&>(baseDesc);
    CopyPhysical(desc.settings, settings);
    desc.field_F4 = field_114;
}

// @func 0x00168114 RigidPropPlacement::FillInstanceDesc
void RigidPropPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ScriptedEntityPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<RigidPropInstanceDesc&>(baseDesc);
    CopyPhysical(desc.settings, settings);
    desc.field_C8 = field_114;
}

// @func 0x0016CCD8 CompoundEntityPlacement::FillInstanceDesc
void CompoundEntityPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ScriptedEntityPlacement::FillInstanceDesc(baseDesc);
    static_cast<CompoundEntityInstanceDesc&>(baseDesc).field_C8 = field_EC;
}

// @func 0x00177FB4 AlterEgoPlacement::FillInstanceDesc
void AlterEgoPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ScriptedEntityPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<AlterEgoInstanceDesc&>(baseDesc);
    CopyPhysical(desc.settings, settings);
    desc.field_F4 = field_114;
    desc.field_F8 = field_118;
    desc.field_FC = field_11C;
    desc.field_100[0] = field_120[0];
    desc.field_100[1] = field_120[1];
    // Placement bit 30 decides description flag 12 again.
    desc.flags = (desc.flags & ~EntityInstanceDesc::kDesc_PlacementBit30) |
                 ((bits84 >> 18) & EntityInstanceDesc::kDesc_PlacementBit30);
}

// @func 0x0017F318 TargetableAPlacement::FillInstanceDesc
void TargetableAPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ScriptedEntityPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<TargetableAInstanceDesc&>(baseDesc);
    CopyPhysical(desc.settings, settings);
    desc.field_F4 = field_114;
    desc.field_F8 = field_118;
    desc.field_FC = field_11C;
    desc.field_100 = field_120;
    desc.field_104 = field_124;
    desc.field_108 = field_128;
    desc.field_10C = field_12C;
    desc.field_110 = field_130;
    desc.field_114 = field_134;
    desc.field_118 = field_138;
    desc.field_11C = field_13C;
    desc.field_120 = field_140;
    desc.field_C8 = field_144;
}

// @func 0x00191024 TargetableBPlacement::FillInstanceDesc
void TargetableBPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ScriptedEntityPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<TargetableBInstanceDesc&>(baseDesc);
    CopyPhysical(desc.settings, settings);
    desc.field_F4 = field_114;
    desc.field_F8 = field_118;
    desc.field_FC = field_11C;
    desc.field_100 = field_120;
    desc.field_104 = field_124;
    desc.field_108 = field_128;
    desc.field_10C = field_12C;
    desc.field_110 = field_130;
    desc.field_114 = field_134;
    desc.field_118 = field_138;
    desc.field_11C = field_13C;
    desc.field_120 = field_140;
    desc.field_124 = field_144;
    desc.field_C8 = field_148;
}

// @func 0x001A5FF4 BreakableChainPlacement::FillInstanceDesc
void BreakableChainPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    EntityPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<BreakableChainInstanceDesc&>(baseDesc);
    for (const BreakableChainPiece& piece : pieces)
        desc.pieces.PushBack(piece);
}

// @func 0x001A7D48 ChainPlacement::FillInstanceDesc
void ChainPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    EntityPlacement::FillInstanceDesc(baseDesc);
    static_cast<ChainInstanceDesc&>(baseDesc).field_90 = field_B4;
}

// @func 0x001AA57C ActorGroupPlacement::FillInstanceDesc
void ActorGroupPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ActorPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<ActorGroupInstanceDesc&>(baseDesc);
    for (int i = 0; i < 4; ++i)
        desc.field_D0[i] = field_F0[i];
    desc.field_E0 = field_100;
}

// @func 0x001ABA70 HitCounterActorPlacement::FillInstanceDesc
void HitCounterActorPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ActorPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<HitCounterActorInstanceDesc&>(baseDesc);
    for (int i = 0; i < 5; ++i)
        desc.field_D0[i] = field_F0[i];
    desc.flags_E4 = TakeTopBits(desc.flags_E4, flags_104);
}

// @func 0x001AE3B4 SoundActorPlacement::FillInstanceDesc
void SoundActorPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ActorPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<SoundActorInstanceDesc&>(baseDesc);
    desc.field_EC = field_10C;
    for (int i = 0; i < 7; ++i)
        desc.sounds[i] = sounds[i];
    // Flags 17-31 give way to the sound flag.
    desc.flags = (desc.flags & 0x1FFFFu) | EntityInstanceDesc::kDesc_Sound;
}

// @func 0x001B0654 SignalActorPlacement::FillInstanceDesc
void SignalActorPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ActorPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<SignalActorInstanceDesc&>(baseDesc);
    desc.field_D0[0] = field_F0[0];
    desc.field_D0[1] = field_F0[1];
    desc.flags_DC = TakeTopBits(desc.flags_DC, flags_FC);
    desc.field_D0[2] = field_F0[2];
}

// @func 0x001B29B8 ActorB15E60Placement::FillInstanceDesc
void ActorB15E60Placement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ActorPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<ActorB15E60InstanceDesc&>(baseDesc);
    for (int i = 0; i < 5; ++i)
        desc.field_D0[i] = field_F0[i];
}

// @func 0x001B57A4 PlacementB16088::FillInstanceDesc
void PlacementB16088::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ScriptedEntityPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<InstanceDescB16088&>(baseDesc);
    desc.field_DC = field_100;
    for (const Entry& entry : field_EC)
        desc.field_C8.PushBack(entry);
}

// @func 0x001BB920 RopePlacement::FillInstanceDesc
void RopePlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    EntityPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<RopeInstanceDesc&>(baseDesc);
    CopyXYZ(desc.field_90, field_C0);
    CopyXYZ(desc.field_A0, field_D0);
    desc.field_B0 = field_E0;
    desc.field_B4 = field_E4;
    if (field_E8)
        desc.field_BC = field_E8;
}

// @func 0x001BDFE8 ScriptObjectActorPlacement::FillInstanceDesc
// NOTE: skips the actor step and uses the scripted copy directly (the same
// work on the console).
void ScriptObjectActorPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ScriptedEntityPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<ScriptObjectActorInstanceDesc&>(baseDesc);
    desc.field_E0 = field_100;
    for (int i = 0; i < 3; ++i) {
        desc.field_D0[i] = field_F0[i];
        desc.field_E8[i] = field_108[i];
    }
}

// @func 0x001BF6C0 SoundActorB16678Placement::FillInstanceDesc
void SoundActorB16678Placement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    SoundActorPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<SoundActorB16678InstanceDesc&>(baseDesc);
    desc.field_F0[0] = field_110[0];
    desc.field_F0[1] = field_110[1];
    desc.field_F8[0] = field_118[0];
    desc.field_F8[1] = field_118[1];
}

// @func 0x001C103C SoundActorB168E0Placement::FillInstanceDesc
void SoundActorB168E0Placement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    SoundActorPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<SoundActorB168E0InstanceDesc&>(baseDesc);
    desc.field_F0 = field_110;
    desc.flags_F4 = TakeTopBits(desc.flags_F4, flags_114);
}

// @func 0x001C3308 SoundActorB16B48Placement::FillInstanceDesc
void SoundActorB16B48Placement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    SoundActorPlacement::FillInstanceDesc(baseDesc);
    static_cast<SoundActorB16B48InstanceDesc&>(baseDesc).field_F0 = field_110;
}

// @func 0x001C6FB0 SoundActorB16DB0Placement::FillInstanceDesc
void SoundActorB16DB0Placement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    SoundActorPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<SoundActorB16DB0InstanceDesc&>(baseDesc);
    for (int i = 0; i < 4; ++i)
        desc.field_F0[i] = field_110[i];
    desc.flags_2E0 = TakeTopBits(desc.flags_2E0, flags_300);
    desc.records = records;
}

// @func 0x001CBD04 SoundActorB17270Placement::FillInstanceDesc
void SoundActorB17270Placement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    SoundActorPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<SoundActorB17270InstanceDesc&>(baseDesc);
    for (int i = 0; i < 9; ++i)
        desc.field_F0[i] = field_110[i];
    desc.field_114[0] = field_134[0];
    desc.field_114[1] = field_134[1];
}

// @func 0x001D4620 SoundActorB17728Placement::FillInstanceDesc
void SoundActorB17728Placement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    SoundActorPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<SoundActorB17728InstanceDesc&>(baseDesc);
    desc.field_F0[0] = field_110[0];
    desc.field_F0[1] = field_110[1];
}

// @func 0x001D80C4 ActorB17990Placement::FillInstanceDesc
void ActorB17990Placement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ActorPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<ActorB17990InstanceDesc&>(baseDesc);
    CopyXYZ(desc.field_D0, field_F0);
    CopyXYZ(desc.field_E0, field_100);
    for (int i = 0; i < 4; ++i)
        desc.field_F0[i] = field_110[i];
    desc.block = block;
    desc.flags_144 = TakeTopBits(desc.flags_144, flags_164);
}

// @func 0x001DEB58 InteractivePropAPlacement::FillInstanceDesc
void InteractivePropAPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ActorPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<InteractivePropAInstanceDesc&>(baseDesc);
    desc.field_D0[0] = field_F0[0];
    desc.field_D0[1] = field_F0[1];
}

// @func 0x001E6F3C SafePlacement::FillInstanceDesc
void SafePlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ActorPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<SafeInstanceDesc&>(baseDesc);
    desc.field_D0 = field_F0;
    for (int i = 0; i < 3; ++i)
        desc.field_E0[i] = field_100[i];
    desc.field_F0 = field_110;
}

// @func 0x001ED4F0 TweenActorPlacement::FillInstanceDesc
void TweenActorPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    ActorPlacement::FillInstanceDesc(baseDesc);
    auto& desc = static_cast<TweenActorInstanceDesc&>(baseDesc);
    for (int i = 0; i < 0x11; ++i)
        desc.field_D0[i] = field_F0[i];
    for (int i = 0; i < 8; ++i)
        desc.field_114[i] = field_134[i];
}

} // namespace objects
} // namespace sly
