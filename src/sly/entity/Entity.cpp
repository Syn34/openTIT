// openTIT - Game entities: lists, lookups, components, children (see Entity.h).
#include "sly/entity/Entity.h"

#include "sly/entity/EntityHooks.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/PhysicsHooks.h"

#include "ape/container/ApeTSortedMap.h"
#include "ape/core/ApeString.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeRenderNode.h"
#include "ape/world/ApeSkeleton.h"
#include "ape/sys/ApeGeometry.h"
#include "ape/math/ApeMatrixOps.h"
#include "agl/AglRenderTarget.h"
#include "sly/camera/CameraHooks.h"
#include "sly/game/AudioSystem.h"
#include "sly/components/AnimComponent.h"

#include <cfloat>
#include <cmath>
#include <cstring>
#include <vector>

namespace sly {
namespace entity {

namespace {

constexpr uint32_t kUserTag_Entity = 0x02235C33u;  // HashNameNoCase("pEntity")
constexpr float kMinAnimDuration = 1e-5f;

// The four global entity lists (0x01013B34, 0x01013B48, 0x01013B5C, 0x01013B70).
std::vector<Entity*>& List(EntityList list)
{
    static std::vector<Entity*> lists[kListCount];
    return lists[list];
}

float DistanceSq(const ape::Vec3& a, const ape::Vec3& b)
{
    const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

template <class Fn>
void ForEachChild(const Entity* entity, Fn fn)
{
    if (!entity->children)
        return;
    for (EntityChildLink& link : *entity->children)
        fn(link);
}

template <class Fn>
void ForEachComponent(const Entity* entity, Fn fn)
{
    if (!entity->components)
        return;
    for (EntityComponent* component : *entity->components)
        fn(component);
}

struct NameSearch {
    uint32_t nameHash;
    ape::world::ApeObj* found;
};

} // namespace

// -- global lists -------------------------------------------------------------

void RegisterEntity(EntityList list, Entity* entity)
{
    List(list).push_back(entity);
}

// @func 0x000D68CC UnregisterEntityB
// @func 0x000D6940 UnregisterEntityC
// @func 0x000D69B4 UnregisterEntityD
// The console has one copy per list; all three remove the first match by
// moving the last entry into its place.
void UnregisterEntity(EntityList list, Entity* entity)
{
    std::vector<Entity*>& entries = List(list);
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i] == entity) {
            entries[i] = entries.back();
            entries.pop_back();
            return;
        }
    }
}

// @func 0x000D64F4 AddToInstanceRenderList
// Entities go to list B ("s_instRenderList" in the warning); a second add is
// refused.
void AddToInstanceRenderList(Entity* entity)
{
    std::vector<Entity*>& entries = List(kList_B);
    for (Entity* existing : entries) {
        if (existing == entity) {
            ape::TempFormat("entity '%s' added to s_instRenderList twice.  this may cause a crash on the next "
                            "frame.",
                            entity ? entity->GetDebugName() : "NULL");
            return;
        }
    }
    entries.push_back(entity);
}

// @func 0x000D6614 AddToInstanceRenderList
// NOTE: the console list holds objects of several classes; the port keeps
// them in the shared entity pointer list.
void AddToInstanceRenderList(EntityComponent* component)
{
    std::vector<Entity*>& entries = List(kList_C);
    Entity* entry = reinterpret_cast<Entity*>(component);
    for (Entity* existing : entries) {
        if (existing == entry) {
            ape::TempFormat("entity '%s' component '%s' added to s_instRenderList twice.  this may cause a "
                            "crash.",
                            component && component->owner && component->owner->object ? component->owner->object->GetName() : "NULL",
                            component ? ape::TempHexString(component->id) : "NULL");
            return;
        }
    }
    entries.push_back(entry);
}

void UnregisterObject(EntityList list, const void* object)
{
    std::vector<Entity*>& entries = List(list);
    for (size_t i = 0; i < entries.size(); ++i) {
        if (static_cast<const void*>(entries[i]) == object) {
            entries[i] = entries.back();
            entries.pop_back();
            return;
        }
    }
}

uint32_t EntityCount(EntityList list) { return uint32_t(List(list).size()); }
Entity* EntityAt(EntityList list, uint32_t index) { return List(list)[index]; }
void ClearEntityList(EntityList list) { List(list).clear(); }
void RemoveEntityAt(EntityList list, uint32_t index)
{
    std::vector<Entity*>& entries = List(list);
    entries[index] = entries.back();
    entries.pop_back();
}

// @func 0x000D6A28 FindEntity
Entity* FindEntity(uint32_t nameHash)
{
    if (nameHash == 0)
        return nullptr;
    for (Entity* entity : List(kList_All))
        if (entity->IsAlive() && entity->nameHash == nameHash)
            return entity;
    return nullptr;
}

// @func 0x000D6B68 FindNearestEntity
Entity* FindNearestEntity(const ape::Vec3& position)
{
    Entity* best = nullptr;
    float bestDistSq = FLT_MAX;
    for (Entity* entity : List(kList_All)) {
        const float d = DistanceSq(entity->Position(), position);
        if (d < bestDistSq) {
            bestDistSq = d;
            best = entity;
        }
    }
    return best;
}

// @func 0x000D6C08 FindNearestEntityOfDef
Entity* FindNearestEntityOfDef(uint32_t defName, const ape::Vec3& position)
{
    Entity* best = nullptr;
    float bestDistSq = FLT_MAX;
    for (Entity* entity : List(kList_All)) {
        if (!entity->def || entity->def->nameHash != defName)
            continue;
        const float d = DistanceSq(entity->Position(), position);
        if (d < bestDistSq) {
            bestDistSq = d;
            best = entity;
        }
    }
    return best;
}

// @func 0x000DD258 CollectEntitiesOfType
void CollectEntitiesOfType(uint32_t bit, ape::ApeTArray<Entity*>& out)
{
    for (Entity* entity : List(kList_All))
        if (entity->HasTypeBit(bit))
            out.PushBack(entity);
}

// @func 0x000D6AA0 EntityFromWorldObject
// The owning entity is stored as "pEntity" user data on its world object.
// Without a direct hit the search climbs to the parent (`searchParents`) or
// walks the sibling and child links (`searchLinks`).
Entity* EntityFromWorldObject(ape::world::ApeObj* object, bool searchParents, bool searchLinks)
{
    if (!object)
        return nullptr;
    if (reinterpret_cast<uintptr_t>(object->field_114) == kUserTag_Entity && object->field_118)
        return static_cast<Entity*>(object->field_118);
    if (searchParents)
        return EntityFromWorldObject(object->GetParent(), true, searchLinks);
    if (!searchLinks)
        return nullptr;
    if (Entity* entity = EntityFromWorldObject(object->GetFirstChild(), false, true))
        return entity;
    return EntityFromWorldObject(object->GetNextSibling(), false, true);
}

