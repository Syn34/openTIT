// openTIT - The game camera entity (see GameCamera.h).
#include "sly/entity/GameCamera.h"

#include "agl/AglPostProc.h"
#include "agl/AglRenderTarget.h"
#include "ape/core/ApeString.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/sys/ApeGeometry.h"
#include "ape/world/ApeObj.h"
#include "sly/camera/CameraHooks.h"
#include "sly/camera/CameraModes.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHooks.h"

#include <algorithm>

namespace sly {
namespace entity {

using camera::CameraViewState;

namespace {

// Message ids handled by the camera.
constexpr uint32_t kMsg_ReactivateMode = 0xCBF1C4F3;   // the top mode starts over
constexpr uint32_t kMsg_ResetBlend = 0x5A5F3B9B;       // restart the blend for a moment

// Animation events that hold / release the animated view.
constexpr uint32_t kEvent_HoldA = 0x5564C0C9;
constexpr uint32_t kEvent_HoldB = 0x1B12498F;
constexpr uint32_t kEvent_ReleaseA = 0xE9B65CFB;
constexpr uint32_t kEvent_ReleaseB = 0xD49B5F41;

const ape::Matrix34 kIdentity = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 0 } };

agl::AglRenderTarget* RenderTarget(void* target)
{
    return static_cast<agl::AglRenderTarget*>(target);
}

// The entity transform (look along +z) in the convention of the camera's
// world object and the renderer (x and z mirrored).
ape::Matrix34 ToObjectConvention(const ape::Matrix34& m)
{
    ape::Matrix34 out;
    out.xAxis = { -m.xAxis.x, -m.xAxis.y, -m.xAxis.z, 0.0f };
    out.yAxis = { m.yAxis.x, m.yAxis.y, m.yAxis.z, 0.0f };
    out.zAxis = { -m.zAxis.x, -m.zAxis.y, -m.zAxis.z, 0.0f };
    out.translation = { m.translation.x, m.translation.y, m.translation.z, 0.0f };
    return out;
}

// v * R for the three axis rows of `m` (row vectors).
ape::Vec3 RotateRow(const ape::Vec3& v, const ape::Matrix34& m)
{
    return { v.x * m.xAxis.x + v.y * m.yAxis.x + v.z * m.zAxis.x,
             v.x * m.xAxis.y + v.y * m.yAxis.y + v.z * m.zAxis.y,
             v.x * m.xAxis.z + v.y * m.yAxis.z + v.z * m.zAxis.z, 0.0f };
}

// Inverse of a rotation + translation matrix.
ape::Matrix34 RigidInverse(const ape::Matrix34& m)
{
    ape::Matrix34 out;
    out.xAxis = { m.xAxis.x, m.yAxis.x, m.zAxis.x, 0.0f };
    out.yAxis = { m.xAxis.y, m.yAxis.y, m.zAxis.y, 0.0f };
    out.zAxis = { m.xAxis.z, m.yAxis.z, m.zAxis.z, 0.0f };
    const ape::Vec3 t = RotateRow(m.translation, out);
    out.translation = { -t.x, -t.y, -t.z, 0.0f };
    return out;
}

} // namespace

// @func 0x000FA214 GameCamera::~GameCamera
// @func 0x000FA2E4 GameCamera::~GameCamera (deleting)
GameCamera::~GameCamera()
{
    if (cameraAnim) {
        camera::DeleteAnimController(cameraAnim);
        cameraAnim = nullptr;
    }
    if (additiveObject) {
        additiveObject->Release();
        additiveObject = nullptr;
    }
}

