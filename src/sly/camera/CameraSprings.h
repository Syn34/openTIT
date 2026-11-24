// openTIT - Damped springs used to ease the game camera between views.
//
// The game camera keeps one spring per channel (position, orientation, field
// of view). Each frame the spring is pulled toward the newest target with an
// implicit (unconditionally stable) step of a critically-damped style spring:
//
//     stiffness k = 36 * f^2,  damping c = 9 * d * f      (f = 1 / ease time)
//     v' = v + dt * (k (target - x) + c (targetVel - v)) / (1 + dt c + dt^2 k)
//     x' = x + dt * v'
//
// so that a blend started with an ease time T has (nearly) settled after T
// seconds. The console classes have a virtual destructor only (vtables
// 0x00B13988 / 0x00B13998 / 0x00B139A8, code outside this range).
#pragma once

#include "ape/math/ApeMath.h"
#include "ape/math/ApeMatrixOps.h"

namespace sly {
namespace camera {

// Shared coefficients of one implicit spring step.
struct SpringStep {
    float pull;    // multiplies (target - value)
    float drag;    // multiplies (targetVelocity - velocity)
    static SpringStep Make(float frequency, float damping, float dt);
};

// Vector spring (console layout: value +0x10, velocity +0x20, target +0x30,
// target velocity +0x40, time left +0x50, use target velocity +0x54,
// damping +0x58).
class SpringVec3 {
public:
    virtual ~SpringVec3() = default;
    // One step toward `target` (0x000F8C70).
    void Step(float frequency, float damping, float dt);

    ape::Vec3 value{};
    ape::Vec3 velocity{};
    ape::Vec3 target{};
    ape::Vec3 targetVelocity{};
    float timeLeft = 0.0f;
    bool useTargetVelocity = false;
    float damping = 0.0f;
};

// Orientation spring; velocity is an angular velocity (axis * radians/s).
class SpringQuat {
public:
    virtual ~SpringQuat() = default;
    // One step toward `target` (0x000F90F8).
    void Step(float frequency, float damping, float dt);

    ape::Quat value{ 0.0f, 0.0f, 0.0f, 1.0f };
    ape::Vec3 angularVelocity{};
    ape::Quat target{ 0.0f, 0.0f, 0.0f, 1.0f };
    ape::Vec3 targetAngularVelocity{};
    float timeLeft = 0.0f;
    bool useTargetVelocity = false;
    float damping = 0.0f;
};

// Scalar spring that tracks a moving target and derives the target's
// velocity from its last two values (console layout +0x04..+0x1C).
class SpringFloat {
public:
    virtual ~SpringFloat() = default;
    // Feeds the newest target and returns the eased value (0x000F9380).
    float Update(float newTarget, float dt);

    float value = 0.0f;           // +0x04
    float velocity = 0.0f;        // +0x08
    float target = 0.0f;          // +0x0C
    float targetVelocity = 0.0f;  // +0x10
    float timeLeft = 0.0f;        // +0x14
    bool useTargetVelocity = false;   // +0x18
    float damping = 0.0f;         // +0x1C
};

// Angular velocity that turns `from` into `to` within `time` seconds
// (axis * angle / time); zero for a non-positive time or no rotation.
ape::Vec3 AngularVelocityBetween(float time, const ape::Quat& from, const ape::Quat& to);

// Shortest ease time the springs accept (the frequency is 1 / time).
constexpr float kMinEaseTime = 0.05f;

} // namespace camera
} // namespace sly
