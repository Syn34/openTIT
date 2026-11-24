// openTIT - Wind sources: per-object wind contributions that fade in and
// out, and the system that switches them all on or off.
#pragma once

#include "ape/math/ApeMath.h"
#include "sly/game/Tween.h"

#include <cstdint>
#include <vector>

namespace sly {

class WorldObject;

enum WindState : int32_t {
    kWind_Off       = 0,
    kWind_FadingIn  = 1,
    kWind_On        = 2,
    kWind_FadingOut = 3,
};

struct WindDef {
    float fadeInTime = 0.0f;    // +0x64 seconds for a full fade in
    float fadeOutTime = 0.0f;   // +0x68 seconds for a full fade out
};

class WindInstance {
public:
    WindInstance();

    // Switches state; fades are scaled by how far the weight has to move.
    void SetState(WindState newState);
    // Per-frame update. TODO(0x004AC820): also drives the gust tween from
    // the owner's movement state.
    void Update(float dt);

    WorldObject* owner = nullptr;   // +0x1C
    const WindDef* def = nullptr;   // +0x20
    WindState state = kWind_Off;    // +0x2C
    FloatTween fade;                // +0x30 writes 'weight'
    FloatTween gust;                // +0x54
    ape::Vec3 position{};           // +0x80 copied from the owner each frame
    float weight = 0.0f;            // +0x90
    float gustTime = 0.0f;          // +0x94
    bool gustEnabled = false;       // +0x9C
};

class WindSystem {
public:
    // Fades every source out (false) or back in (true).
    void SetEnabled(bool enabled);
    bool IsEnabled() const { return enabled; }
    // True if any source (active or fading) belongs to 'owner'.
    bool HasSourceFor(const WorldObject* owner) const;

    std::vector<WindInstance*> active;  // +0x80 on or fading in
    std::vector<WindInstance*> fading;  // +0x94 fading out
    bool enabled = true;                // +0xAC
};

// Removes every occurrence of 'value' from a small pointer array by moving
// the array's last element into its slot. NOTE: the owning class of this
// helper (array at +0x04, count at +0x0C) is not identified; the original
// keeps using the element count from before the loop.
void RemoveFromPointerArray(std::vector<void*>& array, void* value);

// Global wind parameters block (0x010C1F00...). NOTE: field meanings unknown.
struct WindParams {
    ape::Vec3 direction{};      // +0x00 copied from the global default
    float field_10 = 0.0f;
    float field_14 = 0.0f;
    float field_18 = 0.0f;
    float field_1C = 0.0f;
    float field_20 = 0.0f;
    float field_24 = 0.0f;
    float scale = 1.0f;         // +0x28
    bool enabled = true;        // +0x2C
    bool field_2D = false;
    bool field_2E = false;
    bool field_2F = false;
};

void InitWindParams(WindParams& params);
void SetDefaultWindDirection(const ape::Vec3& direction);
// Current global wind strength (0x010C2478).
float GlobalWindStrength();
void SetGlobalWindStrength(float strength);
// Clears the cached wind globals (resources looked up by name hash).
void ResetWindGlobals();

} // namespace sly
