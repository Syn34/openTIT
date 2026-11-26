// openTIT - Entity classes built from a definition's collision object (see PropEntities.h).
#include "sly/entity/PropEntities.h"

#include "sly/entity/AlterEgoBentley.h"

#include "ape/world/ApeDef.h"
#include "ape/world/ApeObj.h"
#include "ape/xml/ApeXml.h"
#include "sly/camera/CameraHooks.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHooks.h"

namespace sly {
namespace entity {

void ReleaseCollisionShape(void* shape)
{
    camera::HavokRemoveReference(shape);
}

// @func 0x001667DC RigidPropDef::~RigidPropDef
// @func 0x0016683C RigidPropDef::~RigidPropDef (deleting)
RigidPropDef::~RigidPropDef()
{
    if (collisionShape)
        ReleaseCollisionShape(collisionShape);
}

// @func 0x0016819C RigidProp::~RigidProp
// @func 0x001681E4 RigidProp::~RigidProp (deleting)
// @func 0x00168258 RigidProp::~RigidProp (deleting thunk, +0x180)
// @func 0x001682D0 RigidProp::~RigidProp (deleting thunk, +0x17C)
// @func 0x00168350 RigidProp::~RigidProp (thunk, +0x180)
// @func 0x001683A0 RigidProp::~RigidProp (thunk, +0x17C)
RigidProp::~RigidProp() = default;

// @func 0x00169260 RigidProp::InteractionOffer
uint32_t RigidProp::InteractionOffer(int kind, Entity* /*other*/)
{
    return Def()->targetingFlags[kind];
}

// @func 0x00169248 RigidProp::InteractionNeeds
uint32_t RigidProp::InteractionNeeds(int kind, Entity* /*other*/)
{
    return Def()->targetingFlags[kind];
}

// @func 0x001762F8 AlterEgoDef::~AlterEgoDef
// @func 0x00176358 AlterEgoDef::~AlterEgoDef (deleting)
AlterEgoDef::~AlterEgoDef()
{
    if (collisionShape)
        ReleaseCollisionShape(collisionShape);
}

// @func 0x00178060 AlterEgoEntity::~AlterEgoEntity
// @func 0x001780E4 AlterEgoEntity::~AlterEgoEntity (deleting)
// @func 0x00178184 AlterEgoEntity::~AlterEgoEntity (deleting thunk, +0x180)
// @func 0x00178228 AlterEgoEntity::~AlterEgoEntity (deleting thunk, +0x17C)
// @func 0x001782D0 AlterEgoEntity::~AlterEgoEntity (thunk, +0x180)
// @func 0x00178358 AlterEgoEntity::~AlterEgoEntity (thunk, +0x17C)
AlterEgoEntity::~AlterEgoEntity()
{
    UnregisterEntity(kList_B, this);
}

// @func 0x00179FE8 AlterEgoEntity::InteractionNeeds
uint32_t AlterEgoEntity::InteractionNeeds(int kind, Entity* /*other*/)
{
    return Def()->targetingFlags[kind];
}

// @func 0x0016A740 AlterEgoBentleyDef::~AlterEgoBentleyDef
// @func 0x0016A7A0 AlterEgoBentleyDef::~AlterEgoBentleyDef (deleting)
AlterEgoBentleyDef::~AlterEgoBentleyDef()
{
    if (collisionShape)
        ReleaseCollisionShape(collisionShape);
}

// @func 0x0017CC7C TargetableDefA::~TargetableDefA
// @func 0x0017CCDC TargetableDefA::~TargetableDefA (deleting)
TargetableDefA::~TargetableDefA()
{
    if (collisionShape)
        ReleaseCollisionShape(collisionShape);
}

// @func 0x0018EDB4 TargetableDefB::~TargetableDefB
// @func 0x0018EEA0 TargetableDefB::~TargetableDefB (deleting)
TargetableDefB::~TargetableDefB()
{
    while (entries.Count() != 0) {
        delete entries[entries.Count() - 1];
        entries.RemoveAtSwap(entries.Count() - 1);
    }
    if (collisionShape)
        ReleaseCollisionShape(collisionShape);
}

// @func 0x001F8F8C TargetableDefC::~TargetableDefC
// @func 0x001F8FEC TargetableDefC::~TargetableDefC (deleting)
TargetableDefC::~TargetableDefC()
{
    if (collisionShape)
        ReleaseCollisionShape(collisionShape);
}

// @func 0x001FC6CC TargetableCBase::InteractionNeeds
uint32_t TargetableCBase::InteractionNeeds(int kind, Entity* /*other*/)
{
    return Def()->targetingFlags[kind];
}

// @func 0x001B3AAC PushBlockDef::~PushBlockDef
// @func 0x001B3B34 PushBlockDef::~PushBlockDef (deleting)
PushBlockDef::~PushBlockDef()
{
    if (collisionShape)
        ReleaseCollisionShape(collisionShape);
}

// @func 0x001A7878 ChainDefDesc::Parse
void ChainDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                         const EntityDef* parent)
{
    (void)root;   // only used for the warning about a parent of another class (stripped)
    InheritDefDesc(loader, *this);
    flags[1] = true;
    ChainDefDesc defaults;
    if (parent && (parent->typeBits & 0x800)) {
        const auto* chain = static_cast<const ChainDef*>(parent);
        defaults.field_4C = chain->field_8C;
        defaults.linkCount = chain->linkCount;
        defaults.field_54 = chain->field_94;
        defaults.field_58 = chain->field_98;
        defaults.field_5C = chain->field_9C;
        defaults.field_60 = chain->field_A0;
        defaults.field_64 = chain->field_A4;
        defaults.field_68 = chain->field_A8;
    }
    elem->GetHash(0x3559A5ABu, &field_4C, defaults.field_4C);
    elem->GetInt(0xBEC3D7F3u, &linkCount, defaults.linkCount);
    elem->GetFloat(0x75DF19EEu, &field_54, defaults.field_54);
    elem->GetFloat(0x807548E2u, &field_58, defaults.field_58);
    elem->GetFloat(0x9E18283Eu, &field_5C, defaults.field_5C);
    elem->GetFloat(0x9E2E88C2u, &field_60, defaults.field_60);
    elem->GetFloat(0x05F9141Fu, &field_64, defaults.field_64);
    elem->GetHash(0x10E70B4Du, &field_68, defaults.field_68);
}

// The chain's five sounds (0x00AD1410; the third is unused).
// NOTE: the sound names behind the hashes are not known.
ChainDef::ChainDef(const ChainDefDesc& desc)
    : EntityDef(desc), field_8C(desc.field_4C), linkCount(desc.linkCount), field_94(desc.field_54),
      field_98(desc.field_58), field_9C(desc.field_5C), field_A0(desc.field_60), field_A4(desc.field_64),
      field_A8(desc.field_68)
{
    static const uint32_t kSounds[5] = {0x63D977D8u, 0x63D977D8u, 0x00000000u, 0x170F1CD3u, 0xEB5155EFu};
    for (int i = 0; i < 5; ++i)
        sounds[i] = audio::CreateSoundName(kSounds[i], 0);
}

// @func 0x001A7AB0 CreateChainDef
// The ChainDef constructor is inlined here.
EntityDef* CreateChainDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                          const EntityDef* parent)
{
    ChainDefDesc desc;
    desc.typeBits = 0x800;
    desc.name = loader.name;
    desc.geomName = loader.geomName;
    desc.templateName = loader.templateName;
    desc.flags[1] = true;
    desc.Parse(root, elem, loader, parent);
    return new ChainDef(desc);
}

