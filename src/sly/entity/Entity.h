// openTIT - Game entities: the base class of every gameplay object.
//
// An entity wraps a world object (an ApeObj instance from the level data or
// spawned at run time), an optional animation controller, a list of
// components (character, AI, weapon, targeting, ...) and a list of attached
// child entities. Entities are spawned from definitions (EntityDef) that are
// registered by name in a sorted map (0x01013B04), and every live entity is
// kept in the global entity list (0x01013B34).
//
// The console class derives from the engine's game object (vtable 0x00B219D0,
// code at 0x00464xxx); that base is reached through EntityHooks.h. Only the
// virtual slots that this module implements or calls are declared; the slot
// number on the console vtable (0x00B12550) is given next to each.
#pragma once

#include "ape/math/ApeBounds.h"
#include "ape/math/ApeMath.h"
#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace ape::world { class ApeObj; struct ApeSkeletonBone; }

namespace sly {
namespace entity {

class Entity;
class EntityComponent;

// Component type numbers (EntityComponent::type, +0x0C). Only the ones whose
// role is known from their users are named.
enum ComponentType : uint32_t {
    kComp_00         = 0x00,
    kComp_01         = 0x01,
    kComp_Character  = 0x02,   // virtual slot 15 of Entity
    kComp_03         = 0x03,
    kComp_04         = 0x04,
    kComp_0D         = 0x0D,
    kComp_0E         = 0x0E,
    kComp_11         = 0x11,
    kComp_12         = 0x12,
    kComp_Skin       = 0x13,   // active skin (0x000D94F4)
    kComp_14         = 0x14,
    kComp_17         = 0x17,   // animation driver (state 7 = "playing script")
    kComp_Targeting  = 0x18,   // targeting gadget
    kComp_Gadget     = 0x1A,   // player gadget (binocucom)
    kComp_Interaction = 0x1B,
    kComp_Carry      = 0x1C,
    kComp_1D         = 0x1D,
    kComp_1E         = 0x1E,
    kComp_Center     = 0x1F,   // offset of the entity's centre from its origin
    kComp_20         = 0x20,
    kComp_25         = 0x25,
};

// Description a component is built from (owner filled in by AddComponent).
struct ComponentDesc {
    ComponentDesc() = default;
    // Named description of a component type, entered in the template table
    // (the base part every XML component parser inlines).
    ComponentDesc(uint32_t nameHash, uint32_t componentType);
    // Leaves the named template table (descriptions built from XML templates
    // are registered under 'name').
    virtual ~ComponentDesc();
    uint32_t name = 0;         // 0x04 template name hash
    Entity* owner = nullptr;   // 0x08 set while a component is built from it
    uint32_t type = 0;         // 0x0C ComponentType of the component it builds
    uint8_t scope = 0;         // 0x10 load scope byte handed to the parser (0xFE = current). NOTE: role assumed
};

// Base of all entity components.
class EntityComponent {
public:
    virtual ~EntityComponent() = default;                          // slots 0/1
    virtual void OnOwnerSpawned(Entity* owner) { (void)owner; }    // slot 5 (+0x14)
    virtual void Update(float dt) { (void)dt; }                    // slot 6 (+0x18)
    virtual void PostUpdate(float dt) { (void)dt; }                // slot 7 (+0x1C)
    // NOTE: slot 11 (+0x2C); the flag's meaning depends on the component.
    virtual void SetOption(bool value) { (void)value; }
    virtual void SetVisible(bool visible, bool immediate) { (void)visible; (void)immediate; } // slot 12
    virtual void SetAlpha(uint8_t alpha, bool children) { (void)alpha; (void)children; }      // slot 14
    virtual void OnTimeScale(uint32_t value, bool recurse) { (void)value; (void)recurse; }    // slot 15

