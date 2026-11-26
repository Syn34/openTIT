// openTIT - Agl post-process chain: the screen effects applied after a render target's scene.
//
// Every render target owns an AglPostProcChain with ten effects (thermal
// vision, bloom, depth of field, blur, radial blur, desaturation, screen
// warp, god rays, motion trail and bloom noise). Game code enables effects
// and sets their parameters; AglRenderTarget::RunPostProcess applies the
// active ones in chain order. The passes themselves (shader sets in
// AglPostProcShaders.h) draw full-screen quads with the scene as input.
#pragma once

#include "agl/AglTypes.h"

#include <cstdint>

namespace agl {

// Base of all effects (console vtable slots: 2 SetEnabled, 3 IsActive,
// 4 Apply, 5 CopySettings).
class AglPostProcEffect {
public:
    explicit AglPostProcEffect(const char* effectName);
    virtual ~AglPostProcEffect();

    AglPostProcEffect(const AglPostProcEffect&) = delete;
    AglPostProcEffect& operator=(const AglPostProcEffect&) = delete;

    // @func 0x007A16BC AglPostProcEffect::SetEnabled
    virtual void SetEnabled(bool on) { enabled = on; }
    // @func 0x007A16C4 AglPostProcEffect::IsActive
    virtual bool IsActive() const { return enabled; }
    // Takes over the user settings of the same effect of another chain
    // (the base part only copies the enable flag).
    // @func 0x00788258 AglPostProcEffect::CopySettings
    virtual void CopySettings(const AglPostProcEffect& other) { enabled = other.enabled; }
    // Draws the effect (render thread).
    virtual void Apply() {}
    // One-time set-up on first use (console vtable slot 6).
    virtual void Init() {}
    // Reads the effect's tunable defaults (slot 7).
    virtual void LoadTuning() {}
    const char* GetName() const { return name; }

    bool enabled = false;          // 0x04
    uint8_t field_5 = 0;
    const char* name = nullptr;    // 0x08
    uint8_t field_C = 0;
    uint32_t nameHash = 0;         // 0x10
};

// An effect that fades out over 'fadeTime' seconds when disabled.
// (vtable 0x00B23630; the motion trail derives from it)
class AglFadingEffect : public AglPostProcEffect {
public:
    using AglPostProcEffect::AglPostProcEffect;
    ~AglFadingEffect() override;
    void Apply() override;

    void SetEnabled(bool on) override;
    // Advances the fade-out; the effect switches off at the end.
    void UpdateFade(float dt);

    float fadeTime = 0.0f;         // 0x50
    float fadeTimer = 0.0f;        // 0x54
    bool fadingOut = false;        // 0x58 bit 7
    bool fadingIn = false;         // 0x58 bit 6
};

// An effect that owns a helper object released with it (screen warp).
class AglOwningEffect : public AglPostProcEffect {
public:
    using AglPostProcEffect::AglPostProcEffect;
    ~AglOwningEffect() override;

    class AglResource* resource = nullptr;   // 0x14
};

// Engine part of the thermal vision effect (vtable 0x00B23670); the game's
// effect (AglThermalVisionEffect, vtable 0x00B24090) draws it.
class AglThermalVisionBase : public AglPostProcEffect {
public:
    AglThermalVisionBase();
    ~AglThermalVisionBase() override;
    void Apply() override;

    void* field_14 = nullptr;
    void* field_18 = nullptr;
    void* field_1C = nullptr;
    uint8_t field_20 = 1;
    uint8_t color[4] = { 0x00, 0x80, 0xFF, 0xFF };   // 0x21
    float field_30 = 0.005f;
    float field_34[4] = {};
    float field_44 = 0.0f;
    float gridSize[2] = { 8.0f, 8.0f };   // 0x48
    float field_50[2] = { 0.75f, 1.0f };
    float field_58[2] = {};
    float field_60[2] = { 0.75f, 1.0f };
    float field_68 = 840.0f;
    float field_6C = 0.0f;
};

class AglThermalVisionEffect : public AglThermalVisionBase {
public:
    ~AglThermalVisionEffect() override;
    void Apply() override;
};

// Distorts the screen with a grid of displaced vertices.
class AglScreenWarpEffect : public AglOwningEffect {
public:
    static constexpr int kMaxGrid = 15;
    static constexpr int kMaxIndices = 0x11E;