// @func 0x001A7058 ChainDef::~ChainDef
// @func 0x001A70D4 ChainDef::~ChainDef (deleting)
ChainDef::~ChainDef()
{
    for (audio::SoundName* sound : sounds)
        audio::SoundNameRelease(sound);
}

// @func 0x001A7D8C ChainEntity::~ChainEntity
// @func 0x001A7EA8 ChainEntity::~ChainEntity (deleting)
ChainEntity::~ChainEntity()
{
    if (!spline)
        return;
    field_76C->Release();
    field_76C = nullptr;
    spline->Release();
    spline = nullptr;
    for (uint32_t i = 0; i < linkCount; ++i) {
        links[i].object->Release();
        links[i].object = nullptr;
    }
}

// @func 0x001A97D0 ChainEntity::PostUpdate
bool ChainEntity::PostUpdate(float /*dt*/)
{
    // The links are placed by the physics step; nothing to do here.
    return true;
}

// @func 0x001A7FE8 ChainEntity::Release
bool ChainEntity::Release(bool kill)
{
    return Entity::Release(kill);
}

// @func 0x00191E54 TargetableB::~TargetableB
// @func 0x00191FEC TargetableB::~TargetableB (deleting)
// @func 0x00192034 TargetableB::~TargetableB (deleting thunk, +0x180)
// @func 0x00192080 TargetableB::~TargetableB (deleting thunk, +0x17C)
// @func 0x001920CC TargetableB::~TargetableB (thunk, +0x180)
// @func 0x001920F4 TargetableB::~TargetableB (thunk, +0x17C)
TargetableB::~TargetableB()
{
    UnregisterEntity(kList_B, this);
    if (MinigameFlag30()) {
        if (parent && parent->IsAlive()) {
            TargetableB* oldParent = parent;
            parent = nullptr;
            oldParent->DetachChild(this);
        }
        for (TargetableB* child : children) {
            if (!child->IsAlive())
                continue;
            child->holder = nullptr;
            child->parent = nullptr;
            child->field_270 = 0;
            if (child->freezeWhenOrphaned)
                child->flags2 |= kEnt2_NoUpdate;
        }
    }
    if (holder) {
        holder->held = nullptr;
        holder = nullptr;
    }
}