// @func 0x000F94C0 GameCamera::SetViewportLayout
void GameCamera::SetViewportLayout(int layout)
{
    float width = camera::ScreenWidth();
    float height = camera::ScreenHeight();
    float x = 0.0f;
    float y = 0.0f;
    const float safeWidth = camera::SafeAreaWidth();
    const float safeHeight = camera::SafeAreaHeight();
    if (safeWidth >= 0.0f && safeHeight >= 0.0f) {
        x = (width - safeWidth) * 0.5f;
        y = (height - safeHeight) * 0.5f;
        width = safeWidth;
        height = safeHeight;
    }

    float left = x, top = y, w = width, h = height;
    switch (layout) {
    case 1:     // letterboxed
        top = y + height / 10.0f;
        h = height - height * 0.125f * 2.0f;
        break;
    case 2:     // two players, top / bottom
    case 3:
        if (layout == 3)
            top = height * 0.5f + y + 2.0f;
        h = height * 0.5f - 2.0f;
        break;
    case 4:     // two players, left / right
    case 5:
        if (layout == 5)
            left = width * 0.5f + x + 2.0f;
        w = width * 0.5f - 2.0f;
        break;
    case 6:     // four quarters
    case 7:
    case 8:
    case 9:
        if (((layout - 2) & 1) == 0)
            left = width * 0.5f + x + 2.0f;
        if (layout > 7)
            top = height * 0.5f + y + 2.0f;
        w = width * 0.5f - 2.0f;
        h = height * 0.5f - 2.0f;
        break;
    case 10:    // picture in picture
        left = width * 0.6041666f + x;
        top = height * 0.0625f + y;
        w = width / 3.0f;
        h = height / 3.0f;
        break;
    default:
        break;
    }
    viewport[0] = static_cast<int16_t>(left);
    viewport[1] = static_cast<int16_t>(top);
    viewport[2] = static_cast<int16_t>(w);
    viewport[3] = static_cast<int16_t>(h);
}

// @func 0x000FA3CC GameCamera::SetRenderEnabled
void GameCamera::SetRenderEnabled(bool enabled)
{
    if (enabled && !renderEnabled) {
        if (!renderTarget)
            renderTarget = agl::AglRenderTarget::CreatePrimary(0, false);
        else
            RenderTarget(renderTarget)->AddRef();
        RenderTarget(renderTarget)->SetViewport(reinterpret_cast<const uint16_t*>(viewport));
    } else if (!enabled && renderEnabled) {
        Entity::SetAnimPaused(0, false);
        // NOTE: the console only forgets the pointer while the target still has
        // references; it is cleared either way here.
        RenderTarget(renderTarget)->Release();
        renderTarget = nullptr;
    }
    renderEnabled = enabled;
}

// @func 0x000FA4F4 GameCamera::Release
bool GameCamera::Release(bool kill)
{
    if (kill) {
        if (renderEnabled) {
            Entity::SetAnimPaused(0, false);
            RenderTarget(renderTarget)->Release();
            renderTarget = nullptr;
        }
        renderEnabled = false;
        modeCount = 0;
    }
    return Entity::Release(kill);
}

// @func 0x000FA660 GameCamera::RestartBlend
void GameCamera::RestartBlend(float time)
{
    CameraMode* top = TopMode();
    if (!top)
        return;
    const float current = std::max({ positionSpring.timeLeft, rotationSpring.timeLeft, fovSpring.timeLeft });
    if (current <= 0.0f || time <= 0.0f) {
        const ape::Vec3 from = transform.translation;
        const float fromFov = fov;
        CameraViewState modeView;
        top->Tick(0.0f, modeView);
        const float t = std::max(time, 0.0f);

        positionSpring.timeLeft = t;
        positionSpring.target = from;
        positionSpring.targetVelocity = {};
        if (t <= 0.0f) {
            positionSpring.value = from;
            positionSpring.velocity = {};
        }
        ape::Quat rotation;
        ape::QuatFromMatrix34(rotation, modeView.transform);
        rotationSpring.timeLeft = t;
        rotationSpring.target = rotation;
        rotationSpring.targetAngularVelocity = {};
        if (t <= 0.0f) {
            rotationSpring.value = rotation;
            rotationSpring.angularVelocity = {};
        }
        fovSpring.timeLeft = t;
        fovSpring.target = fromFov;
        fovSpring.targetVelocity = 0.0f;
        if (t <= 0.0f) {
            fovSpring.value = fromFov;
            fovSpring.velocity = 0.0f;
        }
    } else if (current < time) {
        const float t = std::max(time, 0.0f);
        positionSpring.timeLeft = t;
        rotationSpring.timeLeft = t;
        fovSpring.timeLeft = t;
    }
}

