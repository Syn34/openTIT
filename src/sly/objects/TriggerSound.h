// openTIT - Polled trigger volumes and the two audio triggers built on them:
// ambient sound emitters and sound zones (reverb / mix state regions).
//
// A polled trigger (vtable 0x00B197D8) does not wait for physics reports: for
// sphere and box volumes it tests the audio listener position against the
// volume every frame and reports a single, entity-less enter/leave. Other
// volume shapes fall back to the normal trigger behaviour.
#pragma once

#include "sly/objects/Trigger.h"

#include <cstdint>

namespace sly { namespace audio { struct SoundName; } }

namespace sly {
namespace objects {

class PolledTrigger : public TriggerBase {
public:
    explicit PolledTrigger(const TriggerInstanceDesc& desc);

    bool Update(float dt) override;
    void OnEnter(Entity* entity) override;
    void OnExit(Entity* entity) override;
    void SetEnabled(bool enabled) override;
    // Slot 78: rebuilds the phantom shape from the cached bounds (empty here).
    virtual void RebuildVolume();
    // Slot 79: whether a point is inside the volume.
    virtual bool IsInside(const ape::Vec3& point) const;
    // Reads the volume shape into the polling data (0x0020FFC0).
    void SetupShape();

    bool IsPolled() const { return m_shapeKind == kVolumeShape_Box || m_shapeKind == kVolumeShape_Sphere; }

protected:
    // One polling step shared by the audio triggers.
    void PollListener();

    int32_t m_shapeKind = kVolumeShape_None;   // 0x1AC
    float m_radiusSq = 0.0f;                   // 0x1B0
    ape::Vec3 m_boxMin{};                      // 0x1C0
    ape::Vec3 m_boxMax{};                      // 0x1D0
    bool m_inside = false;                     // 0x1E0
};

// ---------------------------------------------------------------------------
// Ambient sound
// ---------------------------------------------------------------------------
constexpr uint32_t kAttr_Sound = 0x04B6D0EF;   // "Sound"
constexpr uint32_t kAttr_Bank  = 0x001EFE3C;   // "Bank"

class AmbientSoundTriggerDef : public TriggerDef {
public:
    AmbientSoundTriggerDef(const TriggerDefDesc& desc, uint32_t bankHash, uint32_t soundHash);
    ~AmbientSoundTriggerDef() override;
    Entity* Spawn(EntityPlacement* placement) override;
    // Re-resolves the sound reference if the banks were (re)loaded.
    void ResolveSound();

    uint32_t bank = 0;                    // 0xB0
    uint32_t sound = 0;                   // 0xB4
    audio::SoundName* soundName = nullptr;// 0xB8
};

class AmbientSoundTrigger : public PolledTrigger {
public:
    explicit AmbientSoundTrigger(const TriggerInstanceDesc& desc);
    // The destructor runs the class's own volume rebuild once more (called
    // directly on the console).
    ~AmbientSoundTrigger() override;
    void RebuildVolume() override;
    void OnSpawn() override;
    bool Update(float dt) override;
    void OnEnter(Entity* entity) override;
    void OnExit(Entity* entity) override;
    bool IsDebugDisplayed() const override;

private:
    AmbientSoundTriggerDef* SoundDef() const { return static_cast<AmbientSoundTriggerDef*>(def); }
    // Emitter position, volume and spread angle (degrees, 360 = all around)
    // for the listener at `position` (overwritten with the emitter position)
    // (0x00210D80). Records the falloff fraction in m_falloff.
    void ComputeSoundParams(ape::Vec3& position, float& volume, float& spread);