    AglScreenWarpEffect();
    // Frees the three GPU blocks of the game's warp passes (vtable 0x00B24070).
    ~AglScreenWarpEffect() override;
    void Apply() override;
    // Rebuilds the triangle strip of a columns x rows grid (at most 15 x 15).
    void SetGridSize(uint8_t columns, uint8_t rows);

    uint8_t vertexData[0x1418 - 0x18] = {};  // 0x18: displaced grid vertices
    uint16_t indices[kMaxIndices] = {};      // 0x1418
    uint8_t columns = 8;                     // 0x1651
    uint8_t rows = 8;                        // 0x1652
    uint16_t indexCount = 0;                 // 0x1654
    uint8_t field_1650 = 0;
    uint32_t field_1658 = 0;
    uint32_t field_165C = 0;
    float strength[2] = { 1.0f, 1.0f };      // 0x1660
    uint8_t color[4] = {};                   // 0x1668
    uint8_t field_166C = 1;
    uint32_t field_1670 = 0;
    uint32_t field_1674 = 0;
    float rect[6] = {};                      // 0x1678: screen rectangle and size
    AglMemBlock blocks[3];                   // 0x16A4 NOTE: contents not decoded
};

// Smears the previous frames along the camera motion.
class AglMotionTrailEffect : public AglFadingEffect {
public:
    AglMotionTrailEffect();
    // @func 0x0079E6CC AglMotionTrailEffect::~AglMotionTrailEffect
    // @func 0x007A36BC AglMotionTrailEffect::~AglMotionTrailEffect (deleting)
    ~AglMotionTrailEffect() override = default;
    void Apply() override;

    float timeStep = 0.016f;       // 0x14
    float field_18 = 0.016f;
    float field_1C = 0.1f;
    uint32_t field_20 = 0;
    float velocity[4] = {};        // 0x30
    float field_40[4] = {};
    float tint[4] = { 1.0f, 1.0f, 1.0f, 0.75f };   // 0x60
    float field_70[4] = {};
    float field_80 = 0.55f;
    float field_84 = 1.0f;
    float field_88 = 0.000049f;
    float tuning[5] = {};          // 0x8C..0x9C