// Starts a blend from (position, the orientation of `modeView`, fov) over `time`.
static void BeginBlend(GameCamera& camera, float time, const ape::Vec3& from,
                       const CameraViewState& modeView, float fromFov)
{
    const float t = std::max(time, 0.0f);
    camera.positionSpring.timeLeft = t;
    camera.positionSpring.target = from;
    camera.positionSpring.targetVelocity = {};
    if (t <= 0.0f) {
        camera.positionSpring.value = from;
        camera.positionSpring.velocity = {};
    }
    ape::Quat rotation;
    ape::QuatFromMatrix34(rotation, modeView.transform);
    camera.rotationSpring.timeLeft = t;
    camera.rotationSpring.target = rotation;
    camera.rotationSpring.targetAngularVelocity = {};
    if (t <= 0.0f) {
        camera.rotationSpring.value = rotation;
        camera.rotationSpring.angularVelocity = {};
    }
    camera.fovSpring.timeLeft = t;
    camera.fovSpring.target = fromFov;
    camera.fovSpring.targetVelocity = 0.0f;
    if (t <= 0.0f) {
        camera.fovSpring.value = fromFov;
        camera.fovSpring.velocity = 0.0f;
    }
}

// @func 0x000FA9F0 GameCamera::HandleMessage
void GameCamera::HandleMessage(const EntityMessage& msg)
{
    if (msg.id == kMsg_ReactivateMode) {
        if (CameraMode* top = TopMode())
            top->Activate();
    } else if (msg.id == kMsg_ResetBlend) {
        RestartBlend(-1.0f);
        resetTimer = 0.1f;
    }
}

// @func 0x000FAABC GameCamera::PostUpdate
bool GameCamera::PostUpdate(float dt)
{
    view = CameraViewState{};
    view.transform = transform;
    view.fov = fov;
    view.focusDistance = focusDistance;
    if (!attachParent) {
        if (CameraMode* top = TopMode())
            top->Tick(dt, view);
    }
    if (object)
        SetLocalTransform(object->GetLocal());
    camera::QueueEntityTransformCallback(this);

    if (additivePlaying) {
        camera::AnimControllerUpdate(cameraAnim, dt * cullDistance);   // NOTE: +0xD4 acts as a time scale here
        additiveTransform = additiveObject->GetWorld();
        ape::Matrix34NormalizeAxes(additiveTransform);
        void* playing = AnimFindPlaying(cameraAnim, 0);
        if (playing && !AnimIsFinished(playing) && camera::AnimReachedEnd(playing))
            additivePlaying = false;
    }
    return true;
}

// @func 0x000FAE00 GameCamera::ApplyView
void GameCamera::ApplyView(bool force)
{
    ape::Matrix34 cam = ToObjectConvention(transform);
    if (force || (!(flags & 0x40u) && !(flags & 0x20u)))
        SetLocalTransform(cam);

    if (additivePlaying) {
        const ape::Matrix34& add = additiveTransform;
        if (!additiveOnly) {
            // Replace: the animation is expressed in the camera's frame.
            ape::Matrix34 combined;
            combined.xAxis = RotateRow(cam.xAxis, add);
            combined.yAxis = RotateRow(cam.yAxis, add);
            combined.zAxis = RotateRow(cam.zAxis, add);
            const ape::Vec3 t = RotateRow(cam.translation, add);
            combined.translation = { t.x + add.translation.x, t.y + add.translation.y,
                                     t.z + add.translation.z, 0.0f };
            cam = combined;
        } else {
            // Offset: rotate the axes, move by the animation's translation.
            const ape::Vec3 x = RotateRow(cam.xAxis, add);
            const ape::Vec3 y = RotateRow(cam.yAxis, add);
            const ape::Vec3 z = RotateRow(cam.zAxis, add);
            cam.xAxis = x;
            cam.yAxis = y;
            cam.zAxis = z;
            cam.translation.x += add.translation.x;
            cam.translation.y += add.translation.y;
            cam.translation.z += add.translation.z;
        }
    }

    agl::AglRenderTarget* target = RenderTarget(renderTarget);
    target->SetViewMatrix(RigidInverse(cam));
    target->SetFov(fov);

    if (!(cameraFlags & kCamFlag_PostFx) || !target->postProc)
        return;
    agl::AglPostProcEffect* dof = target->postProc->effects[agl::AglPostProcChain::kDoF];
    if ((cameraFlags & kCamFlag_PostFxDof) && dof) {
        if (dofEnabled) {
            const uint8_t passes = static_cast<uint8_t>(dofIterations);
            camera::SetDofEffectParams(dof, focusDistance, ape::Clamp(dofRange, 0.01f, 999.0f), dofBlendAlpha,
                                       passes == 0 ? 1 : std::min<uint8_t>(passes, 4));
        }
        dof->SetEnabled(dofEnabled || camera::BinocucomForcesDof() || camera::WorldForcesDof());
    }
    agl::AglPostProcEffect* filter = target->postProc->effects[agl::AglPostProcChain::kDesaturation];
    if (filter) {
        if (filterEnabled) {
            filter->SetEnabled(true);
            camera::SetFilterEffectParams(filter, ape::Clamp(filterBlend, 0.0f, 1.0f), filterColor);
        }
        filter->SetEnabled(filterEnabled);
    }
}

