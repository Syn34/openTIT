// openTIT - Agl shadow maps: cascade settings and the shadow map object.
#pragma once

#include <atomic>
#include <cstdint>

namespace agl {

// Global shadow settings (static object, vtable 0x00B23D50).
class AglShadowSettings {
public:
    static constexpr int kMaxSplits = 4;

    AglShadowSettings();
    virtual ~AglShadowSettings() = default;

    // Cascade split distances (at most four).
    void SetSplits(const float* splits, uint8_t count);
    // Deferred shadow fragment program variant (index into the
    // ShadowMap_DeferredShadow* table) for a cascade.
    int GetDeferredVariant(uint32_t flags, int cascade) const;

    enum VariantFlags : uint32_t {
        kVariant_Soft     = 1,   // fewer taps, "_OFRF" programs
        kVariant_NoFilter = 2,   // "_ONF" program
    };

    float splits[kMaxSplits] = {};   // 0x04
    uint8_t splitCount = 0;          // 0x14
    float field_18[3] = {};
    float field_24 = -1.0f;
    float field_28 = 0.0f;
    float field_30 = 0.0f;
    float field_34 = 0.0f;
    float field_38 = 0.05f;
    float field_3C = 0.5f;
    float field_40 = 1.0f;
    float depthBias = 0.0015f;       // 0x44 (NOTE: guessed from its size)
    float field_48 = 2.0f;
    float field_4C = 0.0f;
    float field_50 = 0.0f;
    float field_54 = 50.0f;
    float field_58 = 5.0f;
    uint32_t field_5C = 1;
    uint8_t flags = 0x20;            // 0x60
    std::atomic<uint32_t> field_80{0};
};

AglShadowSettings& GetShadowSettings();
void InitShadowSettings();

// Base of the shadow map object (vtable 0x00B24130); the shadow map class
// (vtable 0x00B23D28) adds the render pass.
class AglShadowMapBase {
public:
    AglShadowMapBase();
    virtual ~AglShadowMapBase() = default;

    void ToggleDebugView();
    // Starts a new frame of shadow casters.
    void Reset();

    uint8_t field_4[0x1C - 4] = {};
    uint16_t field_1C[4] = {};
    uint8_t field_24[0x1A0 - 0x24] = {};
    uint32_t field_1A0 = 0;          // copied to field_200 by Reset
    uint8_t field_1A4[0x200 - 0x1A4] = {};
    uint32_t field_200 = 0;
    uint8_t field_204[0x284 - 0x204] = {};
    uint32_t field_284 = 0;
    uint32_t field_288 = 0;
    bool debugView = false;          // 0x28C
    uint32_t casterCounts[4] = {};   // 0x300, 0x380, 0x400, 0x480
};

class AglShadowMap : public AglShadowMapBase {
public:
    AglShadowMap();

    float field_600 = 1.0f;
};

} // namespace agl