    uint32_t id = 0;          // 0x04 per-entity identifier (name hash of the component)
    Entity* owner = nullptr;  // 0x08
    uint32_t type = 0;        // 0x0C ComponentType
    bool enabled = true;      // 0x10 updated by the owner when set
};

// Interaction component (type kComp_Interaction).
class InteractionComponent : public EntityComponent {
public:
    int32_t users = 0;          // 0x44 NOTE: interactions in progress
    uint32_t playerMask = 0;    // 0x58 bit per player slot currently interacting
};

// Interaction kinds passed to CanInteractWith / OnInteractStart / OnInteractEnd.
// NOTE: only kind 3 (player interaction prompt) is special-cased in this module.
constexpr int kInteract_Player = 3;

// One attached child entity (console record of 0xF0 bytes in the array at +0x154).
struct EntityChildLink {
    ape::Vec3 offset;                    // 0x80 attach offset
    ape::Vec4 rotation;                  // 0x90 attach rotation (quaternion)
    uint32_t partName = 0;               // 0xA0 attach part / slot name hash
    uint32_t flags = 0;                  // 0xB0 EntityChildFlags
    Entity* child = nullptr;             // 0xC0
    void* parentLocator = nullptr;       // 0xC4 locator on the parent, or null
    int32_t parentBone = -1;             // 0xC8 bone of the parent, -1 for none
    void* field_CC = nullptr;            // 0xCC NOTE: object whose world matrix is used when no bone
    void* childLocator = nullptr;        // 0xD0 locator on the child, or null
    int32_t childBone = -1;              // 0xD4
    uint32_t restoreState = 0;           // 0xDC passed back to the child on detach
    float restoreBlend = 0.0f;           // 0xE0
};

enum EntityChildFlags : uint32_t {
    kChild_NoTransform     = 1u << 24,  // the child places itself
    kChild_RestoreState    = 1u << 25,  // re-apply restoreState when detached
    kChild_KeepOnDetach    = 1u << 26,  // do not restore the blend on detach
    kChild_ParentRotation  = 1u << 30,  // use the parent's rotation, ignore the bone's
    kChild_ForwardAnim     = 1u << 31,  // anim layer weights are forwarded to the child
};

// Entity flags (+0xAC).
enum EntityFlags : uint32_t {
    kEnt_00000001   = 0x00000001,  // NOTE: slot 45 toggles it together with flags2 bit 4
    kEnt_Active     = 0x00000002,  // slot 44
    kEnt_Suspended  = 0x00000004,
    kEnt_08         = 0x00000008,  // transform owned by someone else (no child update)
    kEnt_80         = 0x00000080,  // slot 23
    kEnt_Visible    = 0x00000100,
    kEnt_RestoreObject = 0x00000400,  // restore the world object's transform when destroyed
    kEnt_TransformChanged = 0x00000800,
    kEnt_4000       = 0x00004000,
    kEnt_8000       = 0x00008000,
    kEnt_SoundCountShift = 19,     // bits 19-21: number of sound slots in use
    kEnt_SoundCountMask  = 0x7u << 19,
    kEnt_Hidden     = 0x00020000,  // bit 17: set by ApplyVisibility(false)
    kEnt_Alive      = 0x00040000,  // bit 18: registered and alive
};

// Second flag word (+0xB0, 16 bits).
enum EntityFlags2 : uint16_t {
    kEnt2_SoundsMuted     = 0x0010,
    kEnt2_AlphaSet        = 0x0020,  // an opacity has been applied (later ones scale the geometry alpha)
    kEnt2_Highlight       = 0x0040,
    kEnt2_PendingVisible  = 0x0080,  // value applied with kEnt2_VisibilityPending
    kEnt2_VisibilityPending = 0x0100,
    kEnt2_NoUpdate        = 0x1000,
    kEnt2_InListenerGroup = 0x2000,  // registered in an event listener group (0x0048F298 removes it)
};

// Class bits (+0xB8, 64 bits on the console).
enum EntityClassBits : uint64_t {
    kClass_Character = 0x08,
    kClass_20        = 0x20,
    kClass_400000    = 0x400000,     // has a transform even without a world object
};

// One sound attached to an entity (+0x100, stride 0x0C).
struct EntitySoundSlot {
    uint32_t voice = 0;       // audio::VoiceId
    int32_t id = -1;
    uint32_t field_08 = 0;
};

// Attachment record a definition adds to every spawn (0xC0 bytes on the
// console). NOTE: contents not decoded; copied as a whole.
struct SpawnAttachmentDesc {
    uint8_t data[0xC0];
};

// Parameters of one spawn (filled by the caller, completed from the definition).
struct SpawnParams {
    ape::ApeTArray<SpawnAttachmentDesc>* attachments = nullptr;   // 0x10
    ape::world::ApeObj* sourceObject = nullptr;   // 0x70 placed object the entity is built from, or null
    char name[0x40] = {};                         // 0x8C requested instance name (may be empty)
    class EntityDef* def = nullptr;          // 0x78
    uint32_t defField24 = 0;                 // 0x7C
    uint32_t defField28 = 0;                 // 0x80
    bool flag20000000 = false;               // 0x84 bit 29, from the definition's option bit 2
};

// Entity definition (registered by name in the definition map 0x01013B04).
class EntityDef {
public:
    virtual ~EntityDef();
    virtual Entity* Spawn(SpawnParams& params) { (void)params; return nullptr; }   // slot 7 (+0x1C)