// Copies the view's field of view, focus and post effect settings.
static void TakeViewSettings(GameCamera& camera, const CameraViewState& view)
{
    camera.fov = view.fov;
    camera.focusDistance = view.focusDistance;
    camera.dofEnabled = view.dofEnabled;
    camera.dofRange = view.dofRange;
    camera.dofBlendAlpha = view.dofBlendAlpha;
    camera.dofIterations = view.dofIterations;
    camera.filterEnabled = view.filterEnabled;
    camera.filterBlend = view.filterBlend;
    for (int i = 0; i < 4; ++i)
        camera.filterColor[i] = view.filterColor[i];
}

// @func 0x000FB31C GameCamera::SetView
void GameCamera::SetView(const CameraViewState& newView, bool force)
{
    if (!renderEnabled)
        return;
    // Entity slot 21 (not overridden): take the view's transform.
    transform = newView.transform;
    TakeViewSettings(*this, newView);
    ApplyView(force);
}

// @func 0x000FB450 GameCamera::UpdateBlend
void GameCamera::UpdateBlend(float dt)
{
    if (resetTimer > 0.0f) {
        resetTimer = std::max(0.0f, resetTimer - dt);
        RestartBlend(-1.0f);
    }

    const ape::Vec3 pos = view.transform.translation;
    if (positionSpring.timeLeft > 0.0f || rotationSpring.timeLeft > 0.0f || fovSpring.timeLeft > 0.0f) {
        // Blending: the springs chase the mode's view.
        if (dt > 0.0f) {
            const float inv = 1.0f / dt;
            positionSpring.targetVelocity = { (pos.x - positionSpring.target.x) * inv,
                                              (pos.y - positionSpring.target.y) * inv,
                                              (pos.z - positionSpring.target.z) * inv, 0.0f };
            positionSpring.target = pos;
            positionSpring.Step(1.0f / std::max(camera::kMinEaseTime, positionSpring.timeLeft),
                                positionSpring.damping, dt);
            positionSpring.timeLeft = std::max(0.0f, positionSpring.timeLeft - dt);
        }
        ape::Quat rotation;
        ape::QuatFromMatrix34(rotation, view.transform);
        if (dt > 0.0f) {
            rotationSpring.targetAngularVelocity = camera::AngularVelocityBetween(dt, rotationSpring.target, rotation);
            rotationSpring.target = rotation;
            rotationSpring.Step(1.0f / std::max(camera::kMinEaseTime, rotationSpring.timeLeft),
                                rotationSpring.damping, dt);
            rotationSpring.timeLeft = std::max(0.0f, rotationSpring.timeLeft - dt);
        }
        const float easedFov = ape::Clamp(fovSpring.Update(view.fov, dt), 1.0f, 179.0f);
        ape::Matrix34FromQuat(view.transform, rotationSpring.value, positionSpring.value);
        view.fov = easedFov;
        return;
    }

    // Settled: keep the springs on the view so a later blend starts smoothly.
    if (dt > 0.0f) {
        const float inv = 1.0f / dt;
        positionSpring.velocity = { (pos.x - positionSpring.value.x) * inv, (pos.y - positionSpring.value.y) * inv,
                                    (pos.z - positionSpring.value.z) * inv, 0.0f };
        positionSpring.value = pos;
        positionSpring.targetVelocity = { (pos.x - positionSpring.target.x) * inv,
                                          (pos.y - positionSpring.target.y) * inv,
                                          (pos.z - positionSpring.target.z) * inv, 0.0f };
        positionSpring.target = pos;
        positionSpring.timeLeft = std::max(0.0f, positionSpring.timeLeft - dt);
    }
    ape::Quat rotation;
    ape::QuatFromMatrix34(rotation, view.transform);
    if (dt > 0.0f) {
        rotationSpring.angularVelocity = camera::AngularVelocityBetween(dt, rotationSpring.value, rotation);
        rotationSpring.value = rotation;
        rotationSpring.targetAngularVelocity = camera::AngularVelocityBetween(dt, rotationSpring.target, rotation);
        rotationSpring.target = rotation;
        rotationSpring.timeLeft = std::max(0.0f, rotationSpring.timeLeft - dt);
    }
    if (dt > 0.0f) {
        const float current = view.fov;
        fovSpring.velocity = (current - fovSpring.value) / dt;
        fovSpring.value = current;
        fovSpring.targetVelocity = (current - fovSpring.target) / dt;
        fovSpring.target = current;
        fovSpring.timeLeft = std::max(0.0f, fovSpring.timeLeft - dt);
    }
}