// -- Entity ---------------------------------------------------------------------

Entity::Entity()
{
    transform.xAxis = { 1.0f, 0.0f, 0.0f, 0.0f };
    transform.yAxis = { 0.0f, 1.0f, 0.0f, 0.0f };
    transform.zAxis = { 0.0f, 0.0f, 1.0f, 0.0f };
    transform.translation = { 0.0f, 0.0f, 0.0f, 1.0f };
}

// @func 0x000D3578 Entity::Entity
// @func 0x000D3B94 Entity::Entity (thunk)
Entity::Entity(const EntityInstanceDesc& desc)
    : objectRestoreTransform(desc.transform), transform(desc.transform), def(desc.def)
{
    GameObjectInit(this, desc.StateLayers(), desc.stateDataSize);
    nameHash = ape::HashNameNoCase(desc.name);

    // Entity flags: the player slot in the top byte, alive, and the
    // description's options.
    const uint32_t opts = desc.flags;
    flags = (uint32_t(desc.player) << 24) | kEnt_Alive | 0x00800000u;
    if (opts & EntityInstanceDesc::kDesc_PlacementBit29)
        flags |= kEnt_Hidden;
    if (opts & EntityInstanceDesc::kDesc_Bit9)
        flags |= kEnt_10000;
    if (opts & EntityInstanceDesc::kDesc_Bit8)
        flags |= kEnt_8000 | kEnt_4000;
    if (opts & EntityInstanceDesc::kDesc_NoObjectSync)
        flags |= kEnt_NoObjectSync;
    if (opts & EntityInstanceDesc::kDesc_PlacementBit31)
        flags |= kEnt_Visible;
    if (opts & EntityInstanceDesc::kDesc_PlacementBit30)
        flags |= kEnt_80;
    if (opts & EntityInstanceDesc::kDesc_NotListed)
        flags |= kEnt_NotListed;
    flags2 = kEnt2_RootMotionObjectScale;
    if (def && def->HasFlag(EntityDef::kDefFlag_Field20))
        flags2 |= kEnt2_4000;
    interactionCount = 1;
    classBits = 0;
    renderId = desc.renderId;
    field_D0 = desc.field_84 >= 0.0f ? desc.field_84 * desc.field_84 : -1.0f;
    cullDistance = 1.0f;
    field_DC = reinterpret_cast<void*>(uintptr_t(desc.field_80));
    typeBits[0] = desc.typeBits[0];
    typeBits[1] = desc.typeBits[1];
    typeBits[2] = desc.typeBits[2];

    EntitySetUpObject(this, desc);

    // Entities with a player slot are players.
    if (desc.player != 0xFF) {
        flags &= ~0x00C00000u;
        classBits |= 1;
    }
    if (!(flags & kEnt_NotListed))
        RegisterEntity(kList_All, this);

    renderId = desc.renderId;
    if (renderId == 0xFFFFFFFFu || renderId == 0)
        renderId = NewRenderId();

    if (def) {
        if (def->HasFlag(EntityDef::kDefFlag_Desc5))
            flags |= kEnt_Hidden;
        if (desc.attachments && desc.attachments->Count() != 0)
            EntityApplyAttachments(this, desc);
        flags2 = def->HasFlag(EntityDef::kDefFlag_Desc6) ? (flags2 | kEnt2_Highlight) : (flags2 & ~kEnt2_Highlight);
        // The definition's components, each built with this entity as owner.
        if (def->componentDescs) {
            for (ComponentDesc* componentDesc : *def->componentDescs) {
                componentDesc->owner = this;
                if (EntityComponent* component = CreateComponent(componentDesc)) {
                    if (!components)
                        components = new ape::ApeTArray<EntityComponent*>(4);
                    components->PushBack(component);
                }
                componentDesc->owner = nullptr;
            }
        }
    }

    if (EntityComponent* anim = GetComponent(kComp_Character))
        static_cast<components::AnimComponent*>(anim)->SetController(static_cast<anim::AnimController*>(animController));
    SetClassBits(0x400000000ull, true, true);
    SetClassBits(0x800000000ull, true, true);
}

// @func 0x000DF8B4 Entity::~Entity
// @func 0x000DF8E0 Entity::~Entity (deleting)
// @func 0x000D7024 Entity::~Entity (base part, vtable 0x00B12278)
// @func 0x000DF934 Entity::~Entity (copy, called by derived destructors)
// @func 0x000D729C Entity::~Entity (base part forwarding copy)
// @func 0x000D7254 Entity::~Entity (deleting, base vtable 0x00B12278)
// Hands the world object back: its original transform is restored when the
// entity had taken it over, it is hidden, tagged with the render id and
// released. The component, watcher and child arrays are freed (the
// components themselves are owned and released elsewhere).
Entity::~Entity()
{
    if (animController) {
        AnimControllerDestroy(animController);
        animController = nullptr;
    }
    if (flags2 & kEnt2_InListenerGroup)
        EntityLeaveListenerGroup(this);
    if (object) {
        if (flags & kEnt_RestoreObject) {
            object->SetLocal(objectRestoreTransform);
            object->MarkDirty();
            object->SetVisible(false, true);
        }
        WorldObjectSetUserData(object, kUserTag_ReleasedEntity, renderId);
        object->Release();
    }
    delete children;
    delete watchers;
    delete components;
}

// @func 0x000DAEC8 Entity::SetScriptState
void Entity::SetScriptState(uint32_t stateHash)
{
    if (void* script = GetAnimScript())
        ScriptInstanceSetState(script, stateHash);
}

// @func 0x000DBCF8 Entity::FindAnimClip
void* Entity::FindAnimClip(uint32_t nameHash)
{
    return AnimClipRegistryFind(nameHash);
}

// @func 0x000DBD20 Entity::FindAnimClipByName
void* Entity::FindAnimClipByName(const char* name)
{
    return FindAnimClip(ape::HashNameNoCase(name));
}

// @func 0x000DA348 Entity::SetAlpha
void Entity::SetAlpha(uint8_t alpha, bool includeChildren, bool flag, bool absolute)
{
    if (object && object->GetType() == ape::world::kObjType_GeomInstance) {
        uint8_t value = alpha;
        if ((flags2 & kEnt2_AlphaSet) && !absolute)
            value = static_cast<uint8_t>((uint32_t)alpha * EntityGeomByteA8(this) / 0xFF);
        object->VirtualSlot13(value, flag);
        flags2 |= kEnt2_AlphaSet;
    }
    if (includeChildren && children) {
        for (EntityChildLink& link : *children) {
            if (link.child)
                link.child->SetAlpha(alpha, includeChildren, absolute, false);
        }
    }
    ForEachComponent(this, [alpha, includeChildren](EntityComponent* c) { c->SetAlpha(alpha, includeChildren); });
}

