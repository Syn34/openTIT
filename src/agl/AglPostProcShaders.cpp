// openTIT - Shader sets of the Agl post-processing and shadow passes.
#include "agl/AglPostProcShaders.h"

#include "agl/AglDisplay.h"
#include "agl/AglPostProc.h"
#include "agl/AglPrimitives.h"
#include "agl/AglResources.h"
#include "agl/AglShaderClass.h"
#include "agl/AglState.h"

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
const AglGlobalShaderParam kColorParams[] = {           // 0x00F6757C / 0x00F67590
    { "vColor", 0 }, { nullptr, 0 },
};
const AglGlobalShaderParam kBlur11Params[] = {          // 0x00F675C4 / 0x00F67628
    { "gvTexelKernel[0]", 0 }, { "gvTexelKernel[1]", 1 }, { "gvTexelKernel[2]", 2 },
    { "gvTexelKernel[3]", 3 }, { "gvTexelKernel[4]", 4 }, { "gvTexelKernel[5]", 5 },
    { "gvTexelKernel[6]", 6 }, { "gvTexelKernel[7]", 7 }, { "gvTexelKernel[8]", 8 },
    { "gvTexelKernel[9]", 9 }, { "gvTexelKernel[10]", 10 }, { nullptr, 0 },
};
const AglGlobalShaderParam kBlur3Params[] = {           // 0x00F6768C / 0x00F676B0
    { "gvTexelKernel[0]", 0 }, { "gvTexelKernel[1]", 1 }, { "gvTexelKernel[2]", 2 }, { nullptr, 0 },
};
const AglGlobalShaderParam kSampleModeParams[] = {      // 0x00F676D0
    { "gps_gfSampleMode", 0 }, { nullptr, 0 },
};
const AglGlobalShaderParam kColorScaleParams[] = {      // 0x00F67550
    { "gps_vColorScale", 0 }, { nullptr, 0 },
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

// Tap positions of the 11-tap blurs in quarter-resolution texels
// (0x00F679A4 horizontal, 0x00F6794C vertical).
constexpr int kBlur11Taps = 11;

// Makes 'fp' the current fragment program and sets its kernel constants to
// the tap offsets scaled to texture coordinates of a quarter size buffer.
void UploadBlurKernel(AglFragmentProgram* fp, float stepX, float stepY)
{
    AglRenderState& state = GetRenderState();
    if (state.fragmentProgram != fp) {
        state.fragmentProgramDirty = true;
        state.fragmentProgram = fp;
    }
    const AglDisplaySettings& display = GetDisplaySettings();
    const float texelU = 1.0f / float(display.width >> 2);
    const float texelV = 1.0f / float(display.height >> 2);
    for (int tap = 0; tap < kBlur11Taps; ++tap) {
        const float offset = float(tap - kBlur11Taps / 2);
        const float value[4] = { offset * stepX * texelU, offset * stepY * texelV, 0.0f, 0.0f };
        SetShaderConstant(uint32_t(tap), value);
    }
}

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

// Technique of the thermal vision object class (vtable 0x00B24190).
class ThermalObjectTechnique : public AglTechnique {
public:
    // TODO(0x0054CE80 ThermalObjectTechnique::BeginMaterial)
    // TODO(0x0054D23C ThermalObjectTechnique::Draw): world x view-projection
    // upload and the object space eye position ("gps_vObjSpcEyePos").
};

// Programs of the thermal object class (0x00F67508): plain and with
// normals, each with a skinned vertex program.
const AglShaderVariant kThermalObjectVariants[] = {
    { "PostProc_ThermalObject", "PostProc_ThermalObject_SK", "PostProc_ThermalObject" },
    { "PostProc_ThermalObject_N", "PostProc_ThermalObject_N_SK", "PostProc_ThermalObject_N" },
};
const AglGlobalShaderParam kThermalObjectParams[] = {    // 0x00F67520
    { "gps_vObjSpcEyePos", 0 }, { nullptr, 0 },
};

// The material class objects of the thermal vision pass draw with
// ("PostProc_ThermalObject", static object 0x011640F0, vtable 0x00B241B0).
class ThermalObjectClass : public AglShaderClass {
public:
    ThermalObjectClass() : AglShaderClass("PostProc_ThermalObject") {}
    // @func 0x007A2BA4 ThermalObjectClass::~ThermalObjectClass
    // @func 0x007A3D44 ThermalObjectClass::~ThermalObjectClass (deleting)
    ~ThermalObjectClass() override = default;
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x0054D4C0 ThermalObjectClass::CreateTechniques
void ThermalObjectClass::CreateTechniques()
{
    AddVariants<ThermalObjectTechnique>(kThermalObjectVariants, 0, 2, true, kThermalObjectParams);
}

// @func 0x0054DAB4 ThermalObjectClass::SelectTechnique
// Skinned geometry takes the "_SK" program; geometry with normals the "_N"
// pair. The pass uses no lights.
AglTechnique* ThermalObjectClass::SelectTechnique(AglMaterial* /*material*/, AglRenderItem* item)
{
    const AglSubMesh* mesh = item->geometry;
    const uint32_t skinned = (mesh->platformData && mesh->skinMatrices) ? 1 : 0;
    uint32_t normals = 0;
    for (const AglVertexStream* stream = mesh->vertexDecl->streams; stream->size != 0; ++stream) {
        if (stream->attribute == 2 && stream->attributeOffset == 0) {
            normals = 2;
            break;
        }
    }
    item->lightMask = 0;
    return GetTechnique(skinned + normals);
}

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

// @func 0x00546F6C AglPostProc::LoadCommonPostProcShaders
void LoadCommonPostProcShaders()
{
    InitPostProcShaders();
    LoadPair(&g_shaders.copy, "PostProc_Copy", kNoParams);
    LoadPair(&g_shaders.fill, "PostProc_Fill", kNoParams);
    LoadPair(&g_shaders.renderTxr, "PostProc_RenderTxr", kColorParams);
    LoadPair(&g_shaders.renderAlphaTxr, "PostProc_RenderAlphaTxr", kColorParams);
    LoadPair(&g_shaders.downSample4x, "PostProc_DownSample4x", kNoParams);
    LoadPair(&g_shaders.upSample4x, "PostProc_UpSample4x", kAlphaParams);
    // (sic: the shader files are named "Guassian")
    LoadPair(&g_shaders.blur11H, "PostProc_GuassianBlur11H", kBlur11Params);
    LoadPair(&g_shaders.blur11V, "PostProc_GuassianBlur11V", kBlur11Params);
    LoadPair(&g_shaders.blur3H, "PostProc_GuassianBlur03H", kBlur3Params);
    LoadPair(&g_shaders.blur3V, "PostProc_GuassianBlur03V", kBlur3Params);
    LoadPair(&g_shaders.downSampleDepth, "PostProc_DownSampleDepth", kSampleModeParams);
    LoadPair(&g_shaders.copyScale, "PostProc_CopyScale", kColorScaleParams);

    EnsureFragmentRegisters(g_shaders.copy.fragment, 32);
    EnsureFragmentRegisters(g_shaders.renderTxr.fragment, 32);
    EnsureFragmentRegisters(g_shaders.renderAlphaTxr.fragment, 32);
    EnsureFragmentRegisters(g_shaders.downSample4x.fragment, 43);
    EnsureFragmentRegisters(g_shaders.upSample4x.fragment, 8);
    EnsureFragmentRegisters(g_shaders.blur3H.fragment, 32);
    EnsureFragmentRegisters(g_shaders.blur3V.fragment, 32);
    EnsureFragmentRegisters(g_shaders.blur11H.fragment, 8);
    EnsureFragmentRegisters(g_shaders.blur11V.fragment, 8);
    EnsureFragmentRegisters(g_shaders.downSampleDepth.fragment, 32);
    EnsureFragmentRegisters(g_shaders.copyScale.fragment, 32);

    UploadBlurKernelH();
    UploadBlurKernelV();
}

// @func 0x0053CFD0 AglPostProc::UploadBlurKernelH
void UploadBlurKernelH()
{
    UploadBlurKernel(g_shaders.blur11H.fragment, 1.0f, 0.0f);
}

// @func 0x0053CE00 AglPostProc::UploadBlurKernelV
void UploadBlurKernelV()
{
    UploadBlurKernel(g_shaders.blur11V.fragment, 0.0f, 1.0f);
}

void LoadDarkScreenShaders()
{
    LoadPair(&g_shaders.darkScreen, "Sly_PostProc_DarkScreen", kDarkScreenParams);
    EnsureFragmentRegisters(g_shaders.darkScreen.fragment, 32);
}

} // namespace agl
