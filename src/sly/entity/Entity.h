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
#include "sly/entity/EntityDef.h"

#include <cstdint>

namespace ape::world { class ApeObj; struct ApeSkeletonBone; }
namespace ape { class ApeAnimScript; class ApeAnimScriptInstance; }

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
    EntityComponent() = default;
    // Base part every component constructor inlines (out of line at
    // 0x00250298): name, owner and type come from the description and the
    // component joins the live component registry (sly/components).
    explicit EntityComponent(const ComponentDesc* desc);
    // Leaves the live component registry and the update list (0x00250384).
    virtual ~EntityComponent();                                    // slots 0/1
    // Slots 2/3 (+0x08/+0x0C): component-specific blocks other systems
    // read; none by default. NOTE: names unknown (the control components
    // return their input flags and input record, other kinds data blocks).
    // @func 0x00737584 EntityComponent::DataBlockA
    virtual void* DataBlockA() { return nullptr; }
    // @func 0x0073758C EntityComponent::DataBlockB
    virtual void* DataBlockB() { return nullptr; }
    // Slot 4 (+0x10). NOTE: name assumed (control components clear their input).
    // @func 0x00737594 EntityComponent::ResetState
    virtual void ResetState() {}
    // @func 0x00762AAC EntityComponent::OnOwnerSpawned
    virtual void OnOwnerSpawned(Entity* owner) { (void)owner; }    // slot 5 (+0x14)
    // @func 0x00762AB0 EntityComponent::Update
    virtual void Update(float dt) { (void)dt; }                    // slot 6 (+0x18)
    // @func 0x00762AB4 EntityComponent::PostUpdate
    virtual void PostUpdate(float dt) { (void)dt; }                // slot 7 (+0x1C)
    // Places the component's geometry before the instance render pass, for
    // components in the instance render list (slot 8, +0x20).
    virtual void RenderInstance();
    // Slot 9 (+0x24). NOTE: empty by default, meaning unknown.
    // @func 0x00737598 EntityComponent::Slot9
    virtual void Slot9() {}
    // Slot 10 (+0x28). NOTE: returns 0 by default, meaning unknown.
    // @func 0x0073759C EntityComponent::Slot10
    virtual int Slot10() { return 0; }
    // Slot 11 (+0x2C): by default switches the component on or off; some
    // components give the flag their own meaning.
    // @func 0x007375A4 EntityComponent::SetOption
    virtual void SetOption(bool value) { enabled = value; }
    // @func 0x007375AC EntityComponent::SetVisible
    virtual void SetVisible(bool visible, bool immediate) { (void)visible; (void)immediate; } // slot 12
    // Slot 13 (+0x34): visibility pushed down from the owner; by default the
    // same as SetVisible. NOTE: name assumed.
    // @func 0x007375B0 EntityComponent::SetVisibleFromOwner
    virtual void SetVisibleFromOwner(bool visible, bool immediate) { SetVisible(visible, immediate); }
    // @func 0x007375F4 EntityComponent::SetAlpha
    virtual void SetAlpha(uint8_t alpha, bool children) { (void)alpha; (void)children; }      // slot 14
    // @func 0x007375F8 EntityComponent::OnTimeScale
    virtual void OnTimeScale(uint32_t value, bool recurse) { (void)value; (void)recurse; }    // slot 15

    uint32_t id = 0;          // 0x04 per-entity identifier (name hash of the component)
    Entity* owner = nullptr;  // 0x08
    uint32_t type = 0;        // 0x0C ComponentType
    bool enabled = true;      // 0x10 updated by the owner when set
    uint32_t field_14 = 0;    // 0x14 NOTE: cleared by the base constructor, role unknown
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

// Attach locator: a frame relative to a bone of an entity's skeleton.
struct EntityLocator {
    ape::Matrix34 transform;             // 0x00 in the bone's space
    uint32_t field_40 = 0;
    int16_t bone = -1;                   // 0x44
};

