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
    // Shared post-process passes (0x01164080..0x011640CC, 0x011640DC,
    // 0x01164110): copy, fill, textured quads, 4x down/up sampling, the two
    // separable Gaussian blurs (11 and 3 taps), depth down sampling and a
    // scaled copy.
    AglShaderPair copy, fill, renderTxr, renderAlphaTxr, downSample4x, upSample4x;
    AglShaderPair blur11H, blur11V, blur3H, blur3V, downSampleDepth, copyScale;
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
    // Thermal vision (0x01164178..0x01164184): one vertex program, normal
    // and "_MN" fragment programs.
    AglVertexProgram* thermalVisionVP = nullptr;
    AglFragmentProgram* thermalVisionFP[2] = {};
    // Depth of field (0x01164148..0x0116415C): plain, no-bloom, directional
    // and directional no-bloom variants.
    AglVertexProgram* dofVP = nullptr;
    AglFragmentProgram* dofFP[4] = {};
    // Bloom noise (0x01164160..0x01164174).
    AglShaderPair bloomNoiseDownSample;
    AglVertexProgram* bloomNoiseVP = nullptr;
    AglFragmentProgram* bloomNoiseNoBloomFP = nullptr;
    AglFragmentProgram* bloomNoiseFP = nullptr;
    // Sly dark screen fade (0x010F0B50).
    AglShaderPair darkScreen;
};

AglPassShaders& GetPassShaders();

// Raises a fragment program's register count to at least 'count'.
void EnsureFragmentRegisters(AglFragmentProgram* fp, uint8_t count);

// Loads the shared post-process shaders and sets up the blur kernels.
void LoadCommonPostProcShaders();
// Texel offsets of the 11-tap blurs for the quarter resolution buffers.
void UploadBlurKernelH();
void UploadBlurKernelV();
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
void LoadThermalVisionShaders();
void LoadDofShaders();
void LoadBloomNoiseShaders();
void LoadDarkScreenShaders();

} // namespace agl
