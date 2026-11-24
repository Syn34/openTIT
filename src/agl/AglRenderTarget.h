// openTIT - Agl render targets: a camera, its projection and viewport, and the surface it renders to.
//
// An AglRenderTarget is what the game calls a "view": a camera (view matrix),
// a projection (perspective, orthographic or perspective with an oblique near
// plane), a viewport into a render surface, two fog settings, render
// callbacks and the post-process chain applied after the scene.
//
// The main thread edits a render target while the render thread draws the
// previous frame, so every render target owns a second instance (its
// "render copy"). At the end of each frame the matrices are brought up to
// date and the whole state is copied over (AglRenderTarget::UpdateAll), and
// the render thread only ever reads the copy.
//
// The console object is 0x730 bytes (pool AglRenderTarget). The base class
// (vtable 0x00B236C0) has no stereo settings; the class actually allocated
// (vtable 0x00B240D0) adds them at 0x6F0. They are one class here.
#pragma once

#include "agl/AglResources.h"

#include "ape/math/ApeMatrixOps.h"

#include <cstdint>

namespace agl {

class AglPostProcChain;
class AglTexture;

// Six normalized clip planes (left, right, top, bottom, near, far) of a
// view-projection matrix, plus the same planes transposed in groups of four
// for SIMD box tests (the last group is padded with two dummy planes).
struct alignas(16) AglFrustum {
    ape::Vec4 planes[6];
    ape::Vec4 soa[8];
};

// Extracts the clip planes of a (row-vector) view-projection matrix.
void FrustumFromMatrix(AglFrustum* frustum, const ape::Matrix44& viewProj);

// Smallest sphere around a slice of a view frustum between zNear and
// zFar (camera looks down -z). 'corners' are opposite corners of the near
// (0, 1) and far (2, 3) rectangles; the result is (0, 0, -centerZ, radius).
// Used to fit shadow map cascades.
void FrustumSliceSphere(float zNear, float zFar, ape::Vec4* sphere, const ape::Vec4 corners[4]);

// Distance fog of one layer (the console keeps two per render target).
struct AglFog {
    bool enable = false;            // 0x00
    uint8_t field_1 = 0;
    uint8_t color[4] = {};          // 0x02: R, G, B and density (0..255)
    float start = 999.0f;           // 0x08
    float end = 1000.0f;            // 0x0C
    float field_10[4] = {};         // 0x10
    // Shader constants derived from the settings above at BeginRender.
    float params0[4] = {};          // 0x20: start, 1 / (end - start), 0, 0
    float params1[4] = {};          // 0x30: start, 1 / (end - start), 0, density
};

// A function called before or after the scene of a render target.
struct AglRenderCallback {
    void (*fn)(void* arg) = nullptr;
    void* arg = nullptr;
    uint32_t pass = 0;              // render pass (AglRenderTarget::pass) the callback was added for
    int8_t priority = 0;            // ascending order
};

struct AglRenderCallbackList {
    static constexpr int kMax = 4;
    AglRenderCallback entries[kMax];
    uint8_t count = 0;
};

enum AglProjectionType : uint8_t {
    kAglProjection_Ortho       = 0,
    kAglProjection_Perspective = 1,
    kAglProjection_Oblique     = 2,   // perspective with an oblique near plane (reflections)
};

// Five stereo layout values of the main view (0x6F8); NOTE: their meaning
// is not known (defaults 2044, 2046, 16.0, 10.0, 240).
struct AglStereoLayout {
    int32_t field_0 = 0;
    int32_t field_4 = 0;
    float field_8 = 0.0f;
    float field_C = 0.0f;
    int32_t field_10 = 0;
};
void SetStereoLayout(AglStereoLayout& layout, int32_t a, int32_t b, float x, float y, int32_t c);

class AglRenderTarget : public AglResource {
public:
    AglRenderTarget();
    ~AglRenderTarget() override;

    static AglRenderTarget* Create();
    static void Destroy(void* object);
    void Release() override;