    // Name for a new instance: the requested name, else the source object's
    // name, else "<def name>_instNNNNN".
    const char* InstanceName(const SpawnParams& params);
    const char* MakeInstanceName();
    // Component description of `type` after `after` (or from the start).
    ComponentDesc* FindComponentDesc(uint32_t type, ComponentDesc* after = nullptr) const;

    int32_t instanceCounter = 0;             // 0x0C
    ape::ApeTArray<ComponentDesc*>* componentDescs = nullptr;   // 0x10
    uint32_t optionFlags = 0;                // 0x14 NOTE: bit 0x200000 = "loot/drop" type (see users)
    uint8_t options = 0;                     // 0x20
    uint32_t field_24 = 0;                   // 0x24
    uint32_t field_28 = 0;                   // 0x28
    uint32_t nameHash = 0;                   // 0x34
    char name[0x20] = {};                    // 0x38
    char* field_80 = nullptr;                // 0x80 NOTE: owned buffer, contents unknown
    ape::ApeTArray<SpawnAttachmentDesc>* attachments = nullptr;   // 0x88
};
void UnregisterEntityDef(EntityDef* def);

class Entity {
public:
    Entity();
    virtual ~Entity();

    // -- virtual interface (console vtable 0x00B12550) --------------------------
    virtual void OnSpawn();                                        // slot 2
    virtual bool Update(float dt);                                 // slot 4
    virtual bool PostUpdate(float dt);                             // slot 5
    // Slot 12. NOTE: the frame time is passed through but unused by the base.
    virtual void UpdateTransform(float dt = 0.0f);                 // slot 12
    virtual EntityComponent* GetCharacterComponent();              // slot 15
    virtual EntityComponent* GetComponent00();                     // slot 16
    virtual EntityComponent* GetComponent0D();                     // slot 17
    virtual ape::Vec3 GetCenter();                                 // slot 20
    virtual void SetVisible(bool visible, bool immediate, bool children, bool components); // slot 22
    virtual void SetFlag80(bool on);                               // slot 23
    virtual uint32_t GetRenderId();                                // slot 25
    virtual void SetRenderId(uint32_t id);                         // slot 26
    virtual void Detached(float blend);                            // slot 28 (+0x70)
    // Opacity of the geometry (scaled by the geometry's own alpha unless
    // `absolute`), optionally of the children too; components are told.
    virtual void SetAlpha(uint8_t alpha, bool children, bool flag, bool absolute); // slot 29 (+0x74)
    virtual void SetHighlight(bool on, bool children);             // slot 31
    virtual void SetTimeScale(uint32_t value, bool children);      // slot 32
    virtual void* GetHudScript();                                  // slot 33
    virtual ape::world::ApeObj* GetRenderObject();                 // slot 36 (+0x90)
    virtual ape::Aabb GetBounds(bool world);                       // slot 40
    virtual void HandleMessage(const struct EntityMessage& msg);   // slot 43 (+0xAC)
    virtual void SetActive(bool active);                           // slot 44
    // @func 0x007373A4 Entity::SetAttachParent
    virtual void SetAttachParent(Entity* parent) { attachParent = parent; }   // slot 46
    virtual void SetFlag1(bool on);                                // slot 45
    // Animation clip registered under a name (slots 47/48; the hash form
    // looks the clip up in the global clip registry).
    virtual void* FindAnimClip(uint32_t nameHash);                 // slot 47 (+0xBC)
    virtual void* FindAnimClipByName(const char* name);            // slot 48 (+0xC0)
    // Slot 53 (+0xD4): the base version only records the state (flags bit 6).
    // @func 0x007373AC Entity::SetActiveVisual
    virtual void SetActiveVisual(bool on) { flags = on ? (flags | 0x40u) : (flags & ~0x40u); }
    virtual void ApplyVisibility(bool visible);                    // slot 55 (+0xDC)
    virtual void OnSuspend();                                      // slot 60 (+0xF0)
    virtual void OnResume();                                       // slot 61 (+0xF4)
    // @func 0x0074705C Entity::GetAnimScript
    virtual void* GetAnimScript() { return nullptr; }              // slot 68 (+0x110)
    // @func 0x007437C0 Entity::InteractionOffer
    virtual uint32_t InteractionOffer(int kind, Entity* other) { (void)kind; (void)other; return 0; }   // slot 74
    // @func 0x007437C8 Entity::InteractionNeeds
    virtual uint32_t InteractionNeeds(int kind, Entity* other) { (void)kind; (void)other; return 0; }   // slot 75
    virtual bool CanInteractWith(Entity* other, int kind);         // slot 76
    virtual void OnInteractStart(Entity* other, int kind);         // slot 77
    virtual void OnInteractEnd(Entity* other, int kind);           // slot 78
    // Slot 10: drops one reference; with `kill` the entity leaves the world.
    virtual bool Release(bool kill);

