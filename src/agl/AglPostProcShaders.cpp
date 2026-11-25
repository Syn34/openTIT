// openTIT - Shader sets of the Agl post-processing and shadow passes.
#include "agl/AglPostProcShaders.h"

#include "agl/AglShaderClass.h"

namespace agl {

namespace {

AglPassShaders g_shaders;

// Pass specific constant tables (slot numbers are per pass).
const AglGlobalShaderParam kGodRaysParams[] = {          // 0x00F6708C
    { "gps_vScreenLightPos", 0 }, { "gps_vParams", 1 }, { "gps_vColorTint", 2 }, { nullptr, 0 },
};
const AglGlobalShaderParam kMotionTrailParams[] = {      // 0x00F670B0
    { "gps_vVelocity", 0 }, { nullptr, 0 },
};
const AglGlobalShaderParam kDeferredShadowParams[] = {   // 0x00F6700C
    { "gps_vShadowColor", 0 }, { "gps_vShadowParams", 1 }, { "gps_vShadowParams1", 7 },
    { "gps_vCamDepthParams", 2 }, { "gps_camToLightClipMtxZ", 3 }, { nullptr, 0 },
};
const AglGlobalShaderParam kThresholdParams[] = {        // 0x00F676E0 / 0x00F67700
    { "vThreshold", 0 }, { nullptr, 0 },
};
const AglGlobalShaderParam kAlphaParams[] = {            // 0x00F676F0 / 0x00F67710
    { "vAlpha", 0 }, { nullptr, 0 },
};
const AglGlobalShaderParam kThermalVisionParams[] = {  // 0x00F67808
    { "gps_vColorTint", 0 }, { "gps_vVisionParams", 1 }, { "gps_vNoiseParams", 2 },
    { "gps_vScanLineParams", 3 }, { nullptr, 0 },
};
const AglGlobalShaderParam kDofParams[] = {             // 0x00F67738
    { "gps_vColor", 0 }, { "gps_vDepthParams", 1 }, { "gps_mCamClipToViewMtx[0]", 2 },
    { "gps_mCamClipToViewMtx[1]", 3 }, { "gps_mCamClipToViewMtx[2]", 4 },
    { "gps_mCamClipToViewMtx[3]", 5 }, { "gps_vViewSpcDir", 6 }, { nullptr, 0 },
};
const AglGlobalShaderParam kBloomNoiseParams[] = {      // 0x00F67790
    { "gps_vIndirectMtx1[0]", 0 }, { "gps_vIndirectMtx1[1]", 1 }, { "gps_vIndirectMtx1[2]", 2 },
    { "gps_vIndirectMtx2[0]", 3 }, { "gps_vIndirectMtx2[1]", 4 }, { "gps_vIndirectMtx2[2]", 5 },
    { "gps_vIndirectMtx3[0]", 6 }, { "gps_vIndirectMtx3[1]", 7 }, { "gps_vIndirectMtx3[2]", 8 },
    { "gps_vWeights", 9 }, { "gps_vNoiseColorTint", 10 }, { "gps_vMaskParamsX", 11 },
    { "gps_vMaskParamsY", 12 }, { nullptr, 0 },
};
const AglGlobalShaderParam kDarkScreenParams[] = {      // 0x00F670C0
    { "gps_vDepthParams", 0 }, { "gps_vParams", 1 }, { nullptr, 0 },
};
const AglGlobalShaderParam kNoParams[] = {               // 0x00F67720, 0x00F67830, 0x00F67838
    { nullptr, 0 },
};

// 0x00F6703C: fragment programs of the deferred shadow pass by variant.
const char* const kDeferredShadowNames[kDeferredShadowVariants] = {
    "ShadowMap_DeferredShadow",
    "ShadowMap_DeferredShadow_2",
    "ShadowMap_DeferredShadow_3",
    "ShadowMap_DeferredShadow_4",
    "ShadowMap_DeferredShadow_5",
    "ShadowMap_DeferredShadow_OFRF",
    "ShadowMap_DeferredShadow_OFRF_2",
    "ShadowMap_DeferredShadow_OFRF_3",
    nullptr,
    nullptr,
    "ShadowMap_DeferredShadow_ONF",
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};

void LoadPair(AglShaderPair* pair, const char* name, const AglGlobalShaderParam* extra)
{
    LoadShaderPrograms(&pair->vertex, &pair->fragment, name, extra);
}

} // namespace

AglPassShaders& GetPassShaders()
{
    return g_shaders;
}

void EnsureFragmentRegisters(AglFragmentProgram* fp, uint8_t count)
{
    if (!fp)
        return;
    if (fp->desc.registerCount < count)
        fp->desc.registerCount = count;
    if (gpu::GetRegisterCount(fp->program) < count)
        gpu::SetFragmentRegisterCount(fp->program, count);
}

// @func 0x00534310 AglPostProc::LoadResolveShaders
void LoadResolveShaders()
{
    static bool loaded = false;
    if (loaded)
        return;
    loaded = true;
    LoadPair(&g_shaders.resolve2x, "PostProc_Resolve2xMSAA", nullptr);
    LoadPair(&g_shaders.resolve4x, "PostProc_Resolve4xMSAA", nullptr);
    LoadPair(&g_shaders.resolveDepth2x, "PostProc_ResolveDepth2xMSAA", nullptr);
    LoadPair(&g_shaders.resolveDepth4x, "PostProc_ResolveDepth4xMSAA", nullptr);
    LoadPair(&g_shaders.restoreZCull, "PostProc_RestoreZCull", nullptr);
    EnsureFragmentRegisters(g_shaders.resolve2x.fragment, 32);
    EnsureFragmentRegisters(g_shaders.resolve4x.fragment, 32);
    EnsureFragmentRegisters(g_shaders.resolveDepth2x.fragment, 32);
    EnsureFragmentRegisters(g_shaders.resolveDepth4x.fragment, 32);
}

// @func 0x0051EE94 AglPostProc::LoadGodRaysShaders
void LoadGodRaysShaders()
{
    g_shaders.godRaysScatter.vertex = LoadVertexProgram("PostProc_GodRaysScatter");
    g_shaders.godRaysScatter.fragment = LoadFragmentProgram("PostProc_GodRaysScatter", kGodRaysParams);
}

// @func 0x00543774 AglPostProc::InitGodRaysShaders
void InitGodRaysShaders()
{
    LoadGodRaysShaders();
    EnsureFragmentRegisters(g_shaders.godRaysScatter.fragment, 32);
}

// @func 0x0051F0FC AglPostProc::LoadMotionTrailShaders
void LoadMotionTrailShaders()
{
    g_shaders.motionTrailSmear.vertex = LoadVertexProgram("PostProc_MotionTrailSmear");
    g_shaders.motionTrailSmear.fragment = LoadFragmentProgram("PostProc_MotionTrailSmear", kMotionTrailParams);
}

// @func 0x005447F4 AglPostProc::InitMotionTrailShaders
void InitMotionTrailShaders()
{
    LoadMotionTrailShaders();
    EnsureFragmentRegisters(g_shaders.motionTrailSmear.fragment, 32);
}

// @func 0x0052DB58 AglShadowMap::LoadShaders
void LoadShadowMapShaders()
{
    static bool loaded = false;
    if (loaded)
        return;
    loaded = true;
    g_shaders.shadowDepthVP = LoadVertexProgram("ShadowMap_Depth");
    g_shaders.shadowDepthSkinnedVP = LoadVertexProgram("ShadowMap_Depth_SK");
    g_shaders.shadowDepthFP = LoadFragmentProgram("ShadowMap_Depth", nullptr);
    g_shaders.deferredShadowVP = LoadVertexProgram(kDeferredShadowNames[0]);
    for (int i = 0; i < kDeferredShadowVariants; ++i) {
        g_shaders.deferredShadowFP[i] = kDeferredShadowNames[i]
            ? LoadFragmentProgram(kDeferredShadowNames[i], kDeferredShadowParams)
            : nullptr;
    }
}

// @func 0x0054CDB0 AglShadowMap::InitShaders
void InitShadowMapShaders()
{
    static bool initialised = false;
    if (initialised)
        return;
    initialised = true;
    LoadShadowMapShaders();
    for (int i = 0; i < 5; ++i)
        EnsureFragmentRegisters(g_shaders.deferredShadowFP[i], 5);
}

// @func 0x0053D800 AglPostProc::LoadBloomShaders
void LoadBloomShaders()
{
    LoadPair(&g_shaders.bloomCopyThreshold, "PostProc_BloomCopyThreshold", kThresholdParams);
    LoadPair(&g_shaders.bloomApply, "PostProc_BloomApply", kAlphaParams);
    LoadPair(&g_shaders.bloomDownSample4x, "PostProc_BloomDownSample4x", kThresholdParams);
    LoadPair(&g_shaders.bloomUpSample4x, "PostProc_BloomUpSample4x", kAlphaParams);
    EnsureFragmentRegisters(g_shaders.bloomCopyThreshold.fragment, 32);
    EnsureFragmentRegisters(g_shaders.bloomApply.fragment, 32);
    EnsureFragmentRegisters(g_shaders.bloomDownSample4x.fragment, 32);
    EnsureFragmentRegisters(g_shaders.bloomUpSample4x.fragment, 8);
}

// @func 0x0053ECE0 AglPostProc::LoadMonochromeShaders
void LoadMonochromeShaders()
{
    LoadPair(&g_shaders.monochrome, "PostProc_Monochrome", kNoParams);
    LoadPair(&g_shaders.invMonochrome, "PostProc_InvMonochrome", kNoParams);
    EnsureFragmentRegisters(g_shaders.monochrome.fragment, 32);
}

// @func 0x0054588C AglPostProc::LoadWarpShaders
void LoadWarpShaders()
{
    LoadPair(&g_shaders.warp, "PostProc_Warp", nullptr);
}

// @func 0x0054BF48 AglStencilShadow::LoadShaders
void LoadStencilShadowShaders()
{
    LoadPair(&g_shaders.stencilShadow, "StencilShadow", kNoParams);
    LoadPair(&g_shaders.stencilShadowDrawQuad, "StencilShadow_DrawQuad", kNoParams);
}

// The material class objects of the thermal vision pass draw with
// ("PostProc_ThermalObject", static object 0x011640F0, vtable 0x00B241B0).
class ThermalObjectClass : public AglShaderClass {
public:
    ThermalObjectClass() : AglShaderClass("PostProc_ThermalObject") {}
    // @func 0x007A2BA4 ThermalObjectClass::~ThermalObjectClass
    // @func 0x007A3D44 ThermalObjectClass::~ThermalObjectClass (deleting)
    ~ThermalObjectClass() override = default;
    // TODO(0x0054D4C0 ThermalObjectClass::CreateTechniques)
    void CreateTechniques() override {}
};

ThermalObjectClass g_thermalObjectClass;

// @func 0x00546334 AglPostProc::LoadThermalVisionShaders
void LoadThermalVisionShaders()
{
    g_shaders.thermalVisionVP = LoadVertexProgram("PostProc_ThermalVision");
    g_shaders.thermalVisionFP[0] = LoadFragmentProgram("PostProc_ThermalVision", kThermalVisionParams);
    g_shaders.thermalVisionFP[1] = LoadFragmentProgram("PostProc_ThermalVision_MN", kThermalVisionParams);
    // NOTE: the console also registers the thermal vision shader class
    // (static object 0x011640F0) here; its class code is outside Agl.
    EnsureFragmentRegisters(g_shaders.thermalVisionFP[0], 32);
    EnsureFragmentRegisters(g_shaders.thermalVisionFP[1], 32);
}

// @func 0x00540824 AglPostProc::LoadDofShaders
void LoadDofShaders()
{
    static const char* const kNames[4] = {
        "PostProc_DofApply", "PostProc_DofApply_NB", "PostProc_DofApply_DIR", "PostProc_DofApply_DIR_NB",
    };
    g_shaders.dofVP = LoadVertexProgram(kNames[0]);
    for (int i = 0; i < 4; ++i)
        g_shaders.dofFP[i] = LoadFragmentProgram(kNames[i], kDofParams);
    for (AglFragmentProgram* fp : g_shaders.dofFP)
        EnsureFragmentRegisters(fp, 32);
}

// @func 0x005420CC AglPostProc::LoadBloomNoiseShaders
void LoadBloomNoiseShaders()
{
    LoadPair(&g_shaders.bloomNoiseDownSample, "PostProc_BloomDownSample4x", kThresholdParams);
    g_shaders.bloomNoiseVP = LoadVertexProgram("PostProc_BloomNoise");
    g_shaders.bloomNoiseNoBloomFP = LoadFragmentProgram("PostProc_BloomNoise_NoBloom", kBloomNoiseParams);
    g_shaders.bloomNoiseFP = LoadFragmentProgram("PostProc_BloomNoise", kBloomNoiseParams);
    EnsureFragmentRegisters(g_shaders.bloomNoiseDownSample.fragment, 32);
    EnsureFragmentRegisters(g_shaders.bloomNoiseNoBloomFP, 32);
    EnsureFragmentRegisters(g_shaders.bloomNoiseFP, 32);
}

void LoadDarkScreenShaders()
{
    LoadPair(&g_shaders.darkScreen, "Sly_PostProc_DarkScreen", kDarkScreenParams);
    EnsureFragmentRegisters(g_shaders.darkScreen.fragment, 32);
}

} // namespace agl