    uint32_t m_voice = 0;                 // 0x1E4
    // Volume shape of the sound. NOTE: filled by the shape set-up
    // (0x002108A8 / 0x00211780, not ported); zero until then.
    float m_innerRadius = 0.0f;           // 0x1E8 sphere: full volume inside
    float m_outerRadius = 0.0f;           // 0x1EC sphere: silent beyond
    ape::Vec3 m_innerMin{};               // 0x1F0 box: full volume inside (local space)
    ape::Vec3 m_innerMax{};               // 0x200
    ape::Vec3 m_falloffMax{};             // 0x230 box: falloff distance past m_innerMax (positive)
    ape::Vec3 m_falloffMin{};             // 0x240 box: falloff distance past m_innerMin (negative)
    float m_falloff = 0.0f;               // 0x260 0 inside, 1 at the edge of the falloff
    uint32_t m_bus = 0;                   // 0x264
};

EntityDef* CreateAmbientSoundTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                        const EntityDefDesc& loader, const EntityDef* parent);

// ---------------------------------------------------------------------------
// Sound zone
// ---------------------------------------------------------------------------
constexpr uint32_t kAttr_SoundZone = 0x3A20DD1B;   // "SoundZone"
constexpr uint32_t kAttr_MixState  = 0xEF44A7B5;   // "MixState"
// NOTE: name not recovered; the fade time used when the mix state is pushed/popped.
constexpr uint32_t kAttr_MixFade   = 0x48AD591B;

class SoundZoneTriggerDef : public TriggerDef {
public:
    SoundZoneTriggerDef(const TriggerDefDesc& desc, uint32_t zoneHash, uint32_t mixHash, float fade);
    Entity* Spawn(EntityPlacement* placement) override;
    // The member-wise destruction is TriggerDef::~TriggerDef (its copies are
    // tagged there); the deleting variant returns the block to the bucket
    // allocator.
    // @func 0x0075AECC SoundZoneTriggerDef::~SoundZoneTriggerDef (deleting)
    ~SoundZoneTriggerDef() override = default;

    uint32_t zoneName = 0;                // 0xB0
    SoundZone* zone = nullptr;            // 0xB4
    uint32_t mixStateName = 0;            // 0xB8
    MixState* mixState = nullptr;         // 0xBC
    float mixFade = 0.0f;                 // 0xC0
};

class SoundZoneTrigger : public PolledTrigger {
public:
    explicit SoundZoneTrigger(const TriggerInstanceDesc& desc);
    // Like AmbientSoundTrigger's destructor.
    ~SoundZoneTrigger() override;
    void RebuildVolume() override;
    void OnSpawn() override;
    bool Update(float dt) override;
    void OnEnter(Entity* entity) override;
    void OnExit(Entity* entity) override;
    bool IsDebugDisplayed() const override;
    bool IsInside(const ape::Vec3& point) const override;

private:
    SoundZoneTriggerDef* ZoneDef() const { return static_cast<SoundZoneTriggerDef*>(def); }
    // Puts this trigger at the head of the zone's active list / takes it out.
    void LinkActive();
    void UnlinkActive();
    // Zone weight for the listener at `listener` inside a fading volume
    // (0x00213870).
    float ComputeFadeWeight(const ape::Vec3& listener) const;

    float m_weight = 0.0f;                // 0x1E4
    // Fade shape. NOTE: filled by the shape set-up (0x00213398, not ported).
    float m_innerRadius = 0.0f;           // 0x1E8 sphere: full weight inside
    float m_outerRadius = 0.0f;           // 0x1EC sphere: zero weight beyond
    ape::Vec3 m_innerMin{};               // 0x1F0 box: full weight inside (local space)
    ape::Vec3 m_innerMax{};               // 0x200
    ape::Vec3 m_falloffMax{};             // 0x230 box: fade distance past m_innerMax (positive)
    ape::Vec3 m_falloffMin{};             // 0x240 box: fade distance past m_innerMin (negative)
    bool m_fades = false;                 // 0x264: the zone fades in over a distance
    bool m_linked = false;                // 0x265
    SoundZoneTrigger* m_next = nullptr;   // 0x268
    SoundZoneTrigger* m_prev = nullptr;   // 0x26C
};

EntityDef* CreateSoundZoneTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                     const EntityDefDesc& loader, const EntityDef* parent);

// Whether the sound system was started without audio (0x010B09E4).
bool IsAudioDisabled();

} // namespace objects
} // namespace sly
