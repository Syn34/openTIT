// openTIT - Follow-camera settings and the per-frame camera view.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>

namespace sly {
namespace camera {

// What a camera mode produces each frame and the game camera eases toward
// (console block of 0x64 bytes, e.g. GameCamera +0x1E0).
struct CameraViewState {
    // Camera orientation and position; the z axis is the look direction (the
    // camera's world object and the renderer use x and z negated).
    ape::Matrix34 transform = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 0 } };
    float fov = 55.0f;                 // 0x40 vertical field of view (degrees)
    float focusDistance = 7.5f;        // 0x44 depth-of-field focus distance
    bool dofEnabled = false;           // 0x48
    float dofRange = 0.0f;             // 0x4C (clamped to 0.01..999 when applied)
    float dofBlendAlpha = 0.0f;        // 0x50
    uint32_t dofIterations = 0;        // 0x54 (applied as 1..4)
    bool filterEnabled = false;        // 0x58 colour filter
    float filterBlend = 0.0f;          // 0x5C (clamped to 0..1 when applied)
    // 0x60 RGB(A). NOTE: the default is the console global 0x01164CA8, assumed opaque black.
    uint8_t filterColor[4] = { 0, 0, 0, 0xFF };
};

// Settings of the default (follow) camera. Characters and vehicles carry their
// own copy in their definition (character def +0x3F0, vehicle def +0x1B0); the
// defaults come from the Camera.Default.* tuning variables (console block of
// 0x80 bytes).
struct CameraSettings {
    CameraSettings();                  // 0x000FD7AC
    // Back to the tuning defaults, clearing the two extra fields (0x000FD974).
    void Reset();
    // Copies the tuning defaults, leaving the two extra fields alone (0x000FD5F0).
    void LoadDefaults();
    // Eases this set toward `target` at the target's own rates (0x000FDB88).
    void BlendToward(float dt, const CameraSettings& target);

    float interp = 0.0f;                 // 0x00 Camera.Default.Interp (rate of most blends)
    float fovInterp = 0.0f;              // 0x04
    float fov = 0.0f;                    // 0x08
    ape::Vec3 posAdjust{};               // 0x10
    float posAdjustTime = 0.0f;          // 0x20
    float gimbalRadius = 0.0f;           // 0x24
    float angleHome = 0.0f;              // 0x28 pitch the camera returns to (degrees)
    float angleMin = 0.0f;               // 0x2C
    float angleMax = 0.0f;               // 0x30
    float lookAheadXZ = 0.0f;            // 0x34
    float lookAheadY = 0.0f;             // 0x38
    float targetYMinSoft = 0.0f;         // 0x3C
    float targetYMaxSoft = 0.0f;         // 0x40
    float horzInputRotScalar = 0.0f;     // 0x44
    float lookAtXZInterp = 0.0f;         // 0x48
    float lookAtYUpInterp = 0.0f;        // 0x4C
    float lookAtYDownInterp = 0.0f;      // 0x50
    float vertInputRotScalar = 0.0f;     // 0x54
    float vertInputRotVelInterp = 0.0f;  // 0x58
    float colRadius = 0.0f;              // 0x5C
    int32_t lookAheadVelMode = 0;        // 0x60
    float headingMaxRate = 0.0f;         // 0x64
    float headingLowSpeed = 0.0f;        // 0x68
    float headingHighSpeed = 0.0f;       // 0x6C
    float headingLowSpeedInterp = 0.0f;  // 0x70
    float headingHighSpeedInterp = 0.0f; // 0x74
    bool field_78 = false;               // 0x78 NOTE: set by the owner's definition only
    // 0x7C: fields an override replaces (bit per field in declaration order:
    // bit 0 interp ... bit 3 posAdjust ... bit 25 headingHighSpeedInterp).
    uint32_t overrideMask = 0;

    // Copies the fields `overrideMask` of `other` selects.
    void ApplyOverride(const CameraSettings& other);
};

} // namespace camera
} // namespace sly