// One attached child entity (console record of 0xF0 bytes in the array at +0x154).
struct EntityChildLink {
    // 0x00 NOTE: name of the link (slot 51 prefixes the children's clip names
    // with it); the size of the buffer is assumed from the next field.
    char name[0x80] = {};
    ape::Vec3 offset;                    // 0x80 attach offset
    ape::Vec4 rotation;                  // 0x90 attach rotation (quaternion)
    uint32_t partName = 0;               // 0xA0 attach part / slot name hash
    uint32_t flags = 0;                  // 0xB0 EntityChildFlags
    Entity* child = nullptr;             // 0xC0
    EntityLocator* parentLocator = nullptr;   // 0xC4 locator on the parent, or null
    int32_t parentBone = -1;             // 0xC8 bone of the parent, -1 for none
    ape::world::ApeObj* parentObject = nullptr;   // 0xCC object whose world matrix is used when no bone
    EntityLocator* childLocator = nullptr;   // 0xD0 locator on the child, or null
    int32_t childBone = -1;              // 0xD4
    uint32_t restoreState = 0;           // 0xDC passed back to the child on detach
    float restoreBlend = 0.0f;           // 0xE0
};

enum EntityChildFlags : uint32_t {
    kChild_NoTransform     = 1u << 24,  // the child places itself
    kChild_RestoreState    = 1u << 25,  // re-apply restoreState when detached
    kChild_KeepOnDetach    = 1u << 26,  // do not restore the blend on detach
    kChild_KeepScale       = 1u << 28,  // the child keeps its own scale
    kChild_KeepRotation    = 1u << 29,  // the child keeps its own rotation
    kChild_ParentRotation  = 1u << 30,  // use the parent's rotation, ignore the bone's
    kChild_ForwardAnim     = 1u << 31,  // anim layer weights are forwarded to the child
};

// Entity flags (+0xAC).
enum EntityFlags : uint32_t {
    kEnt_00000001   = 0x00000001,  // NOTE: slot 45 toggles it together with flags2 bit 4
    kEnt_Active     = 0x00000002,  // slot 44
    kEnt_Suspended  = 0x00000004,
    kEnt_08         = 0x00000008,  // transform owned by someone else (no child update)
    kEnt_NotListed  = 0x00000010,  // kept out of the global entity list (EntityInstanceDesc::kDesc_NotListed)
    kEnt_80         = 0x00000080,  // slot 23
    kEnt_Visible    = 0x00000100,
    kEnt_RestoreObject = 0x00000400,  // restore the world object's transform when destroyed
    kEnt_TransformChanged = 0x00000800,
    kEnt_4000       = 0x00004000,
    kEnt_8000       = 0x00008000,
    kEnt_NoObjectSync = 0x00001000,  // the world object is not moved with the entity
    kEnt_RootMotion = 0x00002000,    // slot 13 applies root motion
    kEnt_10000      = 0x00010000,    // NOTE: updated like kEnt_Visible
    kEnt_SoundCountShift = 19,     // bits 19-21: number of sound slots in use
    kEnt_SoundCountMask  = 0x7u << 19,
    kEnt_Hidden     = 0x00020000,  // bit 17: set by ApplyVisibility(false)
    kEnt_Alive      = 0x00040000,  // bit 18: registered and alive
};

// Second flag word (+0xB0, 16 bits).
enum EntityFlags2 : uint16_t {
    kEnt2_SoundsMuted     = 0x0010,
    kEnt2_RootMotionObjectScale = 0x0400,  // root motion translation scaled with the object
    kEnt2_RootMotionDamped = 0x0800,  // root motion scaled by rootMotionScale / rootMotionWeight
    kEnt2_AlphaSet        = 0x0020,  // an opacity has been applied (later ones scale the geometry alpha)
    kEnt2_Highlight       = 0x0040,
    kEnt2_PendingVisible  = 0x0080,  // value applied with kEnt2_VisibilityPending
    kEnt2_VisibilityPending = 0x0100,
    kEnt2_NoUpdate        = 0x1000,
    kEnt2_InListenerGroup = 0x2000,  // registered in an event listener group (0x0048F298 removes it)
    kEnt2_4000            = 0x4000,  // the definition's kDefFlag_Field20
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
    int32_t id = -1;          // bone the sound follows, -1 for none
    void* attachObject = nullptr;   // world object it follows without a bone (else the entity)
};

// Draws a flash/fade colour (RGBA) over the entity's geometry this frame
// and over every attached child's (0x000D8F08); `additive` selects the
// renderer's additive variant (NOTE: the name of the flag is a guess).
void EntityApplyFade(Entity* entity, const uint8_t* color, bool additive);

class Entity {
public:
    Entity();
    // Builds the entity an instance description describes (0x000D3578):
    // transform, name, flags from the description's options, the world
    // object, the definition's components, and joins the entity list.
    explicit Entity(const EntityInstanceDesc& desc);
    virtual ~Entity();