    void Init() override;
    void LoadTuning() override;
};

// The post-process work buffers (0x01164038..0x0116407C, 0x011640D0..
// 0x011640D8): a quarter resolution surface holding four stacked buffers,
// an eighth resolution one holding two, their texture images and views, a
// quarter resolution depth buffer and a half resolution colour/depth pair.
struct AglPostProcBuffers {
    AglSurface* quarterSurface = nullptr;      // 0x01164038
    AglSurface* eighthSurface = nullptr;       // 0x0116403C
    AglTextureImage quarterImages[4];          // 0x01164590, 0x01164550, 0x01164510, 0x011644D0
    AglSurface* quarterViews[4] = {};          // 0x01164044.. (pointers at 0x01164040 + 8 * i)
    AglTextureImage eighthImages[2];           // 0x01164490, 0x01164450
    AglSurface* eighthViews[2] = {};           // 0x01164064, 0x0116406C
    AglTextureImage eighthOfQuarterImage;      // 0x01164410
    AglSurface* eighthOfQuarterView = nullptr; // 0x01164074: first quarter buffer at eighth size
    AglSurface* quarterDepth = nullptr;        // 0x01164078
    AglSurface* eighthDepthView = nullptr;     // 0x0116407C
    AglTextureImage halfImage;                 // 0x011643D0 (pointer 0x011640D0)
    AglSurface* halfSurface = nullptr;         // 0x011640D4
    AglSurface* halfDepth = nullptr;           // 0x011640D8
};
AglPostProcBuffers& GetPostProcBuffers();
// Creates the buffers for the current display size.
void CreatePostProcBuffers();
// Start-up of the post-process buffers (once).
void InitPostProcBuffers();
void ShutdownPostProcBuffers();

// Game tunables read by the effects (0x01164040..0x0116407C).
// NOTE: 0x005467DC stores the post-process buffer pointers (AglPostProcBuffers)
// in this range, so the effects' LoadTuning functions most likely copy
// buffer pointers, not tunables.
struct AglPostProcTuning {
    float values[16] = {};
};
AglPostProcTuning& GetPostProcTuning();

// Engine-side bases of the effects (Apply is pure in several of them on
// the console; the game derives the final classes, whose destructors are in
// the 0x0079E4xx range).
class AglGodRaysBase : public AglPostProcEffect {        // vtable 0x00B23610
public:
    using AglPostProcEffect::AglPostProcEffect;
    ~AglGodRaysBase() override;
};
class AglBlurBase : public AglPostProcEffect {           // vtable 0x00B23514
public:
    using AglPostProcEffect::AglPostProcEffect;
    ~AglBlurBase() override;
};
class AglDofBase : public AglPostProcEffect {            // vtable 0x00B2356C
public:
    using AglPostProcEffect::AglPostProcEffect;
    ~AglDofBase() override;
};
class AglRadialBlurBase : public AglPostProcEffect {     // vtable 0x00B23590
public:
    using AglPostProcEffect::AglPostProcEffect;
    ~AglRadialBlurBase() override;
};
class AglDesaturationBase : public AglPostProcEffect {   // vtable 0x00B2353C
public:
    using AglPostProcEffect::AglPostProcEffect;
    ~AglDesaturationBase() override;
    bool IsActive() const override;
};
// Bloom (vtable 0x00B234E8): copies its settings from another chain's bloom.
class AglBloomBase : public AglPostProcEffect {
public:
    using AglPostProcEffect::AglPostProcEffect;
    ~AglBloomBase() override;
    void Apply() override;
    // Takes over the enable flag and the bloom parameters of `other`.
    virtual void CopyBloomSettings(const AglBloomBase& other);

    float field_20[3] = {};        // 0x20 NOTE: bloom parameters, meanings unknown
    float field_30 = 0.0f;         // 0x30
    uint8_t field_34 = 0;          // 0x34
};
// Bloom noise (vtable 0x00B235DC): copies its settings from another chain's.
class AglBloomNoiseBase : public AglPostProcEffect {
public:
    using AglPostProcEffect::AglPostProcEffect;
    ~AglBloomNoiseBase() override;
    void Apply() override;
    // Takes over the enable flag and the noise parameters of `other`.
    virtual void CopyNoiseSettings(const AglBloomNoiseBase& other);

    // NOTE: parameter blocks, meanings unknown.
    struct Layer {
        float values[6] = {};
    };
    Layer layers[3];               // 0x14
    uint8_t field_5C[4] = {};      // 0x5C
    float field_60[8] = {};        // 0x60..0x7C
    float field_80 = 0.0f;         // 0x80
    float field_84 = 0.0f;         // 0x84
    float field_90 = 0.0f;         // 0x90
    float field_94 = 0.0f;         // 0x94
    float field_98 = 0.0f;         // 0x98
    float field_A0 = 0.0f;         // 0xA0
};

// The game's bloom, desaturation and radial blur effects.
class AglBloomEffect : public AglBloomBase {             // vtable 0x00B23F38
public:
    AglBloomEffect() : AglBloomBase("Bloom") {}
    ~AglBloomEffect() override;
};
class AglDesaturationEffect : public AglDesaturationBase {   // vtable 0x00B240B0
public:
    AglDesaturationEffect() : AglDesaturationBase("Desaturation") {}
    ~AglDesaturationEffect() override;
};
class AglRadialBlurEffect : public AglRadialBlurBase {   // vtable 0x00B235BC
public:
    AglRadialBlurEffect() : AglRadialBlurBase("RadialBlur") {}
    ~AglRadialBlurEffect() override;
    void Apply() override;
};

class AglGodRaysEffect : public AglGodRaysBase {
public:
    AglGodRaysEffect() : AglGodRaysBase("GodRays") {}
    // @func 0x0079E6BC AglGodRaysEffect::~AglGodRaysEffect
    // @func 0x007A3678 AglGodRaysEffect::~AglGodRaysEffect (deleting)
    ~AglGodRaysEffect() override = default;
    void Init() override;
    void LoadTuning() override;

