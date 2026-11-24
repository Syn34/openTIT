// openTIT - Tweens, delayed states, approach values and easing curves.
#include "sly/game/Tween.h"

#include <cmath>

namespace sly {

namespace {

constexpr float kPi = 3.1415927f;
// The overshoot curve uses a slightly truncated pi literal in the original.
constexpr float kOvershootPi = 3.1415f;

float ComputeEasing(TweenEasing easing, float t)
{
    switch (easing) {
    case kEase_Linear:
        return t;
    case kEase_Cosine:
        return (ape::FastCos(t * kPi) - 1.0f) * -0.5f;
    case kEase_Overshoot: {
        const float x = t * 2.0f;
        const float x2 = x * x;
        if (x2 <= 1.0f)
            return x2;
        const float rest = 2.0f - x2;
        const float s = ape::FastSin(rest * 2.0f * kOvershootPi * 4.0f);
        return 1.0f + s * s * (1.0f - rest);
    }
    }
    return t;
}

} // namespace

// ---------------------------------------------------------------------------
// Tween
// ---------------------------------------------------------------------------

// @func 0x004A0990 Tween::Advance
void Tween::Advance(float dt)
{
    if (duration <= 0.0f) {
        elapsed = 0.0f;
        return;
    }

    switch (loop) {
    case kLoop_Once:
        elapsed += dt;
        if (elapsed > duration)
            elapsed = duration;
        break;
    case kLoop_Repeat:
        elapsed += dt;
        if (elapsed > duration)
            elapsed -= duration;
        break;
    case kLoop_PingPong:
        if (forward) {
            elapsed += dt;
            if (elapsed > duration) {
                forward = false;
                elapsed = duration - (elapsed - duration);
            }
        } else {
            elapsed -= dt;
            if (elapsed < 0.0f) {
                forward = true;
                elapsed = -elapsed;
            }
        }
        break;
    default:
        break;
    }

    if (easing <= kEase_Overshoot)
        progress = ComputeEasing(easing, elapsed / duration);
}

// @func 0x004A0BB0 Tween::IsFinished
bool Tween::IsFinished() const
{
    if (duration < 0.0f)
        return true;
    return loop == kLoop_Once && elapsed >= duration;
}

// @func 0x004A0C08 Tween::Clear
void Tween::Clear()
{
    duration = 0.0f;
    elapsed = 0.0f;
}

// @func 0x004A0C20 Tween::SetDuration
void Tween::SetDuration(float newDuration)
{
    duration = newDuration;
    float t = elapsed < newDuration ? elapsed : newDuration;
    elapsed = t < 0.0f ? 0.0f : t;
}

// NOTE: the console keeps one out-of-line copy of the base destructor per
// subclass translation unit; all of them only restore the base vtable.
// @func 0x004A091C Tween::~Tween
// @func 0x004A0CA4 Tween::~Tween (copy 2)
// @func 0x004A0E08 Tween::~Tween (copy 3)
// @func 0x004A0F70 Tween::~Tween (copy 4)
// @func 0x004A092C Tween::~Tween (deleting)
Tween::~Tween() = default;

// @func 0x004A0C4C IntTween::IntTween
IntTween::IntTween() = default;

// @func 0x004A0CB4 IntTween::~IntTween (deleting)
IntTween::~IntTween() = default;

// @func 0x004A0D00 IntTween::Update
void IntTween::Update(float dt)
{
    Advance(dt);
    value = (int32_t)((1.0f - progress) * (float)from) + (int32_t)(progress * (float)to);
    if (target)
        *target = value;
}

// @func 0x004A0DB0 FloatTween::FloatTween
FloatTween::FloatTween() = default;

// @func 0x004A0E18 FloatTween::~FloatTween (deleting)
FloatTween::~FloatTween() = default;

// @func 0x004A0E60 FloatTween::Update
void FloatTween::Update(float dt)
{
    Advance(dt);
    value = (1.0f - progress) * from + progress * to;
    if (target)
        *target = value;
}

// @func 0x004A0EE0 Vec3Tween::Vec3Tween
Vec3Tween::Vec3Tween() = default;

// @func 0x004A0F80 Vec3Tween::~Vec3Tween (deleting)
Vec3Tween::~Vec3Tween() = default;

// @func 0x004A0FC8 Vec3Tween::Update
void Vec3Tween::Update(float dt)
{
    Advance(dt);
    const float a = 1.0f - progress;
    value.x = a * from.x + progress * to.x;
    value.y = a * from.y + progress * to.y;
    value.z = a * from.z + progress * to.z;
    if (target) {
        target->x = value.x;
        target->y = value.y;
        target->z = value.z;
    }
}

// ---------------------------------------------------------------------------
// DelayedState
// ---------------------------------------------------------------------------

// @func 0x004A08A4 DelayedState::Init
void DelayedState::Init(uint8_t initial, float holdTime)
{
    state = initial;
    elapsed = 0.0f;
    delay = holdTime;
}

// @func 0x004A08B8 DelayedState::Get
uint8_t DelayedStateGet(const DelayedState& s) { return s.state; }

// @func 0x004A08C0 DelayedState::Get (second copy, different class)
uint8_t DelayedStateGetCopy(const DelayedState& s) { return s.state; }

// @func 0x004A08C8 DelayedState::Request
void DelayedState::Request(uint8_t wanted, float dt)
{
    if (state == wanted) {
        elapsed = 0.0f;
        return;
    }
    elapsed += dt;
    if (delay <= elapsed)
        state = wanted;
}

// @func 0x004A090C DelayedState::Force
void DelayedState::Force(uint8_t newState)
{
    state = newState;
    elapsed = 0.0f;
}

// ---------------------------------------------------------------------------
// Approach values
// ---------------------------------------------------------------------------

// @func 0x004A11B4 ApproachFloat::Clear
void ApproachFloat::Clear()
{
    current = 0.0f;
    target = 0.0f;
    rate = 0.0f;
}

// @func 0x004A11CC ApproachFloat::Set
void ApproachFloat::Set(float currentValue, float targetValue, float ratePerSecond)
{
    current = currentValue;
    target = targetValue;
    rate = ratePerSecond;
}

// @func 0x004A11E0 ApproachFloat::Update
void ApproachFloat::Update(float dt)
{
    if (current == target)
        return;
    const float step = rate * dt;
    const float diff = target - current;
    if (std::fabs(diff) <= step)
        current = target;
    else
        current += diff < 0.0f ? -step : step;
}

// @func 0x004A109C ApproachVec3::Clear
void ApproachVec3::Clear()
{
    current = ape::Vec3{};
    target = ape::Vec3{};
    rate = 0.0f;
}

// @func 0x004A10F0 ApproachVec3::Update
void ApproachVec3::Update(float dt)
{
    if (current.x == target.x && current.y == target.y && current.z == target.z)
        return;
    const float dx = target.x - current.x;
    const float dy = target.y - current.y;
    const float dz = target.z - current.z;
    const float step = rate * dt;
    const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (dist <= step) {
        current.x = target.x;
        current.y = target.y;
        current.z = target.z;
        return;
    }
    const float k = step / dist;
    current.x += dx * k;
    current.y += dy * k;
    current.z += dz * k;
}

// ---------------------------------------------------------------------------
// Easing curves
// ---------------------------------------------------------------------------

// @func 0x004A5798 EaseInOutQuad
float EaseInOutQuad(float t)
{
    if (t < 0.5f)
        return 2.0f * t * t;
    const float u = 1.0f - t;
    return 1.0f - 2.0f * u * u;
}

// @func 0x004A6B98 SmoothFalloff
float SmoothFalloff(float a, float range, float b)
{
    const float d = std::fabs(b - a);
    if (range < d)
        return 0.0f;
    const float x = d / range;
    return 1.0f - x * x * (3.0f - 2.0f * x);
}

// @func 0x004A6BF0 RampEnvelope
float RampEnvelope(float rampIn, float hold, float rampOut, float t)
{
    if (t < rampIn)
        return t / rampIn;
    if (rampIn + hold < t) {
        const float down = 1.0f - (t - (rampIn + hold)) / rampOut;
        return down < 0.0f ? 0.0f : down;
    }
    return hold > 0.0f ? 1.0f : 0.0f;
}

} // namespace sly
