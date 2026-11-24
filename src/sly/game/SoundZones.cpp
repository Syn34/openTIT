// openTIT - Sound zones and the zone / mix state registries.
#include "sly/game/SoundZones.h"

#include <cmath>
#include <map>

namespace sly {
namespace audio {

namespace {
SoundZone* s_activeZones = nullptr;   // 0x00F65B64
bool s_zonesDirty = false;            // 0x00F65B68
std::map<uint32_t, SoundZone*> s_zones;      // 0x010AFEC8
std::map<uint32_t, MixState*> s_mixStates;   // 0x010AFEE0
} // namespace

// @func 0x00441C68 ActivateSoundZone
void ActivateSoundZone(SoundZone* zone)
{
    if (zone->active)
        return;
    if (s_activeZones)
        s_activeZones->newer = zone;
    zone->older = s_activeZones;
    zone->newer = nullptr;
    s_activeZones = zone;
    zone->active = true;
    s_zonesDirty = true;
}

// @func 0x00441CB8 DeactivateSoundZone
void DeactivateSoundZone(SoundZone* zone)
{
    if (!zone->active)
        return;
    if (zone->older)
        zone->older->newer = zone->newer;
    if (zone->newer)
        zone->newer->older = zone->older;
    else
        s_activeZones = zone->older;   // the zone was the head
    zone->active = false;
    s_zonesDirty = true;
}

// @func 0x00441D18 SetSoundZoneWeight
void SetSoundZoneWeight(SoundZone* zone, float weight)
{
    if (std::fabs(weight - zone->weight) < 1e-5f)
        return;
    zone->weight = weight;
    s_zonesDirty = true;
}

SoundZone* ActiveSoundZones()
{
    return s_activeZones;
}

bool SoundZonesDirty()
{
    return s_zonesDirty;
}

void ClearSoundZonesDirty()
{
    s_zonesDirty = false;
}

void RegisterSoundZone(uint32_t nameHash, SoundZone* zone)
{
    s_zones[nameHash] = zone;
}

void RegisterMixState(uint32_t nameHash, MixState* state)
{
    s_mixStates[nameHash] = state;
}

// @func 0x0044C404 FindSoundZone
SoundZone* FindSoundZone(uint32_t nameHash)
{
    auto it = s_zones.find(nameHash);
    return it == s_zones.end() ? nullptr : it->second;
}

// @func 0x0044C43C FindMixState
MixState* FindMixState(uint32_t nameHash)
{
    auto it = s_mixStates.find(nameHash);
    return it == s_mixStates.end() ? nullptr : it->second;
}

} // namespace audio
} // namespace sly