    // A render target drawing into the back buffer. 'pass' selects the
    // render pass; pass 0 becomes the primary render target. 'mainView'
    // marks the main game view (stereo settings apply to it).
    static AglRenderTarget* CreatePrimary(uint8_t pass, bool mainView);
    // A render target drawing into a render texture.
    static AglRenderTarget* CreateForTexture(AglTexture* texture);

    // Brings every live render target's matrices up to date and copies it
    // to its render copy (end of frame, main thread part).
    static void UpdateAll();

    // --- camera and projection ---
    void SetViewMatrix(const ape::Matrix34& view);
    void SetCameraMatrix(const ape::Matrix34& camera);
    void SetFov(float degrees);
    void SetNear(float zNear);
    void SetFar(float zFar);
    void SetAspect(float aspect);
    void SetProjectionType(AglProjectionType type);
    // Orthographic projection with the near/far planes kept; switches the
    // render target to externally supplied matrices.
    void SetOrthoProjection();
    // Recomputes the projection and the derived matrices if they changed
    // (or always, with 'force').
    void UpdateMatrices(bool force);
    // Zoom factor relative to the default field of view (<= 1).
    float GetZoom() const;

    // --- surface and viewport ---
    void SetTexture(AglTexture* texture);
    // Viewport in pixels of the render surface (clipped to it).
    void SetViewport(const uint16_t rect[4]);
    // Binds the render surface and sets the viewport (render thread).
    virtual void ApplyViewport();

    // --- projection of points ---
    // Projects a world position to normalized screen coordinates
    // (0..1, y down) in out[0..1]; out[2], out[3] are clip z and w.
    void WorldToScreen(float out[4], const ape::Vec3& position);
    // Direction (camera space) through a point given in clip space.
    void ClipToCamera(float out[3], const ape::Vec4& clip);

    // --- render callbacks ---
    // Adds a callback before (list 0) or after (list 1) the scene.
    void AddRenderCallback(int list, void (*fn)(void*), void* arg, int8_t priority);

    // --- frame ---
    // TODO(0x00521D20 AglRenderTarget::BeginRender) and
    // TODO(0x00522140 AglRenderTarget::EndRender): they drive the
    // renderer's draw buckets and job lists, which are not reconstructed.
    // Applies the enabled post-process effects (mask of effect indices).
    void RunPostProcess(uint32_t effectMask);
    // Publishes the matrices and fog of this render target to the shader
    // constant block (AglViewConstants).
    void PublishViewConstants();

    // Copies the whole state to the render copy.
    void CopyToRenderCopy();

    // Hook called after the projection matrix was rebuilt (stereo offset).
    virtual void OnProjectionUpdated();

    enum Flags34 : uint8_t {
        kFlag_DefaultCamera = 0x20,   // view matrix is the identity
        kFlag_FullScreen    = 0x40,   // viewport covers the whole display
        kFlag_Dirty         = 0x80,   // projection must be rebuilt
    };