namespace {
constexpr uint32_t kMsg_Ignored1 = 0x4F9AEC6Bu;
constexpr uint32_t kMsg_Component1D = 0x2FE47F7Fu;
constexpr uint32_t kMsg_Ignored2 = 0x2B68FE90u;
constexpr uint32_t kMsg_StartDC = 0x21595665u;
constexpr uint32_t kMsg_StopDC = 0x00C73980u;
constexpr uint32_t kMsg_ScriptState = 0xD77F98D6u;   // argument: state of the entity's anim script
} // namespace

// @func 0x000DAF24 Entity::HandleMessage
void Entity::HandleMessage(const EntityMessage& msg)
{
    switch (msg.id) {
    case kMsg_Ignored1:
    case kMsg_Ignored2:
        break;
    case kMsg_Component1D:
        if (EntityComponent* component = GetComponent(kComp_1D))
            Component1DOnMessage(component);
        break;
    case kMsg_StartDC:
        if (field_DC)
            EntityObjectDCStart(field_DC);
        break;
    case kMsg_StopDC:
        if (field_DC)
            EntityObjectDCStop(field_DC);
        break;
    case kMsg_ScriptState:
        if (void* script = GetAnimScript())
            ScriptInstanceSetState(script, msg.argument);
        break;
    default:
        break;
    }
    if (listener)
        listener->OnMessage(msg);
}

// @func 0x000D82BC Entity::OnSpawn
void Entity::OnSpawn()
{
    GameObjectSpawn(this);
    if (renderId == 0 || renderId == 0xFFFFFFFFu)
        renderId = NewRenderId();
    SetRenderId(renderId);
    ForEachComponent(this, [this](EntityComponent* c) { c->OnOwnerSpawned(this); });
    UpdateTransform();
}

// @func 0x000D83C4 Entity::Update
bool Entity::Update(float dt)
{
    if (flags2 & kEnt2_NoUpdate)
        return false;
    if (!GameObjectUpdate(this, dt))
        return false;
    ForEachComponent(this, [dt](EntityComponent* c) {
        if (c->enabled)
            c->Update(dt);
    });
    return true;
}

// @func 0x000D84C4 Entity::PostUpdate
bool Entity::PostUpdate(float dt)
{
    if (!GameObjectPostUpdate(this, dt))
        return false;
    ForEachComponent(this, [dt](EntityComponent* c) {
        if (c->enabled)
            c->PostUpdate(dt);
    });
    return true;
}

// @func 0x000D85A4 Entity::UpdateTransform
// Refreshes the world object (when children hang off an object without a
// render node), places the children unless nobody can see them, and applies
// a deferred visibility change.
void Entity::UpdateTransform(float /*dt*/)
{
    if (object && children && children->Count() != 0 && !object->GetRenderNode())
        object->UpdateWorld(false, false);

    if (object) {
        bool placeChildren = true;
        const bool geometry = object->GetType() == ape::world::kObjType_GeomInstance;
        if (!(flags & kEnt_08) && geometry &&
            !static_cast<const ape::world::ApeRenderObj*>(object)->IsDrawnThisFrame()) {
            // Invisible geometry only needs its children placed when a
            // visibility change, a highlight or a listener depends on them.
            placeChildren = (flags2 & (kEnt2_VisibilityPending | kEnt2_Highlight)) || listener;
        }
        if (placeChildren)
            UpdateChildTransforms();
    }

    if (flags2 & kEnt2_VisibilityPending) {
        ApplyVisibility((flags2 & kEnt2_PendingVisible) != 0);
        flags2 &= uint16_t(~(kEnt2_VisibilityPending | kEnt2_PendingVisible));
    }
}

namespace {
// Inverse of a rigid transform (orthonormal axes).
ape::Matrix34 InverseRigid(const ape::Matrix34& m)
{
    ape::Matrix34 inv;
    inv.xAxis = { m.xAxis.x, m.yAxis.x, m.zAxis.x, 0.0f };
    inv.yAxis = { m.xAxis.y, m.yAxis.y, m.zAxis.y, 0.0f };
    inv.zAxis = { m.xAxis.z, m.yAxis.z, m.zAxis.z, 0.0f };
    const ape::Vec3& t = m.translation;
    inv.translation = { -(t.x * m.xAxis.x + t.y * m.xAxis.y + t.z * m.xAxis.z),
                        -(t.x * m.yAxis.x + t.y * m.yAxis.y + t.z * m.yAxis.z),
                        -(t.x * m.zAxis.x + t.y * m.zAxis.y + t.z * m.zAxis.z), 0.0f };
    return inv;
}

// Frame of a locator in its entity's model space.
ape::Matrix34 LocatorModelTransform(const Entity* entity, const EntityLocator& locator)
{
    const ape::Matrix34 bone = entity->BoneTransform(locator.bone, false, true);
    ape::Matrix34 out;
    ape::world::Matrix34Mul(out, locator.transform, bone);
    return out;
}
} // namespace

// @func 0x000D7A00 Entity::UpdateChildTransforms
// Every attached child that does not place itself goes to its attach offset
// in the frame of the parent's locator, bone or object (or the parent's own
// frame), compensated by the child's own locator or bone so that this point
// of the child sits on the attach point. Rotation and scale can be kept.
void Entity::UpdateChildTransforms()
{
    if (!children)
        return;
    for (EntityChildLink& link : *children) {
        Entity* child = link.child;
        if (!child || (link.flags & kChild_NoTransform))
            continue;
        if (!child->object && !child->HasTypeBit(22))   // NOTE: type bit 22: placed without a world object
            continue;

        ape::Matrix34 local;
        ape::Matrix34FromQuat(local, link.rotation, link.offset);

        ape::Matrix34 frame = transform;
        if (link.parentLocator) {
            const ape::Matrix34 locatorFrame = LocatorModelTransform(this, *link.parentLocator);
            ape::world::Matrix34Mul(frame, locatorFrame, transform);
        } else if (link.parentBone == -1) {
            if (link.parentObject)
                frame = link.parentObject->GetWorld();
        } else {
            frame = BoneTransform(link.parentBone, true, true);
        }
        if (link.flags & kChild_ParentRotation) {
            frame.xAxis = transform.xAxis;
            frame.yAxis = transform.yAxis;
            frame.zAxis = transform.zAxis;
        }

        if (link.childLocator) {
            const ape::Matrix34 inv = InverseRigid(LocatorModelTransform(child, *link.childLocator));
            const ape::Matrix34 attach = local;
            ape::world::Matrix34Mul(local, attach, inv);
        } else if (link.childBone != -1) {
            const ape::Matrix34 inv = InverseRigid(child->BoneTransform(link.childBone, false, true));
            const ape::Matrix34 attach = local;
            ape::world::Matrix34Mul(local, attach, inv);
        }

        ape::Matrix34 placed;
        ape::world::Matrix34Mul(placed, local, frame);
        if (link.flags & kChild_KeepRotation) {
            placed.xAxis = child->transform.xAxis;
            placed.yAxis = child->transform.yAxis;
            placed.zAxis = child->transform.zAxis;
        }
        ape::Matrix34NormalizeAxes(placed);

        if (!(link.flags & kChild_KeepScale)) {
            static const ape::Vec3 kUnitScale = { 1.0f, 1.0f, 1.0f, 0.0f };   // 0x01164B30
            const ape::Vec3& parentScale = object ? object->GetScale() : kUnitScale;
            const ape::Vec3& childScale = child->object ? child->object->GetScale() : kUnitScale;
            if (parentScale.x != childScale.x || parentScale.y != childScale.y || parentScale.z != childScale.z)
                child->SetScale(parentScale, false);
        }
        child->SetTransform(placed);
        if (child->object) {
            ape::Matrix34Copy(child->object->m_local, placed);
            child->object->MarkDirty();
        }
    }
}

