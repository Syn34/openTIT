// openTIT - Agl post-process chain: the screen effects applied after a render target's scene.
//
// Every render target owns an AglPostProcChain with ten effects (thermal
// vision, bloom, depth of field, blur, radial blur, desaturation, screen
// warp, god rays, motion trail and bloom noise). Game code enables effects
// and sets their parameters; AglRenderTarget::RunPostProcess applies the
// active ones in chain order. The passes themselves (shader sets in
// AglPostProcShaders.h) draw full-screen quads with the scene as input.
#pragma once

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
class AglFadingEffect : public AglPostProcEffect {
public:
    using AglPostProcEffect::AglPostProcEffect;

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

class AglThermalVisionEffect : public AglPostProcEffect {
public:
    AglThermalVisionEffect();
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

// Distorts the screen with a grid of displaced vertices.
class AglScreenWarpEffect : public AglOwningEffect {
public:
    static constexpr int kMaxGrid = 15;
    static constexpr int kMaxIndices = 0x11E;

    AglScreenWarpEffect();
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

// Game tunables read by the effects (0x01164040..0x0116407C).
struct AglPostProcTuning {
    float values[16] = {};
};
AglPostProcTuning& GetPostProcTuning();

class AglGodRaysEffect : public AglPostProcEffect {
public:
    AglGodRaysEffect() : AglPostProcEffect("GodRays") {}
    // @func 0x0079E6BC AglGodRaysEffect::~AglGodRaysEffect
    // @func 0x007A3678 AglGodRaysEffect::~AglGodRaysEffect (deleting)
    ~AglGodRaysEffect() override = default;
    void Init() override;
    void LoadTuning() override;

    float tuning[7] = {};          // 0x84..0x9C
};

// Effects whose set-up only marks them initialised.
class AglBlurEffect : public AglPostProcEffect {
public:
    AglBlurEffect() : AglPostProcEffect("Blur") {}
    // @func 0x0079E448 AglBlurEffect::~AglBlurEffect
    // @func 0x007A3568 AglBlurEffect::~AglBlurEffect (deleting)
    ~AglBlurEffect() override = default;
    void Init() override;
};
class AglDofEffect : public AglPostProcEffect {
public:
    AglDofEffect() : AglPostProcEffect("DoF") {}
    // @func 0x0079E488 AglDofEffect::~AglDofEffect
    // @func 0x007A35F0 AglDofEffect::~AglDofEffect (deleting)
    ~AglDofEffect() override = default;
    void Init() override;
};

class AglBloomNoiseEffect : public AglPostProcEffect {
public:
    using AglPostProcEffect::AglPostProcEffect;
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
    virtual ~AglPostProcChain();

    AglPostProcChain(const AglPostProcChain&) = delete;
    AglPostProcChain& operator=(const AglPostProcChain&) = delete;

    // Brackets the effects of one frame (no state of their own on the console).
    virtual void Begin() {}
    virtual void End() {}

    enum EffectIndex {
        kThermalVision = 0, kBloom, kDoF, kBlur, kRadialBlur, kDesaturation,
        kScreenWarp, kGodRays, kMotionTrail, kBloomNoise,
    };

    AglPostProcEffect* effects[kEffectCount] = {};

private:
    AglPostProcEffect m_bloom{"Bloom"};
    AglDofEffect m_dof;
    AglPostProcEffect m_desaturation{"Desaturation"};
    AglBlurEffect m_blur;
    AglPostProcEffect m_radialBlur{"RadialBlur"};
    AglScreenWarpEffect m_screenWarp;
    AglGodRaysEffect m_godRays;
    AglMotionTrailEffect m_motionTrail;
    AglBloomNoiseEffect m_bloomNoise{"BloomNoise"};
    AglThermalVisionEffect m_thermalVision;
};

// Loads the shaders of every post-process pass (start-up).
void InitPostProcShaders();

} // namespace agl
