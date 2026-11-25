// openTIT - Camera modes: the behaviours the game camera stacks up.
//
// The game camera (sly::entity::GameCamera) owns a stack of camera modes; the
// top one produces the view every frame and the camera eases from the old
// view when the stack changes. Every mode derives from CameraMode (console
// root vtable 0x00B0DAEC). The root's own virtuals are empty defaults
// (0x007378A4-0x00737990); the per-mode type and name getters are one-liners
// at 0x00749xxx. Modes are allocated from the
// fixed-size bucket allocator and destroy themselves through Release().
#pragma once

#include "sly/camera/CameraSettings.h"

#include <cstdint>

namespace sly {
namespace entity { class Entity; class EntityComponent; class GameCamera; }

namespace camera {

using entity::Entity;
using entity::GameCamera;

// Value of virtual slot 2 (GetType).
enum CameraModeType : int32_t {
    kCamMode_Spline       = 2,
    kCamMode_Fixed        = 3,
    kCamMode_Binocucom    = 6,
    kCamMode_Vehicle      = 7,
    kCamMode_Spotlight    = 8,
    kCamMode_HavokVehicle = 9,
    kCamMode_Crawl        = 10,
    kCamMode_Default      = 11,
    kCamMode_Shooter      = 12,
    kCamMode_Ortho        = 13,
    kCamMode_Shoulder     = 14,
    kCamMode_Track        = 15,
    kCamMode_Orbit        = 16,
    kCamMode_Strafe       = 17,
};

class CameraMode {
public:
    explicit CameraMode(GameCamera* owner) : camera(owner) {}
    // @func 0x00737850 CameraMode::~CameraMode
    // @func 0x00737860 CameraMode::~CameraMode (deleting)
    virtual ~CameraMode() = default;                                   // slots 0/1
    virtual int32_t GetType() const = 0;                               // slot 2
    virtual const char* GetName() const = 0;                           // slot 3
    // Per-frame work, split in three stages run by Tick (slots 4-6).
    // NOTE: the split is the console's; what each stage does depends on the mode.
    // @func 0x007378A4 CameraMode::Update
    virtual void Update(float dt, CameraViewState& view) { (void)dt; (void)view; }
    // @func 0x007378A8 CameraMode::UpdateStage2
    virtual void UpdateStage2(float dt, CameraViewState& view) { (void)dt; (void)view; }
    // @func 0x007378AC CameraMode::UpdateStage3
    virtual void UpdateStage3(float dt, CameraViewState& view) { (void)dt; (void)view; }
    // The mode became the top of the stack with no blend (slot 7) or blending
    // from the view the camera had (slot 8).
    // @func 0x007378B0 CameraMode::Activate
    virtual void Activate() {}
    // @func 0x007378B4 CameraMode::ActivateFrom
    virtual void ActivateFrom(const CameraViewState& previous) { (void)previous; }
    // Another mode was pushed on top of this one, or this one was removed (slot 9).
    // @func 0x007378B8 CameraMode::Deactivate
    virtual void Deactivate() {}
    // Slot 10. Modes that end on their own (spline, shooter) take themselves
    // off the camera, pass the call to the mode below and delete themselves;
    // others only drop transient state.
    // @func 0x007378BC CameraMode::Finish
    virtual void Finish() {}
    // Back to the mode's default settings (slot 11).
    // @func 0x007378C0 CameraMode::ResetSettings
    virtual void ResetSettings() {}
    // Runs the three stages (slot 12); false would keep the previous view.
    // @func 0x007378C4 CameraMode::Tick
    virtual bool Tick(float dt, CameraViewState& view)
    {
        Update(dt, view);
        UpdateStage2(dt, view);
        UpdateStage3(dt, view);
        return true;
    }
    // After the entity transforms are final (slot 13).
    // @func 0x00737980 CameraMode::PostUpdate
    virtual void PostUpdate(float dt, CameraViewState& view) { (void)dt; (void)view; }
    // Entity the mode follows or looks at, if any (slot 14).
    // @func 0x00737984 CameraMode::GetTarget
    virtual Entity* GetTarget() const { return nullptr; }
    // Debug drawing (slot 15) and the debug overlay text (slot 16).
    // @func 0x0073798C CameraMode::DebugDraw
    virtual void DebugDraw(uint32_t flags) { (void)flags; }
    // @func 0x00737990 CameraMode::GetDebugText
    virtual const char* GetDebugText() { return ""; }

    GameCamera* camera = nullptr;   // 0x04
    // 0x08: stays above modes pushed without the flag (see GameCamera::PushMode).
    bool keepOnTop = false;
};

} // namespace camera
} // namespace sly