// @func 0x000D1288 Entity::GetComponent
EntityComponent* Entity::GetComponent(uint32_t type, EntityComponent* after) const
{
    if (!components)
        return nullptr;
    size_t start = 0;
    if (after) {
        // Resume after `after`; if it is not in the list the search starts
        // from the beginning again (the console does the same).
        for (size_t i = 0; i < components->Count(); ++i) {
            if ((*components)[i] == after) {
                start = i + 1;
                break;
            }
        }
    }
    for (size_t i = start; i < components->Count(); ++i)
        if ((*components)[i]->type == type)
            return (*components)[i];
    return nullptr;
}

// @func 0x000D92B0 Entity::FindComponentById
EntityComponent* Entity::FindComponentById(uint32_t id, EntityComponent* after) const
{
    if (!components)
        return nullptr;
    size_t start = 0;
    if (after) {
        for (size_t i = 0; i < components->Count(); ++i) {
            if ((*components)[i] == after) {
                start = i + 1;
                break;
            }
        }
    }
    for (size_t i = start; i < components->Count(); ++i)
        if ((*components)[i]->id == id)
            return (*components)[i];
    return nullptr;
}

// @func 0x000D135C Entity::AddComponent
EntityComponent* Entity::AddComponent(ComponentDesc* desc, bool registerIt)
{
    desc->owner = this;
    EntityComponent* component = CreateComponent(desc);
    if (component && registerIt) {
        if (!components)
            components = new ape::ApeTArray<EntityComponent*>(4);
        components->PushBack(component);
    }
    return component;
}

// @func 0x000D91BC Entity::RemoveComponent
void Entity::RemoveComponent(EntityComponent* component)
{
    if (!components)
        return;
    for (size_t i = 0; i < components->Count(); ++i) {
        if ((*components)[i] == component) {
            components->RemoveAtSwap(i);
            delete component;
        }
    }
}

// @func 0x000D90EC Entity::SetComponentsOption
void Entity::SetComponentsOption(uint32_t type, bool value)
{
    ForEachComponent(this, [type, value](EntityComponent* c) {
        if (c->type == type)
            c->SetOption(value);
    });
}

namespace {

ape::ApeTSortedMap<ComponentDesc*>& ComponentTemplates()
{
    static ape::ApeTSortedMap<ComponentDesc*> s_templates(0, nullptr, nullptr);
    return s_templates;
}

// Shared body of the three ReplaceComponent functions.
void ReplaceFromTemplate(Entity* entity, uint32_t templateName, uint32_t type)
{
    ComponentDesc* desc = FindComponentTemplate(templateName);
    if (!desc || desc->type != type)
        return;
    desc->owner = entity;
    if (EntityComponent* old = entity->GetComponent(type))
        entity->RemoveComponent(old);
    desc->owner = entity;
    if (EntityComponent* component = CreateComponent(desc)) {
        if (!entity->components)
            entity->components = new ape::ApeTArray<EntityComponent*>(4);
        entity->components->PushBack(component);
    }
    desc->owner = nullptr;
}

} // namespace

void RegisterComponentTemplate(uint32_t name, ComponentDesc* desc)
{
    ComponentTemplates().Add(name, desc);
}

ComponentDesc* FindComponentTemplate(uint32_t name)
{
    return *ComponentTemplates().Find(name);
}

void UnregisterComponentTemplate(uint32_t name)
{
    ComponentTemplates().Remove(name);
}

ComponentDesc::ComponentDesc(uint32_t nameHash, uint32_t componentType)
    : name(nameHash), type(componentType)
{
    RegisterComponentTemplate(name, this);
}

// @func 0x007374D8 ComponentDesc::~ComponentDesc
// @func 0x0073751C ComponentDesc::~ComponentDesc (deleting)
ComponentDesc::~ComponentDesc()
{
    UnregisterComponentTemplate(name);
}

// @func 0x000D99D8 Entity::ReplaceComponent12
void Entity::ReplaceComponent12(uint32_t templateName) { ReplaceFromTemplate(this, templateName, kComp_12); }
// @func 0x000D9B30 Entity::ReplaceCarryComponent
void Entity::ReplaceCarryComponent(uint32_t templateName) { ReplaceFromTemplate(this, templateName, kComp_Carry); }
// @func 0x000D9C88 Entity::ReplaceComponent1E
void Entity::ReplaceComponent1E(uint32_t templateName) { ReplaceFromTemplate(this, templateName, kComp_1E); }

// @func 0x000D1334 Entity::GetCharacterComponent
EntityComponent* Entity::GetCharacterComponent() { return GetComponent(kComp_Character); }
// @func 0x000D9384 Entity::GetComponent00
EntityComponent* Entity::GetComponent00() { return GetComponent(kComp_00); }
// @func 0x000D944C Entity::GetComponent0D
EntityComponent* Entity::GetComponent0D() { return GetComponent(kComp_0D); }

// -- children -----------------------------------------------------------------

// @func 0x000D155C Entity::FindChildLink
EntityChildLink* Entity::FindChildLink(const Entity* child) const
{
    if (!children)
        return nullptr;
    for (EntityChildLink& link : *children)
        if (link.child == child)
            return &link;
    return nullptr;
}

// @func 0x000D72C0 Entity::DetachChild
void Entity::DetachChild(Entity* child)
{
    if (!child || !children)
        return;
    size_t index = 0;
    while (index < children->Count() && (*children)[index].child != child)
        ++index;
    if (index == children->Count())
        return;

    const EntityChildLink& link = (*children)[index];
    if (!(link.flags & kChild_KeepOnDetach)) {
        child->Detached(link.restoreBlend);
        if (link.flags & kChild_RestoreState)
            child->SetRenderId(link.restoreState);
    }
    child->SetAttachParent(nullptr);
    // NOTE: the console also releases the object held by the child's anim
    // script component 0x8C3DFCAA when it is of kind 5; anim script components
    // are not modelled here.

    // Ordered removal (ApeTArray::RemoveAt, 0x0074534C).
    for (size_t i = index; i + 1 < children->Count(); ++i)
        (*children)[i] = (*children)[i + 1];
    children->RemoveAtSwap(children->Count() - 1);
}