// @func 0x000FBA18 GameCamera::UpdateTransform
void GameCamera::UpdateTransform(float dt)
{
    Entity::UpdateTransform(dt);
    if (CameraMode* top = TopMode())
        top->PostUpdate(dt, view);
    UpdateBlend(dt * cullDistance);   // NOTE: +0xD4 acts as a time scale here
    RefreshView();
}

// @func 0x000FBB88 GameCamera::RemoveMode
GameCamera::CameraMode* GameCamera::RemoveMode(float blendTime, CameraMode* mode)
{
    for (int i = modeCount; i > 0; --i) {
        if (modes[i - 1] != mode)
            continue;
        if (i != modeCount) {
            // Not on top: just take it out.
            for (int j = i; j < modeCount; ++j)
                modes[j - 1] = modes[j];
            --modeCount;
            modes[modeCount] = nullptr;
            return mode;
        }

        mode->Deactivate();
        --modeCount;
        if (modeCount) {
            const ape::Vec3 from = transform.translation;
            const float fromFov = fov;
            CameraMode* top = modes[modeCount - 1];
            if (blendTime > 0.0f)
                top->ActivateFrom(view);   // NOTE: the console passes an uninitialised local view
            else
                top->Activate();
            CameraViewState modeView;
            TopMode()->Tick(0.0f, modeView);
            BeginBlend(*this, blendTime, from, modeView, fromFov);
        } else {
            const float t = std::max(blendTime, 0.0f);
            positionSpring.timeLeft = t;
            rotationSpring.timeLeft = t;
            fovSpring.timeLeft = t;
        }
        return mode;
    }
    return nullptr;
}

// @func 0x000FC040 GameCamera::PushMode
void GameCamera::PushMode(float blendTime, CameraMode* mode, bool keepOnTop)
{
    mode->keepOnTop = keepOnTop;
    if (modeCount) {
        if (!keepOnTop && modes[modeCount - 1]->keepOnTop) {
            // Slide in below the modes that stay on top; nothing is activated.
            int i = modeCount;
            for (; i > 0 && modes[i - 1]->keepOnTop; --i)
                modes[i] = modes[i - 1];
            // NOTE: when every mode stays on top the console drops the new mode;
            // here it goes to the bottom.
            modes[i] = mode;
            ++modeCount;
            return;
        }
        modes[modeCount - 1]->Deactivate();
    }

    modes[modeCount++] = mode;
    if (blendTime <= 0.0f || modeCount < 2)
        mode->Activate();
    else
        mode->ActivateFrom(view);   // NOTE: the console passes an uninitialised local view

    const ape::Vec3 from = transform.translation;
    const float fromFov = fov;
    CameraViewState modeView;
    mode->Tick(0.0f, modeView);
    BeginBlend(*this, blendTime, from, modeView, fromFov);
}

// @func 0x000FC5DC GameCamera::HasMode
bool GameCamera::HasMode(const CameraMode* mode) const
{
    for (int i = modeCount - 1; i >= 0; --i)
        if (modes[i] == mode)
            return true;
    return false;
}

// @func 0x000FCBD0 GameCamera::ModeBelow
GameCamera::CameraMode* GameCamera::ModeBelow(const CameraMode* mode) const
{
    for (int i = modeCount - 1; i >= 0; --i)
        if (modes[i] == mode)
            return i > 0 ? modes[i - 1] : nullptr;
    return nullptr;
}

// @func 0x000FC628 GameCamera::SetActiveVisual
void GameCamera::SetActiveVisual(bool on)
{
    if (!on) {
        if (animMode) {
            RemoveMode(animBlendTime, animMode);
            delete animMode;
            animMode = nullptr;
            Entity::SetAnimPaused(0, true);
            if (object)
                camera::ResetObjectPivot(object);
        }
        if (CameraMode* top = TopMode())
            top->Activate();
    } else {
        if (object && !attachParent)
            SetLocalTransform(kIdentity);
        if (!animMode) {
            animMode = new camera::AnimCameraMode(this);
            PushMode(animBlendTime, animMode, false);
        } else if (!HasMode(animMode)) {
            PushMode(animBlendTime, animMode, false);
        }
    }
    flags = on ? (flags | 0x40u) : (flags & ~0x40u);
}