    // -- components -------------------------------------------------------------
    // First component of `type` after `after` (or from the start).
    EntityComponent* GetComponent(uint32_t type, EntityComponent* after = nullptr) const;
    EntityComponent* FindComponentById(uint32_t id, EntityComponent* after = nullptr) const;
    EntityComponent* AddComponent(ComponentDesc* desc, bool registerIt);
    void RemoveComponent(EntityComponent* component);
    void SetComponentsOption(uint32_t type, bool value);
    // Replaces the component of the template's type with a new one built from
    // the named component template (only types 0x12, 0x1C and 0x1E).
    void ReplaceComponent12(uint32_t templateName);
    void ReplaceCarryComponent(uint32_t templateName);
    void ReplaceComponent1E(uint32_t templateName);

    // -- children ---------------------------------------------------------------
    EntityChildLink* FindChildLink(const Entity* child) const;
    void DetachChild(Entity* child);
    Entity* FindDescendant(uint32_t nameHash) const;
    void SetCullDistance(float distance, bool children);
    // Sets or clears EntityClassBits, optionally on all attached children.
    void SetClassBits(uint64_t bits, bool on, bool children);
    // @func 0x0074B684 Entity::SetClassBit
    void SetClassBit(uint64_t bits, bool on) { classBits = on ? (classBits | bits) : (classBits & ~bits); }
    void SetScale(const ape::Vec3& scale, bool children);
    void SetLocalTransform(const ape::Matrix34& transform);

    // -- skeleton ---------------------------------------------------------------
    int FindBone(uint32_t nameHash) const;
    ape::world::ApeSkeletonBone* GetBone(int index) const;
    ape::world::ApeSkeletonBone* GetBoneByName(uint32_t nameHash) const;

