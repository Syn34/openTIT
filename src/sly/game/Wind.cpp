// openTIT - Wind sources and the wind on/off system.
#include "sly/game/Wind.h"

#include "ape/core/ApeString.h"
#include "sly/game/EconomyTuning.h"
#include "sly/game/WorldInterface.h"

namespace sly {

namespace {


// Cached wind globals cleared by ResetWindGlobals (0x00F65F74..0x00F65F80,
// 0x010C2488). The two resources are looked up lazily by name hash
// (0x42361175 and 0x02D45643).
void* g_windResourceA = nullptr;
void* g_windResourceB = nullptr;
uint32_t g_windGlobal74 = 0;
uint32_t g_windGlobal80 = 0;
uint32_t g_windGlobal2488 = 0;

void WarnInvalidState(WindState state)
{
    ape::TempFormat("invalid wind state %h", (int)state);
}

void StartFade(FloatTween& fade, float from, float to, float duration)
{
    fade.from = from;
    fade.to = to;
    fade.elapsed = 0.0f;
    fade.SetDuration(duration);
}

} // namespace

WindInstance::WindInstance()
{
    fade.target = &weight;
}

// @func 0x004A9130 WindInstance::SetState
void WindInstance::SetState(WindState newState)
{
    state = newState;
    switch (newState) {
    case kWind_Off:
        weight = 0.0f;
        break;
    case kWind_FadingIn:
        if (!def || def->fadeInTime == 0.0f) {
            SetState(kWind_On);
            return;
        }
        StartFade(fade, weight, 1.0f, def->fadeInTime * (1.0f - weight));
        break;
    case kWind_On:
        weight = 1.0f;
        break;
    case kWind_FadingOut:
        if (!def || def->fadeOutTime == 0.0f) {
            SetState(kWind_Off);
            return;
        }
        StartFade(fade, weight, 0.0f, def->fadeOutTime * weight);
        break;
    default:
        break;
    }
}

void WindInstance::Update(float dt)
{
    if (owner)
        position = CharacterPosition(owner);
    if (state == kWind_FadingIn || state == kWind_FadingOut) {
        fade.Update(dt);
        if (fade.IsFinished())
            SetState(state == kWind_FadingIn ? kWind_On : kWind_Off);
    }
}

// @func 0x004AD580 WindSystem::SetEnabled
void WindSystem::SetEnabled(bool enable)
{
    if (!enable) {
        for (WindInstance* w : active) {
            if (w->state != kWind_FadingIn && w->state != kWind_On)
                WarnInvalidState(w->state);
            w->SetState(kWind_FadingOut);
            fading.push_back(w);
        }
        active.clear();
    } else {
        for (WindInstance* w : fading) {
            if (w->state != kWind_FadingOut)
                WarnInvalidState(w->state);
            w->SetState(kWind_FadingIn);
            active.push_back(w);
        }
        fading.clear();
    }
    enabled = enable;
}

// @func 0x004ACC34 WindSystem::HasSourceFor
bool WindSystem::HasSourceFor(const WorldObject* owner) const
{
    for (const WindInstance* w : active) {
        if (w->owner == owner)
            return true;
    }
    for (const WindInstance* w : fading) {
        if (w->owner == owner)
            return true;
    }
    return false;
}

// @func 0x004ACB90 RemoveFromPointerArray
void RemoveFromPointerArray(std::vector<void*>& array, void* value)
{
    const size_t originalCount = array.size();
    size_t count = originalCount;
    for (size_t i = 0; i < originalCount && i < count; ++i) {
        if (array[i] == value) {
            array[i] = array[originalCount - 1];
            --count;
        }
    }
    array.resize(count);
}

// @func 0x004AE360 InitWindParams
// NOTE: 0x010C1F00 is the value of the tuning variable
// Game.Economy.ThrowCollectible.FromSpawnerOffset, so this block probably
// belongs to the collectible throw code rather than to the wind.
void InitWindParams(WindParams& params)
{
    const float* offset = g_throwFromSpawnerOffset.Get();
    params.direction.x = offset[0];
    params.direction.y = offset[1];
    params.direction.z = offset[2];
    params.field_10 = 0.0f;
    params.field_14 = 0.0f;
    params.field_18 = 0.0f;
    params.field_1C = 0.0f;
    params.field_20 = 0.0f;
    params.field_24 = 0.0f;
    params.scale = 1.0f;
    params.enabled = true;
    params.field_2D = false;
    params.field_2E = false;
    params.field_2F = false;
}

void SetDefaultWindDirection(const ape::Vec3& direction)
{
    const float value[3] = { direction.x, direction.y, direction.z };
    g_throwFromSpawnerOffset.SetVec3(value);
}

// @func 0x004AFCA4 GlobalWindStrength
// NOTE: 0x010C2478 is the value of Game.Economy.ThrowCollectible.BoltIncreasePercent.
float GlobalWindStrength()
{
    return g_throwBoltIncreasePercent.Get();
}

void SetGlobalWindStrength(float strength)
{
    g_throwBoltIncreasePercent.SetFloat(strength);
}

// @func 0x004AE458 ResetWindGlobals
void ResetWindGlobals()
{
    g_windGlobal74 = 0;
    g_windGlobal2488 = 0;
    g_windResourceA = nullptr;
    g_windResourceB = nullptr;
    g_windGlobal80 = 0;
}

} // namespace sly
