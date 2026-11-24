// openTIT - Damped camera springs (see CameraSprings.h).
#include "sly/camera/CameraSprings.h"

#include "crt/libc/Libc.h"

#include <algorithm>
#include <cmath>

namespace sly {
namespace camera {

SpringStep SpringStep::Make(float frequency, float damping, float dt)
{
    const float stiffness = frequency * frequency * 36.0f;
    const float friction = damping * frequency * 9.0f;
    const float inv = 1.0f / (dt * dt * stiffness + dt * friction + 1.0f);
    return { stiffness * inv, (friction + dt * stiffness) * inv };
}

// @func 0x000F8C70 SpringVec3::Step
void SpringVec3::Step(float frequency, float springDamping, float dt)
{
    const SpringStep k = SpringStep::Make(frequency, springDamping, dt);
    const ape::Vec3 wanted = useTargetVelocity ? targetVelocity : ape::Vec3{};
    velocity.x += dt * (k.pull * (target.x - value.x) + k.drag * (wanted.x - velocity.x));
    velocity.y += dt * (k.pull * (target.y - value.y) + k.drag * (wanted.y - velocity.y));
    velocity.z += dt * (k.pull * (target.z - value.z) + k.drag * (wanted.z - velocity.z));
    value.x += dt * velocity.x;
    value.y += dt * velocity.y;
    value.z += dt * velocity.z;
}

// @func 0x000F8E30 AngularVelocityBetween
ape::Vec3 AngularVelocityBetween(float time, const ape::Quat& from, const ape::Quat& to)
{
    if (time <= 0.0f)
        return {};

    // Take the shorter way round, then delta = to * from^-1.
    ape::Quat start = from;
    if (start.x * to.x + start.y * to.y + start.z * to.z + start.w * to.w < 0.0f)
        start = { -start.x, -start.y, -start.z, -start.w };
    const float lenSq = start.x * start.x + start.y * start.y + start.z * start.z + start.w * start.w;
    const ape::Quat inverse = { -start.x / lenSq, -start.y / lenSq, -start.z / lenSq, start.w / lenSq };
    ape::Quat delta;
    ape::QuatMul(delta, to, inverse);

    const float invLen = 1.0f / std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z +
                                          delta.w * delta.w);
    const float w = delta.w * invLen;
    const float angle = crt::ACosF(ape::Clamp(w, -1.0f, 1.0f)) * 2.0f;
    const float sinHalf = std::sqrt(std::max(0.0f, 1.0f - w * w));
    if (sinHalf < 0.0001f)
        return {};

    ape::Vec3 axis = { delta.x * invLen / sinHalf, delta.y * invLen / sinHalf, delta.z * invLen / sinHalf, 0.0f };
    const float speed = (angle / time) / std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    return { axis.x * speed, axis.y * speed, axis.z * speed, 0.0f };
}

// @func 0x000F90F8 SpringQuat::Step
void SpringQuat::Step(float frequency, float springDamping, float dt)
{
    const SpringStep k = SpringStep::Make(frequency, springDamping, dt);
    const ape::Vec3 wanted = useTargetVelocity ? targetAngularVelocity : ape::Vec3{};
    // Rotation still to go, expressed as an angular velocity over one second.
    const ape::Vec3 offset = AngularVelocityBetween(1.0f, value, target);

    angularVelocity.x += dt * (k.drag * (wanted.x - angularVelocity.x) + k.pull * offset.x);
    angularVelocity.y += dt * (k.drag * (wanted.y - angularVelocity.y) + k.pull * offset.y);
    angularVelocity.z += dt * (k.drag * (wanted.z - angularVelocity.z) + k.pull * offset.z);

    const ape::Vec3 turn = { angularVelocity.x * dt, angularVelocity.y * dt, angularVelocity.z * dt, 0.0f };
    const float angle = std::sqrt(turn.x * turn.x + turn.y * turn.y + turn.z * turn.z);
    if (angle > 0.001f) {
        const float s = ape::FastSin(angle * 0.5f) / angle;
        const ape::Quat step = { turn.x * s, turn.y * s, turn.z * s, ape::FastCos(angle * 0.5f) };
        ape::Quat rotated;
        ape::QuatMul(rotated, step, value);
        value = rotated;
    }
}

// @func 0x000F9380 SpringFloat::Update
float SpringFloat::Update(float newTarget, float dt)
{
    if (dt <= 0.0f)
        return value;

    const float previous = target;
    target = newTarget;
    targetVelocity = (newTarget - previous) / dt;

    const float easeTime = std::max(kMinEaseTime, timeLeft);
    const SpringStep k = SpringStep::Make(1.0f / easeTime, damping, dt);
    const float wanted = useTargetVelocity ? targetVelocity : 0.0f;
    velocity += dt * (k.pull * (newTarget - value) + k.drag * (wanted - velocity));
    value += dt * velocity;
    timeLeft = std::max(0.0f, timeLeft - dt);
    return value;
}

} // namespace camera
} // namespace sly