// @func 0x000DB290 Entity::FindDescendant
Entity* Entity::FindDescendant(uint32_t name) const
{
    if (!children)
        return nullptr;
    for (const EntityChildLink& link : *children) {
        if (link.child->nameHash == name)
            return link.child;
        if (Entity* found = link.child->FindDescendant(name))
            return found;
    }
    return nullptr;
}

// @func 0x000D8F08 EntityApplyFade
// Flash/fade colour drawn over the entity's geometry this frame, and over
// the geometry of every entity attached to it.
void EntityApplyFade(Entity* entity, const uint8_t* color, bool additive)
{
    QueueObjectFlash(entity->object, color, additive);
    ForEachChild(entity, [color, additive](EntityChildLink& link) {
        EntityApplyFade(link.child, color, additive);
    });
}

// @func 0x000DA85C Entity::ObjectForCollidable
// Base version: the bodies of the entity's physics components (type 0)
// stand for its whole world object.
ape::world::ApeObj* Entity::ObjectForCollidable(const void* collidable, int32_t* bone)
{
    void* body = CollidableOwnerBody(collidable);
    if (!body || !components)
        return nullptr;
    for (EntityComponent* component : *components) {
        if (component && component->type == kComp_00 && PhysicsCompRigidBody(component) == body) {
            *bone = -1;
            return object;
        }
    }
    return nullptr;
}

// @func 0x000D6F90 ObjectSetTint
// Every LOD mesh of a geometry instance gets the colour and amount.
void ObjectSetTint(float amount, ape::world::ApeObj* object, const uint8_t color[4])
{
    if (!object || object->GetType() != ape::world::kObjType_GeomInstance)
        return;
    auto* def = static_cast<ape::world::ApeGeomDef*>(object->GetDef());
    for (uint32_t i = 0; i < def->m_lodCount; ++i) {
        if (def->m_lods[i].mesh)
            MeshSetTint(def->m_lods[i].mesh, color, amount);
    }
}

// @func 0x000CE688 Entity::BoneTransform
ape::Matrix34 Entity::BoneTransform(int bone, bool world, bool keepScale) const
{
    ape::Matrix34 out = world ? transform : ape::world::kWorldIdentity;
    if (object && world)
        out = object->GetWorld();
    if (!object || bone < 0)
        return out;
    ape::world::ApeSkeleton* skeleton = object->GetSkeleton();
    if (!skeleton)
        return out;
    const ape::Matrix34& model = skeleton->model[bone];
    if (!world)
        return model;
    if (keepScale) {
        ape::world::Matrix34Mul(out, model, object->GetWorld());
    } else {
        ape::Matrix34 frame;
        ape::Matrix34NormalizeAxes(frame, object->GetWorld());
        ape::world::Matrix34Mul(out, model, frame);
    }
    return out;
}

// @func 0x000DAABC Entity::BoneTransformByName
ape::Matrix34 Entity::BoneTransformByName(uint32_t boneHash, bool world, bool keepScale) const
{
    int bone = -1;
    if (boneHash != 0 && object) {
        if (ape::world::ApeSkeleton* skeleton = object->GetSkeleton())
            bone = skeleton->FindBone(boneHash);
    }
    return BoneTransform(bone, world, keepScale);
}

// @func 0x000DADA8 Entity::IsOffScreen
bool Entity::IsOffScreen(float margin) const
{
    agl::AglRenderTarget* target = camera::MainRenderTarget();
    if (!target)
        return false;
    float screen[4];
    target->WorldToScreen(screen, Position());
    return screen[0] < -margin || screen[0] > margin + 1.0f || screen[1] < -margin || screen[1] > margin + 1.0f;
}

// @func 0x000DD368 Entity::OnAnimEvent
void Entity::OnAnimEvent(const char* name, uint32_t argument, Entity* source)
{
    (void)argument;
    constexpr uint32_t kIgnored[] = { 0x5294E198u, 0x1B12498Fu, 0xD49B5F41u };   // NOTE: names unknown
    if (!source)
        return;
    const uint32_t hash = ape::HashNameNoCase(name);
    for (uint32_t ignored : kIgnored) {
        if (hash == ignored)
            return;
    }
    if (std::strncmp(name, "hkAttach", 8) != 0)
        return;
    const uint32_t part = ape::HashNameNoCase(name + 8);
    if (Entity* parent = source->attachParent) {
        if (EntityChildLink* link = parent->FindChildLink(this))
            link->partName = part;
    }
}

// @func 0x000DD4A0 EntityAnimEventListener
void EntityAnimEventListener(const char* name, uint32_t argument, void* user)
{
    if (Entity* entity = static_cast<Entity*>(user))
        entity->OnAnimEvent(name, argument, entity);
}

// @func 0x000DA25C Entity::Detached
// Base version: just tells the children.
void Entity::Detached(float blend)
{
    ForEachChild(this, [blend](EntityChildLink& link) {
        if (link.child)
            link.child->Detached(blend);
    });
}

// @func 0x000DAB90 Entity::SetCullDistance
void Entity::SetCullDistance(float distance, bool recurse)
{
    cullDistance = distance;
    CullManagerUpdate(object);
    if (!recurse)
        return;
    ForEachChild(this, [distance](EntityChildLink& link) {
        if (link.child)
            link.child->SetCullDistance(distance, true);
    });
}

// @func 0x007348D8 Entity::SetClassBits
void Entity::SetClassBits(uint64_t bits, bool on, bool recurse)
{
    SetClassBit(bits, on);
    if (!recurse)
        return;
    ForEachChild(this, [bits, on](EntityChildLink& link) {
        if (link.child)
            link.child->SetClassBits(bits, on, true);
    });
}

// @func 0x000D7904 Entity::SetScale
void Entity::SetScale(const ape::Vec3& scale, bool recurse)
{
    if (object)
        object->SetScale(scale, recurse);
    if (!recurse)
        return;
    ForEachChild(this, [&scale](EntityChildLink& link) {
        if (link.child)
            link.child->SetScale(scale, true);
    });
}

// @func 0x000D87CC Entity::SetLocalTransform
void Entity::SetLocalTransform(const ape::Matrix34& m)
{
    if (!object)
        return;
    ape::Matrix34& local = object->m_local;
    auto same = [](const ape::Vec3& a, const ape::Vec3& b) { return a.x == b.x && a.y == b.y && a.z == b.z; };
    if (!same(local.xAxis, m.xAxis) || !same(local.yAxis, m.yAxis) || !same(local.zAxis, m.zAxis) ||
        !same(local.translation, m.translation)) {
        ape::Matrix34Copy(local, m);
        object->MarkDirty();
    }
    flags |= kEnt_TransformChanged;
}

