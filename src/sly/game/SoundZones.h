// openTIT - Sound zones (ambient regions whose weights drive the audio mix)
// and the name registries of zones and mix states.
#pragma once

#include <cstdint>

namespace sly {
namespace audio {

struct MixState;

// A sound zone loaded from the level's sound zone XML. Only the fields the
// game code touches are modelled.
struct SoundZone {
    float radius = 0.0f;              // +0x00
    float fadeCurve = 1.0f;           // +0x04 exponent of the trigger fade-in curve. NOTE: default assumed
    float fadeDistance = 0.0f;        // +0x10
    float weight = 0.0f;              // +0x60 blend weight set by triggers
    bool active = false;              // +0x64 linked in the active list
    SoundZone* older = nullptr;       // +0x68 next zone towards the list tail
    SoundZone* newer = nullptr;       // +0x6C previous zone (towards the head)
    void* activeTriggers = nullptr;   // +0x70 triggers currently inside the zone
};

// Adds the zone at the head of the active list / removes it. Both mark the
// zone mix as needing an update.
void ActivateSoundZone(SoundZone* zone);
void DeactivateSoundZone(SoundZone* zone);
// Changes a zone's weight (ignoring changes below 1e-5).
void SetSoundZoneWeight(SoundZone* zone, float weight);
// Most recently activated zone (0x00F65B64) and the dirty flag (0x00F65B68).
SoundZone* ActiveSoundZones();
bool SoundZonesDirty();
void ClearSoundZonesDirty();

// Name registries (0x010AFEC8 zones, 0x010AFEE0 mix states), filled by the
// sound zone and mix state loaders.
void RegisterSoundZone(uint32_t nameHash, SoundZone* zone);
void RegisterMixState(uint32_t nameHash, MixState* state);
SoundZone* FindSoundZone(uint32_t nameHash);
MixState* FindMixState(uint32_t nameHash);

} // namespace audio
} // namespace sly
