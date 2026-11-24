// openTIT - Small value animation helpers used by the game layer: tweens,
// debounced state timers, rate-limited approach values and easing curves.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace sly {

// ---------------------------------------------------------------------------
// Tween: a clock that runs from 0 to a duration and produces an eased
// progress value in [0, 1] (vtable 0x00B21B58). Subclasses blend between two
// values with that progress and optionally write the result to a target.
// ---------------------------------------------------------------------------
enum TweenEasing : uint16_t {
    kEase_Linear    = 0,
    kEase_Cosine    = 1,  // slow-in/slow-out half cosine
    kEase_Overshoot = 2,  // NOTE: quadratic rise to 1 at half time, then a
                          // decaying oscillation around 1; name is a guess
};

enum TweenLoop : uint16_t {
    kLoop_Once     = 0,   // clamp at the end
    kLoop_Repeat   = 1,   // wrap around
    kLoop_PingPong = 2,   // bounce between the ends
};

class Tween {
public:
    Tween() = default;
    virtual ~Tween();

    // Advances the clock by dt and recomputes the eased progress.
    void Advance(float dt);
    // True once a non-looping tween reached its duration, or if the tween has
    // a negative (disabled) duration.
    bool IsFinished() const;
    // Clears both the elapsed time and the duration.
    void Clear();
    // Changes the duration, keeping the elapsed time inside [0, duration].
    void SetDuration(float duration);

    float elapsed = 0.0f;              // +0x04
    float duration = 0.0f;             // +0x08
    float progress = 0.0f;             // +0x0C eased value of elapsed/duration
    TweenEasing easing = kEase_Linear; // +0x10
    TweenLoop loop = kLoop_Once;       // +0x12 bits 1..15
    bool forward = true;               // +0x13 bit 0 (ping-pong direction)
};

// Blends two integers (vtable 0x00B21B70).
class IntTween : public Tween {
public:
    IntTween();
    ~IntTween() override;
    void Update(float dt);

    int32_t from = 0;                 // +0x14
    int32_t to = 0;                   // +0x18
    int32_t value = 0;                // +0x1C
    int32_t* target = nullptr;        // +0x20 optional output
};

// Blends two floats (vtable 0x00B21B88).
class FloatTween : public Tween {
public:
    FloatTween();
    ~FloatTween() override;
    void Update(float dt);

    float from = 0.0f;                // +0x14
    float to = 0.0f;                  // +0x18
    float value = 0.0f;               // +0x1C
    float* target = nullptr;          // +0x20 optional output
};

// Blends two vectors (vtable 0x00B21BA8).
class Vec3Tween : public Tween {
public:
    Vec3Tween();
    ~Vec3Tween() override;
    void Update(float dt);

    ape::Vec3 from{};                 // +0x20
    ape::Vec3 to{};                   // +0x30
    ape::Vec3 value{};                // +0x40
    ape::Vec3* target = nullptr;      // +0x50 optional output
};

// ---------------------------------------------------------------------------
// Debounced state: a requested state only becomes current after it has been
// requested continuously for 'delay' seconds.
// ---------------------------------------------------------------------------
struct DelayedState {
    uint8_t state = 0;      // +0x00
    float elapsed = 0.0f;   // +0x04 time the pending request has been held
    float delay = 0.0f;     // +0x08

    void Init(uint8_t initial, float holdTime);
    uint8_t Get() const { return state; }
    void Request(uint8_t wanted, float dt);
    void Force(uint8_t newState);
};

uint8_t DelayedStateGet(const DelayedState& s);
uint8_t DelayedStateGetCopy(const DelayedState& s);

// ---------------------------------------------------------------------------
// Values that move towards a target at a fixed rate per second.
// ---------------------------------------------------------------------------
struct ApproachFloat {
    float current = 0.0f;   // +0x00
    float target = 0.0f;    // +0x04
    float rate = 0.0f;      // +0x08 units per second

    void Clear();
    void Set(float currentValue, float targetValue, float ratePerSecond);
    void Update(float dt);
};

struct ApproachVec3 {
    ape::Vec3 current{};    // +0x00
    ape::Vec3 target{};     // +0x10
    float rate = 0.0f;      // +0x20 units per second

    void Clear();
    void Update(float dt);
};

// ---------------------------------------------------------------------------
// Easing curves.
// ---------------------------------------------------------------------------
// Quadratic ease-in/ease-out over t in [0, 1].
float EaseInOutQuad(float t);
// Smooth (cubic Hermite) falloff: 1 when a == b, 0 once |a - b| >= range.
float SmoothFalloff(float a, float range, float b);
// Trapezoid envelope: ramps up over 'rampIn', holds for 'hold', then ramps
// down over 'rampOut'. NOTE: parameter names inferred from the arithmetic.
float RampEnvelope(float rampIn, float hold, float rampOut, float t);

} // namespace sly