    float tuning[7] = {};          // 0x84..0x9C
};

// Effects whose set-up only marks them initialised.
class AglBlurEffect : public AglBlurBase {
public:
    AglBlurEffect() : AglBlurBase("Blur") {}
    // @func 0x0079E448 AglBlurEffect::~AglBlurEffect
    // @func 0x007A3568 AglBlurEffect::~AglBlurEffect (deleting)
    ~AglBlurEffect() override = default;
    void Init() override;
};
class AglDofEffect : public AglDofBase {
public:
    AglDofEffect() : AglDofBase("DoF") {}
    // @func 0x0079E488 AglDofEffect::~AglDofEffect
    // @func 0x007A35F0 AglDofEffect::~AglDofEffect (deleting)
    ~AglDofEffect() override = default;
    void Init() override;
};

class AglBloomNoiseEffect : public AglBloomNoiseBase {
public:
    using AglBloomNoiseBase::AglBloomNoiseBase;
    // @func 0x0079E4B8 AglBloomNoiseEffect::~AglBloomNoiseEffect
    // @func 0x007A3634 AglBloomNoiseEffect::~AglBloomNoiseEffect (deleting)
    ~AglBloomNoiseEffect() override = default;
    void LoadTuning() override;
    // Picks a new random offset into the noise texture.
    void RandomizeOffset();

    float noiseOffset[2] = {};     // 0x40
};

class AglPostProcChain {
public:
    static constexpr int kEffectCount = 10;

    AglPostProcChain();
    // @func 0x007A1B48 AglPostProcChain::~AglPostProcChain
    // @func 0x007A1CBC AglPostProcChain::~AglPostProcChain (deleting)
    virtual ~AglPostProcChain();

    AglPostProcChain(const AglPostProcChain&) = delete;
    AglPostProcChain& operator=(const AglPostProcChain&) = delete;

    // Brackets the effects of one frame (no state of their own on the console).
    // @func 0x007A1B40 AglPostProcChain::Begin
    virtual void Begin() {}
    // @func 0x007A1B44 AglPostProcChain::End
    virtual void End() {}

    enum EffectIndex {
        kThermalVision = 0, kBloom, kDoF, kBlur, kRadialBlur, kDesaturation,
        kScreenWarp, kGodRays, kMotionTrail, kBloomNoise,
    };

    AglPostProcEffect* effects[kEffectCount] = {};

private:
    AglBloomEffect m_bloom;
    AglDofEffect m_dof;
    AglDesaturationEffect m_desaturation;
    AglBlurEffect m_blur;
    AglRadialBlurEffect m_radialBlur;
    AglScreenWarpEffect m_screenWarp;
    AglGodRaysEffect m_godRays;
    AglMotionTrailEffect m_motionTrail;
    AglBloomNoiseEffect m_bloomNoise{"BloomNoise"};
    AglThermalVisionEffect m_thermalVision;
};

// The chain a render target creates for its view (vtable 0x00B236A8, built
// by InitView): adds nothing to the base chain but its own destructor.
class AglViewPostProcChain final : public AglPostProcChain {
public:
    ~AglViewPostProcChain() override;
};

// Loads the shaders of every post-process pass (start-up).
void InitPostProcShaders();

} // namespace agl