    uint16_t resourceFlags = 0;
    uint32_t id = 0;                         // 0x08
    uint32_t field_C = 0;
    char name[32] = {};                      // 0x10
    uint8_t pass = 0;                        // 0x30: render pass the target belongs to
    uint8_t projectionType = kAglProjection_Perspective;   // 0x31
    uint8_t screenRotation = 0;              // bits 13..15 of word 0x30: quarter turns of the output
    bool flipRotated = false;                // 0x32 bit 4: mirror the rotated projection
    uint8_t lodGroup = 0;                    // 0x33: index of the LOD scale table
    uint8_t flags = kFlag_Dirty;             // 0x34 (Flags34)
    bool mainView = false;                   // 0x34 bit 2: the primary view
    float fov = 0.96f;                       // 0x38, radians (55 degrees)
    float zNear = 1.0f;                      // 0x3C
    float zFar = 100.0f;                     // 0x40
    float field_44 = 1.0f;
    float field_48 = 1.0f;
    float orthoRect[4] = {};                 // 0x50: left, top, right, bottom
    uint16_t viewport[4] = {};               // 0x60: x, y, width, height
    uint16_t scissor[4] = {};                // 0x68
    float aspect = 1.0f;                     // 0x70
    AglFog fog[2];                           // 0x80
    int32_t field_100 = -1;
    AglFrustum frustum;                      // 0x110
    AglRenderSurface* surface = nullptr;     // 0x200: where the scene is drawn
    AglTexture* texture = nullptr;           // 0x204: render texture owning 'surface' (referenced)
    AglRenderCallbackList callbacks[2];      // 0x208: before / after the scene
    AglPostProcChain* postProc = nullptr;    // 0x290
    void (*drawScene)() = nullptr;           // 0x294
    uint32_t field_298 = 0;
    uint32_t frameCount = 0;                 // 0x29C: frames rendered (0: no motion history yet)
    AglRenderTarget* renderCopy = nullptr;   // 0x2A0
    ape::Matrix34 previousView;              // 0x2B0
    ape::Matrix34 view;                      // 0x2F0: world to camera
    ape::Matrix34 camera;                    // 0x330: camera to world
    ape::Matrix44 viewProj;                  // 0x370
    ape::Matrix44 viewProjT;                 // 0x3B0: transposed for vertex constants
    ape::Matrix44 projection;                // 0x3F0
    ape::Matrix44 cameraRotation;            // 0x430: view rotation, translation removed
    ape::Matrix34 yawRotation;               // 0x470: rotation about Y facing the camera (billboards)
    ape::Matrix34 previousCamera;            // 0x4B0
    ape::Matrix44 baseProjection;            // 0x4F0: projection before the screen rotation
    ape::Matrix44 screenMatrix;              // 0x530: view * baseProjection
    uint8_t field_570[0x6F0 - 0x570] = {};

    // Stereo settings of the main view (the derived console class).
    bool stereoEnabled = false;              // 0x6F0 bit 7
    bool customStereo = false;               // 0x6F0 bit 5: use stereoParams instead of the global ones
    AglStereoLayout stereoLayout;            // 0x6F8
    uint64_t field_710 = 0;
    uint8_t field_718 = 0;
    float stereoParams[2] = { 4.0f, 0.01f }; // 0x720: separation and convergence
};

// Shader constants of the render target being drawn (globals 0x010DD780..0x010DDA5C).
struct alignas(16) AglViewConstants {
    ape::Matrix44 projection;                // 0x010DD780
    ape::Matrix44 viewProj;                  // 0x010DD7C0
    ape::Matrix44 viewProjT;                 // 0x010DD800
    ape::Matrix34 view;                      // 0x010DD840
    ape::Matrix34 viewRotationT;             // 0x010DD880: transposed view rotation, row 3 = eye
    ape::Matrix34 previousCamera;            // 0x010DD8C0
    ape::Matrix44 cameraRotation;            // 0x010DD900
    ape::Vec4 fogColor[2];                   // 0x010DD980
    ape::Vec4 fogParams[2][2];               // 0x010DD9A0
    float zoom = 1.0f;                       // 0x010DD9E4
    uint32_t frame = 0;                      // 0x010DD9E8
    ape::Vec4 ambient[6];                    // 0x010DD9F0: light colors scaled by the global intensity
    ape::Vec4 field_A50 = { 0.0f, 1.0f, 0.0f, 0.0f };
};

const AglViewConstants& GetViewConstants();

// The render target the render thread is drawing (0x010DDA74), and its
// render copy, which holds the state being drawn.
AglRenderTarget* GetRenderingTarget();
AglRenderTarget* GetRenderingView();
// The primary render target (0x00F66FC4), or its render copy when called
// from the render thread.
AglRenderTarget* GetCurrentView();
AglRenderTarget* GetPrimaryRenderTarget();
// The back buffer render surface (0x00F66FBC).
AglRenderSurface* GetBackBuffer();
void SetBackBuffer(AglRenderSurface* surface);
// Binds the surface and viewport of the view being drawn.
void ApplyRenderingViewport();

} // namespace agl