// @func 0x000DD880 Entity::GetLocalTransform
const ape::Matrix34& Entity::GetLocalTransform() const
{
    static const ape::Matrix34 kIdentity = {
        { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } };   // 0x01164BD0
    return object ? object->m_local : kIdentity;
}

// -- visibility and flags -------------------------------------------------------

// @func 0x000D9F1C Entity::SetVisible
void Entity::SetVisible(bool visible, bool immediate, bool recurseChildren, bool recurseComponents)
{
    if (object)
        WorldObjectSetVisible(object, visible, immediate);
    flags = visible ? (flags | kEnt_Visible) : (flags & ~uint32_t(kEnt_Visible));
    if (recurseChildren) {
        ForEachChild(this, [&](EntityChildLink& link) {
            if (link.child)
                link.child->SetVisible(visible, immediate, recurseChildren, recurseComponents);
        });
    }
    if (recurseComponents)
        ForEachComponent(this, [&](EntityComponent* c) { c->SetVisible(visible, immediate); });
}

// @func 0x000DA0CC Entity::SetFlag80
void Entity::SetFlag80(bool on, int part)
{
    flags = on ? (flags | kEnt_80) : (flags & ~uint32_t(kEnt_80));
}

// @func 0x000DB258 Entity::SetFlag1
void Entity::SetFlag1(bool on)
{
    if (!on)
        flags2 &= uint16_t(~kEnt2_SoundsMuted);
    flags = on ? (flags | kEnt_00000001) : (flags & ~uint32_t(kEnt_00000001));
}

// @func 0x000DB0C0 Entity::SetActive
void Entity::SetActive(bool active)
{
    flags = active ? (flags | kEnt_Active) : (flags & ~uint32_t(kEnt_Active));
    SetActiveVisual(active);
    if (active)
        flags = (flags & ~uint32_t(kEnt_4000)) | kEnt_8000;
    else
        flags &= ~uint32_t(kEnt_4000 | kEnt_8000);
}

// @func 0x000DA138 Entity::SetRenderId
void Entity::SetRenderId(uint32_t id)
{
    // NOTE: the console also writes the id into bits 16-31 of the render
    // object's word +0x2C and re-sorts it in its render list (0x00930B70).
    renderId = id;
    ForEachChild(this, [id](EntityChildLink& link) {
        if (link.child)
            link.child->SetRenderId(id);
    });
}

// @func 0x000DA0DC Entity::GetRenderId
// NOTE: the console reads the id back from the render object (+0x2C bits
// 16-31) when there is one; SetRenderId keeps both in step.
uint32_t Entity::GetRenderId()
{
    return renderId;
}

// @func 0x000DA550 Entity::SetHighlight
void Entity::SetHighlight(bool on, bool recurse)
{
    if (object && object->GetType() == ape::world::kObjType_GeomInstance && GeomGetHighlight(object) != on)
        GeomSetHighlight(object, on);
    if (!recurse)
        return;
    ForEachChild(this, [on](EntityChildLink& link) {
        if (link.child)
            link.child->SetHighlight(on, true);
    });
}

// @func 0x000DA684 Entity::SetTimeScale
void Entity::SetTimeScale(uint32_t value, bool recurse)
{
    if (recurse) {
        ForEachChild(this, [value](EntityChildLink& link) {
            if (link.child)
                link.child->SetTimeScale(value, true);
        });
    }
    ForEachComponent(this, [value, recurse](EntityComponent* c) { c->OnTimeScale(value, recurse); });
}

// @func 0x000DA31C EntityGeomByteA8
uint8_t EntityGeomByteA8(const Entity* entity)
{
    ape::world::ApeObj* object = entity->object;
    if (!object || object->GetType() != ape::world::kObjType_GeomInstance)
        return 0;
    return GeomByteA8(object);
}

// @func 0x000DA7C8 Entity::GetHudScript
// NOTE: returns field +0x08 of the anim script's +0x04 object (the script's
// owner record); its type is not modelled, so null is returned until anim
// scripts expose it.
void* Entity::GetHudScript()
{
    return nullptr;
}

// Only characters have a separate render object.
// @func 0x00747054 Entity::GetRenderObject
ape::world::ApeObj* Entity::GetRenderObject()
{
    return nullptr;
}

// @func 0x007372FC Entity::GetDebugName
const char* Entity::GetDebugName()
{
    return object ? object->GetName() : "<unknown>";
}

// @func 0x007373D4 Entity::AddWatcher
bool Entity::AddWatcher(void* watcher)
{
    if (!watchers)
        watchers = new ape::ApeTArray<void*>();
    watchers->PushBack(watcher);
    return true;
}

// @func 0x000D9E48 Entity::GetCenter
ape::Vec3 Entity::GetCenter()
{
    ape::Vec3 center = Position();
    if (EntityComponent* c = GetComponent(kComp_Center)) {
        const ape::Vec3 offset = CenterComponentOffset(c);
        center.x += offset.x;
        center.y += offset.y;
        center.z += offset.z;
    }
    return center;
}

// @func 0x000DAC64 Entity::GetBounds
ape::Aabb Entity::GetBounds(bool world)
{
    ape::Aabb box;
    box.min = { FLT_MAX, FLT_MAX, FLT_MAX, 0.0f };
    box.max = { -FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f };
    if (!object) {
        // A point: the entity position in world space, the origin locally.
        static const ape::Vec3 kZero = { 0.0f, 0.0f, 0.0f, 0.0f };
        ape::AabbFromPoint(box, world ? Position() : kZero);
        return box;
    }
    const ape::world::ApeDef* objectDef = object->GetDef();
    if (world) {
        // Definition box carried into world space (0x005772E4).
        ape::AabbTransform(box, objectDef->GetBounds(), object->GetWorld());
        return box;
    }
    // Local box: the definition box scaled by the object scale.
    box = objectDef->GetBounds();
    ape::AabbScale(box, object->m_scale);
    return box;
}

// @func 0x000DAE6C Entity::DistanceToCamera
float Entity::DistanceToCamera() const
{
    return std::sqrt(DistanceSq(Position(), GameCameraPosition()));
}

// -- skeleton -------------------------------------------------------------------

// @func 0x000D14DC Entity::FindBone
int Entity::FindBone(uint32_t boneName) const
{
    if (boneName == 0 || !object)
        return -1;
    const ape::world::ApeSkeleton* skeleton = object->GetSkeleton();
    return skeleton ? skeleton->FindBone(boneName) : -1;
}

// @func 0x000DA954 Entity::GetBone
ape::world::ApeSkeletonBone* Entity::GetBone(int index) const
{
    if (!object || index < 0)
        return nullptr;
    ape::world::ApeSkeleton* skeleton = object->GetSkeleton();
    return skeleton ? &skeleton->bones[index] : nullptr;
}

