// openTIT - The game camera entity (console vtable 0x00B139B8, 0x390 bytes).
//
// The game camera is an entity that owns a stack of camera modes
// (sly/camera/CameraMode.h). Every frame the top mode fills a view
// (CameraViewState); the camera eases from the previous view with three
// springs (position, orientation, field of view) whenever the stack changes,
// writes the result to its world object (an ApeCameraInstance) and to the
// render target, and applies the depth-of-field / colour-filter settings the
// view carries. An optional camera animation (a second, "_additive" camera
// object) can be layered on top.
#pragma once

#include "sly/camera/CameraMode.h"
#include "sly/camera/CameraSettings.h"
#include "sly/camera/CameraSprings.h"
#include "sly/entity/Entity.h"

namespace ape::world { class ApeObj; }

namespace sly {
namespace entity {

// View handed to the renderer and the camera blends.
struct CameraView {
    ape::Matrix34 transform;
    float field_40 = 0.0f;   // camera +0x164 field of view
    float field_44 = 0.0f;   // camera +0x168 focus distance
};

// Bits of GameCamera::cameraFlags (+0x1D4).
enum GameCameraFlags : uint32_t {
    kCamFlag_PostFxDof   = 1u << 29,   // apply the view's depth of field
    kCamFlag_PostFx      = 1u << 30,   // apply the view's post effects at all
    // Set and cleared by the camera animation's events; while set the "Anim"
    // mode leaves the view alone. NOTE: name from use.
    kCamFlag_AnimHold    = 1u << 31,
};

class GameCamera : public Entity {
public:
    // NOTE: the stack runs from +0x170 to the anim mode pointer at +0x1B0.
    static constexpr int kMaxModes = 16;
    using CameraMode = camera::CameraMode;
    using CameraViewState = camera::CameraViewState;

    ~GameCamera() override;

    // -- virtual overrides -------------------------------------------------------
    bool PostUpdate(float dt) override;                       // slot 5
    bool Release(bool kill) override;                         // slot 10
    void UpdateTransform(float dt) override;                  // slot 12
    void HandleMessage(const EntityMessage& msg) override;    // slot 43
    void SetAttachParent(Entity* parent) override;            // slot 46
    // Slot 53: on, the camera follows its own animated camera object through
    // an "Anim" mode; off, that mode is removed again.
    void SetActiveVisual(bool on) override;
    // Slot 59 (base 0x000DD368): animation events that hold or release the
    // animated view. NOTE: the base slot is not declared on Entity yet.
    void OnAnimEvent(const char* name, uint32_t arg0, uint32_t arg1, bool fired);

    // -- view ---------------------------------------------------------------------
    void GetView(CameraView& out) const;
    // Renders through a primary render target while enabled (0x000FA3CC).
    void SetRenderEnabled(bool enabled);
    // Applies the eased view to the entity transform and the render target.
    void ApplyView(bool force);                                // 0x000FAE00
    // Applies an explicit view at once (0x000FB31C).
    void SetView(const CameraViewState& view, bool force);
    // Applies the current eased view again (0x000FCC3C).
    void RefreshView();
    // Puts the world object back to the identity transform (0x000FCD1C).
    void ResetObjectTransform();
    // Advances the springs toward the current mode view (0x000FB450).
    void UpdateBlend(float dt);
    // Restarts the blend from the current camera position, orientation and
    // field of view over `time` seconds (negative: keep the current time).
    void RestartBlend(float time);                              // 0x000FA660
    // Splits the screen for player `slot` of a split-screen layout (0x000F94C0).
    void SetViewportLayout(int layout);

    // -- mode stack ----------------------------------------------------------------
    bool HasMode(const CameraMode* mode) const;
    // Mode pushed just before `mode`, or null when it is the oldest or absent.
    CameraMode* ModeBelow(const CameraMode* mode) const;
    CameraMode* TopMode() const { return modeCount ? modes[modeCount - 1] : nullptr; }
    // Pushes a mode and blends to it over `blendTime` (0x000FC040).
    void PushMode(float blendTime, CameraMode* mode, bool keepOnTop);
    // Removes a mode (blending to the one below over `blendTime` when it was on
    // top). Returns the mode, or null when it was not on the stack (0x000FBB88).
    CameraMode* RemoveMode(float blendTime, CameraMode* mode);

    // -- animation -------------------------------------------------------------------
    // Pauses or resumes the camera's own animation (layer 0).
    void SetAnimPaused(bool paused);
    // Slots 49-51: plays a clip on the camera rig (directly, by name hash or
    // by name). A started clip releases the anim hold; the hash form also
    // remembers the clip as lastAnim. NOTE: the entity base (0x000DBE78) is
    // reached through a hook; `blend`, `speed` and `fade` follow its use.
    virtual bool PlayClip(float blend, float speed, float fade, void* clip, uint32_t layer, bool loop);
    virtual bool PlayClipByHash(float blend, float speed, float fade, uint32_t clip, uint32_t layer, bool loop);
    virtual bool PlayClipByName(float blend, float speed, float fade, const char* clip, uint32_t layer, bool loop);
    // Starts an additive camera animation by name hash (0x000FCE18).
    bool PlayAdditiveAnim(float blend, uint32_t clip, bool loop, bool additive);

    // -- data (console offsets) -------------------------------------------------------
    bool renderEnabled = false;          // 0x158 bit 7
    uint8_t viewportLayout = 0;          // 0x158 bits 3-6 (from the placement)
    uint8_t field_158_bits = 0;          // 0x158 bits 0-2 (from the placement)
    uint8_t modeCount = 0;               // 0x159
    int16_t viewport[4] = {};            // 0x15A x, y, width, height
    float fov = 55.0f;                   // 0x164
    float focusDistance = 7.5f;          // 0x168
    void* renderTarget = nullptr;        // 0x16C AglRenderTarget
    CameraMode* modes[kMaxModes] = {};   // 0x170 oldest first
    CameraMode* animMode = nullptr;      // 0x1B0 mode created by SetActiveVisual
    // 0x1B8..0x1D3: post effect settings last applied (copied from a view).
    bool dofEnabled = false;             // 0x1B8
    float dofRange = 0.0f;               // 0x1BC
    float dofBlendAlpha = 0.0f;          // 0x1C0
    uint32_t dofIterations = 0;          // 0x1C4
    bool filterEnabled = false;          // 0x1C8
    float filterBlend = 0.0f;            // 0x1CC
    uint8_t filterColor[4] = { 0, 0, 0, 0xFF };   // 0x1D0
    uint32_t cameraFlags = kCamFlag_PostFx | kCamFlag_PostFxDof;   // 0x1D4 GameCameraFlags
    CameraViewState view;                // 0x1E0 eased view
    float animBlendTime = -1.0f;         // 0x250 blend used for the "Anim" mode
    float resetTimer = -1.0f;            // 0x254 while positive the blend restarts every frame
    uint32_t lastAnim = 0;               // 0x258 last clip started by name
    bool additivePlaying = false;        // 0x25C
    bool additiveOnly = false;           // 0x25D the clip offsets the view instead of replacing it
    void* cameraAnim = nullptr;          // 0x260 animation controller of the additive rig
    ape::world::ApeObj* additiveObject = nullptr;   // 0x264 "<name>_additive" camera object
    ape::Matrix34 additiveTransform;     // 0x270
    camera::SpringVec3 positionSpring;   // 0x2B0
    camera::SpringQuat rotationSpring;   // 0x310
    camera::SpringFloat fovSpring;       // 0x370
};

} // namespace entity
} // namespace sly
