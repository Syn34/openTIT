// openTIT - Prop objects: plain entities with an optional rigid body made
// from their definition's template body, an optional anim script, a
// lifetime and an alpha buffer (console vtable 0x00B123D0 with the contact
// listener at +0x158; definition vtable 0x00B123A0, 0xF0 bytes; code
// 0x000DDA30-0x000DF8B4). Entities of this class carry type bit 70 and fade
// with a character that holds them.
//
// NOTE: the class name is descriptive. On the console it derives from the
// lower entity base (71 virtual slots) rather than from the full Entity;
// slots 71-73 are its own (body sync, body placement and the contact
// callback that also overrides the listener's slot 5).
//
// The constructor (0x000DDCA8), the definition spawn (0x000DE208) and the
// placement copy (0x000DE454) need the entity constructor family and are not
// ported.
#pragma once

#include "sly/entity/Entity.h"
#include "sly/entity/EntityInterfaces.h"

namespace ape { class ApeAnimScript; class ApeAnimScriptInstance; }

namespace sly {
namespace entity {

class PropObjectDef : public EntityDef {
public:
    ~PropObjectDef() override;
    // Slots 4/5: the words at +0xE0/+0xE4. NOTE: meanings unknown.
    // @func 0x007470BC PropObjectDef::DefSlot4
    uint32_t DefSlot4() const override { return field_E0; }
    // @func 0x007470C4 PropObjectDef::DefSlot5
    uint32_t DefSlot5() const override { return field_E4; }
    // Slot 6: the nav mode makes the props static obstacles (same four
    // modes as ActorDef::DefSlot6).
    uint32_t DefSlot6() const override;

    void* bodyTemplate = nullptr;            // 0xA4 Havok rigid body the instances clone
    ape::ApeAnimScript* script = nullptr;    // 0xB0 default script
    uint32_t scriptName = 0;                 // 0xB4 its name (placements may pick another)
    ape::ApeAnimScript* contactScript = nullptr;   // 0xB8 started when the body touches something
    float lifetime = -1.0f;                  // 0xBC seconds, negative: forever
    uint32_t navMode = 0;                    // 0xC0 nav mode hash (see ActorDef)
    uint32_t field_E0 = 0;                   // 0xE0 (returned by slot 4)
    uint32_t field_E4 = 0;                   // 0xE4 (returned by slot 5)
    uint32_t defFlags = 0;                   // 0xEC bits 0-1 become entity flags 22-23
};

class PropObjectEntity : public Entity, public EntityInterface17C {
public:
    // Flags (+0x178).
    enum PropFlags : uint32_t {
        kProp_ScriptOneShot   = 1u << 28,    // the script is dropped once it finishes
        kProp_Contact         = 1u << 29,    // the body touched something this frame
        kProp_SpawnPending    = 1u << 30,    // run the script's "Spawn" state next update
        kProp_UsePlacement    = 1u << 31,    // hand the placement values to the script
    };

    // Frees the alpha buffer.
    ~PropObjectEntity() override;
    // Slot 36: the rigid body (see NOTES on slot 36).
    ape::world::ApeObj* GetRenderObject() override;
    void OnSpawn() override;                                   // slot 2
    bool Update(float dt) override;                            // slot 4
    bool PostUpdate(float dt) override;                        // slot 5
    bool Release(bool kill) override;                          // slot 10
    void SetTransform(const ape::Matrix34& m) override;        // slot 21
    void SetFlag80(bool on, int part = -1) override;           // slot 23
    void SetTimeScale(uint32_t value, bool children) override; // slot 32
    void SetVelocity(const ape::Vec3& velocity) override;      // slot 34
    ape::Vec3 GetVelocity() override;                          // slot 35
    void OnSuspend() override;                                 // slot 60
    void OnResume() override;                                  // slot 61
    // @func 0x00747398 PropObjectEntity::GetAnimScript
    void* GetAnimScript() override { return script; }          // slot 68
    // Slot 71: the entity follows a simulated body.
    virtual void SyncFromBody();
    // Slot 72: the body follows the entity.
    virtual void PlaceBody();
    // Slot 73 / listener slot 5: a contact point was added; the update
    // starts the definition's contact script and the listener hears of it.
    void Interface17CSlot5(const void* event) override;

    // Per-part alpha values of the geometry (copied once, `count` bytes x4).
    void SetAlphaBuffer(const uint32_t* values, int count);
    // Opacity while faded with a holder: `fading` keeps the alpha buffer
    // and forces LOD field 6 (the old value is restored when it ends).
    void SetFadeAlpha(bool fading, uint8_t alpha, bool immediate);
    void ApplyFadeBuffer();

    const PropObjectDef* Def() const { return static_cast<const PropObjectDef*>(def); }

    void* body = nullptr;                            // 0x15C
    ape::ApeAnimScriptInstance* script = nullptr;    // 0x160
    float age = 0.0f;                                // 0x164
    uint8_t alpha = 0xFF;                            // 0x16C
    int32_t savedLodField = 0;                       // 0x170
    uint32_t* alphaBuffer = nullptr;                 // 0x174 owned
    uint32_t propFlags = 0;                          // 0x178 PropFlags
};

} // namespace entity
} // namespace sly
