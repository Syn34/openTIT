// openTIT - Follow-camera settings (see CameraSettings.h).
#include "sly/camera/CameraSettings.h"

#include "sly/game/GameMath.h"
#include "sly/tuning/CameraTuning.h"

#include <algorithm>
#include <cmath>

namespace sly {
namespace camera {

// @func 0x000FD7AC CameraSettings::CameraSettings
CameraSettings::CameraSettings()
{
    LoadDefaults();
    field_78 = false;
    field_7C = 0.0f;
}

// @func 0x000FD974 CameraSettings::Reset
void CameraSettings::Reset()
{
    field_78 = false;
    field_7C = 0.0f;
    LoadDefaults();
}

// @func 0x000FD5F0 CameraSettings::LoadDefaults
void CameraSettings::LoadDefaults()
{
    interp = g_cameraDefaultInterp;
    fovInterp = g_cameraDefaultFOVInterp;
    fov = g_cameraDefaultFOV;
    const float* adjust = g_cameraDefaultPosAdjust.Get();
    posAdjust = { adjust[0], adjust[1], adjust[2], posAdjust.pad };
    posAdjustTime = g_cameraDefaultPosAjustTime;
    gimbalRadius = g_cameraDefaultGimbalRadius;
    angleHome = g_cameraDefaultAngleHome;
    angleMin = g_cameraDefaultAngleMin;
    angleMax = g_cameraDefaultAngleMax;
    lookAheadXZ = g_cameraDefaultLookAheadXZ;
    lookAheadY = g_cameraDefaultLookAheadY;
    targetYMinSoft = g_cameraDefaultTargetYMinSoft;
    targetYMaxSoft = g_cameraDefaultTargetYMaxSoft;
    horzInputRotScalar = g_cameraDefaultHorzInputRotScalar;
    lookAtXZInterp = g_cameraDefaultLookAtXZInterp;
    lookAtYUpInterp = g_cameraDefaultLookAtYUpInterp;
    lookAtYDownInterp = g_cameraDefaultLookAtYDownInterp;
    vertInputRotScalar = g_cameraDefaultVertInputRotScalar;
    vertInputRotVelInterp = g_cameraDefaultVertInputRotVelInterp;
    colRadius = g_cameraDefaultColRadius;
    lookAheadVelMode = g_cameraDefaultLookAheadVelMode;
    headingMaxRate = g_cameraDefaultHeadingInterpMaxRate;
    headingLowSpeed = g_cameraDefaultHeadingInterpLowSpeed;
    headingHighSpeed = g_cameraDefaultHeadingInterpHighSpeed;
    headingLowSpeedInterp = g_cameraDefaultHeadingInterpLowSpeedInterp;
    headingHighSpeedInterp = g_cameraDefaultHeadingInterpHighSpeedInterp;
}

namespace {

constexpr float kSettleEpsilon = 1e-05f;

// Eases one value toward its target unless it is already there.
void Approach(float& value, float target, float rate, float dt)
{
    if (std::fabs(value - target) >= kSettleEpsilon)
        value = ApproachExp(value, target, rate, dt);
}

} // namespace

// @func 0x000FDB88 CameraSettings::BlendToward
void CameraSettings::BlendToward(float dt, const CameraSettings& target)
{
    const float rate = target.interp;

    Approach(fov, target.fov, target.fovInterp, dt);
    if (std::fabs(posAdjust.x - target.posAdjust.x) >= kSettleEpsilon ||
        std::fabs(posAdjust.y - target.posAdjust.y) >= kSettleEpsilon ||
        std::fabs(posAdjust.z - target.posAdjust.z) >= kSettleEpsilon) {
        ape::Vec3 eased;
        ApproachExp3(eased, rate, dt, posAdjust, target.posAdjust);
        posAdjust = { eased.x, eased.y, eased.z, posAdjust.pad };
    }
    // NOTE: gimbalRadius and horzInputRotScalar are left alone, as on the console.
    Approach(posAdjustTime, target.posAdjustTime, rate, dt);
    Approach(angleHome, target.angleHome, rate, dt);
    Approach(angleMin, target.angleMin, rate, dt);
    Approach(angleMax, target.angleMax, rate, dt);
    Approach(lookAheadXZ, target.lookAheadXZ, rate, dt);
    Approach(lookAheadY, target.lookAheadY, rate, dt);
    Approach(targetYMinSoft, target.targetYMinSoft, rate, dt);
    Approach(targetYMaxSoft, target.targetYMaxSoft, rate, dt);
    Approach(lookAtXZInterp, target.lookAtXZInterp, rate, dt);
    Approach(lookAtYUpInterp, target.lookAtYUpInterp, rate, dt);
    Approach(lookAtYDownInterp, target.lookAtYDownInterp, rate, dt);
    Approach(vertInputRotScalar, target.vertInputRotScalar, rate, dt);
    Approach(vertInputRotVelInterp, target.vertInputRotVelInterp, rate, dt);
    Approach(colRadius, target.colRadius, rate, dt);
    lookAheadVelMode = target.lookAheadVelMode;
    Approach(headingMaxRate, target.headingMaxRate, rate, dt);
    headingLowSpeed = target.headingLowSpeed;
    headingHighSpeed = target.headingHighSpeed;
    headingLowSpeedInterp = target.headingLowSpeedInterp;
    headingHighSpeedInterp = target.headingHighSpeedInterp;

    colRadius = ape::Clamp(colRadius, 0.1f, 1.0f);
    angleMin = ape::Clamp(angleMin, -89.0f, 89.0f);
    angleMax = ape::Clamp(angleMax, -89.0f, 89.0f);
    angleHome = std::max(std::min(angleHome, angleMax), angleMin);
}

} // namespace camera
} // namespace sly