    // -- virtual interface (console vtable 0x00B12550) --------------------------
    virtual void OnSpawn();                                        // slot 2
    virtual bool Update(float dt);                                 // slot 4
    virtual bool PostUpdate(float dt);                             // slot 5
    // Slot 12. NOTE: the frame time is passed through but unused by the base.
    virtual void UpdateTransform(float dt = 0.0f);                 // slot 12
    // Slot 13 (+0x34): applies the root motion of the playing animation to
    // the transform while flags bit 13 is set (cleared when a non-looping
    // animation is over).
    virtual void ApplyRootMotion(float dt);
    // Slot 14 (+0x38): fade and flash pass of the frame (characters, props);
    // nothing by default.
    // @func 0x000D8FC0 Entity::UpdateFade
    virtual void UpdateFade() {}
    virtual EntityComponent* GetCharacterComponent();              // slot 15
    virtual EntityComponent* GetComponent00();                     // slot 16
    virtual EntityComponent* GetComponent0D();                     // slot 17
    virtual ape::Vec3 GetCenter();                                 // slot 20
    virtual void SetVisible(bool visible, bool immediate, bool children, bool components); // slot 22
    virtual void SetFlag80(bool on, int part = -1);                               // slot 23
    virtual uint32_t GetRenderId();                                // slot 25
    virtual void SetRenderId(uint32_t id);                         // slot 26
    virtual void Detached(float blend);                            // slot 28 (+0x70)
    // Opacity of the geometry (scaled by the geometry's own alpha unless
    // `absolute`), optionally of the children too; components are told.
    virtual void SetAlpha(uint8_t alpha, bool children, bool flag, bool absolute); // slot 29 (+0x74)
    // Slot 30 (+0x78): colour tint of the model (amount 0..1).
    virtual void SetTint(float amount, const uint8_t color[4], bool immediate);
    virtual void SetHighlight(bool on, bool children);             // slot 31
    virtual void SetTimeScale(uint32_t value, bool children);      // slot 32
    virtual void* GetHudScript();                                  // slot 33
    virtual ape::world::ApeObj* GetRenderObject();                 // slot 36 (+0x90)
    // Slot 37 (+0x94): world object (and bone, -1 for the whole object)
    // that a Havok collidable belongs to; null when it is not one of the
    // entity's bodies.
    virtual ape::world::ApeObj* ObjectForCollidable(const void* collidable, int32_t* bone);
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
    // Slot 49 (+0xC4): plays `clip` on the controller layer created for
    // `layer` (cross-fading over `blend` seconds, the module default when
    // negative), or starts it on the first layer when there is no such layer.
    // A positive `duration` stretches the clip to last that long. `listener`
    // names the control listener to attach. False without a controller or clip.
    virtual bool PlayClip(float speed, float blend, float duration, void* clip, uint32_t layer, bool loop,
                          uint32_t listener = 0);
    // Slot 50 (+0xC8): the same for a clip given by name hash.
    virtual bool PlayClipByHash(float speed, float blend, float duration, uint32_t clip, uint32_t layer,
                                bool loop, uint32_t listener = 0);
    // Slot 51 (+0xCC): the same by name (prefixed with `prefix` when given).
    // Children linked with kChild_ForwardAnim first play "<link name>_<name>"
    // with no stretching.
    virtual bool PlayClipByName(float speed, float blend, float duration, const char* clip, uint32_t layer,
                                bool loop, const char* prefix = nullptr, uint32_t listener = 0);
    // Slot 52 (+0xD0): motion of the root of the clip playing on `layer`
    // this frame (identity without one); see kEnt2_RootMotion*. NOTE:
    // `consume` is not read by the base.
    virtual void RootMotionDelta(ape::Matrix34& out, bool consume, uint32_t layer, float dt);
    // Slot 53 (+0xD4): the base version only records the state (flags bit 6).
    // @func 0x007373AC Entity::SetActiveVisual
    virtual void SetActiveVisual(bool on) { flags = on ? (flags | 0x40u) : (flags & ~0x40u); }
    // Slot 54 (+0xD8): records flags bit 5. NOTE: meaning unknown.
    // @func 0x007373BC Entity::SetFlag20
    virtual void SetFlag20(bool on) { flags = on ? (flags | 0x20u) : (flags & ~0x20u); }
    virtual void ApplyVisibility(bool visible);                    // slot 55 (+0xDC)
    virtual void OnSuspend();                                      // slot 60 (+0xF0)
    virtual void OnResume();                                       // slot 61 (+0xF4)
    // @func 0x0074705C Entity::GetAnimScript
    virtual void* GetAnimScript() { return nullptr; }              // slot 68 (+0x110)
    // @func 0x007437C0 Entity::InteractionOffer
    virtual uint32_t InteractionOffer(int kind, Entity* other) { (void)kind; (void)other; return 0; }   // slot 74
    // @func 0x007437C8 Entity::InteractionNeeds
    virtual uint32_t InteractionNeeds(int kind, Entity* other) { (void)kind; (void)other; return 0; }   // slot 75
    virtual bool CanInteractWith(Entity* other, int kind, float* priority = nullptr);         // slot 76
    virtual void OnInteractStart(Entity* other, int kind);         // slot 77
    virtual void OnInteractEnd(Entity* other, int kind);           // slot 78
    // Slot 10: drops one reference; with `kill` the entity leaves the world.
    virtual bool Release(bool kill);