// @func 0x000DA9D0 Entity::GetBoneByName
ape::world::ApeSkeletonBone* Entity::GetBoneByName(uint32_t boneName) const
{
    return GetBone(FindBone(boneName));
}

// @func 0x000F8528 MatchObjectName
// @func 0x000F85BC MatchObjectName (copy 2)
// @func 0x004D0ABC MatchObjectName (copy 3)
// @func 0x004D0B50 MatchObjectName (copy 4)
// Hierarchy visitor: stops (returns false) at the object whose name hash, or
// hash of its name when no hash is stored, equals the one searched for.
static bool MatchObjectName(ape::world::ApeObj* object, void* arg)
{
    NameSearch* search = static_cast<NameSearch*>(arg);
    uint32_t hash = object->GetNameHash();
    if (hash != search->nameHash && hash == 0)
        hash = ape::HashNameNoCase(object->GetName());
    if (hash == search->nameHash) {
        search->found = object;
        return false;
    }
    return true;
}

// @func 0x000D1454 EntityFindObject
// The entity's world object if it carries that name, else the first object
// below it with that name.
ape::world::ApeObj* EntityFindObject(const Entity* entity, uint32_t name)
{
    ape::world::ApeObj* object = entity->object;
    if (!object)
        return nullptr;
    if (object->GetNameHash() == name)
        return object;
    if (!object->GetFirstChild())
        return nullptr;
    NameSearch search{ name, nullptr };
    object->ForEachInHierarchy(MatchObjectName, &search, false);
    return search.found;
}

// @func 0x000F8650 CompareObjectNames
// Sort predicate on two world objects by name.
int CompareObjectNames(ape::world::ApeObj* const* a, ape::world::ApeObj* const* b)
{
    return ape::StrCmp((*a)->GetName(), (*b)->GetName());
}

// -- sounds and animations ---------------------------------------------------------

// @func 0x000DBC28 Entity::FindSoundVoice
uint32_t Entity::FindSoundVoice(uint32_t soundHash) const
{
    for (uint32_t i = 0; i < SoundSlotCount(); ++i)
        if (audio::VoiceSoundHash(sounds[i].voice) == soundHash)
            return sounds[i].voice;
    return 0;
}

// @func 0x000DB180 Entity::SetSoundsMuted
void Entity::SetSoundsMuted(bool muted)
{
    if (muted) {
        for (uint32_t i = 0; i < SoundSlotCount(); ++i) {
            audio::StopVoice(sounds[i].voice, true);
            sounds[i].voice = 0;
            sounds[i].attachObject = nullptr;
            sounds[i].id = -1;
        }
    }
    flags2 = muted ? uint16_t(flags2 | kEnt2_SoundsMuted) : uint16_t(flags2 & ~kEnt2_SoundsMuted);
}

// @func 0x000DD02C Entity::SetVisibilityDeferred
// A deferred change is applied by the next UpdateTransform.
void Entity::SetVisibilityDeferred(bool visible, bool deferred)
{
    if (!deferred) {
        flags2 &= uint16_t(~kEnt2_VisibilityPending);
        ApplyVisibility(visible);
        return;
    }
    flags2 = uint16_t((flags2 & ~kEnt2_PendingVisible) | (visible ? kEnt2_PendingVisible : 0) |
                      kEnt2_VisibilityPending);
}

// @func 0x000DD7B8 Entity::UpdateWorldTransform
void Entity::UpdateWorldTransform(bool recurse)
{
    if (object)
        object->UpdateWorld(false, false);
    if (!recurse)
        return;
    ForEachChild(this, [](EntityChildLink& link) {
        if (link.child)
            link.child->UpdateWorldTransform(true);
    });
}

// @func 0x000DD1CC Entity::RemoveWatcher
void Entity::RemoveWatcher(void* watcher)
{
    if (!watchers)
        return;
    for (size_t i = 0; i < watchers->Count(); ++i) {
        if ((*watchers)[i] == watcher) {
            watchers->RemoveAtSwap(i);
            return;
        }
    }
}

// @func 0x000D9DE0 Entity::SetName
void Entity::SetName(const char* name)
{
    if (object) {
        object->SetName(name, true);
        nameHash = object->GetNameHash();
    } else {
        nameHash = ape::HashNameNoCase(name);
    }
}

void* Entity::FindPlayingAnim(uint32_t anim) const
{
    return animController ? AnimFindPlaying(animController, anim) : nullptr;
}

// @func 0x000DC418 Entity::GetAnimDuration
float Entity::GetAnimDuration(uint32_t anim) const
{
    void* playing = FindPlayingAnim(anim);
    return playing ? AnimDuration(playing) : 0.0f;
}

// @func 0x000DC488 Entity::GetAnimElapsed
// With `normalized` the time is returned as a fraction of the length
// (1 for animations too short to measure).
float Entity::GetAnimElapsed(bool normalized, uint32_t anim) const
{
    void* playing = FindPlayingAnim(anim);
    if (!playing)
        return 0.0f;
    const float elapsed = AnimElapsed(playing);
    if (!normalized)
        return elapsed;
    const float duration = AnimDuration(playing);
    return duration > kMinAnimDuration ? elapsed / duration : 1.0f;
}

// @func 0x000DC6A0 Entity::GetAnimProgress
float Entity::GetAnimProgress(uint32_t anim) const
{
    void* playing = FindPlayingAnim(anim);
    return playing ? AnimElapsed(playing) / AnimDuration(playing) : 0.0f;
}

// @func 0x000DC550 Entity::SetAnimElapsed
void Entity::SetAnimElapsed(float time, bool normalized, uint32_t anim)
{
    ForEachChild(this, [&](EntityChildLink& link) {
        if (link.flags & kChild_ForwardAnim)
            link.child->SetAnimElapsed(time, normalized, anim);
    });
    void* playing = FindPlayingAnim(anim);
    if (!playing)
        return;
    if (normalized) {
        const float duration = AnimDuration(playing);
        time = duration > kMinAnimDuration ? duration * time : 1.0f;
    }
    AnimSetElapsed(playing, time);
}

// @func 0x000DBD7C Entity::SetAnimWeight
void Entity::SetAnimWeight(float weight, uint32_t anim)
{
    ForEachChild(this, [&](EntityChildLink& link) {
        if (link.flags & kChild_ForwardAnim)
            link.child->SetAnimWeight(weight, anim);
    });
    if (void* playing = FindPlayingAnim(anim))
        AnimSetWeight(playing, weight);
}

// @func 0x000DC314 Entity::SetAnimPaused
void Entity::SetAnimPaused(uint32_t anim, bool paused)
{
    ForEachChild(this, [&](EntityChildLink& link) {
        if (link.flags & kChild_ForwardAnim)
            link.child->SetAnimPaused(anim, paused);
    });
    if (void* playing = FindPlayingAnim(anim))
        AnimSetPaused(playing, paused);
}

