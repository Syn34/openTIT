// openTIT - Agl render targets: a camera, its projection and viewport, and the surface it renders to.
#include "agl/AglRenderTarget.h"

#include "agl/AglDisplay.h"
#include "agl/AglPostProc.h"
#include "agl/AglState.h"
#include "agl/AglTexture.h"

#include "ape/core/ApeString.h"
#include "ape/math/ApeMath.h"
#include "ape/memory/ApeMem.h"
#include "ape/memory/ApeTFreeList.h"
#include "ape/sys/ApeGeometry.h"

#include <cmath>
#include <cstring>
#include <mutex>
#include <thread>

namespace agl {

namespace {

constexpr float kDefaultFovRadians = 0.47996554f * 2.0f;   // 55 degrees
constexpr float kMinNear = 0.1f;
constexpr float kMinFar = 10.0f;
constexpr float kMaxFar = 5000.0f;
constexpr float kMatrixTolerance = 1e-5f;

using RenderTargetPool = ape::ApeTListAllocator<AglRenderTarget>;

std::mutex g_renderTargetLock;                 // 0x010DDAF4 (shared resource lock)
uint32_t g_nextResourceId = 0;                 // 0x01164CD0 (shared with other resources)
AglRenderTarget* g_primary = nullptr;          // 0x00F66FC4
AglRenderSurface* g_backBuffer = nullptr;      // 0x00F66FBC
AglRenderTarget* g_rendering = nullptr;        // 0x010DDA74
uint32_t g_renderSerial = 0;                   // 0x010DDA78
AglViewConstants g_viewConstants;              // 0x010DD780
std::thread::id g_mainThread = std::this_thread::get_id();   // 0x011DE290

const ape::Vec3 kYAxis = { 0.0f, 1.0f, 0.0f, 0.0f };   // 0x01164B60

ape::Matrix34 Identity34()
{
    ape::Matrix34 m = {};
    m.xAxis.x = 1.0f;
    m.yAxis.y = 1.0f;
    m.zAxis.z = 1.0f;
    return m;
}

// Rigid inverse of a rotation + translation: the transposed rotation and
// the translation rotated back and negated.
ape::Matrix34 RigidInverse(const ape::Matrix34& m)
{
    ape::Matrix34 r = {};
    r.xAxis = { m.xAxis.x, m.yAxis.x, m.zAxis.x, 0.0f };
    r.yAxis = { m.xAxis.y, m.yAxis.y, m.zAxis.y, 0.0f };
    r.zAxis = { m.xAxis.z, m.yAxis.z, m.zAxis.z, 0.0f };
    const ape::Vec3& t = m.translation;
    r.translation = { -(t.x * r.xAxis.x + t.y * r.yAxis.x + t.z * r.zAxis.x),
                      -(t.x * r.xAxis.y + t.y * r.yAxis.y + t.z * r.zAxis.y),
                      -(t.x * r.xAxis.z + t.y * r.yAxis.z + t.z * r.zAxis.z), 0.0f };
    return r;
}

// An affine transform as a 4x4 row-vector matrix.
ape::Matrix44 To44(const ape::Matrix34& m)
{
    ape::Matrix44 r;
    r.rows[0] = { m.xAxis.x, m.xAxis.y, m.xAxis.z, 0.0f };
    r.rows[1] = { m.yAxis.x, m.yAxis.y, m.yAxis.z, 0.0f };
    r.rows[2] = { m.zAxis.x, m.zAxis.y, m.zAxis.z, 0.0f };
    r.rows[3] = { m.translation.x, m.translation.y, m.translation.z, 1.0f };
    return r;
}

ape::Matrix44 Transpose(const ape::Matrix44& m)
{
    const float* a = &m.rows[0].x;
    ape::Matrix44 r;
    float* b = &r.rows[0].x;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            b[i * 4 + j] = a[j * 4 + i];
    return r;
}

ape::Vec4 Column(const ape::Matrix44& m, int c)
{
    const float* a = &m.rows[0].x;
    return { a[c], a[4 + c], a[8 + c], a[12 + c] };
}

ape::Vec4 Add(const ape::Vec4& a, const ape::Vec4& b) { return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
ape::Vec4 Sub(const ape::Vec4& a, const ape::Vec4& b) { return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }

uint16_t ClampU16(int value, int limit)
{
    return static_cast<uint16_t>(value < limit ? value : limit);
}

// Shared base initialisation of both console constructors: reference count,
// resource id, empty viewports, default fog layers and empty light slots.
void InitResourceHeader(AglRenderTarget* rt)
{
    rt->refCount = 1;
    rt->resourceFlags = 0x10;
    rt->id = ++g_nextResourceId;
    rt->field_C = 0;
    std::memset(rt->viewport, 0, sizeof(rt->viewport));
    std::memset(rt->scissor, 0, sizeof(rt->scissor));
    const AglDisplaySettings& display = GetDisplaySettings();
    for (AglFog& fog : rt->fog) {
        fog.enable = false;
        fog.field_1 = 1;
        std::memcpy(fog.color, display.defaultFogColor, sizeof(fog.color));
        fog.end = 50.0f;
    }
    for (AglRenderCallbackList& list : rt->callbacks)
        list = AglRenderCallbackList();
}

} // namespace

// --- frustum -------------------------------------------------------------

// @func 0x004FDEA0 AglFrustum::FromMatrix
void FrustumFromMatrix(AglFrustum* frustum, const ape::Matrix44& m)
{
    const ape::Vec4 x = Column(m, 0);
    const ape::Vec4 y = Column(m, 1);
    const ape::Vec4 z = Column(m, 2);
    const ape::Vec4 w = Column(m, 3);
    frustum->planes[0] = Add(w, x);   // left
    frustum->planes[1] = Sub(w, x);   // right
    frustum->planes[2] = Sub(w, y);   // top
    frustum->planes[3] = Add(w, y);   // bottom
    frustum->planes[4] = z;           // near (z >= 0 clip space)
    frustum->planes[5] = Sub(w, z);   // far
    for (ape::Vec4& p : frustum->planes) {
        const float inv = 1.0f / std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        p = { p.x * inv, p.y * inv, p.z * inv, p.w * inv };
    }
    // Transposed copies: planes 0..3, then 4, 5 and two planes that
    // contain everything.
    const ape::Vec4 always = { 0.0f, 0.0f, 0.0f, 1.0f };
    const ape::Vec4 group1[4] = { frustum->planes[4], frustum->planes[5], always, always };
    for (int i = 0; i < 4; ++i) {
        const ape::Vec4& a = frustum->planes[0];
        const ape::Vec4& b = frustum->planes[1];
        const ape::Vec4& c = frustum->planes[2];
        const ape::Vec4& d = frustum->planes[3];
        const float* comp[4] = { &a.x, &b.x, &c.x, &d.x };
        frustum->soa[i] = { comp[0][i], comp[1][i], comp[2][i], comp[3][i] };
        const float* comp1[4] = { &group1[0].x, &group1[1].x, &group1[2].x, &group1[3].x };
        frustum->soa[4 + i] = { comp1[0][i], comp1[1][i], comp1[2][i], comp1[3][i] };
    }
}

// @func 0x0052B210 FrustumSliceSphere
void FrustumSliceSphere(float zNear, float zFar, ape::Vec4* sphere, const ape::Vec4 corners[4])
{
    auto dist2 = [](const ape::Vec4& a, const ape::Vec4& b) {
        const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
        return dx * dx + dy * dy + dz * dz;
    };
    const float depth = zFar - zNear;
    const float nearDiag2 = dist2(corners[0], corners[1]);
    const float farDiag2 = dist2(corners[2], corners[3]);
    // Distance from the near plane of the point equidistant to both
    // diagonals' ends.
    const float t = (depth + (farDiag2 - nearDiag2) / (depth * 4.0f)) * 0.5f;
    float centerZ = zFar;
    float radius;
    if (depth < t) {
        // The far rectangle alone decides: center on the far plane.
        radius = std::sqrt(farDiag2) * 0.5f;
    } else {
        centerZ = t + zNear;
        radius = std::sqrt(nearDiag2 * 0.25f + t * t);
    }
    *sphere = { 0.0f, 0.0f, -centerZ, radius };
}

// --- globals -------------------------------------------------------------

const AglViewConstants& GetViewConstants()
{
    return g_viewConstants;
}

AglRenderTarget* GetRenderingTarget()
{
    return g_rendering;
}

// @func 0x004FDBA8 GetRenderingView
AglRenderTarget* GetRenderingView()
{
    return g_rendering ? g_rendering->renderCopy : nullptr;
}

// @func 0x00527108 ApplyRenderingViewport
void ApplyRenderingViewport()
{
    if (AglRenderTarget* view = GetRenderingView())
        view->ApplyViewport();
}

AglRenderTarget* GetPrimaryRenderTarget()
{
    return g_primary;
}

// @func 0x00522C98 GetCurrentView
AglRenderTarget* GetCurrentView()
{
    if (!g_primary)
        return nullptr;
    return std::this_thread::get_id() == g_mainThread ? g_primary : g_primary->renderCopy;
}

AglRenderSurface* GetBackBuffer()
{
    return g_backBuffer;
}

void SetBackBuffer(AglRenderSurface* surface)
{
    g_backBuffer = surface;
}

// @func 0x0053C9F8 SetStereoLayout
void SetStereoLayout(AglStereoLayout& layout, int32_t a, int32_t b, float x, float y, int32_t c)
{
    layout.field_0 = a;
    layout.field_4 = b;
    layout.field_8 = x;
    layout.field_C = y;
    layout.field_10 = c;
}

// --- construction --------------------------------------------------------

AglRenderTarget::AglRenderTarget()
{
    InitResourceHeader(this);
}

// @func 0x005214A4 AglRenderTarget::~AglRenderTarget
// @func 0x0052163C AglRenderTarget::~AglRenderTarget (copy 2)
// @func 0x00521564 AglRenderTarget::~AglRenderTarget (deleting)
AglRenderTarget::~AglRenderTarget()
{
    if (renderCopy) {
        // The render copy only mirrors this object: it owns nothing.
        renderCopy->texture = nullptr;
        renderCopy->postProc = nullptr;
        renderCopy->renderCopy = nullptr;
        renderCopy->~AglRenderTarget();
        ape::MemFree(renderCopy);
        renderCopy = nullptr;
    }
    if (postProc) {
        delete postProc;
        postProc = nullptr;
    }
    if (texture) {
        texture->Release();
        texture = nullptr;
    }
}

namespace {

// @func 0x005203A8 AglRenderTarget::InitView
// Defaults shared by every render target that is not a render copy.
void InitView(AglRenderTarget* rt)
{
    rt->frameCount = 0;
    rt->projectionType = kAglProjection_Perspective;
    rt->lodGroup = 0;
    rt->screenRotation = 0;
    rt->flipRotated = false;
    rt->resourceFlags = static_cast<uint16_t>((rt->resourceFlags & ~0xFC00u) | 0x1400u);
    rt->fov = kDefaultFovRadians;
    rt->zFar = 100.0f;
    rt->flags = static_cast<uint8_t>((rt->flags & ~0x10u) | AglRenderTarget::kFlag_Dirty);
    rt->field_44 = 1.0f;
    rt->field_48 = 1.0f;
    rt->zNear = 1.0f;
    for (AglFog& fog : rt->fog) {
        fog.enable = false;
        fog.color[0] = 0xFF;
        fog.color[1] = 0;
        fog.color[2] = 0xFF;
        fog.color[3] = 0;
        fog.field_10[0] = fog.field_10[2] = 1.0f;     // 255 / 255
        fog.field_10[1] = fog.field_10[3] = 0.0f;
        fog.start = 999.0f;
        fog.end = 1000.0f;
    }
    rt->SetViewMatrix(Identity34());
    rt->previousView = Identity34();
    if (ape::Matrix34Equal(kMatrixTolerance, rt->previousView, Identity34()))
        rt->flags = static_cast<uint8_t>(rt->flags & ~AglRenderTarget::kFlag_DefaultCamera);
    else
        rt->flags |= AglRenderTarget::kFlag_DefaultCamera;

    const uint16_t fullScreen[4] = { 0, 0, 0xFFFF, 0xFFFF };
    rt->SetViewport(fullScreen);
    rt->orthoRect[0] = 0.0f;
    rt->orthoRect[1] = 0.0f;
    rt->orthoRect[2] = static_cast<float>(rt->surface ? rt->surface->width : 0);
    rt->orthoRect[3] = static_cast<float>(rt->surface ? rt->surface->height : 0);
    rt->SetAspect(GetDisplaySettings().aspect);
    ape::StrCopyBounded(rt->name, "RenderTarget", sizeof(rt->name));
    rt->callbacks[0].count = 0;
    rt->callbacks[1].count = 0;
    rt->techniqueOverride = nullptr;
    rt->postProc = new AglPostProcChain();
    rt->drawScene = nullptr;
    rt->field_100 = -1;
}

// The render copy: same base initialisation, no defaults of its own (it
// is overwritten from its owner every frame).
AglRenderTarget* CreateRenderCopy()
{
    void* storage = ape::MemAlloc(sizeof(AglRenderTarget));
    std::memset(storage, 0, sizeof(AglRenderTarget));
    AglRenderTarget* copy = new (storage) AglRenderTarget();
    copy->stereoEnabled = false;
    copy->customStereo = false;
    return copy;
}

} // namespace

// @func 0x00520D88 AglRenderTarget::AglRenderTarget (primary)
// @func 0x00522FB8 AglRenderTarget::CreatePrimary
AglRenderTarget* AglRenderTarget::CreatePrimary(uint8_t pass, bool mainView)
{
    AglRenderTarget* rt = Create();
    if (!rt)
        return nullptr;
    rt->pass = pass;
    rt->surface = g_backBuffer;
    rt->texture = nullptr;
    InitView(rt);
    ape::StrCopyBounded(rt->name, "RenderTarget_Primary", sizeof(rt->name));
    if (!g_primary && pass == 0)
        g_primary = rt;
    rt->mainView = g_primary == rt;
    rt->renderCopy = CreateRenderCopy();

    rt->stereoEnabled = mainView;
    rt->customStereo = false;
    SetStereoLayout(rt->stereoLayout, 0x7FC, 0x7FE, 16.0f, 10.0f, 0xF0);
    return rt;
}

// @func 0x00521110 AglRenderTarget::AglRenderTarget (texture)
// @func 0x0052317C AglRenderTarget::CreateForTexture
AglRenderTarget* AglRenderTarget::CreateForTexture(AglTexture* texture)
{
    AglRenderTarget* rt = Create();
    if (!rt)
        return nullptr;
    rt->pass = 1;
    rt->texture = texture;
    rt->surface = static_cast<AglRenderSurface*>(texture->attachment);
    texture->AddRef();
    InitView(rt);
    ape::StrCopyBounded(rt->name, "RenderTarget_Surface", sizeof(rt->name));
    if (g_primary == rt)
        g_primary = nullptr;
    rt->mainView = g_primary == rt;
    rt->renderCopy = CreateRenderCopy();
    rt->stereoEnabled = false;
    rt->customStereo = false;
    return rt;
}

AglRenderTarget* AglRenderTarget::Create()
{
    return RenderTargetPool::GetAllocator().New();
}

// @func 0x00522E70 AglRenderTarget::Destroy
void AglRenderTarget::Destroy(void* object)
{
    std::lock_guard<std::mutex> guard(g_renderTargetLock);
    AglRenderTarget* rt = static_cast<AglRenderTarget*>(object);
    if (g_primary == rt)
        g_primary = nullptr;
    RenderTargetPool::GetAllocator().Delete(rt);
}

// @func 0x005216FC AglRenderTarget::Release
void AglRenderTarget::Release()
{
    if (DropRef())
        ScheduleDestroy(this, &AglRenderTarget::Destroy);
}

// --- camera and projection ---------------------------------------------

// @func 0x005201B0 AglRenderTarget::SetViewMatrix
void AglRenderTarget::SetViewMatrix(const ape::Matrix34& m)
{
    view = m;
    camera = RigidInverse(m);
    flags |= kFlag_Dirty;
}

// @func 0x00522630 AglRenderTarget::SetCameraMatrix
void AglRenderTarget::SetCameraMatrix(const ape::Matrix34& m)
{
    camera = m;
    view = RigidInverse(m);
    flags |= kFlag_Dirty;
}

// @func 0x00520368 AglRenderTarget::SetFov
void AglRenderTarget::SetFov(float degrees)
{
    flags |= kFlag_Dirty;
    fov = degrees * 0.017453292f;
}

// @func 0x00520338 AglRenderTarget::SetNear
void AglRenderTarget::SetNear(float value)
{
    flags |= kFlag_Dirty;
    zNear = value - kMinNear < 0.0f ? kMinNear : value;
}

// @func 0x005202C8 AglRenderTarget::SetFar
void AglRenderTarget::SetFar(float value)
{
    const float limited = value < kMaxFar ? value : kMaxFar;
    zFar = limited > kMinFar ? limited : kMinFar;
    flags |= kFlag_Dirty;
}

// @func 0x00520094 AglRenderTarget::SetAspect
void AglRenderTarget::SetAspect(float value)
{
    aspect = value;
    flags |= kFlag_Dirty;
}

// @func 0x0052038C AglRenderTarget::SetProjectionType
void AglRenderTarget::SetProjectionType(AglProjectionType type)
{
    projectionType = type;
}

// @func 0x005206F8 AglRenderTarget::GetZoom
float AglRenderTarget::GetZoom() const
{
    static const float defaultTan = std::tan(kDefaultFovRadians * 0.5f);
    const float ratio = std::tan(fov * 0.5f) / defaultTan;
    return ratio - 1.0f < 0.0f ? ratio : 1.0f;
}

// @func 0x00522A64 AglRenderTarget::SetOrthoProjection
void AglRenderTarget::SetOrthoProjection()
{
    ape::MatrixOrtho(orthoRect[0], orthoRect[2], orthoRect[3], orthoRect[1], zNear, zFar, projection);
    ape::Matrix44Mul(viewProj, To44(view), projection);
    viewProjT = Transpose(viewProj);
    FrustumFromMatrix(&frustum, viewProj);
    PublishViewConstants();
}

// @func 0x005115F0 AglRenderTarget::UpdateMatrices
void AglRenderTarget::UpdateMatrices(bool force)
{
    if (!force && !(flags & kFlag_Dirty))
        return;
    flags = static_cast<uint8_t>(flags & ~kFlag_Dirty);

    switch (projectionType) {
    case kAglProjection_Ortho:
        ape::MatrixOrtho(orthoRect[0], orthoRect[2], orthoRect[3], orthoRect[1], zNear, zFar, projection);
        break;
    case kAglProjection_Oblique: {
        ape::MatrixPerspectiveFov(fov, aspect, zNear, zFar, projection);
        // NOTE: the clip plane comes from an engine helper outside Agl
        // (0x0055E940) applied to the view matrix; the plane stored in the
        // view's translation row is what reflections pass in.
        const ape::Vec4 plane = { view.translation.x, view.translation.y, view.translation.z, 0.0f };
        ape::MatrixSetObliqueNearPlane(projection, plane);
        break;
    }
    default:
        ape::MatrixPerspectiveFov(fov, aspect, zNear, zFar, projection);
        break;
    }
    baseProjection = projection;
    ape::Matrix44Mul(screenMatrix, To44(view), baseProjection);
    OnProjectionUpdated();

    if (screenRotation != 0) {
        // Quarter turns of the output (portrait displays).
        ape::Matrix34 rotation;
        ape::Matrix34RotationZ(static_cast<float>(screenRotation) * 1.5707964f, rotation);
        ape::Matrix44 rotated;
        ape::Matrix44Mul(rotated, projection, To44(rotation));
        projection = rotated;
        if ((screenRotation == 1 || screenRotation == 3) && flipRotated) {
            ape::Matrix34 mirror;
            const ape::Vec3 scale = { -1.0f, 1.0f, 1.0f, 0.0f };
            ape::Matrix34FromScale(mirror, scale);
            ape::Matrix44Mul(rotated, projection, To44(mirror));
            projection = rotated;
        }
    }

    ape::Matrix44Mul(viewProj, To44(view), projection);
    viewProjT = Transpose(viewProj);
    FrustumFromMatrix(&frustum, viewProj);

    // View rotation without translation, and a yaw-only rotation facing
    // the camera.
    const ape::Matrix34 inv = RigidInverse(view);
    cameraRotation = To44(inv);
    cameraRotation.rows[3] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const float yaw = std::atan2(view.zAxis.x, view.zAxis.z);
    ape::MatrixAxisAngle(-yaw, yawRotation, kYAxis);
}

// @func 0x005483EC AglRenderTarget::OnProjectionUpdated
void AglRenderTarget::OnProjectionUpdated()
{
    if (projectionType != kAglProjection_Perspective || GetStereoMode() == kAglStereo_Off)
        return;
    const float separation = customStereo ? stereoParams[0] : GetStereoSeparation();
    const float convergence = customStereo ? stereoParams[1] : GetStereoConvergence();
    ape::Matrix44 adjusted;
    StereoAdjustProjection(adjusted, projection, separation, convergence, GetStereoMode());
    projection = adjusted;
}

// --- surface and viewport -------------------------------------------------

// @func 0x005229B0 AglRenderTarget::SetTexture
void AglRenderTarget::SetTexture(AglTexture* newTexture)
{
    if (texture) {
        texture->Release();
        texture = nullptr;
    }
    surface = static_cast<AglRenderSurface*>(newTexture->attachment);
    texture = newTexture;
    newTexture->AddRef();
    const uint16_t fullScreen[4] = { 0, 0, 0xFFFF, 0xFFFF };
    SetViewport(fullScreen);
}

// @func 0x005200AC AglRenderTarget::SetViewport
void AglRenderTarget::SetViewport(const uint16_t rect[4])
{
    const int surfaceWidth = surface ? static_cast<int>(surface->width) : 0;
    const int surfaceHeight = surface ? static_cast<int>(surface->height) : 0;
    viewport[0] = rect[0];
    viewport[1] = rect[1];
    viewport[2] = ClampU16(rect[2], surfaceWidth);
    viewport[3] = ClampU16(rect[3], surfaceHeight);
    std::memcpy(scissor, viewport, sizeof(scissor));

    const AglDisplaySettings& display = GetDisplaySettings();
    if (viewport[0] == 0 && viewport[1] == 0 && viewport[2] == display.width && viewport[3] == display.height)
        flags |= kFlag_FullScreen;
    else
        flags = static_cast<uint8_t>(flags & ~kFlag_FullScreen);
}

// @func 0x00520030 AglRenderTarget::ApplyViewport
void AglRenderTarget::ApplyViewport()
{
    if (surface != g_backBuffer)
        BindRenderSurface(surface, true);
    SetViewportRect(viewport);
}

// --- projection of points ---------------------------------------------

// @func 0x00522750 AglRenderTarget::WorldToScreen
void AglRenderTarget::WorldToScreen(float out[4], const ape::Vec3& position)
{
    UpdateMatrices(false);
    ape::Vec4 clip;
    ape::Matrix44TransformPoint(clip, position, screenMatrix);
    out[0] = (clip.x / clip.w + 1.0f) * 0.5f;
    out[1] = (clip.y / clip.w - 1.0f) * -0.5f;
    out[2] = clip.z;
    out[3] = clip.w;
}

// @func 0x00522838 AglRenderTarget::ClipToCamera
// @func 0x0052296C AglRenderTarget::ClipToCamera (wrapper)
void AglRenderTarget::ClipToCamera(float out[3], const ape::Vec4& clip)
{
    UpdateMatrices(false);
    ape::Matrix44 inverse;
    ape::Matrix44SetInverse(inverse, screenMatrix);
    ape::Vec4 p;
    ape::Vec4MulMatrix44(p, clip, inverse);
    if (projectionType == kAglProjection_Ortho) {
        out[0] = p.x;
        out[1] = p.y;
        out[2] = p.z;
        return;
    }
    const float invW = 1.0f / p.w;
    out[0] = p.x * invW;
    out[1] = p.y * invW;
    out[2] = p.z * invW;
}

// --- render callbacks ---------------------------------------------------

// @func 0x00522500 AglRenderTarget::AddRenderCallback
void AglRenderTarget::AddRenderCallback(int listIndex, void (*fn)(void*), void* arg, int8_t priority)
{
    AglRenderCallbackList& list = callbacks[listIndex];
    // Keep the list sorted by priority: shift later entries up.
    int slot = list.count;
    while (slot > 0 && list.entries[slot - 1].priority > priority) {
        if (slot < AglRenderCallbackList::kMax)
            list.entries[slot] = list.entries[slot - 1];
        --slot;
    }
    if (slot >= AglRenderCallbackList::kMax)
        return;
    list.entries[slot].fn = fn;
    list.entries[slot].arg = arg;
    list.entries[slot].pass = pass;
    list.entries[slot].priority = priority;
    ++list.count;
}

// --- frame ----------------------------------------------------------------

// @func 0x005207D0 AglRenderTarget::PublishViewConstants
void AglRenderTarget::PublishViewConstants()
{
    AglViewConstants& c = g_viewConstants;
    c.projection = projection;
    c.viewProj = viewProj;
    c.viewProjT = viewProjT;
    c.view = view;
    // Transposed view rotation with the eye position (camera translation)
    // in the last row.
    c.viewRotationT = RigidInverse(view);
    c.previousCamera = previousCamera;
    c.cameraRotation = cameraRotation;
    for (int i = 0; i < 2; ++i) {
        const AglFog& f = fog[i];
        c.fogColor[i] = { f.field_10[0], f.field_10[1], f.field_10[2], f.field_10[3] };
        c.fogParams[i][0] = { f.params0[0], f.params0[1], f.params0[2], f.params0[3] };
        c.fogParams[i][1] = { f.params1[0], f.params1[1], f.params1[2], f.params1[3] };
    }
    c.zoom = GetZoom();
    const AglLightingSettings& lighting = GetLightingSettings();
    for (int i = 0; i < 6; ++i) {
        const float* color = lighting.colors[i];
        const float s = lighting.intensity;
        c.ambient[i] = { color[0] * s, color[1] * s, color[2] * s, color[3] * s };
    }
    c.field_A50 = { 0.0f, 1.0f, 0.0f, 0.0f };
    c.frame = g_renderSerial;
}

// @func 0x00522D18 AglRenderTarget::RunPostProcess
void AglRenderTarget::RunPostProcess(uint32_t effectMask)
{
    if (postProc && IsPostProcessEnabled()) {
        postProc->Begin();
        for (int i = 0; i < AglPostProcChain::kEffectCount; ++i) {
            if (!(effectMask & (1u << i)))
                continue;
            AglPostProcEffect* effect = postProc->effects[i];
            if (effect->IsActive())
                effect->Apply();
        }
        postProc->End();
    }
    InvalidateRenderState();
}

// @func 0x00511F8C AglRenderTarget::UpdateAll
void AglRenderTarget::UpdateAll()
{
    RenderTargetPool::GetAllocator().ForEach([](AglRenderTarget* rt) {
        if (rt->refCount < 1)
            return;
        rt->UpdateMatrices(false);
        rt->CopyToRenderCopy();
    });
}

// @func 0x005109B0 AglRenderTarget::CopyToRenderCopy
void AglRenderTarget::CopyToRenderCopy()
{
    AglRenderTarget* copy = renderCopy;
    if (!copy)
        return;
    copy->refCount = refCount;
    copy->resourceFlags = resourceFlags;
    copy->id = id;
    if (copy->field_C != field_C)
        copy->field_C = field_C;
    std::memcpy(copy->name, name, sizeof(name));
    copy->pass = pass;
    copy->projectionType = projectionType;
    copy->screenRotation = screenRotation;
    copy->flipRotated = flipRotated;
    copy->lodGroup = lodGroup;
    copy->flags = flags;
    copy->mainView = mainView;
    copy->fov = fov;
    copy->zNear = zNear;
    copy->zFar = zFar;
    copy->field_44 = field_44;
    copy->field_48 = field_48;
    std::memcpy(copy->orthoRect, orthoRect, sizeof(orthoRect));
    std::memcpy(copy->viewport, viewport, sizeof(viewport));
    std::memcpy(copy->scissor, scissor, sizeof(scissor));
    copy->aspect = aspect;
    copy->fog[0] = fog[0];
    copy->fog[1] = fog[1];
    copy->field_100 = field_100;
    copy->frustum = frustum;
    copy->surface = surface;
    copy->texture = texture;
    copy->callbacks[0] = callbacks[0];
    copy->callbacks[1] = callbacks[1];
    copy->postProc = postProc;
    copy->drawScene = drawScene;
    copy->techniqueOverride = techniqueOverride;
    copy->frameCount = frameCount;
    // The copy's own render copy pointer is left alone.
    copy->previousView = previousView;
    copy->view = view;
    copy->camera = camera;
    copy->viewProj = viewProj;
    copy->viewProjT = viewProjT;
    copy->projection = projection;
    copy->cameraRotation = cameraRotation;
    copy->yawRotation = yawRotation;
    copy->previousCamera = previousCamera;
    copy->baseProjection = baseProjection;
    copy->screenMatrix = screenMatrix;
    std::memcpy(copy->field_570, field_570, sizeof(field_570));
    copy->stereoEnabled = stereoEnabled;
    copy->customStereo = customStereo;
    copy->stereoLayout = stereoLayout;
    copy->field_710 = field_710;
    copy->field_718 = field_718;
    copy->stereoParams[0] = stereoParams[0];
    copy->stereoParams[1] = stereoParams[1];
}

} // namespace agl