    // Slot 19 (+0x4C): name for messages: the world object's name.
    virtual const char* GetDebugName();
    // Slot 21 (+0x54): replaces the world transform.
    // @func 0x007435D0 Entity::SetTransform
    virtual void SetTransform(const ape::Matrix34& m)
    {
        transform.xAxis = {m.xAxis.x, m.xAxis.y, m.xAxis.z, transform.xAxis.pad};
        transform.yAxis = {m.yAxis.x, m.yAxis.y, m.yAxis.z, transform.yAxis.pad};
        transform.zAxis = {m.zAxis.x, m.zAxis.y, m.zAxis.z, transform.zAxis.pad};
        transform.translation = {m.translation.x, m.translation.y, m.translation.z, transform.translation.pad};
    }
    // Moves the entity, keeping its orientation (through SetTransform).
    void SetPosition(const ape::Vec3& position);
    // Slot 24 (+0x60): flags bit 3; turning it on also wakes a suspended
    // entity (scripted entities mirror it in their state flags).
    // @func 0x00737350 Entity::SetBusyFlag
    virtual void SetBusyFlag(bool on)
    {
        flags = on ? (flags | kEnt_08) : (flags & ~uint32_t(kEnt_08));
        if (on)
            SetSuspended(false);
    }
    // Slot 39 (+0x9C): position of a locator (its translation), or of the
    // entity without one.
    // @func 0x00743720 Entity::GetLocatorPosition
    virtual ape::Vec3 GetLocatorPosition(const ape::Matrix34* locator)
    {
        return locator ? locator->translation : transform.translation;
    }
    // Slot 57 (+0xE4): adds an object interested in this entity (see
    // RemoveWatcher); always accepted.
    virtual bool AddWatcher(void* watcher);

