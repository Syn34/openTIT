// openTIT - Shader sets of the Agl post-processing and shadow passes.
//
// Each pass loads its vertex/fragment program pairs once, on first use.
// Some passes also raise the register count of their fragment programs: the
// hardware runs programs with fewer registers faster but these passes rely
// on a minimum.
#pragma once

#include "agl/AglShader.h"

namespace agl {

struct AglShaderPair {
    AglVertexProgram* vertex = nullptr;
    AglFragmentProgram* fragment = nullptr;
};

constexpr int kDeferredShadowVariants = 15;

struct AglPassShaders {
    // MSAA resolve (0x010F0FF4..0x010F1018).
    AglShaderPair resolve2x, resolve4x, resolveDepth2x, resolveDepth4x, restoreZCull;
    // God rays / motion trail (0x010F0B40..0x010F0B4C).
    AglShaderPair godRaysScatter, motionTrailSmear;
    // Shadow maps (0x010F0AD0..).
    AglVertexProgram* shadowDepthVP = nullptr;
    AglVertexProgram* shadowDepthSkinnedVP = nullptr;
    AglFragmentProgram* shadowDepthFP = nullptr;
    AglVertexProgram* deferredShadowVP = nullptr;
    AglFragmentProgram* deferredShadowFP[kDeferredShadowVariants] = {};
    // Bloom (0x01164118..).
    AglShaderPair bloomCopyThreshold, bloomApply, bloomDownSample4x, bloomUpSample4x;
    // Monochrome (0x01164138..).
    AglShaderPair monochrome, invMonochrome;
    // Screen warp (0x01151A04).
    AglShaderPair warp;
    // Stencil shadows (0x010DF950..).
    AglShaderPair stencilShadow, stencilShadowDrawQuad;
};

AglPassShaders& GetPassShaders();

// Raises a fragment program's register count to at least 'count'.
void EnsureFragmentRegisters(AglFragmentProgram* fp, uint8_t count);

void LoadResolveShaders();
void LoadGodRaysShaders();
void InitGodRaysShaders();
void LoadMotionTrailShaders();
void InitMotionTrailShaders();
void LoadShadowMapShaders();
void InitShadowMapShaders();
void LoadBloomShaders();
void LoadMonochromeShaders();
void LoadWarpShaders();
void LoadStencilShadowShaders();

} // namespace agl
