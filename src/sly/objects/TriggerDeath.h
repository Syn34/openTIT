// openTIT - Death trigger volumes: characters that enter die (fall, drown,
// burn, ...), optionally only in one physics state.
//
// Definition vtable 0x00B1A418, placement 0x00B1A448, entity 0x00B1A460.
#pragma once

#include "sly/objects/Trigger.h"

#include <cstdint>

namespace sly { namespace audio { struct SoundName; } }

namespace sly {
namespace objects {

// Def class / placement flag of death triggers.
constexpr uint32_t kTrigType_Death = 0x10000000;

constexpr uint32_t kAttr_DeathType     = 0x63C8D78E;   // NOTE: name unknown (hash of the death kind)
constexpr uint32_t kAttr_DamageAmount  = 0x7D873487;   // "DamageAmount"
// NOTE: the five flags below have no recovered names.
constexpr uint32_t kAttr_DeathFlag0 = 0x25320E69;
constexpr uint32_t kAttr_DeathFlag1 = 0x98FD6F46;
constexpr uint32_t kAttr_DeathFlag2 = 0x945416B0;
constexpr uint32_t kAttr_DeathFlag3 = 0xF1D47360;
constexpr uint32_t kAttr_DeathFlag4 = 0xF403C89E;

// Death kinds (table 0x00AD3654). Index 0 is the fall/pit kind that plays the
// two fall sounds; the default is index 3 (hash 0x03D3CDC0).
// NOTE: names of kinds 0 and 3 unknown.
constexpr uint32_t kDeathTypes[6] = {
    0x419905DF, 0x07DC7227 /* "Physics" */, 0x04E8CCF7 /* "Water" */,
    0x03D3CDC0, 0x2DA0A51D /* "Electric" */, 0x0020EE36 /* "Fire" */,
};
constexpr uint32_t kDefaultDeathType = 0x03D3CDC0;

class DeathTriggerDef : public TriggerDef {
public:
    DeathTriggerDef(const TriggerDefDesc& desc, uint32_t deathType, uint32_t scriptName,
                    int physicsState, float damage, bool flag0, bool flag1);
    ObjEntity* Create(EntityPlacement* placement) override;
    // The member-wise destruction is TriggerDef::~TriggerDef (its copies are
    // tagged there); the deleting variant returns the block to the bucket
    // allocator.
    // @func 0x0075B4B8 DeathTriggerDef::~DeathTriggerDef (deleting)
    ~DeathTriggerDef() override = default;

    uint32_t deathType = kDefaultDeathType; // 0xB0
    uint32_t scriptName = 0;                // 0xB4 "ScriptLifetime"
    void* script = nullptr;                 // 0xB8 the script, looked up at load
    int physicsState = -1;                  // 0xBC
    float damage = 0.0f;                    // 0xC0
    bool flag0 = false;                     // 0xC4 bit 31 (attribute 0x98FD6F46)
    bool flag1 = false;                     // 0xC4 bit 30 (attribute 0xF403C89E)
};

class DeathTriggerPlacement : public TriggerPlacement {
public:
    explicit DeathTriggerPlacement(const char* placementName);
    // @func 0x00759A40 DeathTriggerPlacement::~DeathTriggerPlacement
    // @func 0x0075B510 DeathTriggerPlacement::~DeathTriggerPlacement (deleting)
    ~DeathTriggerPlacement() override = default;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;

    uint32_t deathType = 0;     // 0xBC
    int physicsState = -1;      // 0xC0
    float damage = 0.0f;        // 0xC4
    bool deathFlags[5] = {};    // 0xC8 bits 31..27
};

struct DeathTriggerInstanceDesc : TriggerInstanceDesc {
    DeathTriggerInstanceDesc(const char* instanceName, DeathTriggerDef* ownerDef);
    uint32_t deathType = kDefaultDeathType; // 0x9C
    int physicsState = -1;                  // 0xA0
    float damage = 0.0f;                    // 0xA4
    bool flags[5] = {};                     // 0xA8 bits 31..27
};

class DeathTrigger : public TriggerBase {
public:
    explicit DeathTrigger(const DeathTriggerInstanceDesc& desc);
    ~DeathTrigger() override;

    void Update(float dt) override;
    void SetEnabled(bool enabled) override;
    bool IsDebugDisplayed() const override;
    // TODO(0x0021C8B8 DeathTrigger::OnEnter) kills the character (2.5 KB of
    // reaction selection); the base behaviour is kept until it is ported.

private:
    void* m_script = nullptr;                  // 0x1AC script instance of the definition
    audio::SoundName* m_fallSound = nullptr;   // 0x1B0 (kind 0 only)
    audio::SoundName* m_fallSound2 = nullptr;  // 0x1B4
    uint32_t m_deathKind = 1;                  // 0x1B8 index into kDeathTypes
    int m_physicsState = -1;                   // 0x1BC
    float m_damage = 0.0f;                     // 0x1C0
    bool m_flags[5] = {};                      // 0x1C4 bits 31..27
};

EntityDef* CreateDeathTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                 const EntityDefDesc& loader, const EntityDef* parent);
EntityPlacement* ParseDeathTriggerPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                            const char* name);

} // namespace objects
} // namespace sly