    // Slots whose callers are not ported; named by their slot number.
    // NOTE: meanings unknown. Trigger classes reuse slots 71-73 for their
    // occupant callbacks.
    // @func 0x007435B8 Entity::Slot9
    virtual bool Slot9() { return true; }
    // Slot 11: back to the idle state (characters reset physics and props).
    // @func 0x007435CC Entity::ResetState
    virtual void ResetState() {}
    // Slot 27: gravity factor of the physics body (slot 28 sets it).
    // @func 0x00743640 Entity::GetGravityFactor
    virtual float GetGravityFactor() { return 0.0f; }
    // Slot 34 (+0x88): sets the linear velocity of the physics body (rigid
    // body entities); nothing by default.
    // @func 0x00737390 Entity::SetVelocity
    virtual void SetVelocity(const ape::Vec3& velocity) { (void)velocity; }
    // Slot 35 (+0x8C): linear velocity (none by default).
    // @func 0x007436E0 Entity::GetVelocity
    virtual ape::Vec3 GetVelocity() { return {0.0f, 0.0f, 0.0f, 0.0f}; }
    // Slot 38: turn rate of the physics body (characters). NOTE: name from
    // the character component field it returns.
    // @func 0x00743710 Entity::GetTurnRate
    virtual float GetTurnRate() { return 0.0f; }
    // @func 0x00737394 Entity::Slot41
    virtual int Slot41() { return 0; }
    // @func 0x0073739C Entity::Slot42
    virtual int Slot42() { return 0; }
    // @func 0x007373CC Entity::Slot56
    virtual int Slot56() { return 0; }
    // Slots 62-64: an interaction with the entity begins, is cancelled or
    // ends. NOTE: names from the actors that override them.
    // @func 0x007374A8 Entity::InteractionBegin
    virtual bool InteractionBegin(Entity* user) { (void)user; return true; }
    // @func 0x007374B0 Entity::InteractionCancel
    virtual void InteractionCancel() {}
    // @func 0x007374B4 Entity::InteractionEnd
    virtual void InteractionEnd(bool completed) { (void)completed; }
    // @func 0x007374B8 Entity::Slot65
    virtual bool Slot65() { return true; }
    // @func 0x007374C0 Entity::Slot66
    virtual void Slot66() {}
    // @func 0x007374C4 Entity::Slot67
    virtual void Slot67() {}
    // Slot 69 (+0x114): entity this one tracks (the Bentley alter ego's
    // current target); none by default.
    // @func 0x007374C8 Entity::GetTrackedTarget
    virtual Entity* GetTrackedTarget() { return nullptr; }
    // Slot 70 (+0x118): one of the entity's physics bodies by name hash
    // (the vehicles answer for their body); none by default.
    // @func 0x007374D0 Entity::Slot70
    virtual void* Slot70(uint32_t nameHash) { (void)nameHash; return nullptr; }
    // Slot 71 (+0x11C): centre of mass (the position by default).
    // @func 0x00743754 Entity::GetAimPoint
    virtual ape::Vec3 GetAimPoint() { return transform.translation; }
    // Slots 72/73: axes of the surface the entity moves on (characters).
    // @func 0x00743778 Entity::GetUpVector
    virtual ape::Vec3 GetUpVector() { return {0.0f, 0.0f, 0.0f, 0.0f}; }
    // @func 0x0074379C Entity::GetSideVector
    virtual ape::Vec3 GetSideVector() { return {0.0f, 0.0f, 0.0f, 0.0f}; }

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
    // Model transform of bone `bone` of the world object's skeleton (the
    // identity, or the entity's frame, without one). With `world` it is
    // carried into world space by the object's transform (unscaled unless
    // `keepScale`); without a bone the result is then the object's (or the
    // entity's) transform.
    ape::Matrix34 BoneTransform(int bone, bool world, bool keepScale) const;
    // Same for a bone given by name (no bone for 0 or an unknown name).
    ape::Matrix34 BoneTransformByName(uint32_t boneHash, bool world, bool keepScale) const;
    // The position projects outside the screen by more than `margin`
    // (fraction of the screen) on either axis.
    bool IsOffScreen(float margin) const;
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
    // Slot 58 (+0xE8): forgets a watcher added by AddWatcher.
    virtual void RemoveWatcher(void* watcher);
    // Slot 59 (+0xEC): animation event `name` from `source`'s controller.
    // "hkAttach<part>" moves the entity to another attach part of its parent.
    virtual void OnAnimEvent(const char* name, uint32_t argument, Entity* source);
    // @func 0x00735000 Entity::HasTypeBit
    bool HasTypeBit(uint32_t bit) const { return (typeBits[bit >> 5] & (1u << (bit & 31))) != 0; }
    void SetName(const char* name);
    // Switches the entity's anim script (slot 68) to a state.
    void SetScriptState(uint32_t stateHash);
    // Runs `script` for this entity: "this" refers to the entity, "thisName"
    // holds its name and the matrix variable the transform of `bone` (or of
    // the entity). With `autoRun` the instance is registered and removed
    // once finished. Null without a script.
    ape::ApeAnimScriptInstance* StartScript(ape::ApeAnimScript* script, uint32_t bone, bool autoRun);
    // Same with a script of the default library by hash / by name.
    ape::ApeAnimScriptInstance* StartScript(uint32_t scriptHash, uint32_t bone, bool autoRun);
    ape::ApeAnimScriptInstance* StartScriptByName(const char* scriptName, uint32_t bone, bool autoRun);
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
    // Frame of something attached to the entity: a bone (when `bone` >= 0),
    // else `attachObject`'s world transform, else the entity's, with a local
    // rotation and offset applied first.
    ape::Matrix34 AttachTransform(int bone, const ape::world::ApeObj* attachObject, const ape::Vec4& rotation,
                                  const ape::Vec3& offset) const;
    // Velocity estimate and the positions of the attached sounds (finished
    // ones are dropped).
    void UpdateMotion(float dt);