// @func 0x000FC88C GameCamera::OnAnimEvent
void GameCamera::OnAnimEvent(const char* name, uint32_t argument, Entity* source)
{
    if (!source)
        return;
    const uint32_t hash = ape::HashNameNoCase(name);
    if (hash == kEvent_HoldA || hash == kEvent_HoldB)
        cameraFlags |= kCamFlag_AnimHold;
    else if (hash == kEvent_ReleaseA || hash == kEvent_ReleaseB)
        cameraFlags &= ~kCamFlag_AnimHold;
    else
        Entity::OnAnimEvent(name, argument, source);
}

// @func 0x000FCBC8 GameCamera::SetAttachParent
void GameCamera::SetAttachParent(Entity* parent)
{
    attachParent = parent;
}

// @func 0x000FCC3C GameCamera::RefreshView
void GameCamera::RefreshView()
{
    if (!renderEnabled)
        return;
    transform = view.transform;
    TakeViewSettings(*this, view);
    ApplyView(false);
}

// @func 0x000FCD1C GameCamera::ResetObjectTransform
// @func 0x000FCE08 GameCamera::ResetObjectTransform (static-init fragment)
void GameCamera::ResetObjectTransform()
{
    if (animController)
        camera::AnimControllerStopAll(animController);
    if (object)
        camera::ResetObjectPivot(object);
}

// @func 0x000FC968 GameCamera::PlayClip
bool GameCamera::PlayClip(float blend, float speed, float fade, void* clip, uint32_t layer, bool loop)
{
    const bool started = EntityPlayClip(this, blend, speed, fade, clip, layer, loop);
    if (started)
        cameraFlags &= ~kCamFlag_AnimHold;
    return started;
}

// @func 0x000FC9B8 GameCamera::PlayClipByHash
bool GameCamera::PlayClipByHash(float blend, float speed, float fade, uint32_t clip, uint32_t layer, bool loop)
{
    if (animController)
        camera::AnimPrepareClip(animController, clip);
    const bool started = PlayClip(blend, speed, fade, FindAnimClip(clip), layer, loop);
    if (started)
        lastAnim = clip;
    return started;
}

// @func 0x000FCAE4 GameCamera::PlayClipByName
bool GameCamera::PlayClipByName(float blend, float speed, float fade, const char* clip, uint32_t layer, bool loop)
{
    return PlayClip(blend, speed, fade, FindAnimClipByName(clip), layer, loop);
}

// @func 0x000FCE18 GameCamera::PlayAdditiveAnim
bool GameCamera::PlayAdditiveAnim(float blend, uint32_t clip, bool loop, bool additive)
{
    if (!cameraAnim)
        return false;
    camera::AnimPrepareClip(cameraAnim, clip);
    void* clipData = FindAnimClip(clip);
    if (!clipData || !cameraAnim)
        return false;
    if (void* playing = AnimFindPlaying(cameraAnim, 0))
        camera::AnimCrossfadeClip(blend, playing, clipData, loop);
    else
        camera::AnimStartClip(blend, cameraAnim, clipData, loop);
    additiveTransform = kIdentity;
    additivePlaying = true;
    additiveOnly = additive;
    return true;
}

// @func 0x000FA5D0 GameCamera::GetView
void GameCamera::GetView(CameraView& out) const
{
    out.transform.xAxis = { transform.xAxis.x, transform.xAxis.y, transform.xAxis.z, out.transform.xAxis.pad };
    out.transform.yAxis = { transform.yAxis.x, transform.yAxis.y, transform.yAxis.z, out.transform.yAxis.pad };
    out.transform.zAxis = { transform.zAxis.x, transform.zAxis.y, transform.zAxis.z, out.transform.zAxis.pad };
    out.transform.translation = { transform.translation.x, transform.translation.y, transform.translation.z,
                                  out.transform.translation.pad };
    out.field_40 = fov;
    out.field_44 = focusDistance;
}

// @func 0x000FCFDC GameCamera::SetAnimPaused
void GameCamera::SetAnimPaused(bool paused)
{
    if (!cameraAnim)
        return;
    if (void* playing = AnimFindPlaying(cameraAnim, 0))
        AnimSetPaused(playing, paused);
}

} // namespace entity
} // namespace sly