// @func 0x000D86F8 Entity::IsAnimFinished
bool Entity::IsAnimFinished(uint32_t anim) const
{
    void* playing = FindPlayingAnim(anim);
    return playing && AnimIsFinished(playing);
}

// @func 0x000D8760 Entity::StopAnim
// True (nothing to do) when the animation is not playing.
bool Entity::StopAnim(uint32_t anim, bool blend)
{
    void* playing = FindPlayingAnim(anim);
    return playing ? AnimStop(playing, blend) : true;
}

} // namespace entity
} // namespace sly

namespace sly {
namespace entity {

// @func 0x000DF960 Entity::CanInteractWith
// Kind-3 interactions need an enabled interaction component with at most one
// user. When the other side is a player (class bit 0), what this entity
// needs from it must all be offered by it.
bool Entity::CanInteractWith(Entity* other, int kind, float* priority)
{
    if (kind == kInteract_Player) {
        auto* comp = static_cast<InteractionComponent*>(GetComponent(kComp_Interaction));
        if (!comp || !comp->enabled || comp->users > 1)
            return false;
    }
    if (listener && !listener->AllowsInteraction())
        return false;
    if (other && (other->classBits & 1)) {
        const uint32_t needs = InteractionNeeds(kind, other);
        if (needs != 0)
            return (other->InteractionOffer(kind, this) & needs) == needs;
    }
    return true;
}

// @func 0x000DFAC8 Entity::OnInteractStart
void Entity::OnInteractStart(Entity* other, int kind)
{
    AddRef();
    if (kind != kInteract_Player || !(other->classBits & 1))
        return;
    if (auto* comp = static_cast<InteractionComponent*>(GetComponent(kComp_Interaction)))
        comp->playerMask |= 1u << other->PlayerSlot();
}

// @func 0x000DFB48 Entity::OnInteractEnd
void Entity::OnInteractEnd(Entity* other, int kind)
{
    Release(false);
    if (kind != kInteract_Player || !(other->classBits & 1))
        return;
    if (auto* comp = static_cast<InteractionComponent*>(GetComponent(kComp_Interaction)))
        comp->playerMask &= ~(1u << other->PlayerSlot());
}

// @func 0x000D74C0 Entity::Release
// Drops one reference. With `kill` the entity also leaves the world: it is
// no longer alive, the game world, its watchers and its spawner forget it, it
// leaves its parent, its children are released (killed too when asked), its
// components are deleted, its sounds stopped and it is cleared from the
// global target tables and the second entity list.
// NOTE: when the count goes negative the console only fetches the name
// (slot 19) for a stripped warning.
bool Entity::Release(bool kill)
{
    if (--interactionCount < 0)
        return true;
    if (!kill)
        return true;

    flags &= ~kEnt_Alive;
    GameWorldForgetEntity(this);

    if (watchers) {
        for (void* watcher : *watchers)
            WatcherEntityReleased(watcher, this);
        watchers->Clear();
    }
    if (field_DC) {
        SpawnerEntityReleased(field_DC, this);
        field_DC = nullptr;
    }
    if (attachParent)
        attachParent->DetachChild(this);

    if (children) {
        for (EntityChildLink& link : *children) {
            if (Entity* child = link.child) {
                child->SetAttachParent(nullptr);
                child->Release(kill);
            }
        }
        children->Clear();
    }

    // The first component is taken each time and the last moved into its slot.
    while (components && components->Count() != 0) {
        EntityComponent* component = (*components)[0];
        const uint32_t last = components->Count() - 1u;
        components->PopBack();
        if (last != 0)
            (*components)[0] = (*components)[last];
        delete component;
    }

    for (uint32_t i = 0; i < SoundSlotCount(); ++i) {
        audio::StopVoice(sounds[i].voice, true);
        sounds[i].voice = 0;
        sounds[i].attachObject = nullptr;
        sounds[i].id = -1;
    }

    ClearEntityFromTargetTables(this);
    UnregisterEntity(kList_B, this);
    return true;
}

} // namespace entity
} // namespace sly

namespace sly {
namespace entity {

// @func 0x000CFEC4 Entity::SetSuspended
void Entity::SetSuspended(bool suspended)
{
    if (((flags & kEnt_Suspended) != 0) == suspended)
        return;
    flags = suspended ? (flags | kEnt_Suspended) : (flags & ~uint32_t(kEnt_Suspended));
    if (suspended)
        OnSuspend();
    else
        OnResume();
}

namespace {

bool HasRenderBlock(ape::world::ApeObj* object)
{
    if (!object)
        return false;
    const ape::world::ApeRenderNode* node = ape::world::GetRenderNodeOf(*object);
    return node && node->field_88;
}

} // namespace

// @func 0x000DD504 Entity::OnSuspend
void Entity::OnSuspend()
{
    if (EntityComponent* c = GetComponent0D())
        c->SetOption(false);
    if (EntityComponent* c = GetComponent00())
        c->SetOption(false);
    if (HasRenderBlock(object))
        RenderNodeSetHidden(object, true);
}

// @func 0x000DD620 Entity::OnResume
// The render node stays hidden only if component 0x20 (NOTE: a visibility
// controller, its byte +0x140 bit 7) asks for it.
void Entity::OnResume()
{
    if (EntityComponent* c = GetComponent0D())
        c->SetOption(true);
    if (EntityComponent* c = GetComponent00())
        c->SetOption(true);
    if (HasRenderBlock(object)) {
        bool hidden = false;
        if (EntityComponent* c = GetComponent(kComp_20))
            hidden = ComponentHidesRender(c);
        RenderNodeSetHidden(object, hidden);
    }
}

// @func 0x000DD0A4 Entity::ApplyVisibility
void Entity::ApplyVisibility(bool visible)
{
    flags = visible ? (flags & ~uint32_t(kEnt_Hidden)) : (flags | kEnt_Hidden);
    if (ForwardVisibilityToComponents()) {
        if (EntityComponent* c = GetComponent0D())
            c->SetOption(visible);
    }
    if (!GetComponent(kComp_20) && HasRenderBlock(object))
        RenderNodeSetHidden(object, !visible);
}

// @func 0x000D2E68 ClearRenderRefs
void ClearRenderRefs(ape::world::ApeObj* object, bool recurse)
{
    if (!object)
        return;
    if (ape::world::ApeRenderNode* node = ape::world::GetRenderNodeOf(*object))
        ape::world::ClearRenderNodeRefs(node);
    if (!recurse)
        return;
    for (ape::world::ApeObj* child = object->GetFirstChild(); child; child = child->GetNextSibling())
        ClearRenderRefs(child, true);
}

// @func 0x00734EC8 Entity::SetPosition
void Entity::SetPosition(const ape::Vec3& position)
{
    ape::Matrix34 m;
    ape::Matrix34FromRotationTranslation(m, transform, position);
    SetTransform(m);
}

} // namespace entity
} // namespace sly