    // -- data -------------------------------------------------------------------
    // 0x20 local transform the world object had before the entity took it
    // over; put back on destruction when kEnt_RestoreObject is set.
    ape::Matrix34 objectRestoreTransform{};
    ape::Matrix34 transform;                       // 0x60 world transform (translation = position)
    // @func 0x00734E60 Entity::Position
    ape::Vec3& Position() { return transform.translation; }   // 0x90
    const ape::Vec3& Position() const { return transform.translation; }
    ape::world::ApeObj* object = nullptr;          // 0xA0 world object
    void* animController = nullptr;                // 0xA4
    uint32_t nameHash = 0;                         // 0xA8
    uint32_t flags = 0;                            // 0xAC EntityFlags
    uint16_t flags2 = 0;                           // 0xB0 EntityFlags2
    int16_t interactionCount = 0;                  // 0xB2
    uint64_t classBits = 0;                        // 0xB8 EntityClassBits
    uint32_t typeBits[3] = {};                     // 0xC0 bitset of entity types (words +0xC0..+0xC8)
    uint32_t renderId = 0;                         // 0xCC
    float field_D0 = -1.0f;                        // 0xD0 square of EntityInstanceDesc::field_84 (-1: none)
    float cullDistance = 0.0f;                     // 0xD4
    EntityDef* def = nullptr;                      // 0xD8
    void* field_DC = nullptr;                      // 0xDC NOTE: object driven by messages 0x21595665/0x00C73980
    ape::Vec3 rootMotionScale{1.0f, 1.0f, 1.0f, 0.0f};   // 0xE0 (kEnt2_RootMotionDamped)
    float rootMotionWeight = 1.0f;                 // 0xF0 share of the root rotation kept (kEnt2_RootMotionDamped)
    class EntityListener* listener = nullptr;      // 0xF4
    ape::ApeTArray<EntityComponent*>* components = nullptr;   // 0xF8 (allocated on first add)
    ape::ApeTArray<void*>* watchers = nullptr;     // 0xFC NOTE: objects interested in this entity
    EntitySoundSlot sounds[4];                     // 0x100
    ape::Vec3 lastPosition{};                      // 0x130 position at the last update
    ape::Vec3 velocity{};                          // 0x140 estimated from the position change
    Entity* attachParent = nullptr;                // 0x150
    ape::ApeTArray<EntityChildLink>* children = nullptr;      // 0x154 (records of 0xF0 bytes)
};

// Message delivered to entities and game event listeners: the record of the
// game message queue (0x14 bytes on the console, see sly/game/GameMessages.h).
struct EntityMessage {
    Entity* sender = nullptr;  // +0x00
    void* receiver = nullptr;  // +0x04 entity or GameEventListener
    uint32_t id = 0;           // +0x08
    uint32_t argument = 0;     // +0x0C NOTE: some senders pass a 32-bit pointer here
    bool toEntity = false;     // +0x10 the receiver is an entity
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
// Removes any object from a list (the lists also hold components).
void UnregisterObject(EntityList list, const void* object);
void UnregisterEntity(EntityList list, Entity* entity);
// Adds a component to the instance render list (list C, "s_instRenderList");
// a second add is refused with a warning (0x000D6614).
void AddToInstanceRenderList(EntityComponent* component);
// The same for an entity (list B, 0x000D64F4).
void AddToInstanceRenderList(Entity* entity);
uint32_t EntityCount(EntityList list);
Entity* EntityAt(EntityList list, uint32_t index);
// Drops entry `index` by moving the last entry into its place.
void RemoveEntityAt(EntityList list, uint32_t index);

// Listener the entities install on their animation controller: forwards
// an event to the entity (`user`) (0x000DD4A0).
void EntityAnimEventListener(const char* name, uint32_t argument, void* user);

// Game world callback that runs the entity's slot 12 (UpdateTransform)
// later in the frame (lists 0 and 2) (0x000DD778).
void QueuedUpdateTransform(void* owner, Entity* entity);

// One entity's share of the frame: the update step (flags, slot 4, child
// transforms, motion) and the post-update step (slot 5, world object sync,
// root motion, animation).
void EntityUpdateStep(Entity* entity, float dt);
void EntityPostUpdateStep(Entity* entity, float dt);

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