    // -- misc -------------------------------------------------------------------
    const ape::Matrix34& GetLocalTransform() const;
    float DistanceToCamera() const;
    uint32_t FindSoundVoice(uint32_t soundHash) const;
    void SetSoundsMuted(bool muted);
    void SetVisibilityDeferred(bool visible, bool deferred);
    void SetSuspended(bool suspended);
    void UpdateWorldTransform(bool children);
    // Places the attached children (0x000D7A00).
    void UpdateChildTransforms();
    void RemoveWatcher(void* watcher);
    bool HasTypeBit(uint32_t bit) const { return (typeBits[bit >> 5] & (1u << (bit & 31))) != 0; }
    void SetName(const char* name);
    float GetAnimDuration(uint32_t anim) const;
    float GetAnimElapsed(bool normalized, uint32_t anim) const;
    float GetAnimProgress(uint32_t anim) const;
    // These three also apply to children linked with kChild_ForwardAnim.
    void SetAnimElapsed(float time, bool normalized, uint32_t anim);
    void SetAnimWeight(float weight, uint32_t anim);
    void SetAnimPaused(uint32_t anim, bool paused);
    bool IsAnimFinished(uint32_t anim) const;
    bool StopAnim(uint32_t anim, bool blend);

    void* FindPlayingAnim(uint32_t anim) const;
    // Player slot kept in the top byte of the flags (bits 24-29).
    uint32_t PlayerSlot() const { return (flags >> 24) & 0x3F; }
    bool IsAlive() const { return (flags & kEnt_Alive) != 0; }
    // @func 0x0074B5A0 Entity::IsActive
    bool IsActive() const { return (flags & kEnt_Active) != 0; }
    // @func 0x0074D808 Entity::IsFlag80Set
    bool IsFlag80Set() const { return (flags & kEnt_80) != 0; }
    // Takes a reference (the counter Release() drops; interactions hold one).
    // @func 0x0074CC74 Entity::AddRef
    void AddRef() { ++interactionCount; }
    uint32_t SoundSlotCount() const { return (flags & kEnt_SoundCountMask) >> kEnt_SoundCountShift; }

    // -- data -------------------------------------------------------------------
    // 0x20 local transform the world object had before the entity took it
    // over; put back on destruction when kEnt_RestoreObject is set.
    ape::Matrix34 objectRestoreTransform{};
    ape::Matrix34 transform;                       // 0x60 world transform (translation = position)
    ape::Vec3& Position() { return transform.translation; }   // 0x90
    const ape::Vec3& Position() const { return transform.translation; }
    ape::world::ApeObj* object = nullptr;          // 0xA0 world object
    void* animController = nullptr;                // 0xA4
    uint32_t nameHash = 0;                         // 0xA8
    uint32_t flags = 0;                            // 0xAC EntityFlags
    uint16_t flags2 = 0;                           // 0xB0 EntityFlags2
    int16_t interactionCount = 0;                  // 0xB2
    uint64_t classBits = 0;                        // 0xB8 EntityClassBits
    uint32_t typeBits[2] = {};                     // 0xC0 bitset of entity types. NOTE: length unknown
    uint32_t renderId = 0;                         // 0xCC
    float cullDistance = 0.0f;                     // 0xD4
    EntityDef* def = nullptr;                      // 0xD8
    void* field_DC = nullptr;                      // 0xDC NOTE: object driven by messages 0x21595665/0x00C73980
    class EntityListener* listener = nullptr;      // 0xF4
    ape::ApeTArray<EntityComponent*>* components = nullptr;   // 0xF8 (allocated on first add)
    ape::ApeTArray<void*>* watchers = nullptr;     // 0xFC NOTE: objects interested in this entity
    EntitySoundSlot sounds[4];                     // 0x100
    Entity* attachParent = nullptr;                // 0x150
    ape::ApeTArray<EntityChildLink>* children = nullptr;      // 0x154 (records of 0xF0 bytes)
};

// Message delivered to entities (id at +0x08, argument at +0x0C).
struct EntityMessage {
    uint32_t field_00 = 0;
    uint32_t field_04 = 0;
    uint32_t id = 0;          // +0x08
    uint32_t argument = 0;    // +0x0C
};

// Receives the entity's messages (+0xF4, virtual slot 12).
class EntityListener {
public:
    virtual ~EntityListener() = default;
    // Slot 21 (+0x54): false vetoes interactions with the entity.
    virtual bool AllowsInteraction() { return true; }
    virtual void OnMessage(const struct EntityMessage& msg) { (void)msg; }
};

// -- global entity lists --------------------------------------------------------
// Every live entity (0x01013B34) and three secondary lists (0x01013B48,
// 0x01013B5C, 0x01013B70). NOTE: the secondary lists' roles are unknown; the
// second one is used by 81 callers (most entity destructors).
enum EntityList { kList_All, kList_B, kList_C, kList_D, kListCount };
void RegisterEntity(EntityList list, Entity* entity);
void UnregisterEntity(EntityList list, Entity* entity);
uint32_t EntityCount(EntityList list);
Entity* EntityAt(EntityList list, uint32_t index);

Entity* FindEntity(uint32_t nameHash);
Entity* FindNearestEntity(const ape::Vec3& position);
// Appends every live entity that has type bit `bit` to `out`.
void CollectEntitiesOfType(uint32_t bit, ape::ApeTArray<Entity*>& out);
Entity* FindNearestEntityOfDef(uint32_t defName, const ape::Vec3& position);
Entity* EntityFromWorldObject(ape::world::ApeObj* object, bool searchParents, bool searchLinks);

// World object of the entity, or the first object below it, named `name`.
ape::world::ApeObj* EntityFindObject(const Entity* entity, uint32_t name);
// Clears the vectors the render nodes of `object` (and, with `recurse`, of its
// descendants) borrow from their owners.
void ClearRenderRefs(ape::world::ApeObj* object, bool recurse);
// Sort predicate on world objects by name (StrCmp of the names).
int CompareObjectNames(ape::world::ApeObj* const* a, ape::world::ApeObj* const* b);
// Byte +0xA8 of the entity's geometry instance, 0 for other objects.
uint8_t EntityGeomByteA8(const Entity* entity);

} // namespace entity
} // namespace sly