ape::world::ApeObj* TargetableA::s_sharedObjects[TargetableA::kSharedObjects] = {};

namespace {
// Script message sent when a held entity is let go. NOTE: names unknown.
constexpr uint32_t kMsgKey_Dropped = 0x0C8B8AB8u;
constexpr uint32_t kMsg_Dropped = 0x00531BA0u;
} // namespace

// @func 0x00184280 TargetableA::~TargetableA
// @func 0x00184700 TargetableA::~TargetableA (deleting)
// @func 0x00184748 TargetableA::~TargetableA (deleting thunk, +0x180)
// @func 0x00184794 TargetableA::~TargetableA (deleting thunk, +0x17C)
// @func 0x001847E0 TargetableA::~TargetableA (thunk, +0x180)
// @func 0x00184808 TargetableA::~TargetableA (thunk, +0x17C)
TargetableA::~TargetableA()
{
    const bool minigame = MinigameFlag30();
    if (health > 0.0f && minigame && object && object->GetDef())
        MinigameFigureRemoved(object->GetDef()->GetAnimHash());
    UnregisterEntity(kList_B, this);
    field_68C.size = 0;

    // A held entity is put down where the figure stands, keeping its rotation.
    if (minigame && held && held->IsAlive()) {
        ape::Matrix34 placed = held->transform;
        placed.translation = { Position().x, Position().y, Position().z, placed.translation.pad };
        EntitySetWorldTransform(held, placed);
        held->holder = nullptr;
        held->field_270 = 0;
        EntityPostScriptMessage(this, kMsgKey_Dropped, kMsg_Dropped, 0);
    }
    if (held) {
        held->holder = nullptr;
        held = nullptr;
    }

    // The shared objects are dropped by every instance; the last one (the
    // first object still had a single reference) forgets them.
    bool last = false;
    if (s_sharedObjects[0]) {
        last = s_sharedObjects[0]->GetRefCount() == 1;
        s_sharedObjects[0]->Release();
    }
    for (int i = 1; i < kSharedObjects; ++i) {
        if (s_sharedObjects[i])
            s_sharedObjects[i]->Release();
    }
    if (last) {
        for (ape::world::ApeObj*& shared : s_sharedObjects)
            shared = nullptr;
    }

    if (object_530)
        object_530->Release();
    if (animScript524)
        AttachAnimScript(static_cast<ape::ApeAnimScriptInstance*>(animScript524));
}

// @func 0x0019324C TargetableB::InteractionOffer
uint32_t TargetableB::InteractionOffer(int kind, Entity* /*other*/)
{
    return Def()->targetingFlags[kind];
}

// @func 0x00193234 TargetableB::InteractionNeeds
uint32_t TargetableB::InteractionNeeds(int kind, Entity* /*other*/)
{
    return Def()->targetingFlags[kind];
}

// @func 0x00196768 TargetableB::Interface17CSlot5
// @func 0x0019676C TargetableB::Interface17CSlot5 (thunk, +0x17C)
void TargetableB::Interface17CSlot5(const void* event) { (void)event; }

// TODO(0x00191944 TargetableB::DetachChild)
void TargetableB::DetachChild(TargetableB* child)
{
    for (uint32_t i = 0; i < children.Count(); ++i) {
        if (children[i] == child) {
            children.RemoveAtSwap(i);
            break;
        }
    }
}

// @func 0x0016EC4C CompoundEntity::~CompoundEntity
// @func 0x0016EE20 CompoundEntity::~CompoundEntity (deleting)
// @func 0x0016EE68 CompoundEntity::~CompoundEntity (deleting thunk, +0x180)
// @func 0x0016EEB4 CompoundEntity::~CompoundEntity (deleting thunk, +0x17C)
// @func 0x0016EF00 CompoundEntity::~CompoundEntity (thunk, +0x180)
// @func 0x0016EF28 CompoundEntity::~CompoundEntity (thunk, +0x17C)
CompoundEntity::~CompoundEntity()
{
    DequeueEntityTransformCallback(this);
    UnregisterEntity(kList_B, this);
    for (Part* part : parts)
        delete part;
}

// @func 0x001728E8 CompoundEntity::Interface17CSlot5
// @func 0x001728EC CompoundEntity::Interface17CSlot5 (thunk, +0x17C)
void CompoundEntity::Interface17CSlot5(const void* event) { (void)event; }

// @func 0x00172B70 CompoundEntity::Interface17CSlot6
// @func 0x00172B74 CompoundEntity::Interface17CSlot6 (thunk, +0x17C)
void CompoundEntity::Interface17CSlot6() {}

} // namespace entity
} // namespace sly