namespace sly {
namespace entity {
// Typed component accessors (one console function per type).
EntityComponent* EntityComponent01(const Entity* e);
EntityComponent* EntityComponent03(const Entity* e);
EntityComponent* EntityComponent04(const Entity* e);
EntityComponent* EntityComponent0E(const Entity* e);
EntityComponent* EntityComponent11(const Entity* e);
EntityComponent* EntityComponent12(const Entity* e);
EntityComponent* EntitySkin(const Entity* e);
EntityComponent* EntityComponent14(const Entity* e);
EntityComponent* EntityTargeting(const Entity* e);
EntityComponent* EntityGadget(const Entity* e);
EntityComponent* EntityInteraction(const Entity* e);
EntityComponent* EntityCarry(const Entity* e);
EntityComponent* EntityComponent1E(const Entity* e);
EntityComponent* EntityComponent1F(const Entity* e);
EntityComponent* EntityComponent20(const Entity* e);
EntityComponent* EntityComponent25(const Entity* e);
EntityComponent* EntityCharacterComp(const Entity* e);
} // namespace entity
} // namespace sly

namespace sly {
namespace entity {
// Named component templates (0x01056F04).
void RegisterComponentTemplate(uint32_t name, ComponentDesc* desc);
ComponentDesc* FindComponentTemplate(uint32_t name);
void UnregisterComponentTemplate(uint32_t name);

// -- definitions ------------------------------------------------------------------
void RegisterEntityDef(EntityDef* def);
EntityDef* FindEntityDef(uint32_t nameHash);
Entity* SpawnFromDef(EntityDef* def, SpawnParams& params);
Entity* SpawnEntity(uint32_t defName, SpawnParams& params);

// Deferred entity events, processed once per frame (0x01013B84).
struct DeferredEntityEvent {
    void* target;
    uint32_t event;
    uintptr_t data;
};
void QueueDeferredEvent(void* target, uint32_t event, uintptr_t data);
uint32_t DeferredEventCount();
DeferredEntityEvent* DeferredEvents();
void ClearDeferredEvents();
} // namespace entity
} // namespace sly
