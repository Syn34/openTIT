// openTIT - The built-in Agl shader classes (material types) and their techniques.
//
// Program names and parameter tables are the ones the console shipped
// with. Most classes keep a table of technique variants: pairs of a normal
// and a skinned (or UV animated) vertex program sharing a fragment
// program, for 0..4 dynamic lights and for 1..3 texture layers. Their
// SelectTechnique computes the index into that table.
#include "agl/AglShaderClass.h"

#include "agl/AglResources.h"
#include "agl/AglState.h"
#include "agl/AglTexture.h"

#include "ape/core/ApeString.h"

#include <algorithm>
#include <cstring>

namespace agl {

namespace {

// --- parameter and variant tables -------------------------------------------------

const AglGlobalShaderParam kParams_F88B5C[] = {
    { "gfBlendAlpha", 0 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F88BD4[] = {
    { "gvBumpOffsets[0]", 1 },
    { "gvBumpOffsets[1]", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F88C5C[] = {
    { "gvParams", 0 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F88C7C[] = {
    { "gvDepthParams", 0 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F88CD4[] = {
    { "gps_mCamClipToProjClip[0]", 0 },
    { "gps_mCamClipToProjClip[1]", 1 },
    { "gps_mCamClipToProjClip[2]", 2 },
    { "gps_mCamClipToProjClip[3]", 3 },
    { "gps_vParams", 4 },
    { "gps_vSsViewDir", 5 },
    { "gps_vSsNormal", 6 },
    { "gps_vCamDepthParams", 7 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F88EB4[] = {
    { "gfBlendSwitch", 0 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8A484[] = {
    { "gvBumpOffsets[0]", 1 },
    { "gvBumpOffsets[1]", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F88CAC[] = {
    { "gfBlendSwitch", 0 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F88D1C[] = {
    { "gvColor", 0 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F88D3C[] = {
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F88D5C[] = {
    { "gvParams", 0 },
    { "gObjSpcEyePos", 1 },
    { "gfBlendAlpha", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F88F4C[] = {
    { "gvParams", 0 },
    { "gfBlendAlpha", 1 },
    { "gmObjToCam[0]", 2 },
    { "gmObjToCam[1]", 3 },
    { "gmObjToCam[2]", 4 },
    { "gvObjSpcCamPos", 5 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F890BC[] = {
    { "gvParams", 0 },
    { "gObjSpcEyePos", 1 },
    { "gfBlendAlpha", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F890F4[] = {
    { "gvParams", 0 },
    { "gObjSpcEyePos", 1 },
    { "gfBlendAlpha", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F89114[] = {
    { "gps_vBumpParams", 1 },
    { "gps_mIndirectMtx1[0]", 2 },
    { "gps_mIndirectMtx1[1]", 3 },
    { "gps_mIndirectMtx1[2]", 4 },
    { "gps_mIndirectMtx2[0]", 5 },
    { "gps_mIndirectMtx2[1]", 6 },
    { "gps_mIndirectMtx2[2]", 7 },
    { "gps_fBlendAlpha", 8 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8919C[] = {
    { "gfBlendAlpha", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F891AC[] = {
    { "gfBlendAlpha", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F891BC[] = {
    { "gfBlendAlpha", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F891CC[] = {
    { "gfBlendAlpha", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F891DC[] = {
    { "gfBlendAlpha", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F891EC[] = {
    { "gvParams", 0 },
    { "gfBlendAlpha", 1 },
    { "gmObjToCam[0]", 2 },
    { "gmObjToCam[1]", 3 },
    { "gmObjToCam[2]", 4 },
    { "gvObjSpcCamPos", 5 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8923C[] = {
    { "gfBlendSwitch", 0 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F892A0[] = {
    { "gps_params", 0 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F89338[] = {
    { "gvParams", 0 },
    { "gObjSpcEyePos", 1 },
    { "gfBlendAlpha", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F894D0[] = {
    { "gvParams", 0 },
    { "gObjSpcEyePos", 1 },
    { "gfBlendAlpha", 2 },
    { "gps_vRimParams", 3 },
    { "gps_vLightMultParams", 4 },
    { "gps_vDebugParam", 5 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8961C[] = {
    { "gLocalSpcEyeBone", 0 },
    { "gObjSpcEyePos", 1 },
    { "gfBlendAlpha", 2 },
    { "gfEyeParams", 3 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F89704[] = {
    { "gps_vFurParams0", 0 },
    { "gps_vRimParams", 1 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F897C4[] = {
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F89858[] = {
    { "gvParams", 0 },
    { "gObjSpcEyePos", 1 },
    { "gfBlendAlpha", 2 },
    { "gps_vRimParams", 3 },
    { "gps_vLightMultParams", 4 },
    { "gps_vFurParams", 5 },
    { "gps_vDebugParam", 6 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8995C[] = {
    { "gvParams", 0 },
    { "gObjSpcEyePos", 1 },
    { "gfBlendAlpha", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F89AB4[] = {
    { "gvParams", 0 },
    { "gObjSpcEyePos", 1 },
    { "gfBlendSwitch", 3 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F89C1C[] = {
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F89C94[] = {
    { "gvParams", 0 },
    { "gfBlendAlpha", 1 },
    { "gmObjToCam[0]", 2 },
    { "gmObjToCam[1]", 3 },
    { "gmObjToCam[2]", 4 },
    { "gObjSpcEyePos", 5 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F89E2C[] = {
    { "gps_vDepthParams", 0 },
    { "gps_vFogParams", 1 },
    { "gps_fBlendAlpha", 2 },
    { "gps_vIndirectMtx1[0]", 3 },
    { "gps_vIndirectMtx1[1]", 4 },
    { "gps_vIndirectMtx1[2]", 5 },
    { "gps_vIndirectMtx2[0]", 6 },
    { "gps_vIndirectMtx2[1]", 7 },
    { "gps_vIndirectMtx2[2]", 8 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F89EDC[] = {
    { "gvParams", 0 },
    { "gfBlendAlpha", 1 },
    { "gMtlColor", 2 },
    { "gmObjToCam[0]", 3 },
    { "gmObjToCam[1]", 4 },
    { "gmObjToCam[2]", 5 },
    { "gObjSpcEyePos", 6 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8A054[] = {
    { "gvParams", 0 },
    { "gObjSpcEyePos", 1 },
    { "gfBlendAlpha", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8A1F4[] = {
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8A234[] = {
    { "gvParams", 0 },
    { "gObjSpcEyePos", 1 },
    { "gfBlendAlpha", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8A38C[] = {
    { "gvParams", 0 },
    { "gObjSpcEyePos", 1 },
    { "gfBlendAlpha", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8A4D4[] = {
    { "gps_fAlphaCull", 0 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8A514[] = {
    { "gObjSpcEyePos", 2 },
    { "gps_fBlendAlpha", 0 },
    { "gps_vSparkleParams", 1 },
    { "gps_vSnowRimColor", 3 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8A5F8[] = {
    { "gps_fBlendAlpha", 0 },
    { "gps_vSnowUVParams", 1 },
    { "gps_vSparkleParams", 2 },
    { "gObjSpcEyePos", 3 },
    { "gps_vSnowRimColor", 4 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8A6E8[] = {
    { "gvBumpParams", 1 },
    { "gfBlendAlpha", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8A738[] = {
    { "gps_vBumpParams", 1 },
    { "gps_vIndirectMtx1[0]", 2 },
    { "gps_vIndirectMtx1[1]", 3 },
    { "gps_vIndirectMtx1[2]", 4 },
    { "gps_vIndirectMtx2[0]", 5 },
    { "gps_vIndirectMtx2[1]", 6 },
    { "gps_vIndirectMtx2[2]", 7 },
    { "gps_fBlendAlpha", 8 },
    { "gps_vParams", 9 },
    { "gps_ObjSpcEyePos", 10 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8A860[] = {
    { "gps_vBumpParams", 0 },
    { "gvParams", 1 },
    { "gObjSpcEyePos", 2 },
    { "gps_vRefrParams", 3 },
    { "gps_vIntensities", 4 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8AC30[] = {
    { "gps_vBumpParams", 0 },
    { "gvParams", 1 },
    { "gObjSpcEyePos", 2 },
    { "gps_vRefrWarpParams[0]", 3 },
    { "gps_vRefrWarpParams[1]", 4 },
    { "gps_vIntensities", 5 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8AF80[] = {
    { "gvParams", 0 },
    { "gObjSpcEyePos", 1 },
    { "gfBlendAlpha", 2 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8B010[] = {
    { "gvBumpParams", 1 },
    { "gvIndirectMtx1[0]", 2 },
    { "gvIndirectMtx1[1]", 3 },
    { "gvIndirectMtx1[2]", 4 },
    { "gvIndirectMtx2[0]", 5 },
    { "gvIndirectMtx2[1]", 6 },
    { "gvIndirectMtx2[2]", 7 },
    { "gfBlendAlpha", 8 },
    { nullptr, 0 },
};

const AglGlobalShaderParam kParams_F8B078[] = {
    { "gfBlendAlpha", 2 },
    { nullptr, 0 },
};

const AglShaderVariant kVariants_Decal[] = {
    { "Decal", "Decal_SK", "Decal" },
    { "Decal_Lit", "Decal_Lit_SK", "Decal_Lit" },
    { "Decal_Lit_1", "Decal_Lit_1_SK", "Decal_Lit" },
    { "Decal_Lit_2", "Decal_Lit_2_SK", "Decal_Lit" },
    { "Decal_Lit_3", "Decal_Lit_3_SK", "Decal_Lit" },
    { "Decal_Lit_4", "Decal_Lit_4_SK", "Decal_Lit" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
};

const AglShaderVariant kVariants_Diffuse[] = {
    { "Diffuse", "Diffuse_SK", "Diffuse" },
    { "Diffuse_Lit", "Diffuse_Lit_SK", "Diffuse_Lit" },
    { "Diffuse_Lit_1", "Diffuse_Lit_1_SK", "Diffuse_Lit" },
    { "Diffuse_Lit_2", "Diffuse_Lit_2_SK", "Diffuse_Lit" },
    { "Diffuse_Lit_3", "Diffuse_Lit_3_SK", "Diffuse_Lit" },
    { "Diffuse_Lit_4", "Diffuse_Lit_4_SK", "Diffuse_Lit" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "DiffusePP_Lit", "DiffusePP_Lit_SK", "DiffusePP_Lit" },
    { "DiffusePP_Lit_1", "DiffusePP_Lit_1_SK", "DiffusePP_Lit_1" },
    { "DiffusePP_Lit_1", "DiffusePP_Lit_1_SK", "DiffusePP_Lit_2" },
    { "DiffusePP_Lit_1", "DiffusePP_Lit_1_SK", "DiffusePP_Lit_3" },
    { "DiffusePP_Lit_1", "DiffusePP_Lit_1_SK", "DiffusePP_Lit_4" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "DiffusePP_Lit_Spec", "DiffusePP_Lit_Spec_SK", "DiffusePP_Lit_Spec" },
    { "DiffusePP_Lit_Spec_1", "DiffusePP_Lit_Spec_1_SK", "DiffusePP_Lit_Spec_1" },
    { "DiffusePP_Lit_Spec_1", "DiffusePP_Lit_Spec_1_SK", "DiffusePP_Lit_Spec_2" },
    { "DiffusePP_Lit_Spec_1", "DiffusePP_Lit_Spec_1_SK", "DiffusePP_Lit_Spec_3" },
    { "DiffusePP_Lit_Spec_1", "DiffusePP_Lit_Spec_1_SK", "DiffusePP_Lit_Spec_4" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
};

const AglShaderVariant kVariants_EnvMap[] = {
    { "EnvMapS", "EnvMapS_SK", "EnvMapS" },
    { "EnvMapS_Lit", "EnvMapS_Lit_SK", "EnvMapS_Lit" },
    { "EnvMapS_Lit_1", "EnvMapS_Lit_1_SK", "EnvMapS_Lit" },
    { "EnvMapS_Lit_2", "EnvMapS_Lit_2_SK", "EnvMapS_Lit" },
    { "EnvMapS_Lit_3", "EnvMapS_Lit_3_SK", "EnvMapS_Lit" },
    { "EnvMapS_Lit_4", "EnvMapS_Lit_4_SK", "EnvMapS_Lit" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "EnvMapSPP_Lit", "EnvMapSPP_Lit_SK", "EnvMapSPP_Lit" },
    { "EnvMapSPP_Lit_1", "EnvMapSPP_Lit_1_SK", "EnvMapSPP_Lit_1" },
    { "EnvMapSPP_Lit_1", "EnvMapSPP_Lit_1_SK", "EnvMapSPP_Lit_2" },
    { "EnvMapSPP_Lit_1", "EnvMapSPP_Lit_1_SK", "EnvMapSPP_Lit_3" },
    { "EnvMapSPP_Lit_1", "EnvMapSPP_Lit_1_SK", "EnvMapSPP_Lit_4" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "EnvMapSPP_Lit_Spec", "EnvMapSPP_Lit_Spec_SK", "EnvMapSPP_Lit_Spec" },
    { "EnvMapSPP_Lit_Spec_1", "EnvMapSPP_Lit_Spec_1_SK", "EnvMapSPP_Lit_Spec_1" },
    { "EnvMapSPP_Lit_Spec_1", "EnvMapSPP_Lit_Spec_1_SK", "EnvMapSPP_Lit_Spec_2" },
    { "EnvMapSPP_Lit_Spec_1", "EnvMapSPP_Lit_Spec_1_SK", "EnvMapSPP_Lit_Spec_3" },
    { "EnvMapSPP_Lit_Spec_1", "EnvMapSPP_Lit_Spec_1_SK", "EnvMapSPP_Lit_Spec_4" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
};

const AglShaderVariant kVariants_Sly2UV[] = {
    { "Sly2UV", "Sly2UV_UVX", "Sly2UV" },
    { "Sly2UV_Lit", "Sly2UV_Lit_UVX", "Sly2UV_Lit" },
    { "Sly2UV_Lit_1", "Sly2UV_Lit_1_UVX", "Sly2UV_Lit" },
    { "Sly2UV_Lit_2", "Sly2UV_Lit_2_UVX", "Sly2UV_Lit" },
    { "Sly2UV_Lit_3", "Sly2UV_Lit_3_UVX", "Sly2UV_Lit" },
};

const AglShaderVariant kVariants_SlyCaustics[] = {
    { "SlyCaustics", "SlyCaustics_UVX", "SlyCaustics" },
    { "SlyCaustics_Lit", "SlyCaustics_Lit_UVX", "SlyCaustics_Lit" },
    { "SlyCaustics_Lit_1", "SlyCaustics_Lit_1_UVX", "SlyCaustics_Lit" },
    { "SlyCaustics_Lit_2", "SlyCaustics_Lit_2_UVX", "SlyCaustics_Lit" },
    { "SlyCaustics_Lit_3", "SlyCaustics_Lit_3_UVX", "SlyCaustics_Lit" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
};

const AglShaderVariant kVariants_SlyCharacter[] = {
    { "SlyCharacter", "SlyCharacter_SK", "SlyCharacter" },
    { "SlyCharacter_Lit", "SlyCharacter_Lit_SK", "SlyCharacter_Lit" },
    { "SlyCharacter_Lit_1", "SlyCharacter_Lit_1_SK", "SlyCharacter_Lit" },
    { "SlyCharacter_Lit_2", "SlyCharacter_Lit_2_SK", "SlyCharacter_Lit" },
    { "SlyCharacter_Lit_3", "SlyCharacter_Lit_3_SK", "SlyCharacter_Lit" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyCharacterPP_Lit", "SlyCharacterPP_Lit_SK", "SlyCharacterPP_Lit" },
    { "SlyCharacterPP_Lit_1", "SlyCharacterPP_Lit_1_SK", "SlyCharacterPP_Lit_1" },
    { "SlyCharacterPP_Lit_1", "SlyCharacterPP_Lit_1_SK", "SlyCharacterPP_Lit_2" },
    { "SlyCharacterPP_Lit_1", "SlyCharacterPP_Lit_1_SK", "SlyCharacterPP_Lit_3" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyCharacterPP_Lit", "SlyCharacterPP_Lit_SK", "SlyCharacterPP_Lit_Spec" },
    { "SlyCharacterPP_Lit_1", "SlyCharacterPP_Lit_1_SK", "SlyCharacterPP_Lit_Spec_1" },
    { "SlyCharacterPP_Lit_1", "SlyCharacterPP_Lit_1_SK", "SlyCharacterPP_Lit_Spec_2" },
    { "SlyCharacterPP_Lit_1", "SlyCharacterPP_Lit_1_SK", "SlyCharacterPP_Lit_Spec_3" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
};

const AglShaderVariant kVariants_SlyCharacter2[] = {
    { "SlyCharacter2", "SlyCharacter2_SK", "SlyCharacter2" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyCharacter2_Lit", "SlyCharacter2_Lit_SK", "SlyCharacter2PP_Lit" },
    { "SlyCharacter2_Lit", "SlyCharacter2_Lit_SK", "SlyCharacter2PP_Lit_1" },
    { "SlyCharacter2_Lit", "SlyCharacter2_Lit_SK", "SlyCharacter2PP_Lit_2" },
    { "SlyCharacter2_Lit", "SlyCharacter2_Lit_SK", "SlyCharacter2PP_Lit_3" },
    { "SlyCharacter2_Lit_Nrm", "SlyCharacter2_Lit_Nrm_SK", "SlyCharacter2PP_Lit_Nrm" },
    { "SlyCharacter2_Lit_Nrm", "SlyCharacter2_Lit_Nrm_SK", "SlyCharacter2PP_Lit_Nrm_1" },
    { "SlyCharacter2_Lit_Nrm", "SlyCharacter2_Lit_Nrm_SK", "SlyCharacter2PP_Lit_Nrm_2" },
    { "SlyCharacter2_Lit_Nrm", "SlyCharacter2_Lit_Nrm_SK", "SlyCharacter2PP_Lit_Nrm_3" },
    { "SlyCharacter2_Lit_Nrm", "SlyCharacter2_Lit_Nrm_SK", "SlyCharacter2PP_Lit_Nrm_Spec" },
    { "SlyCharacter2_Lit_Nrm", "SlyCharacter2_Lit_Nrm_SK", "SlyCharacter2PP_Lit_Nrm_Spec_1" },
    { "SlyCharacter2_Lit_Nrm", "SlyCharacter2_Lit_Nrm_SK", "SlyCharacter2PP_Lit_Nrm_Spec_2" },
    { "SlyCharacter2_Lit_Nrm", "SlyCharacter2_Lit_Nrm_SK", "SlyCharacter2PP_Lit_Nrm_Spec_3" },
};

const AglShaderVariant kVariants_SlyCharacterEye[] = {
    { "SlyCharacterEye", "SlyCharacterEye_SK", "SlyCharacterEye" },
    { "SlyCharacterEye_Lit", "SlyCharacterEye_Lit_SK", "SlyCharacterEye_Lit" },
    { "SlyCharacterEye_Lit_1", "SlyCharacterEye_Lit_1_SK", "SlyCharacterEye_Lit" },
    { "SlyCharacterEye_Lit_2", "SlyCharacterEye_Lit_2_SK", "SlyCharacterEye_Lit" },
    { "SlyCharacterEye_Lit_3", "SlyCharacterEye_Lit_3_SK", "SlyCharacterEye_Lit" },
    { nullptr, nullptr, nullptr },
};

const AglShaderVariant kVariants_SlyCharacterFur[] = {
    { "SlyCharacterFur", "SlyCharacterFur_SK", "SlyCharacterFur" },
    { "SlyCharacterFur_0", "SlyCharacterFur_0_SK", "SlyCharacterFur" },
    { "SlyCharacterFur_1", "SlyCharacterFur_1_SK", "SlyCharacterFur" },
    { "SlyCharacterFur_2", "SlyCharacterFur_2_SK", "SlyCharacterFur" },
    { "SlyCharacterFur_3", "SlyCharacterFur_3_SK", "SlyCharacterFur" },
    { "SlyCharacterFur_S", "SlyCharacterFur_S_SK", "SlyCharacterFur_S" },
    { "SlyCharacterFur_S_0", "SlyCharacterFur_S_0_SK", "SlyCharacterFur_S" },
    { "SlyCharacterFur_S_1", "SlyCharacterFur_S_1_SK", "SlyCharacterFur_S" },
    { "SlyCharacterFur_S_2", "SlyCharacterFur_S_2_SK", "SlyCharacterFur_S" },
    { "SlyCharacterFur_S_3", "SlyCharacterFur_S_3_SK", "SlyCharacterFur_S" },
};

const AglShaderVariant kVariants_SlyCharacterFurFin[] = {
    { "SlyCharacterFurFin", "SlyCharacterFurFin_SK", "SlyCharacterFurFin" },
    { "SlyCharacterFurFin_0", "SlyCharacterFurFin_0_SK", "SlyCharacterFurFin" },
    { "SlyCharacterFurFin_1", "SlyCharacterFurFin_1_SK", "SlyCharacterFurFin" },
    { "SlyCharacterFurFin_2", "SlyCharacterFurFin_2_SK", "SlyCharacterFurFin" },
    { "SlyCharacterFurFin_3", "SlyCharacterFurFin_3_SK", "SlyCharacterFurFin" },
};

const AglShaderVariant kVariants_SlyCharacterFakeFur[] = {
    { "SlyCharacterFakeFur", "SlyCharacterFakeFur_SK", "SlyCharacterFakeFur" },
    { "SlyCharacterFakeFur_Lit", "SlyCharacterFakeFur_Lit_SK", "SlyCharacterFakeFurPP_Lit" },
    { "SlyCharacterFakeFur_Lit", "SlyCharacterFakeFur_Lit_SK", "SlyCharacterFakeFurPP_Lit_1" },
    { "SlyCharacterFakeFur_Lit", "SlyCharacterFakeFur_Lit_SK", "SlyCharacterFakeFurPP_Lit_2" },
    { "SlyCharacterFakeFur_Lit", "SlyCharacterFakeFur_Lit_SK", "SlyCharacterFakeFurPP_Lit_3" },
    { "SlyCharacterFakeFur_Lit_Nrm", "SlyCharacterFakeFur_Lit_Nrm_SK", "SlyCharacterFakeFurPP_Lit_Nrm" },
    { "SlyCharacterFakeFur_Lit_Nrm", "SlyCharacterFakeFur_Lit_Nrm_SK", "SlyCharacterFakeFurPP_Lit_Nrm_1" },
    { "SlyCharacterFakeFur_Lit_Nrm", "SlyCharacterFakeFur_Lit_Nrm_SK", "SlyCharacterFakeFurPP_Lit_Nrm_2" },
    { "SlyCharacterFakeFur_Lit_Nrm", "SlyCharacterFakeFur_Lit_Nrm_SK", "SlyCharacterFakeFurPP_Lit_Nrm_3" },
    { "SlyCharacterFakeFur_Lit_Nrm", "SlyCharacterFakeFur_Lit_Nrm_SK", "SlyCharacterFakeFurPP_Lit_Nrm_Spec" },
    { "SlyCharacterFakeFur_Lit_Nrm", "SlyCharacterFakeFur_Lit_Nrm_SK", "SlyCharacterFakeFurPP_Lit_Nrm_Spec_1" },
    { "SlyCharacterFakeFur_Lit_Nrm", "SlyCharacterFakeFur_Lit_Nrm_SK", "SlyCharacterFakeFurPP_Lit_Nrm_Spec_2" },
    { "SlyCharacterFakeFur_Lit_Nrm", "SlyCharacterFakeFur_Lit_Nrm_SK", "SlyCharacterFakeFurPP_Lit_Nrm_Spec_3" },
};

const AglShaderVariant kVariants_SlyCharacterCollectible[] = {
    { "SlyCharacter", "SlyCharacter_SK", "SlyCharacter" },
    { "SlyCharacter_Lit", "SlyCharacter_Lit_SK", "SlyCharacter_Lit" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyCharacterPP_Lit", "SlyCharacterPP_Lit_SK", "SlyCharacterPP_Lit" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyCharacterPP_Lit", "SlyCharacterPP_Lit_SK", "SlyCharacterPP_Lit_Spec" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
};

const AglShaderVariant kVariants_SlyDecal[] = {
    { "SlyDecal", "SlyDecal_UVX", "SlyDecal" },
    { "SlyDecal_Lit", "SlyDecal_Lit_UVX", "SlyDecal_Lit" },
    { "SlyDecal_Lit_1", "SlyDecal_Lit_1_UVX", "SlyDecal_Lit" },
    { "SlyDecal_Lit_2", "SlyDecal_Lit_2_UVX", "SlyDecal_Lit" },
    { "SlyDecal_Lit_3", "SlyDecal_Lit_3_UVX", "SlyDecal_Lit" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyDecalPP_Lit", "SlyDecalPP_Lit_UVX", "SlyDecalPP_Lit" },
    { "SlyDecalPP_Lit_1", "SlyDecalPP_Lit_1_UVX", "SlyDecalPP_Lit_1" },
    { "SlyDecalPP_Lit_1", "SlyDecalPP_Lit_1_UVX", "SlyDecalPP_Lit_2" },
    { "SlyDecalPP_Lit_1", "SlyDecalPP_Lit_1_UVX", "SlyDecalPP_Lit_3" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyDecalPP_Lit", "SlyDecalPP_Lit_UVX", "SlyDecalPP_Lit_Spec" },
    { "SlyDecalPP_Lit_1", "SlyDecalPP_Lit_1_UVX", "SlyDecalPP_Lit_Spec_1" },
    { "SlyDecalPP_Lit_1", "SlyDecalPP_Lit_1_UVX", "SlyDecalPP_Lit_Spec_2" },
    { "SlyDecalPP_Lit_1", "SlyDecalPP_Lit_1_UVX", "SlyDecalPP_Lit_Spec_3" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
};

const AglShaderVariant kVariants_SlyDecal4[] = {
    { "SlyDecal4", nullptr, "SlyDecal4" },
    { "SlyDecal4_Lit", nullptr, "SlyDecal4_Lit" },
    { "SlyDecal4_Lit_1", nullptr, "SlyDecal4_Lit" },
    { "SlyDecal4_Lit_2", nullptr, "SlyDecal4_Lit" },
    { "SlyDecal4_Lit_3", nullptr, "SlyDecal4_Lit" },
    { nullptr, nullptr, nullptr },
};

const AglShaderVariant kVariants_SlyEnvMap[] = {
    { "SlyEnvMap", "SlyEnvMap_UVX", "SlyEnvMap" },
    { "SlyEnvMap_Lit", "SlyEnvMap_Lit_UVX", "SlyEnvMap_Lit" },
    { "SlyEnvMap_Lit_1", "SlyEnvMap_Lit_1_UVX", "SlyEnvMap_Lit" },
    { "SlyEnvMap_Lit_2", "SlyEnvMap_Lit_2_UVX", "SlyEnvMap_Lit" },
    { "SlyEnvMap_Lit_3", "SlyEnvMap_Lit_3_UVX", "SlyEnvMap_Lit" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyEnvMapPP_Lit", "SlyEnvMapPP_Lit_UVX", "SlyEnvMapPP_Lit" },
    { "SlyEnvMapPP_Lit_1", "SlyEnvMapPP_Lit_1_UVX", "SlyEnvMapPP_Lit_1" },
    { "SlyEnvMapPP_Lit_1", "SlyEnvMapPP_Lit_1_UVX", "SlyEnvMapPP_Lit_2" },
    { "SlyEnvMapPP_Lit_1", "SlyEnvMapPP_Lit_1_UVX", "SlyEnvMapPP_Lit_3" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyEnvMapPP_Lit", "SlyEnvMapPP_Lit_UVX", "SlyEnvMapPP_Lit_Spec" },
    { "SlyEnvMapPP_Lit_1", "SlyEnvMapPP_Lit_1_UVX", "SlyEnvMapPP_Lit_Spec_1" },
    { "SlyEnvMapPP_Lit_1", "SlyEnvMapPP_Lit_1_UVX", "SlyEnvMapPP_Lit_Spec_2" },
    { "SlyEnvMapPP_Lit_1", "SlyEnvMapPP_Lit_1_UVX", "SlyEnvMapPP_Lit_Spec_3" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
};

const AglShaderVariant kVariants_SlyHeightFog[] = {
    { "SlyHeightFog", "SlyHeightFog_UVX", "SlyHeightFog" },
    { "SlyHeightFog_W", "SlyHeightFog_W_UVX", "SlyHeightFog_W" },
    { "SlyHeightFog_R", "SlyHeightFog_R_UVX", "SlyHeightFog_R" },
    { "SlyHeightFog_W_R", "SlyHeightFog_W_R_UVX", "SlyHeightFog_W_R" },
};

const AglShaderVariant kVariants_SlyMirror[] = {
    { "SlyMirror", "SlyMirror_UVX", "SlyMirror" },
    { "SlyMirror_Lit", "SlyMirror_Lit_UVX", "SlyMirror_Lit" },
    { "SlyMirror_Lit_1", "SlyMirror_Lit_1_UVX", "SlyMirror_Lit" },
    { "SlyMirror_Lit_2", "SlyMirror_Lit_2_UVX", "SlyMirror_Lit" },
    { "SlyMirror_Lit_3", "SlyMirror_Lit_3_UVX", "SlyMirror_Lit" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyMirrorPP_Lit", "SlyMirrorPP_Lit_UVX", "SlyMirrorPP_Lit" },
    { "SlyMirrorPP_Lit_1", "SlyMirrorPP_Lit_1_UVX", "SlyMirrorPP_Lit_1" },
    { "SlyMirrorPP_Lit_1", "SlyMirrorPP_Lit_1_UVX", "SlyMirrorPP_Lit_2" },
    { "SlyMirrorPP_Lit_1", "SlyMirrorPP_Lit_1_UVX", "SlyMirrorPP_Lit_3" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyMirrorPP_Lit", "SlyMirrorPP_Lit_UVX", "SlyMirrorPP_Lit_Spec" },
    { "SlyMirrorPP_Lit_1", "SlyMirrorPP_Lit_1_UVX", "SlyMirrorPP_Lit_Spec_1" },
    { "SlyMirrorPP_Lit_1", "SlyMirrorPP_Lit_1_UVX", "SlyMirrorPP_Lit_Spec_2" },
    { "SlyMirrorPP_Lit_1", "SlyMirrorPP_Lit_1_UVX", "SlyMirrorPP_Lit_Spec_3" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
};

const AglShaderVariant kVariants_SlyPanorama[] = {
    { "SlyPanorama", "SlyPanorama_UVX", "SlyPanorama" },
    { "SlyPanorama_Lit", "SlyPanorama_Lit_UVX", "SlyPanorama_Lit" },
    { "SlyPanorama_Lit_1", "SlyPanorama_Lit_1_UVX", "SlyPanorama_Lit" },
    { "SlyPanorama_Lit_2", "SlyPanorama_Lit_2_UVX", "SlyPanorama_Lit" },
    { "SlyPanorama_Lit_3", "SlyPanorama_Lit_3_UVX", "SlyPanorama_Lit" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyPanoramaPP_Lit", "SlyPanoramaPP_Lit_UVX", "SlyPanoramaPP_Lit" },
    { "SlyPanoramaPP_Lit_1", "SlyPanoramaPP_Lit_1_UVX", "SlyPanoramaPP_Lit_1" },
    { "SlyPanoramaPP_Lit_1", "SlyPanoramaPP_Lit_1_UVX", "SlyPanoramaPP_Lit_2" },
    { "SlyPanoramaPP_Lit_1", "SlyPanoramaPP_Lit_1_UVX", "SlyPanoramaPP_Lit_3" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyPanoramaPP_Lit", "SlyPanoramaPP_Lit_UVX", "SlyPanoramaPP_Lit_Spec" },
    { "SlyPanoramaPP_Lit_1", "SlyPanoramaPP_Lit_1_UVX", "SlyPanoramaPP_Lit_Spec_1" },
    { "SlyPanoramaPP_Lit_1", "SlyPanoramaPP_Lit_1_UVX", "SlyPanoramaPP_Lit_Spec_2" },
    { "SlyPanoramaPP_Lit_1", "SlyPanoramaPP_Lit_1_UVX", "SlyPanoramaPP_Lit_Spec_3" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
};

const AglShaderVariant kVariants_SlyProjTxr[] = {
    { "SlyProjTxr", nullptr, "SlyProjTxr" },
    { "SlyProjTxr_SS", nullptr, "SlyProjTxr_SS" },
};

const AglShaderVariant kVariants_SlyProp[] = {
    { "SlyProp", "SlyProp_UVX", "SlyProp" },
    { "SlyProp_Lit", "SlyProp_Lit_UVX", "SlyProp_Lit" },
    { "SlyProp_Lit_1", "SlyProp_Lit_1_UVX", "SlyProp_Lit" },
    { "SlyProp_Lit_2", "SlyProp_Lit_2_UVX", "SlyProp_Lit" },
    { "SlyProp_Lit_3", "SlyProp_Lit_3_UVX", "SlyProp_Lit" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyPropPP_Lit", "SlyPropPP_Lit_UVX", "SlyPropPP_Lit" },
    { "SlyPropPP_Lit_1", "SlyPropPP_Lit_1_UVX", "SlyPropPP_Lit_1" },
    { "SlyPropPP_Lit_1", "SlyPropPP_Lit_1_UVX", "SlyPropPP_Lit_2" },
    { "SlyPropPP_Lit_1", "SlyPropPP_Lit_1_UVX", "SlyPropPP_Lit_3" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyPropPP_Lit", "SlyPropPP_Lit_UVX", "SlyPropPP_Lit_Spec" },
    { "SlyPropPP_Lit_1", "SlyPropPP_Lit_1_UVX", "SlyPropPP_Lit_Spec_1" },
    { "SlyPropPP_Lit_1", "SlyPropPP_Lit_1_UVX", "SlyPropPP_Lit_Spec_2" },
    { "SlyPropPP_Lit_1", "SlyPropPP_Lit_1_UVX", "SlyPropPP_Lit_Spec_3" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
};

const AglShaderVariant kVariants_SlyTerrain[] = {
    { "SlyTerrain", "SlyTerrain_UVX", "SlyTerrain" },
    { "SlyTerrain_NoFog", "SlyTerrain_UVX_NoFog", "SlyTerrain_NoFog" },
    { "SlyTerrain", "SlyTerrain_UVX", "SlyTerrain_NoFade" },
    { "SlyTerrain_NoFog", "SlyTerrain_UVX_NoFog", "SlyTerrain_NoFog_NoFade" },
    { "SlyTerrain_Lit", "SlyTerrain_Lit_UVX", "SlyTerrain_Lit" },
    { "SlyTerrain_Lit_1", "SlyTerrain_Lit_1_UVX", "SlyTerrain_Lit" },
    { "SlyTerrain_Lit_2", "SlyTerrain_Lit_2_UVX", "SlyTerrain_Lit" },
    { "SlyTerrain_Lit_3", "SlyTerrain_Lit_3_UVX", "SlyTerrain_Lit" },
    { "SlyTerrainPP_Lit", "SlyTerrainPP_Lit_UVX", "SlyTerrainPP_Lit" },
    { "SlyTerrainPP_Lit_1", "SlyTerrainPP_Lit_1_UVX", "SlyTerrainPP_Lit_1" },
    { "SlyTerrainPP_Lit_1", "SlyTerrainPP_Lit_1_UVX", "SlyTerrainPP_Lit_2" },
    { "SlyTerrainPP_Lit_1", "SlyTerrainPP_Lit_1_UVX", "SlyTerrainPP_Lit_3" },
    { "SlyTerrainPP_Lit", "SlyTerrainPP_Lit_UVX", "SlyTerrainPP_Lit_Spec" },
    { "SlyTerrainPP_Lit_1", "SlyTerrainPP_Lit_1_UVX", "SlyTerrainPP_Lit_Spec_1" },
    { "SlyTerrainPP_Lit_1", "SlyTerrainPP_Lit_1_UVX", "SlyTerrainPP_Lit_Spec_2" },
    { "SlyTerrainPP_Lit_1", "SlyTerrainPP_Lit_1_UVX", "SlyTerrainPP_Lit_Spec_3" },
};

const AglShaderVariant kVariants_SlyTerrainSnow[] = {
    { nullptr, nullptr, nullptr },
    { "SlyTerrainSnow", "SlyTerrainSnow_UVX", "SlyTerrainSnow_Spark" },
    { "SlyTerrainSnow", "SlyTerrainSnow_UVX", "SlyTerrainSnow_Spark_Rim" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyTerrainSnow_Lit", "SlyTerrainSnow_Lit_UVX", "SlyTerrainSnow_Spark_Rim" },
    { "SlyTerrainSnow_Lit_1", "SlyTerrainSnow_Lit_1_UVX", "SlyTerrainSnow_Spark_Rim" },
    { "SlyTerrainSnow_Lit_2", "SlyTerrainSnow_Lit_2_UVX", "SlyTerrainSnow_Spark_Rim" },
    { "SlyTerrainSnow_Lit_3", "SlyTerrainSnow_Lit_3_UVX", "SlyTerrainSnow_Spark_Rim" },
};

const AglShaderVariant kVariants_SlyTerrainSnowDecal[] = {
    { nullptr, nullptr, nullptr },
    { "SlyTerrainSnowDecal", "SlyTerrainSnowDecal_UVX", "SlyTerrainSnowDecal_Spark" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyTerrainSnowDecal_Lit", "SlyTerrainSnowDecal_Lit_UVX", "SlyTerrainSnowDecal_Lit_Spark" },
    { "SlyTerrainSnowDecal_Lit_1", "SlyTerrainSnowDecal_Lit_1_UVX", "SlyTerrainSnowDecal_Lit_Spark" },
    { "SlyTerrainSnowDecal_Lit_2", "SlyTerrainSnowDecal_Lit_2_UVX", "SlyTerrainSnowDecal_Lit_Spark" },
    { "SlyTerrainSnowDecal_Lit_3", "SlyTerrainSnowDecal_Lit_3_UVX", "SlyTerrainSnowDecal_Lit_Spark" },
};

const AglShaderVariant kVariants_SlyWater[] = {
    { "SlyWater", "SlyWater_UVX", "SlyWater" },
    { "SlyWater_1", "SlyWater_1_UVX", "SlyWater_1" },
    { "SlyWater_W", "SlyWater_W_UVX", "SlyWater_W" },
    { "SlyWater_W_1", "SlyWater_W_1_UVX", "SlyWater_W_1" },
};

const AglShaderVariant kVariants_SlyWater2[] = {
    { "SlyWater2", nullptr, "SlyWater2" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyWater2", nullptr, "SlyWater2_W" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyWater2", nullptr, "SlyWater2_Refr" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyWater2", nullptr, "SlyWater2_W_Refr" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyWater2_D", nullptr, "SlyWater2" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyWater2_D", nullptr, "SlyWater2_W" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyWater2_D", nullptr, "SlyWater2_Refr" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyWater2_D", nullptr, "SlyWater2_W_Refr" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyWater2_0", nullptr, "SlyWater2" },
    { "SlyWater2_1", nullptr, "SlyWater2" },
    { "SlyWater2_2", nullptr, "SlyWater2" },
    { "SlyWater2_3", nullptr, "SlyWater2" },
    { "SlyWater2_0", nullptr, "SlyWater2_W" },
    { "SlyWater2_1", nullptr, "SlyWater2_W" },
    { "SlyWater2_2", nullptr, "SlyWater2_W" },
    { "SlyWater2_3", nullptr, "SlyWater2_W" },
    { "SlyWater2_0", nullptr, "SlyWater2_Refr" },
    { "SlyWater2_1", nullptr, "SlyWater2_Refr" },
    { "SlyWater2_2", nullptr, "SlyWater2_Refr" },
    { "SlyWater2_3", nullptr, "SlyWater2_Refr" },
    { "SlyWater2_0", nullptr, "SlyWater2_W_Refr" },
    { "SlyWater2_1", nullptr, "SlyWater2_W_Refr" },
    { "SlyWater2_2", nullptr, "SlyWater2_W_Refr" },
    { "SlyWater2_3", nullptr, "SlyWater2_W_Refr" },
    { "SlyWater2_0_D", nullptr, "SlyWater2" },
    { "SlyWater2_1_D", nullptr, "SlyWater2" },
    { "SlyWater2_2_D", nullptr, "SlyWater2" },
    { "SlyWater2_3_D", nullptr, "SlyWater2" },
    { "SlyWater2_0_D", nullptr, "SlyWater2_W" },
    { "SlyWater2_1_D", nullptr, "SlyWater2_W" },
    { "SlyWater2_2_D", nullptr, "SlyWater2_W" },
    { "SlyWater2_3_D", nullptr, "SlyWater2_W" },
    { "SlyWater2_0_D", nullptr, "SlyWater2_Refr" },
    { "SlyWater2_1_D", nullptr, "SlyWater2_Refr" },
    { "SlyWater2_2_D", nullptr, "SlyWater2_Refr" },
    { "SlyWater2_3_D", nullptr, "SlyWater2_Refr" },
    { "SlyWater2_0_D", nullptr, "SlyWater2_W_Refr" },
    { "SlyWater2_1_D", nullptr, "SlyWater2_W_Refr" },
    { "SlyWater2_2_D", nullptr, "SlyWater2_W_Refr" },
    { "SlyWater2_3_D", nullptr, "SlyWater2_W_Refr" },
};

const AglShaderVariant kVariants_SlyWater3[] = {
    { "SlyWater3", nullptr, "SlyWater3" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyWater3", nullptr, "SlyWater3_W" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyWater3_Refr", nullptr, "SlyWater3_Refr" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyWater3_Refr", nullptr, "SlyWater3_W_Refr" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyWater3_D", nullptr, "SlyWater3" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyWater3_D", nullptr, "SlyWater3_W" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyWater3_Refr_D", nullptr, "SlyWater3_Refr" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyWater3_Refr_D", nullptr, "SlyWater3_W_Refr" },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr },
    { "SlyWater3_0", nullptr, "SlyWater3" },
    { "SlyWater3_1", nullptr, "SlyWater3" },
    { "SlyWater3_2", nullptr, "SlyWater3" },
    { "SlyWater3_3", nullptr, "SlyWater3" },
    { "SlyWater3_0", nullptr, "SlyWater3_W" },
    { "SlyWater3_1", nullptr, "SlyWater3_W" },
    { "SlyWater3_2", nullptr, "SlyWater3_W" },
    { "SlyWater3_3", nullptr, "SlyWater3_W" },
    { "SlyWater3_0_Refr", nullptr, "SlyWater3_Refr" },
    { "SlyWater3_1_Refr", nullptr, "SlyWater3_Refr" },
    { "SlyWater3_2_Refr", nullptr, "SlyWater3_Refr" },
    { "SlyWater3_3_Refr", nullptr, "SlyWater3_Refr" },
    { "SlyWater3_0_Refr", nullptr, "SlyWater3_W_Refr" },
    { "SlyWater3_1_Refr", nullptr, "SlyWater3_W_Refr" },
    { "SlyWater3_2_Refr", nullptr, "SlyWater3_W_Refr" },
    { "SlyWater3_3_Refr", nullptr, "SlyWater3_W_Refr" },
    { "SlyWater3_0_D", nullptr, "SlyWater3" },
    { "SlyWater3_1_D", nullptr, "SlyWater3" },
    { "SlyWater3_2_D", nullptr, "SlyWater3" },
    { "SlyWater3_3_D", nullptr, "SlyWater3" },
    { "SlyWater3_0_D", nullptr, "SlyWater3_W" },
    { "SlyWater3_1_D", nullptr, "SlyWater3_W" },
    { "SlyWater3_2_D", nullptr, "SlyWater3_W" },
    { "SlyWater3_3_D", nullptr, "SlyWater3_W" },
    { "SlyWater3_0_Refr_D", nullptr, "SlyWater3_Refr" },
    { "SlyWater3_1_Refr_D", nullptr, "SlyWater3_Refr" },
    { "SlyWater3_2_Refr_D", nullptr, "SlyWater3_Refr" },
    { "SlyWater3_3_Refr_D", nullptr, "SlyWater3_Refr" },
    { "SlyWater3_0_Refr_D", nullptr, "SlyWater3_W_Refr" },
    { "SlyWater3_1_Refr_D", nullptr, "SlyWater3_W_Refr" },
    { "SlyWater3_2_Refr_D", nullptr, "SlyWater3_W_Refr" },
    { "SlyWater3_3_Refr_D", nullptr, "SlyWater3_W_Refr" },
};

const AglShaderVariant kVariants_SHODefault[] = {
    { "SHO_Default", "SHO_Default_UVX", "SHO_Default" },
    { "SHO_Default_NoFog", "SHO_Default_UVX_NoFog", "SHO_Default_NoFog" },
    { "SHO_Default", "SHO_Default_UVX", "SHO_Default_NoFade" },
    { "SHO_Default_NoFog", "SHO_Default_UVX_NoFog", "SHO_Default_NoFog_NoFade" },
    { "SHO_Default_Lit", "SHO_Default_Lit_UVX", "SHO_Default_Lit" },
    { "SHO_Default_Lit_1", "SHO_Default_Lit_1_UVX", "SHO_Default_Lit" },
    { "SHO_Default_Lit_2", "SHO_Default_Lit_2_UVX", "SHO_Default_Lit" },
    { "SHO_Default_Lit_3", "SHO_Default_Lit_3_UVX", "SHO_Default_Lit" },
};

const AglShaderVariant kVariants_SHONightBlend[] = {
    { "SHO_NightBlend", "SHO_NightBlend_UVX", "SHO_NightBlend" },
    { "SHO_NightBlend_NoFog", "SHO_NightBlend_UVX_NoFog", "SHO_NightBlend_NoFog" },
    { "SHO_NightBlend", "SHO_NightBlend_UVX", "SHO_NightBlend_NoFade" },
    { "SHO_NightBlend_NoFog", "SHO_NightBlend_UVX_NoFog", "SHO_NightBlend_NoFog_NoFade" },
    { "SHO_NightBlend_Lit", "SHO_NightBlend_Lit_UVX", "SHO_NightBlend_Lit" },
    { "SHO_NightBlend_Lit_1", "SHO_NightBlend_Lit_1_UVX", "SHO_NightBlend_Lit" },
    { "SHO_NightBlend_Lit_2", "SHO_NightBlend_Lit_2_UVX", "SHO_NightBlend_Lit" },
    { "SHO_NightBlend_Lit_3", "SHO_NightBlend_Lit_3_UVX", "SHO_NightBlend_Lit" },
};

// --- selection helpers --------------------------------------------------------------

AglTechnique* Pick(const AglMaterial* material, uint32_t index)
{
    return material->shaderClass->GetTechnique(index);
}

// Index of a normal/alternative pair for the item's light variant.
uint32_t LightPair(const AglRenderItem* item, bool alt)
{
    return uint32_t(item->lights->variant) << 1 | (alt ? 1u : 0u);
}

// Characters use their skinned programs only with a full skeleton.
bool IsCharacterSkinned(const AglRenderItem* item)
{
    return item->geometry->platformData && item->geometry->skinMatrices;
}

// Material parameters from level data are ignored while the sign bit of
// this flag is set (0x010C5AB8). NOTE: what sets it is not known.
int8_t g_materialParamsFlag = 0;

bool MaterialParamsEnabled()
{
    return g_materialParamsFlag >= 0;
}

bool FloatBitsNonZero(float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits != 0;
}

// Unlit pair 0, or one light set at 0x02 / 0x10 / 0x20 depending on the
// number of texture layers ('one' layers, 'two' layers, more).
AglTechnique* SelectLayeredLit(AglMaterial* material, AglRenderItem* item, bool alt, uint8_t one, uint8_t two)
{
    item->lightMask = 0;
    if (!IsLit(item))
        return Pick(material, alt ? 1 : 0);
    const uint32_t pair = LightPair(item, alt);
    if (material->textureCount == one) {
        item->lightMask = 1;
        return Pick(material, pair + 0x02);
    }
    if (material->textureCount == two) {
        item->lightMask = 3;
        return Pick(material, pair + 0x10);
    }
    item->lightMask = 7;
    return Pick(material, pair + 0x20);
}

// Unlit pair 0 or the lit pair at 2.
AglTechnique* SelectSingleLit(AglMaterial* material, AglRenderItem* item, bool alt)
{
    item->lightMask = 0;
    if (!IsLit(item))
        return Pick(material, alt ? 1 : 0);
    item->lightMask = 1;
    return Pick(material, LightPair(item, alt) + 2);
}

// Screen warp materials: the distortion texture plus eight parameters.
// NOTE: ScreenWarp and SlyScreenWarp have identical, separate copies of
// this code on the console; field_2D = 8 is not decoded.
bool SetupWarpMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    AglTexture* texture = FindTexture(desc->diffuseHash);
    if (!texture)
        return false;
    texture->SetWrap(1, 1, 1);
    material->AddTexture(texture);
    material->field_2D = 8;
    for (uint32_t i = 0; i < 8; ++i)
        *material->GetConstant(i) = desc->params[i];
    return true;
}

} // namespace

// --- techniques -------------------------------------------------------------------

class DebugTechnique : public AglTechnique {
    // TODO(0x005ADA18 DebugTechnique::Draw)
public:
    // @func 0x005ADA08 DebugTechnique::BeginMaterial
    void BeginMaterial(AglMaterial* /*material*/) override {}
    // @func 0x007ABB3C DebugTechnique::~DebugTechnique
    // @func 0x007ABEB0 DebugTechnique::~DebugTechnique (deleting)
    ~DebugTechnique() override = default;
    // @func 0x007ABEAC DebugTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class PrimTxrTechnique : public AglTechnique {
    // TODO(0x005ADD54 PrimTxrTechnique::Draw)
public:
    // @func 0x005ADD50 PrimTxrTechnique::BeginMaterial
    void BeginMaterial(AglMaterial* /*material*/) override {}
    // @func 0x007ABB4C PrimTxrTechnique::~PrimTxrTechnique
    // @func 0x007AC134 PrimTxrTechnique::~PrimTxrTechnique (deleting)
    ~PrimTxrTechnique() override = default;
    // @func 0x007AC130 PrimTxrTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class SimpleColorTxrTechnique : public AglTechnique {
    // TODO(0x005ADEE4 SimpleColorTxrTechnique::Draw)
public:
    // @func 0x005ADEE0 SimpleColorTxrTechnique::BeginMaterial
    void BeginMaterial(AglMaterial* /*material*/) override {}
    // @func 0x007ABB5C SimpleColorTxrTechnique::~SimpleColorTxrTechnique
    // @func 0x007AC454 SimpleColorTxrTechnique::~SimpleColorTxrTechnique (deleting)
    ~SimpleColorTxrTechnique() override = default;
    // @func 0x007AC450 SimpleColorTxrTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class SimpleColorTxrUVTechnique : public AglTechnique {
    // TODO(0x005AE0F0 SimpleColorTxrUVTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    // @func 0x007ABB6C SimpleColorTxrUVTechnique::~SimpleColorTxrUVTechnique
    // @func 0x007AC608 SimpleColorTxrUVTechnique::~SimpleColorTxrUVTechnique (deleting)
    ~SimpleColorTxrUVTechnique() override = default;
    // @func 0x007AC604 SimpleColorTxrUVTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class TextureWarpTechnique : public AglTechnique {
    // TODO(0x005AE9A0 TextureWarpTechnique::BeginMaterial)
    // TODO(0x005AEB00 TextureWarpTechnique::Draw)
public:
    // @func 0x005AF07C TextureWarpTechnique::End
    void End(AglMaterial* /*material*/) override {}
    // @func 0x007ABB7C TextureWarpTechnique::~TextureWarpTechnique
    // @func 0x007AC7BC TextureWarpTechnique::~TextureWarpTechnique (deleting)
    ~TextureWarpTechnique() override = default;
};

class ScreenWarpTechnique : public AglTechnique {
    // TODO(0x005AF518 ScreenWarpTechnique::BeginMaterial)
    // TODO(0x005AF730 ScreenWarpTechnique::Draw)
public:
    // @func 0x005AFAB8 ScreenWarpTechnique::End
    void End(AglMaterial* /*material*/) override {}
    // @func 0x007ABB8C ScreenWarpTechnique::~ScreenWarpTechnique
    // @func 0x007AC874 ScreenWarpTechnique::~ScreenWarpTechnique (deleting)
    ~ScreenWarpTechnique() override = default;
};

class LineOutlineTechnique : public AglTechnique {
    // TODO(0x005AFAD0 LineOutlineTechnique::Draw)
public:
    // @func 0x005AFACC LineOutlineTechnique::BeginMaterial
    void BeginMaterial(AglMaterial* /*material*/) override {}
    // @func 0x007ABB9C LineOutlineTechnique::~LineOutlineTechnique
    // @func 0x007ACCAC LineOutlineTechnique::~LineOutlineTechnique (deleting)
    ~LineOutlineTechnique() override = default;
    // @func 0x007ACCA8 LineOutlineTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class OutlineTechnique : public AglTechnique {
    // TODO(0x005AFBF8 OutlineTechnique::Draw)
public:
    // @func 0x005AFBE8 OutlineTechnique::BeginMaterial
    void BeginMaterial(AglMaterial* /*material*/) override {}
    // @func 0x007ABBAC OutlineTechnique::~OutlineTechnique
    // @func 0x007ACD68 OutlineTechnique::~OutlineTechnique (deleting)
    ~OutlineTechnique() override = default;
    // @func 0x007ACD64 OutlineTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class GeoParticleTechnique : public AglTechnique {
    // TODO(0x005B0388 GeoParticleTechnique::BeginMaterial)
    // TODO(0x005B0440 GeoParticleTechnique::Draw)
public:
    // @func 0x007ABBBC GeoParticleTechnique::~GeoParticleTechnique
    // @func 0x007AD018 GeoParticleTechnique::~GeoParticleTechnique (deleting)
    ~GeoParticleTechnique() override = default;
    // @func 0x007AD014 GeoParticleTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class SprParticleTechnique : public AglTechnique {
    // TODO(0x005B06B0 SprParticleTechnique::BeginMaterial)
    // TODO(0x005B095C SprParticleTechnique::Draw)
public:
    void End(AglMaterial* material) override;
    // @func 0x007ABBCC SprParticleTechnique::~SprParticleTechnique
    // @func 0x007AD1CC SprParticleTechnique::~SprParticleTechnique (deleting)
    ~SprParticleTechnique() override = default;
};

class ProjTxrTechnique : public AglTechnique {
    // TODO(0x005B0FB0 ProjTxrTechnique::Draw)
public:
    // @func 0x007ABBDC ProjTxrTechnique::~ProjTxrTechnique
    // @func 0x007AD514 ProjTxrTechnique::~ProjTxrTechnique (deleting)
    ~ProjTxrTechnique() override = default;
    // @func 0x007AD510 ProjTxrTechnique::End
    void End(AglMaterial* /*material*/) override {}
    void BeginMaterial(AglMaterial* material) override;
};

class ProjTxrDeferredTechnique : public AglTechnique {
    // TODO(0x005B1830 ProjTxrDeferredTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    // @func 0x007ABBEC ProjTxrDeferredTechnique::~ProjTxrDeferredTechnique
    // @func 0x007AD5D0 ProjTxrDeferredTechnique::~ProjTxrDeferredTechnique (deleting)
    ~ProjTxrDeferredTechnique() override = default;
    // @func 0x007AD5CC ProjTxrDeferredTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class FluidSimTechnique : public AglTechnique {
public:
    void BeginMaterial(AglMaterial* material) override;
    void Draw(AglRenderItem* item) override;
    // @func 0x007ABBFC FluidSimTechnique::~FluidSimTechnique
    // @func 0x007AD85C FluidSimTechnique::~FluidSimTechnique (deleting)
    ~FluidSimTechnique() override = default;
    // @func 0x007AD858 FluidSimTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class ZPassTechnique : public AglTechnique {
    // TODO(0x005B1AE8 ZPassTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    // @func 0x007ABC0C ZPassTechnique::~ZPassTechnique
    // @func 0x007AD918 ZPassTechnique::~ZPassTechnique (deleting)
    ~ZPassTechnique() override = default;
    // @func 0x007AD914 ZPassTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class DiffuseTechnique : public AglTechnique {
    // TODO(0x005B1FA0 DiffuseTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    void End(AglMaterial* material) override;
    // @func 0x007ABC1C DiffuseTechnique::~DiffuseTechnique
    // @func 0x007AD9D0 DiffuseTechnique::~DiffuseTechnique (deleting)
    ~DiffuseTechnique() override = default;
};

class DecalTechnique : public AglTechnique {
    // TODO(0x005B3148 DecalTechnique::Draw)
public:
    // @func 0x007ABC2C DecalTechnique::~DecalTechnique
    // @func 0x007ADA8C DecalTechnique::~DecalTechnique (deleting)
    ~DecalTechnique() override = default;
    // @func 0x007ADA88 DecalTechnique::End
    void End(AglMaterial* /*material*/) override {}
    void BeginMaterial(AglMaterial* material) override;
};

class EnvMapTechnique : public AglTechnique {
    // TODO(0x005B3590 EnvMapTechnique::Draw)
public:
    // @func 0x007ABC3C EnvMapTechnique::~EnvMapTechnique
    // @func 0x007AE180 EnvMapTechnique::~EnvMapTechnique (deleting)
    ~EnvMapTechnique() override = default;
    // @func 0x007AE17C EnvMapTechnique::End
    void End(AglMaterial* /*material*/) override {}
    void BeginMaterial(AglMaterial* material) override;
};

class SelfLightTechnique : public AglTechnique {
    // TODO(0x005B48E8 SelfLightTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    // @func 0x007ABC4C SelfLightTechnique::~SelfLightTechnique
    // @func 0x007AE23C SelfLightTechnique::~SelfLightTechnique (deleting)
    ~SelfLightTechnique() override = default;
    // @func 0x007AE238 SelfLightTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class AngleIndexTechnique : public AglTechnique {
    // TODO(0x005B5150 AngleIndexTechnique::Draw)
public:
    // @func 0x007ABC5C AngleIndexTechnique::~AngleIndexTechnique
    // @func 0x007AE2F8 AngleIndexTechnique::~AngleIndexTechnique (deleting)
    ~AngleIndexTechnique() override = default;
    // @func 0x007AE2F4 AngleIndexTechnique::End
    void End(AglMaterial* /*material*/) override {}
    void BeginMaterial(AglMaterial* material) override;
};

class WaterTechnique : public AglTechnique {
    // TODO(0x005B5990 WaterTechnique::BeginMaterial)
    // TODO(0x005B5E00 WaterTechnique::Draw)
public:
    void End(AglMaterial* material) override;
    // @func 0x007ABC6C WaterTechnique::~WaterTechnique
    // @func 0x007AE3B0 WaterTechnique::~WaterTechnique (deleting)
    ~WaterTechnique() override = default;
};

class SacSkinnedTechnique : public AglTechnique {
    // TODO(0x005B67C8 SacSkinnedTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    // @func 0x007ABC7C SacSkinnedTechnique::~SacSkinnedTechnique
    // @func 0x007AE46C SacSkinnedTechnique::~SacSkinnedTechnique (deleting)
    ~SacSkinnedTechnique() override = default;
    // @func 0x007AE468 SacSkinnedTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class SacEnvironmentTechnique : public AglTechnique {
    // TODO(0x005B7260 SacEnvironmentTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    // @func 0x007ABC8C SacEnvironmentTechnique::~SacEnvironmentTechnique
    // @func 0x007AE528 SacEnvironmentTechnique::~SacEnvironmentTechnique (deleting)
    ~SacEnvironmentTechnique() override = default;
    // @func 0x007AE524 SacEnvironmentTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class SacPropTechnique : public AglTechnique {
    // TODO(0x005B78A8 SacPropTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    // @func 0x007ABC9C SacPropTechnique::~SacPropTechnique
    // @func 0x007AE5E4 SacPropTechnique::~SacPropTechnique (deleting)
    ~SacPropTechnique() override = default;
    // @func 0x007AE5E0 SacPropTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class SacLitPropTechnique : public AglTechnique {
    // TODO(0x005B7F18 SacLitPropTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    // @func 0x007ABCAC SacLitPropTechnique::~SacLitPropTechnique
    // @func 0x007AE6A0 SacLitPropTechnique::~SacLitPropTechnique (deleting)
    ~SacLitPropTechnique() override = default;
    // @func 0x007AE69C SacLitPropTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class SacFastPropTechnique : public AglTechnique {
    // TODO(0x005B8640 SacFastPropTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    // @func 0x007ABCBC SacFastPropTechnique::~SacFastPropTechnique
    // @func 0x007AE75C SacFastPropTechnique::~SacFastPropTechnique (deleting)
    ~SacFastPropTechnique() override = default;
    // @func 0x007AE758 SacFastPropTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class SacEnvMapTechnique : public AglTechnique {
    // TODO(0x005B8EC0 SacEnvMapTechnique::Draw)
public:
    // @func 0x007ABCCC SacEnvMapTechnique::~SacEnvMapTechnique
    // @func 0x007AE818 SacEnvMapTechnique::~SacEnvMapTechnique (deleting)
    ~SacEnvMapTechnique() override = default;
    // @func 0x007AE814 SacEnvMapTechnique::End
    void End(AglMaterial* /*material*/) override {}
    void BeginMaterial(AglMaterial* material) override;
};

class Sly2UVTechnique : public AglTechnique {
    // TODO(0x005B9D0C Sly2UVTechnique::Draw)
public:
    // @func 0x007ABCDC Sly2UVTechnique::~Sly2UVTechnique
    // @func 0x007AEA44 Sly2UVTechnique::~Sly2UVTechnique (deleting)
    ~Sly2UVTechnique() override = default;
    // @func 0x007AEA40 Sly2UVTechnique::End
    void End(AglMaterial* /*material*/) override {}
    void BeginMaterial(AglMaterial* material) override;
};

class SlyCausticsTechnique : public AglTechnique {
    // TODO(0x005BA8BC SlyCausticsTechnique::Draw)
public:
    // @func 0x007ABCEC SlyCausticsTechnique::~SlyCausticsTechnique
    // @func 0x007AEB00 SlyCausticsTechnique::~SlyCausticsTechnique (deleting)
    ~SlyCausticsTechnique() override = default;
    // @func 0x007AEAFC SlyCausticsTechnique::End
    void End(AglMaterial* /*material*/) override {}
    void BeginMaterial(AglMaterial* material) override;
};

class SlyCharacterTechnique : public AglTechnique {
    // TODO(0x005BB4D8 SlyCharacterTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    void End(AglMaterial* material) override;
    // @func 0x007ABCFC SlyCharacterTechnique::~SlyCharacterTechnique
    // @func 0x007AEBB8 SlyCharacterTechnique::~SlyCharacterTechnique (deleting)
    ~SlyCharacterTechnique() override = default;
};

class SlyCharacter2Technique : public AglTechnique {
    // TODO(0x005BD3B0 SlyCharacter2Technique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    void End(AglMaterial* material) override;
    // @func 0x007ABD0C SlyCharacter2Technique::~SlyCharacter2Technique
    // @func 0x007AEC70 SlyCharacter2Technique::~SlyCharacter2Technique (deleting)
    ~SlyCharacter2Technique() override = default;
};

class SlyCharacterEyeTechnique : public AglTechnique {
    // TODO(0x005BE100 SlyCharacterEyeTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    void End(AglMaterial* material) override;
    // @func 0x007ABD1C SlyCharacterEyeTechnique::~SlyCharacterEyeTechnique
    // @func 0x007AED28 SlyCharacterEyeTechnique::~SlyCharacterEyeTechnique (deleting)
    ~SlyCharacterEyeTechnique() override = default;
};

class SlyCharacterFurTechnique : public AglTechnique {
    // TODO(0x005BFD20 SlyCharacterFurTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    void End(AglMaterial* material) override;
    // @func 0x007ABD2C SlyCharacterFurTechnique::~SlyCharacterFurTechnique
    // @func 0x007AEDE0 SlyCharacterFurTechnique::~SlyCharacterFurTechnique (deleting)
    ~SlyCharacterFurTechnique() override = default;
};

class SlyCharacterFurFinTechnique : public AglTechnique {
    // TODO(0x005C13DC SlyCharacterFurFinTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    void End(AglMaterial* material) override;
    // @func 0x007ABD3C SlyCharacterFurFinTechnique::~SlyCharacterFurFinTechnique
    // @func 0x007AEE98 SlyCharacterFurFinTechnique::~SlyCharacterFurFinTechnique (deleting)
    ~SlyCharacterFurFinTechnique() override = default;
};

class SlyCharacterFakeFurTechnique : public AglTechnique {
    // TODO(0x005C1EB8 SlyCharacterFakeFurTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    void End(AglMaterial* material) override;
    // @func 0x007ABD4C SlyCharacterFakeFurTechnique::~SlyCharacterFakeFurTechnique
    // @func 0x007AEF50 SlyCharacterFakeFurTechnique::~SlyCharacterFakeFurTechnique (deleting)
    ~SlyCharacterFakeFurTechnique() override = default;
};

class SlyCharacterCollectibleTechnique : public AglTechnique {
    // TODO(0x005C2880 SlyCharacterCollectibleTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    void End(AglMaterial* material) override;
    // @func 0x007ABD5C SlyCharacterCollectibleTechnique::~SlyCharacterCollectibleTechnique
    // @func 0x007AF008 SlyCharacterCollectibleTechnique::~SlyCharacterCollectibleTechnique (deleting)
    ~SlyCharacterCollectibleTechnique() override = default;
};

class SlyDecalTechnique : public AglTechnique {
    // TODO(0x005C39C8 SlyDecalTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    // @func 0x007ABD6C SlyDecalTechnique::~SlyDecalTechnique
    // @func 0x007AF0C4 SlyDecalTechnique::~SlyDecalTechnique (deleting)
    ~SlyDecalTechnique() override = default;
    // @func 0x007AF0C0 SlyDecalTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class SlyDecal4Technique : public AglTechnique {
    // TODO(0x005C49C0 SlyDecal4Technique::BeginMaterial)
    // TODO(0x005C4C3C SlyDecal4Technique::Draw)
public:
    // @func 0x007ABD7C SlyDecal4Technique::~SlyDecal4Technique
    // @func 0x007AF180 SlyDecal4Technique::~SlyDecal4Technique (deleting)
    ~SlyDecal4Technique() override = default;
    // @func 0x007AF17C SlyDecal4Technique::End
    void End(AglMaterial* /*material*/) override {}
};

class SlyEnvMapTechnique : public AglTechnique {
    // TODO(0x005C57B0 SlyEnvMapTechnique::Draw)
public:
    // @func 0x007ABD8C SlyEnvMapTechnique::~SlyEnvMapTechnique
    // @func 0x007AF23C SlyEnvMapTechnique::~SlyEnvMapTechnique (deleting)
    ~SlyEnvMapTechnique() override = default;
    // @func 0x007AF238 SlyEnvMapTechnique::End
    void End(AglMaterial* /*material*/) override {}
    void BeginMaterial(AglMaterial* material) override;
};

class SlyHeightFogTechnique : public AglTechnique {
    // TODO(0x005C6D38 SlyHeightFogTechnique::BeginMaterial)
    // TODO(0x005C7210 SlyHeightFogTechnique::Draw)
public:
    // @func 0x007ABD9C SlyHeightFogTechnique::~SlyHeightFogTechnique
    // @func 0x007AF2F8 SlyHeightFogTechnique::~SlyHeightFogTechnique (deleting)
    ~SlyHeightFogTechnique() override = default;
    // @func 0x007AF2F4 SlyHeightFogTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class SlyMirrorTechnique : public AglTechnique {
    // TODO(0x005C8338 SlyMirrorTechnique::Draw)
public:
    // @func 0x007ABDAC SlyMirrorTechnique::~SlyMirrorTechnique
    // @func 0x007AF3B4 SlyMirrorTechnique::~SlyMirrorTechnique (deleting)
    ~SlyMirrorTechnique() override = default;
    // @func 0x007AF3B0 SlyMirrorTechnique::End
    void End(AglMaterial* /*material*/) override {}
    void BeginMaterial(AglMaterial* material) override;
};

class SlyPanoramaTechnique : public AglTechnique {
    // TODO(0x005C9948 SlyPanoramaTechnique::Draw)
public:
    // @func 0x007ABDBC SlyPanoramaTechnique::~SlyPanoramaTechnique
    // @func 0x007AF470 SlyPanoramaTechnique::~SlyPanoramaTechnique (deleting)
    ~SlyPanoramaTechnique() override = default;
    // @func 0x007AF46C SlyPanoramaTechnique::End
    void End(AglMaterial* /*material*/) override {}
    void BeginMaterial(AglMaterial* material) override;
};

class SlyProjTxrTechnique : public AglTechnique {
public:
    void BeginMaterial(AglMaterial* material) override;
    // @func 0x007ABDCC SlyProjTxrTechnique::~SlyProjTxrTechnique
    // @func 0x007AF530 SlyProjTxrTechnique::~SlyProjTxrTechnique (deleting)
    ~SlyProjTxrTechnique() override = default;
    // @func 0x007AF52C SlyProjTxrTechnique::End
    void End(AglMaterial* /*material*/) override {}
    // @func 0x007AF528 SlyProjTxrTechnique::Draw
    void Draw(AglRenderItem* /*item*/) override {}
};

class SlyPropTechnique : public AglTechnique {
    // TODO(0x005CAE18 SlyPropTechnique::Draw)
public:
    // @func 0x007ABDDC SlyPropTechnique::~SlyPropTechnique
    // @func 0x007AF5EC SlyPropTechnique::~SlyPropTechnique (deleting)
    ~SlyPropTechnique() override = default;
    // @func 0x007AF5E8 SlyPropTechnique::End
    void End(AglMaterial* /*material*/) override {}
    void BeginMaterial(AglMaterial* material) override;
};

class SlyScreenWarpTechnique : public AglTechnique {
    // TODO(0x005CD480 SlyScreenWarpTechnique::BeginMaterial)
    // TODO(0x005CD6F0 SlyScreenWarpTechnique::Draw)
public:
    void End(AglMaterial* material) override;
    // @func 0x007ABDFC SlyScreenWarpTechnique::~SlyScreenWarpTechnique
    // @func 0x007AF760 SlyScreenWarpTechnique::~SlyScreenWarpTechnique (deleting)
    ~SlyScreenWarpTechnique() override = default;
};

class SlyShrubTechnique : public AglTechnique {
    // TODO(0x005CDC38 SlyShrubTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    // @func 0x007ABE0C SlyShrubTechnique::~SlyShrubTechnique
    // @func 0x007AFC50 SlyShrubTechnique::~SlyShrubTechnique (deleting)
    ~SlyShrubTechnique() override = default;
    // @func 0x007AFC4C SlyShrubTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class SlyTerrainTechnique : public AglTechnique {
    // TODO(0x005CC240 SlyTerrainTechnique::Draw)
public:
    // @func 0x007ABDEC SlyTerrainTechnique::~SlyTerrainTechnique
    // @func 0x007AF6A8 SlyTerrainTechnique::~SlyTerrainTechnique (deleting)
    ~SlyTerrainTechnique() override = default;
    // @func 0x007AF6A4 SlyTerrainTechnique::End
    void End(AglMaterial* /*material*/) override {}
    void BeginMaterial(AglMaterial* material) override;
};

class SlyTerrainSnowTechnique : public AglTechnique {
    // TODO(0x005CE508 SlyTerrainSnowTechnique::BeginMaterial)
    // TODO(0x005CE6A0 SlyTerrainSnowTechnique::Draw)
public:
    // @func 0x007ABE1C SlyTerrainSnowTechnique::~SlyTerrainSnowTechnique
    // @func 0x007AFD0C SlyTerrainSnowTechnique::~SlyTerrainSnowTechnique (deleting)
    ~SlyTerrainSnowTechnique() override = default;
    // @func 0x007AFD08 SlyTerrainSnowTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class SlyTerrainSnowDecalTechnique : public AglTechnique {
    // TODO(0x005CFA68 SlyTerrainSnowDecalTechnique::BeginMaterial)
    // TODO(0x005CFDC0 SlyTerrainSnowDecalTechnique::Draw)
public:
    // @func 0x007ABE2C SlyTerrainSnowDecalTechnique::~SlyTerrainSnowDecalTechnique
    // @func 0x007AFDC8 SlyTerrainSnowDecalTechnique::~SlyTerrainSnowDecalTechnique (deleting)
    ~SlyTerrainSnowDecalTechnique() override = default;
    // @func 0x007AFDC4 SlyTerrainSnowDecalTechnique::End
    void End(AglMaterial* /*material*/) override {}
};

class SlyTextureWarpTechnique : public AglTechnique {
    // TODO(0x005D1628 SlyTextureWarpTechnique::BeginMaterial)
    // TODO(0x005D17E8 SlyTextureWarpTechnique::Draw)
public:
    void End(AglMaterial* material) override;
    // @func 0x007ABE3C SlyTextureWarpTechnique::~SlyTextureWarpTechnique
    // @func 0x007AFE80 SlyTextureWarpTechnique::~SlyTextureWarpTechnique (deleting)
    ~SlyTextureWarpTechnique() override = default;
};

class SlyWaterTechnique : public AglTechnique {
    // TODO(0x005D2360 SlyWaterTechnique::BeginMaterial)
    // TODO(0x005D2614 SlyWaterTechnique::Draw)
public:
    void End(AglMaterial* material) override;
    // @func 0x007ABE4C SlyWaterTechnique::~SlyWaterTechnique
    // @func 0x007AFF38 SlyWaterTechnique::~SlyWaterTechnique (deleting)
    ~SlyWaterTechnique() override = default;
};

class SlyWater2Technique : public AglTechnique {
    // TODO(0x005D33C8 SlyWater2Technique::BeginMaterial)
    // TODO(0x005D3750 SlyWater2Technique::Draw)
public:
    void End(AglMaterial* material) override;
    // @func 0x007ABE5C SlyWater2Technique::~SlyWater2Technique
    // @func 0x007AFFF0 SlyWater2Technique::~SlyWater2Technique (deleting)
    ~SlyWater2Technique() override = default;
};

class SlyWater3Technique : public AglTechnique {
    // TODO(0x005D4C80 SlyWater3Technique::BeginMaterial)
    // TODO(0x005D5220 SlyWater3Technique::Draw)
public:
    void End(AglMaterial* material) override;
    // @func 0x007ABE6C SlyWater3Technique::~SlyWater3Technique
    ~SlyWater3Technique() override = default;
};

class ShoDefaultTechnique : public AglTechnique {
    // TODO(0x005D65C0 ShoDefaultTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    // @func 0x007ABE7C ShoDefaultTechnique::~ShoDefaultTechnique
    ~ShoDefaultTechnique() override = default;
};

class ShoTextureWarpTechnique : public AglTechnique {
    // TODO(0x005D7438 ShoTextureWarpTechnique::BeginMaterial)
    // TODO(0x005D7598 ShoTextureWarpTechnique::Draw)
public:
    void End(AglMaterial* material) override;
    // @func 0x007ABE8C ShoTextureWarpTechnique::~ShoTextureWarpTechnique
    ~ShoTextureWarpTechnique() override = default;
};

class ShoNightBlendTechnique : public AglTechnique {
    // TODO(0x005D8100 ShoNightBlendTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    // @func 0x007ABE9C ShoNightBlendTechnique::~ShoNightBlendTechnique
    ~ShoNightBlendTechnique() override = default;
};

// @func 0x005B0AF0 SprParticleTechnique::End
void SprParticleTechnique::End(AglMaterial* material)
{
    // Materials with a depth bias turned polygon offset on.
    const float bias = (material && material->constants) ? material->constants[4] : 0.0f;
    if (bias != 0.0f)
        Cmd().SetPolygonOffset(false, 0.0f, 0.0f);
}

// --- material constants shared by the BeginMaterial handlers --------------------

// Material float constant `index`, or 0 when the material has none.
static float MaterialConstant(const AglMaterial* material, int index)
{
    return material->constants ? material->constants[index] : 0.0f;
}

// NOTE: materials whose word 0x78 top nibble is 2 (the default) get 1.0 in
// the slot the shaders use as a switch; its meaning is not decoded.
static float MaterialSwitchValue(const AglMaterial* material)
{
    return material->field_78 == 2 ? 1.0f : 0.0f;
}

// Fragment constant 0 = material colour constants 0..2, w = 'w'.
static void UploadMaterialColor(const AglMaterial* material, float w)
{
    const float value[4] = {MaterialConstant(material, 0), MaterialConstant(material, 1),
                            MaterialConstant(material, 2), w};
    SetShaderConstant(0, value);
}

// @func 0x005B0C78 ProjTxrTechnique::BeginMaterial
void ProjTxrTechnique::BeginMaterial(AglMaterial* material)
{
    const float value[4] = {MaterialSwitchValue(material), 0.0f, 0.0f, 0.0f};
    SetShaderConstant(0, value);
}

// @func 0x005B1E88 DiffuseTechnique::BeginMaterial
void DiffuseTechnique::BeginMaterial(AglMaterial* material)
{
    UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
    UploadMaterialColor(material, 0.0f);
    // Constant 3 holds an integer alpha reference; a positive one turns on
    // the alpha test.
    if (material->constants) {
        int32_t alphaRef = 0;
        std::memcpy(&alphaRef, &material->constants[3], sizeof(alphaRef));
        if (alphaRef > 0) {
            Cmd().SetAlphaFunc(gpu::CompareFunc::GEqual, uint32_t(alphaRef));
            Cmd().SetAlphaTestEnable(true);
        }
    }
}

// @func 0x005B30A0 DecalTechnique::BeginMaterial
void DecalTechnique::BeginMaterial(AglMaterial* material)
{
    // Constants 0 and 1 scale the texture coordinates.
    UploadUVTransform(material->anim, 4, MaterialConstant(material, 0), MaterialConstant(material, 1));
    const float value[4] = {MaterialSwitchValue(material), 0.0f, 0.0f, 0.0f};
    SetShaderConstant(0, value);
}

// @func 0x005B34C0 EnvMapTechnique::BeginMaterial
void EnvMapTechnique::BeginMaterial(AglMaterial* material)
{
    UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
    UploadMaterialColor(material, MaterialSwitchValue(material));
}

// @func 0x005B50A0 AngleIndexTechnique::BeginMaterial
void AngleIndexTechnique::BeginMaterial(AglMaterial* material)
{
    UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
    UploadMaterialColor(material, 0.0f);
}

// @func 0x005B8DF0 SacEnvMapTechnique::BeginMaterial
void SacEnvMapTechnique::BeginMaterial(AglMaterial* material)
{
    UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
    UploadMaterialColor(material, MaterialSwitchValue(material));
}

// @func 0x005B9C80 Sly2UVTechnique::BeginMaterial
void Sly2UVTechnique::BeginMaterial(AglMaterial* material)
{
    if (material->anim)
        UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
    const float value[4] = {MaterialSwitchValue(material), 0.0f, 0.0f, 0.0f};
    SetShaderConstant(0, value);
}

// @func 0x005BA7D0 SlyCausticsTechnique::BeginMaterial
void SlyCausticsTechnique::BeginMaterial(AglMaterial* material)
{
    // Constants 0/1 scale the texture coordinates, 2/3 are periods sent
    // as reciprocals. NOTE: a material without constants divides by zero, as
    // on the console.
    if (material->anim)
        UploadUVTransform(material->anim, 4, MaterialConstant(material, 0), MaterialConstant(material, 1));
    const float value[4] = {1.0f / MaterialConstant(material, 2), 1.0f / MaterialConstant(material, 3),
                            MaterialSwitchValue(material), 0.0f};
    SetShaderConstant(0, value);
}

// @func 0x005BB408 SlyCharacterTechnique::BeginMaterial
void SlyCharacterTechnique::BeginMaterial(AglMaterial* material)
{
    UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
    UploadMaterialColor(material, MaterialSwitchValue(material));
}

// @func 0x005C27B0 SlyCharacterCollectibleTechnique::BeginMaterial
void SlyCharacterCollectibleTechnique::BeginMaterial(AglMaterial* material)
{
    UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
    UploadMaterialColor(material, MaterialSwitchValue(material));
}

// @func 0x005C56E0 SlyEnvMapTechnique::BeginMaterial
void SlyEnvMapTechnique::BeginMaterial(AglMaterial* material)
{
    if (material->anim)
        UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
    UploadMaterialColor(material, MaterialSwitchValue(material));
}

// @func 0x005C8268 SlyMirrorTechnique::BeginMaterial
void SlyMirrorTechnique::BeginMaterial(AglMaterial* material)
{
    if (material->anim)
        UploadUVTransform(material->anim, 8, 1.0f, 1.0f);
    UploadMaterialColor(material, MaterialSwitchValue(material));
}

// @func 0x005C9898 SlyPanoramaTechnique::BeginMaterial
void SlyPanoramaTechnique::BeginMaterial(AglMaterial* material)
{
    if (material->anim)
        UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
    UploadMaterialColor(material, MaterialSwitchValue(material));
}

// @func 0x005CAD48 SlyPropTechnique::BeginMaterial
void SlyPropTechnique::BeginMaterial(AglMaterial* material)
{
    if (material->anim)
        UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
    UploadMaterialColor(material, MaterialSwitchValue(material));
}

// @func 0x005CC170 SlyTerrainTechnique::BeginMaterial
void SlyTerrainTechnique::BeginMaterial(AglMaterial* material)
{
    if (material->anim)
        UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
    UploadMaterialColor(material, MaterialSwitchValue(material));
}

// Shared by the second generation character techniques: colour constants
// and the rim parameters (constants 10 and 12) in vertex register 0x21 and
// fragment constant 4.
static void BeginCharacter2Material(AglMaterial* material)
{
    UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
    UploadMaterialColor(material, 0.0f);
    const float rim[4] = {MaterialConstant(material, 10), MaterialConstant(material, 12), 0.0f, 0.0f};
    Cmd().SetVertexProgramConstants(0x21, 4, rim);
    SetShaderConstant(4, rim);
}

// @func 0x005BD298 SlyCharacter2Technique::BeginMaterial
void SlyCharacter2Technique::BeginMaterial(AglMaterial* material)
{
    BeginCharacter2Material(material);
}

// @func 0x005C1E40 SlyCharacterFakeFurTechnique::BeginMaterial
void SlyCharacterFakeFurTechnique::BeginMaterial(AglMaterial* material)
{
    BeginCharacter2Material(material);
    const float fur[4] = {MaterialConstant(material, 13), 0.0f, 0.0f, 0.0f};
    SetShaderConstant(5, fur);
}

// @func 0x005C1368 SlyCharacterFurFinTechnique::BeginMaterial
void SlyCharacterFurFinTechnique::BeginMaterial(AglMaterial* material)
{
    const float fin[4] = {MaterialConstant(material, 0), MaterialConstant(material, 1), 0.0f, 0.0f};
    Cmd().SetVertexProgramConstants(7, 4, fin);
}

// @func 0x005C38C0 SlyDecalTechnique::BeginMaterial
void SlyDecalTechnique::BeginMaterial(AglMaterial* material)
{
    // Constants 0/1 scale the texture coordinates, 2..4 tint the decal.
    if (material->anim)
        UploadUVTransform(material->anim, 4, MaterialConstant(material, 0), MaterialConstant(material, 1));
    const float flag[4] = {MaterialSwitchValue(material), 0.0f, 0.0f, 0.0f};
    SetShaderConstant(3, flag);
    const float tint[4] = {MaterialConstant(material, 2), MaterialConstant(material, 3),
                           MaterialConstant(material, 4), 0.0f};
    SetShaderConstant(0, tint);
}

// @func 0x005B182C ProjTxrDeferredTechnique::BeginMaterial
void ProjTxrDeferredTechnique::BeginMaterial(AglMaterial* material)
{
    (void)material;
}

// @func 0x005B19CC FluidSimTechnique::BeginMaterial
void FluidSimTechnique::BeginMaterial(AglMaterial* material)
{
    (void)material;
}

// @func 0x005B19D0 FluidSimTechnique::Draw
void FluidSimTechnique::Draw(AglRenderItem* item)
{
    (void)item;
}

// @func 0x005B1AE4 ZPassTechnique::BeginMaterial
void ZPassTechnique::BeginMaterial(AglMaterial* material)
{
    (void)material;
}

// @func 0x005B2914 DiffuseTechnique::End
void DiffuseTechnique::End(AglMaterial* material)
{
    (void)material;
    // The lit variants enable alpha test for their alpha masked layers.
    Cmd().SetAlphaTestEnable(false);
}

// @func 0x005B47B8 SelfLightTechnique::BeginMaterial
void SelfLightTechnique::BeginMaterial(AglMaterial* material)
{
    UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
}

// @func 0x005B6420 WaterTechnique::End
void WaterTechnique::End(AglMaterial* material)
{
    (void)material;
}

// @func 0x005B6780 SacSkinnedTechnique::BeginMaterial
void SacSkinnedTechnique::BeginMaterial(AglMaterial* material)
{
    UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
}

// @func 0x005B7220 SacEnvironmentTechnique::BeginMaterial
void SacEnvironmentTechnique::BeginMaterial(AglMaterial* material)
{
    UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
}

// @func 0x005B78A0 SacPropTechnique::BeginMaterial
void SacPropTechnique::BeginMaterial(AglMaterial* material)
{
    (void)material;
}

// @func 0x005B7F0C SacLitPropTechnique::BeginMaterial
void SacLitPropTechnique::BeginMaterial(AglMaterial* material)
{
    (void)material;
}

// @func 0x005B8638 SacFastPropTechnique::BeginMaterial
void SacFastPropTechnique::BeginMaterial(AglMaterial* material)
{
    (void)material;
}

// @func 0x005BC02C SlyCharacterTechnique::End
void SlyCharacterTechnique::End(AglMaterial* material)
{
    (void)material;
}

// @func 0x005BD79C SlyCharacter2Technique::End
void SlyCharacter2Technique::End(AglMaterial* material)
{
    (void)material;
}

// @func 0x005BE0D4 SlyCharacterEyeTechnique::BeginMaterial
void SlyCharacterEyeTechnique::BeginMaterial(AglMaterial* material)
{
    (void)material;
}

// @func 0x005BF144 SlyCharacterEyeTechnique::End
void SlyCharacterEyeTechnique::End(AglMaterial* material)
{
    (void)material;
}

// @func 0x005BFB08 SlyCharacterFurTechnique::BeginMaterial
void SlyCharacterFurTechnique::BeginMaterial(AglMaterial* material)
{
    (void)material;
}

// @func 0x005C0C24 SlyCharacterFurTechnique::End
void SlyCharacterFurTechnique::End(AglMaterial* material)
{
    (void)material;
}

// @func 0x005C1A0C SlyCharacterFurFinTechnique::End
void SlyCharacterFurFinTechnique::End(AglMaterial* material)
{
    (void)material;
}

// @func 0x005C22A4 SlyCharacterFakeFurTechnique::End
void SlyCharacterFakeFurTechnique::End(AglMaterial* material)
{
    (void)material;
}

// @func 0x005C3208 SlyCharacterCollectibleTechnique::End
void SlyCharacterCollectibleTechnique::End(AglMaterial* material)
{
    (void)material;
}

// @func 0x005CAA90 SlyProjTxrTechnique::BeginMaterial
void SlyProjTxrTechnique::BeginMaterial(AglMaterial* material)
{
    (void)material;
}

// @func 0x005CDC14 SlyScreenWarpTechnique::End
void SlyScreenWarpTechnique::End(AglMaterial* material)
{
    (void)material;
}

// @func 0x005CDC28 SlyShrubTechnique::BeginMaterial
void SlyShrubTechnique::BeginMaterial(AglMaterial* material)
{
    (void)material;
}

// @func 0x005D1EC0 SlyTextureWarpTechnique::End
void SlyTextureWarpTechnique::End(AglMaterial* material)
{
    (void)material;
}

// @func 0x005D2C00 SlyWaterTechnique::End
void SlyWaterTechnique::End(AglMaterial* material)
{
    (void)material;
}

// @func 0x005D3F84 SlyWater2Technique::End
void SlyWater2Technique::End(AglMaterial* material)
{
    (void)material;
}

// @func 0x005D5A68 SlyWater3Technique::End
void SlyWater3Technique::End(AglMaterial* material)
{
    (void)material;
}

// @func 0x005D6578 ShoDefaultTechnique::BeginMaterial
void ShoDefaultTechnique::BeginMaterial(AglMaterial* material)
{
    if (material->anim)
        UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
}

// @func 0x005D7BCC ShoTextureWarpTechnique::End
void ShoTextureWarpTechnique::End(AglMaterial* material)
{
    (void)material;
}

// --- shader classes -----------------------------------------------------------------

class DebugClass : public AglShaderClass {
public:
    // @func 0x007AA82C DebugClass::~DebugClass
    // @func 0x007AC0BC DebugClass::~DebugClass (deleting)
    ~DebugClass() override = default;
    DebugClass() : AglShaderClass("Debug") {}
    void CreateTechniques() override;
};

// @func 0x007ABEF4 DebugClass::CreateTechniques
void DebugClass::CreateTechniques()
{
    AddShader<DebugTechnique>("FlatColor", "FlatColor", nullptr);
    AddShader<DebugTechnique>("FlatColor_SK", "FlatColor", nullptr);
}

class PrimTxrClass : public AglShaderClass {
public:
    // @func 0x007AA87C PrimTxrClass::~PrimTxrClass
    // @func 0x007AC270 PrimTxrClass::~PrimTxrClass (deleting)
    ~PrimTxrClass() override = default;
    PrimTxrClass() : AglShaderClass("PrimTxr") {}
    void CreateTechniques() override;
};

// @func 0x007AC178 PrimTxrClass::CreateTechniques
void PrimTxrClass::CreateTechniques()
{
    AddShader<PrimTxrTechnique>("PrimTxr", "PrimTxr", nullptr);
}

class PrimNoTxrClass : public AglShaderClass {
public:
    // @func 0x007AA8CC PrimNoTxrClass::~PrimNoTxrClass
    // @func 0x007AC3DC PrimNoTxrClass::~PrimNoTxrClass (deleting)
    ~PrimNoTxrClass() override = default;
    PrimNoTxrClass() : AglShaderClass("PrimNoTxr") {}
    void CreateTechniques() override;
};

// @func 0x007AC2E4 PrimNoTxrClass::CreateTechniques
void PrimNoTxrClass::CreateTechniques()
{
    // Shares the technique type of PrimTxr.
    AddShader<PrimTxrTechnique>("PrimNoTxr", "PrimNoTxr", nullptr);
}

class SimpleColorTxrClass : public AglShaderClass {
public:
    // @func 0x007AA91C SimpleColorTxrClass::~SimpleColorTxrClass
    // @func 0x007AC590 SimpleColorTxrClass::~SimpleColorTxrClass (deleting)
    ~SimpleColorTxrClass() override = default;
    SimpleColorTxrClass() : AglShaderClass("SimpleColorTxr") {}
    void CreateTechniques() override;
};

// @func 0x007AC498 SimpleColorTxrClass::CreateTechniques
void SimpleColorTxrClass::CreateTechniques()
{
    AddShader<SimpleColorTxrTechnique>("SimpleColorTxr", "SimpleColorTxr", nullptr);
}

class SimpleColorTxrUVClass : public AglShaderClass {
public:
    // @func 0x007AA96C SimpleColorTxrUVClass::~SimpleColorTxrUVClass
    // @func 0x007AC748 SimpleColorTxrUVClass::~SimpleColorTxrUVClass (deleting)
    ~SimpleColorTxrUVClass() override = default;
    SimpleColorTxrUVClass() : AglShaderClass("SimpleColorTxrUV") {}
    void CreateTechniques() override;
};

// @func 0x007AC64C SimpleColorTxrUVClass::CreateTechniques
void SimpleColorTxrUVClass::CreateTechniques()
{
    AddShader<SimpleColorTxrUVTechnique>("SimpleColorTxrUV", "SimpleColorTxrUV", kParams_F88B5C);
}

class TextureWarpClass : public AglShaderClass {
public:
    // @func 0x007AA9BC TextureWarpClass::~TextureWarpClass
    // @func 0x007AC800 TextureWarpClass::~TextureWarpClass (deleting)
    ~TextureWarpClass() override = default;
    TextureWarpClass() : AglShaderClass("TextureWarp") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    // Warp layer and parameters; also used by the water classes.
    bool SetupWarp(AglMaterial* material, const AglMaterialDesc* desc);
};

// TODO(0x005AF080 TextureWarpClass::CreateTechniques)
void TextureWarpClass::CreateTechniques()
{
}

class ScreenWarpClass : public AglShaderClass {
public:
    // @func 0x007AAA0C ScreenWarpClass::~ScreenWarpClass
    // @func 0x007ACC34 ScreenWarpClass::~ScreenWarpClass (deleting)
    ~ScreenWarpClass() override = default;
    ScreenWarpClass() : AglShaderClass("ScreenWarp") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
};

// @func 0x007AC8B8 ScreenWarpClass::CreateTechniques
void ScreenWarpClass::CreateTechniques()
{
    AddShader<ScreenWarpTechnique>("ScreenWarp", nullptr, kParams_F88BD4);
    AddShader<ScreenWarpTechnique>("ScreenWarp_Lit", nullptr, kParams_F88BD4);
}

// @func 0x007ACA84 ScreenWarpClass::SetupMaterial
bool ScreenWarpClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    return SetupWarpMaterial(material, desc);
}

class LineOutlineClass : public AglShaderClass {
public:
    // @func 0x007AAA5C LineOutlineClass::~LineOutlineClass
    // @func 0x007ACCF0 LineOutlineClass::~LineOutlineClass (deleting)
    ~LineOutlineClass() override = default;
    LineOutlineClass() : AglShaderClass("LineOutline") {}
    void CreateTechniques() override;
};

// TODO(0x005AFAD4 LineOutlineClass::CreateTechniques)
void LineOutlineClass::CreateTechniques()
{
}

class OutlineClass : public AglShaderClass {
public:
    // @func 0x007AAAAC OutlineClass::~OutlineClass
    // @func 0x007ACDAC OutlineClass::~OutlineClass (deleting)
    ~OutlineClass() override = default;
    OutlineClass() : AglShaderClass("Outline") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// TODO(0x005AFF0C OutlineClass::CreateTechniques)
void OutlineClass::CreateTechniques()
{
}

// @func 0x005B0280 OutlineClass::SetupMaterial
bool OutlineClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    // Outlines are drawn late, behind the surface, alpha blended.
    material->sortKey = 0xFFF;
    material->depthFunc = gpu::CompareFunc::Less;
    const float width = desc->paramCount != 0 ? desc->params[0] : 30.0f;
    *material->GetConstant(0) = width * width;
    material->blendEquation = gpu::BlendEquation::Add;
    material->blendEnable = true;
    material->blendSrc = gpu::BlendFactor::SrcAlpha;
    material->blendDst = gpu::BlendFactor::OneMinusSrcAlpha;
    return true;
}

// @func 0x005B0330 OutlineClass::SelectTechnique
AglTechnique* OutlineClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    // Silhouette pass (field_40 set) or the regular outline, each skinned or not.
    const uint32_t base = item->field_40 ? 0 : 2;
    return Pick(material, base + (IsSkinned(item) ? 1 : 0));
}

class ParticleMaterialClass : public AglShaderClass {
public:
    // @func 0x007AAAFC ParticleMaterialClass::~ParticleMaterialClass
    // @func 0x007ACFA0 ParticleMaterialClass::~ParticleMaterialClass (deleting)
    ~ParticleMaterialClass() override = default;
    ParticleMaterialClass() : AglShaderClass("ParticleMaterial") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x007ACE20 ParticleMaterialClass::CreateTechniques
void ParticleMaterialClass::CreateTechniques()
{
    // Particle materials only carry parameters; nothing is drawn with them.
}

// @func 0x007ACE38 ParticleMaterialClass::SetupMaterial
bool ParticleMaterialClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    // Up to five exported parameters; missing ones take their defaults.
    // NOTE: the meaning of the individual values is not known. Parameter 2
    // is copied bit for bit (an integer), parameter 3 is an integer that is
    // converted to float.
    const uint8_t count = desc->paramCount;
    *material->GetConstant(0) = count > 0 ? desc->params[0] * -2.0f : 0.0f;
    *material->GetConstant(1) = count > 1 ? desc->params[1] * -2.0f : 0.0f;
    const uint32_t raw = count > 2 ? desc->ParamBits(2) : 0u;
    std::memcpy(material->GetConstant(2), &raw, sizeof(raw));
    *material->GetConstant(3) = count > 3 ? float(desc->ParamBits(3)) : 1.0f;
    *material->GetConstant(4) = count > 4 ? desc->params[4] : 0.0f;
    return true;
}

// @func 0x007ACF98 ParticleMaterialClass::SelectTechnique
AglTechnique* ParticleMaterialClass::SelectTechnique(AglMaterial* /*material*/, AglRenderItem* /*item*/)
{
    return nullptr;
}

class GeoParticleClass : public AglShaderClass {
public:
    // @func 0x007AAB4C GeoParticleClass::~GeoParticleClass
    // @func 0x007AD158 GeoParticleClass::~GeoParticleClass (deleting)
    ~GeoParticleClass() override = default;
    GeoParticleClass() : AglShaderClass("GeoParticle") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x007AD05C GeoParticleClass::CreateTechniques
void GeoParticleClass::CreateTechniques()
{
    AddShader<GeoParticleTechnique>("GeoParticle", "GeoParticle", kParams_F88C5C);
}

// @func 0x005B068C GeoParticleClass::SelectTechnique
AglTechnique* GeoParticleClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    (void)item;
    return Pick(material, 0);
}

class SprParticleClass : public AglShaderClass {
public:
    // @func 0x007AAB9C SprParticleClass::~SprParticleClass
    // @func 0x007AD410 SprParticleClass::~SprParticleClass (deleting)
    ~SprParticleClass() override = default;
    SprParticleClass() : AglShaderClass("SprParticle") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x007AD210 SprParticleClass::CreateTechniques
void SprParticleClass::CreateTechniques()
{
    // Technique 0 draws hard sprites, technique 1 depth-faded ("soft") ones.
    AddShader<SprParticleTechnique>("SprParticle", nullptr, kParams_F88C7C);
    AddShader<SprParticleTechnique>("SprParticle_Soft", nullptr, kParams_F88C7C);
}

// @func 0x007AD3DC SprParticleClass::SelectTechnique
AglTechnique* SprParticleClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    // Constant 2 holds the technique index as an integer (see
    // ParticleMaterialClass::SetupMaterial). NOTE: the index is also stored
    // as the item's light mask, as on the console.
    uint32_t index = 0;
    if (material->constants)
        std::memcpy(&index, &material->constants[2], sizeof(index));
    item->lightMask = uint8_t(index);
    return Pick(material, index);
}

class ShadowClass : public AglShaderClass {
public:
    // @func 0x007AABEC ShadowClass::~ShadowClass
    // @func 0x007AD49C ShadowClass::~ShadowClass (deleting)
    ~ShadowClass() override = default;
    ShadowClass() : AglShaderClass("Shadow") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
};

// @func 0x007AD484 ShadowClass::CreateTechniques
void ShadowClass::CreateTechniques()
{
    // Retired class: its materials are redirected in SetupMaterial.
}

// @func 0x007AD488 ShadowClass::SetupMaterial
bool ShadowClass::SetupMaterial(AglMaterial* /*material*/, const AglMaterialDesc* desc)
{
    // NOTE: 0x03DE9E33 is the name hash of the replacement class; the name
    // itself was not recovered.
    desc->replacementClass = 0x03DE9E33u;
    return false;
}

class ProjTxrClass : public AglShaderClass {
public:
    // @func 0x007AAC3C ProjTxrClass::~ProjTxrClass
    // @func 0x007AD558 ProjTxrClass::~ProjTxrClass (deleting)
    ~ProjTxrClass() override = default;
    ProjTxrClass() : AglShaderClass("ProjTxr") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005B155C ProjTxrClass::CreateTechniques
void ProjTxrClass::CreateTechniques()
{
    AddShader<ProjTxrTechnique>("ProjTxr", "ProjTxr", kParams_F88CAC);
    AddShader<ProjTxrTechnique>("ProjTxr", "ProjTxr", kParams_F88CAC);
}

// @func 0x005B1728 ProjTxrClass::SetupMaterial
bool ProjTxrClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    AglTexture* texture = FindTexture(desc->textureHash);
    if (!texture)
        return false;
    texture->SetWrap(1, 1, 1);
    material->AddTexture(texture);
    material->field_78 = desc->wrapMode == 2 ? 2 : 3;
    return true;
}

// @func 0x005B17EC ProjTxrClass::SelectTechnique
AglTechnique* ProjTxrClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    item->lightMask = 0;
    return Pick(material, IsSkinned(item) ? 1 : 0);
}

class ProjTxrDeferredClass : public AglShaderClass {
public:
    // @func 0x007AAC8C ProjTxrDeferredClass::~ProjTxrDeferredClass
    // @func 0x007AD7E4 ProjTxrDeferredClass::~ProjTxrDeferredClass (deleting)
    ~ProjTxrDeferredClass() override = default;
    ProjTxrDeferredClass() : AglShaderClass("ProjTxr_Deferred") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x007AD614 ProjTxrDeferredClass::CreateTechniques
void ProjTxrDeferredClass::CreateTechniques()
{
    AddShader<ProjTxrDeferredTechnique>("ProjTxr_Deferred", "ProjTxr_Deferred", kParams_F88CD4);
    AddShader<ProjTxrDeferredTechnique>("ProjTxr_Deferred", "ProjTxr_Deferred_NF", kParams_F88CD4);
}

// @func 0x005B19B4 ProjTxrDeferredClass::SelectTechnique
AglTechnique* ProjTxrDeferredClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    (void)item;
    return Pick(material, 0);
}

class FluidSimClass : public AglShaderClass {
public:
    // @func 0x007AACDC FluidSimClass::~FluidSimClass
    // @func 0x007AD8A0 FluidSimClass::~FluidSimClass (deleting)
    ~FluidSimClass() override = default;
    FluidSimClass() : AglShaderClass("FluidSim") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005B19D4 FluidSimClass::CreateTechniques
void FluidSimClass::CreateTechniques()
{
    AddShader<FluidSimTechnique>("FluidSim", "FluidSim", kParams_F88D1C);
}

// @func 0x005B1ACC FluidSimClass::SetupMaterial
bool FluidSimClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    (void)material;
    (void)desc;
    return true;
}

// @func 0x005B1AD4 FluidSimClass::SelectTechnique
AglTechnique* FluidSimClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    (void)item;
    return Pick(material, 0);
}

class ZPassClass : public AglShaderClass {
public:
    // @func 0x007AAD2C ZPassClass::~ZPassClass
    // @func 0x007AD95C ZPassClass::~ZPassClass (deleting)
    ~ZPassClass() override = default;
    ZPassClass() : AglShaderClass("ZPass") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005B1C6C ZPassClass::CreateTechniques
void ZPassClass::CreateTechniques()
{
    AddShader<ZPassTechnique>("ZPass", "ZPass", kParams_F88D3C);
    AddShader<ZPassTechnique>("ZPass_SK", "ZPass", kParams_F88D3C);
}

// @func 0x005B1E3C ZPassClass::SelectTechnique
AglTechnique* ZPassClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    item->lightMask = 0;
    return Pick(material, IsSkinned(item) ? 1 : 0);
}

class DiffuseClass : public AglShaderClass {
public:
    // @func 0x007AAD7C DiffuseClass::~DiffuseClass
    // @func 0x007ADA14 DiffuseClass::~DiffuseClass (deleting)
    ~DiffuseClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    DiffuseClass() : AglShaderClass("Diffuse") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005B2940 DiffuseClass::CreateTechniques
void DiffuseClass::CreateTechniques()
{
    AddShader<DiffuseTechnique>("Diffuse", "Diffuse", kParams_F88D5C);
    AddShader<DiffuseTechnique>("Diffuse_SK", "Diffuse", kParams_F88D5C);
    AddVariants<DiffuseTechnique>(kVariants_Diffuse, 1, 24, true, kParams_F88D5C);
}

// @func 0x005B2FD8 DiffuseClass::SelectTechnique
AglTechnique* DiffuseClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    return SelectLayeredLit(material, item, IsSkinned(item), 1, 2);
}

class DecalClass : public AglShaderClass {
public:
    // @func 0x007AADCC DecalClass::~DecalClass
    // @func 0x007AE108 DecalClass::~DecalClass (deleting)
    ~DecalClass() override = default;
    DecalClass() : AglShaderClass("Decal") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x007ADAD0 DecalClass::CreateTechniques
void DecalClass::CreateTechniques()
{
    AddShader<DecalTechnique>("Decal", "Decal", kParams_F88EB4);
    AddShader<DecalTechnique>("Decal_SK", "Decal", kParams_F88EB4);
    AddVariants<DecalTechnique>(kVariants_Decal, 1, 8, true, kParams_F88EB4);
}

// @func 0x007ADF80 DecalClass::SetupMaterial
bool DecalClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    AglTexture* texture = FindTexture(desc->textureHash);
    if (!texture)
        return false;
    texture->SetWrap(1, 1, 1);
    material->AddTexture(texture);
    material->field_78 = desc->wrapMode == 3 ? 3 : 2;
    *material->GetConstant(0) = desc->params[0];
    *material->GetConstant(1) = desc->params[1];
    return true;
}

// @func 0x007AE08C DecalClass::SelectTechnique
AglTechnique* DecalClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    return SelectSingleLit(material, item, IsSkinned(item));
}

class EnvMapClass : public AglShaderClass {
public:
    // @func 0x007AAE1C EnvMapClass::~EnvMapClass
    // @func 0x007AE1C4 EnvMapClass::~EnvMapClass (deleting)
    ~EnvMapClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    EnvMapClass() : AglShaderClass("EnvMap") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005B4004 EnvMapClass::CreateTechniques
void EnvMapClass::CreateTechniques()
{
    AddShader<EnvMapTechnique>("EnvMapS", "EnvMapS", kParams_F88F4C);
    AddShader<EnvMapTechnique>("EnvMapS_SK", "EnvMapS", kParams_F88F4C);
    AddVariants<EnvMapTechnique>(kVariants_EnvMap, 1, 24, true, kParams_F88F4C);
}

// @func 0x005B46F8 EnvMapClass::SelectTechnique
AglTechnique* EnvMapClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    return SelectLayeredLit(material, item, IsSkinned(item), 2, 3);
}

class SelfLightClass : public AglShaderClass {
public:
    // @func 0x007AAE6C SelfLightClass::~SelfLightClass
    // @func 0x007AE280 SelfLightClass::~SelfLightClass (deleting)
    ~SelfLightClass() override = default;
    SelfLightClass() : AglShaderClass("SelfLight") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005B4EC8 SelfLightClass::CreateTechniques
void SelfLightClass::CreateTechniques()
{
    AddShader<SelfLightTechnique>("SelfLight_Lit", "SelfLight_Lit", kParams_F890BC);
}

// @func 0x005B4FC4 SelfLightClass::SetupMaterial
bool SelfLightClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    for (uint32_t i = 0; i < 4; ++i)
        *material->GetConstant(i) = desc->params[i];
    return true;
}

// @func 0x005B5084 SelfLightClass::SelectTechnique
AglTechnique* SelfLightClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    (void)item;
    return Pick(material, 0);
}

class AngleIndexClass : public AglShaderClass {
public:
    // @func 0x007AAEBC AngleIndexClass::~AngleIndexClass
    // @func 0x007AE33C AngleIndexClass::~AngleIndexClass (deleting)
    ~AngleIndexClass() override = default;
    AngleIndexClass() : AglShaderClass("AngleIndex") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005B56B0 AngleIndexClass::CreateTechniques
void AngleIndexClass::CreateTechniques()
{
    AddShader<AngleIndexTechnique>("AngleIndex", "AngleIndex", kParams_F890F4);
    AddShader<AngleIndexTechnique>("AngleIndex_SK", "AngleIndex", kParams_F890F4);
}

// @func 0x005B5880 AngleIndexClass::SetupMaterial
bool AngleIndexClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    AglTexture* texture = FindTexture(desc->diffuseHash);
    if (texture) {
        texture->SetWrap(3, 3, 1);
        material->AddTexture(texture);
    }
    return texture != nullptr;
}

// @func 0x005B5914 AngleIndexClass::SelectTechnique
AglTechnique* AngleIndexClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    item->lightMask = 0;
    return Pick(material, IsSkinned(item) ? 1 : 0);
}

class WaterClass : public AglShaderClass {
public:
    // @func 0x007AAF0C WaterClass::~WaterClass
    // @func 0x007AE3F4 WaterClass::~WaterClass (deleting)
    ~WaterClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    WaterClass() : AglShaderClass("Water") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005B6424 WaterClass::CreateTechniques
void WaterClass::CreateTechniques()
{
    AddShader<WaterTechnique>("Water", nullptr, kParams_F89114);
}

// @func 0x005B6760 WaterClass::SelectTechnique
AglTechnique* WaterClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    (void)item;
    return Pick(material, 0);
}

class SacSkinnedClass : public AglShaderClass {
public:
    // @func 0x007AAF5C SacSkinnedClass::~SacSkinnedClass
    // @func 0x007AE4B0 SacSkinnedClass::~SacSkinnedClass (deleting)
    ~SacSkinnedClass() override = default;
    SacSkinnedClass() : AglShaderClass("SAC_Skinned") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005B6F54 SacSkinnedClass::CreateTechniques
void SacSkinnedClass::CreateTechniques()
{
    AddShader<SacSkinnedTechnique>("SAC_Skinned", "SAC_Skinned", kParams_F8919C);
    AddShader<SacSkinnedTechnique>("SAC_Skinned_EnvMap", "SAC_Skinned_EnvMap", kParams_F8919C);
}

// @func 0x005B7118 SacSkinnedClass::SetupMaterial
bool SacSkinnedClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    AglTexture* texture = FindTexture(desc->diffuseHash);
    if (texture) {
        texture->SetWrap(1, 3, 1);
        material->AddTexture(texture);
        material->field_78 = desc->wrapMode == 2 ? 2 : 3;
    }
    return true;
}

// @func 0x005B71D4 SacSkinnedClass::SelectTechnique
AglTechnique* SacSkinnedClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    // Technique 1 adds the environment map layer.
    item->lightMask = 1;
    return Pick(material, material->textureCount > 1 ? 1 : 0);
}

class SacEnvironmentClass : public AglShaderClass {
public:
    // @func 0x007AAFAC SacEnvironmentClass::~SacEnvironmentClass
    // @func 0x007AE56C SacEnvironmentClass::~SacEnvironmentClass (deleting)
    ~SacEnvironmentClass() override = default;
    SacEnvironmentClass() : AglShaderClass("SAC_Environment") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005B7780 SacEnvironmentClass::CreateTechniques
void SacEnvironmentClass::CreateTechniques()
{
    AddShader<SacEnvironmentTechnique>("SAC_Environment", "SAC_Environment", kParams_F891AC);
}

// @func 0x005B7878 SacEnvironmentClass::SetupMaterial
bool SacEnvironmentClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    (void)material;
    (void)desc;
    return true;
}

// @func 0x005B7880 SacEnvironmentClass::SelectTechnique
AglTechnique* SacEnvironmentClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    item->lightMask = 0;
    return Pick(material, 0);
}

class SacPropClass : public AglShaderClass {
public:
    // @func 0x007AAFFC SacPropClass::~SacPropClass
    // @func 0x007AE628 SacPropClass::~SacPropClass (deleting)
    ~SacPropClass() override = default;
    SacPropClass() : AglShaderClass("SAC_Prop") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005B7DEC SacPropClass::CreateTechniques
void SacPropClass::CreateTechniques()
{
    AddShader<SacPropTechnique>("SAC_Prop", "SAC_Prop", kParams_F891BC);
}

// @func 0x005B7EE4 SacPropClass::SetupMaterial
bool SacPropClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    (void)material;
    (void)desc;
    return true;
}

// @func 0x005B7EEC SacPropClass::SelectTechnique
AglTechnique* SacPropClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    item->lightMask = 0;
    return Pick(material, 0);
}

class SacLitPropClass : public AglShaderClass {
public:
    // @func 0x007AB04C SacLitPropClass::~SacLitPropClass
    // @func 0x007AE6E4 SacLitPropClass::~SacLitPropClass (deleting)
    ~SacLitPropClass() override = default;
    SacLitPropClass() : AglShaderClass("SAC_LitProp") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005B8518 SacLitPropClass::CreateTechniques
void SacLitPropClass::CreateTechniques()
{
    AddShader<SacLitPropTechnique>("SAC_LitProp", "SAC_LitProp", kParams_F891CC);
}

// @func 0x005B8610 SacLitPropClass::SetupMaterial
bool SacLitPropClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    (void)material;
    (void)desc;
    return true;
}

// @func 0x005B8618 SacLitPropClass::SelectTechnique
AglTechnique* SacLitPropClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    item->lightMask = 0;
    return Pick(material, 0);
}

class SacFastPropClass : public AglShaderClass {
public:
    // @func 0x007AB09C SacFastPropClass::~SacFastPropClass
    // @func 0x007AE7A0 SacFastPropClass::~SacFastPropClass (deleting)
    ~SacFastPropClass() override = default;
    SacFastPropClass() : AglShaderClass("SAC_FastProp") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005B8C60 SacFastPropClass::CreateTechniques
void SacFastPropClass::CreateTechniques()
{
    AddShader<SacFastPropTechnique>("SAC_FastProp", "SAC_FastProp", kParams_F891DC);
}

// @func 0x005B8D58 SacFastPropClass::SetupMaterial
bool SacFastPropClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (MaterialParamsEnabled()) {
        // The parameter is copied as raw bits; all ones when absent.
        uint32_t bits = 0xFFFFFFFFu;
        if (desc->paramCount != 0)
            std::memcpy(&bits, &desc->params[0], sizeof(bits));
        std::memcpy(material->GetConstant(0), &bits, sizeof(bits));
    }
    return true;
}

// @func 0x005B8DC8 SacFastPropClass::SelectTechnique
AglTechnique* SacFastPropClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    item->lightMask = 0;
    return Pick(material, 0);
}

class SacEnvMapClass : public AglShaderClass {
public:
    // @func 0x007AB0EC SacEnvMapClass::~SacEnvMapClass
    // @func 0x007AE85C SacEnvMapClass::~SacEnvMapClass (deleting)
    ~SacEnvMapClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SacEnvMapClass() : AglShaderClass("SAC_EnvMap") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005B9924 SacEnvMapClass::CreateTechniques
void SacEnvMapClass::CreateTechniques()
{
    AddShader<SacEnvMapTechnique>("SAC_EnvMap", "SAC_EnvMap", kParams_F891EC);
}

// @func 0x005B9C60 SacEnvMapClass::SelectTechnique
AglTechnique* SacEnvMapClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    item->lightMask = 0;
    return Pick(material, 0);
}

class SacPanoramaClass : public AglShaderClass {
public:
    // @func 0x007AB13C SacPanoramaClass::~SacPanoramaClass
    // @func 0x007AE938 SacPanoramaClass::~SacPanoramaClass (deleting)
    ~SacPanoramaClass() override = default;
    SacPanoramaClass() : AglShaderClass("SAC_Panorama") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x007AE8D0 SacPanoramaClass::CreateTechniques
void SacPanoramaClass::CreateTechniques()
{
    // Retired class. The console printed a one-time warning here (the text
    // was compiled out); only the "warned" latch remains.
    static bool s_warned = false;
    if (!s_warned)
        s_warned = true;
}

// @func 0x007AE8F0 SacPanoramaClass::SetupMaterial
bool SacPanoramaClass::SetupMaterial(AglMaterial* /*material*/, const AglMaterialDesc* /*desc*/)
{
    // Retired: warns once (text compiled out) and rejects the material.
    static bool s_warned = false;
    if (!s_warned)
        s_warned = true;
    return false;
}

// @func 0x007AE914 SacPanoramaClass::SelectTechnique
AglTechnique* SacPanoramaClass::SelectTechnique(AglMaterial* /*material*/, AglRenderItem* /*item*/)
{
    static bool s_warned = false;
    if (!s_warned)
        s_warned = true;
    return nullptr;
}

class SacParticleClass : public AglShaderClass {
public:
    // @func 0x007AB18C SacParticleClass::~SacParticleClass
    // @func 0x007AE9CC SacParticleClass::~SacParticleClass (deleting)
    ~SacParticleClass() override = default;
    SacParticleClass() : AglShaderClass("SAC_Particle") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x007AE9AC SacParticleClass::CreateTechniques
void SacParticleClass::CreateTechniques()
{
    // Retired class: its materials are redirected in SetupMaterial.
}

// @func 0x007AE9B0 SacParticleClass::SetupMaterial
bool SacParticleClass::SetupMaterial(AglMaterial* /*material*/, const AglMaterialDesc* desc)
{
    // Drawn by the class with name hash 0xD08FCDAD instead (the same target
    // as the name aliases in AglShaderClass.cpp).
    desc->replacementClass = 0xD08FCDADu;
    return false;
}

// @func 0x007AE9C4 SacParticleClass::SelectTechnique
AglTechnique* SacParticleClass::SelectTechnique(AglMaterial* /*material*/, AglRenderItem* /*item*/)
{
    return nullptr;
}

class Sly2UVClass : public AglShaderClass {
public:
    // @func 0x007AB1DC Sly2UVClass::~Sly2UVClass
    // @func 0x007AEA88 Sly2UVClass::~Sly2UVClass (deleting)
    ~Sly2UVClass() override = default;
    Sly2UVClass() : AglShaderClass("Sly2UV") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005BA1DC Sly2UVClass::CreateTechniques
void Sly2UVClass::CreateTechniques()
{
    AddShader<Sly2UVTechnique>("Sly2UV", "Sly2UV", kParams_F8923C);
    AddShader<Sly2UVTechnique>("Sly2UV_UVX", "Sly2UV", kParams_F8923C);
    AddVariants<Sly2UVTechnique>(kVariants_Sly2UV, 1, 5, true, kParams_F8923C);
}

// @func 0x005BA68C Sly2UVClass::SetupMaterial
bool Sly2UVClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    AglTexture* texture = FindTexture(desc->textureHash);
    if (!texture)
        return false;
    texture->SetWrap(1, 1, 1);
    material->AddTexture(texture);
    material->field_78 = desc->wrapMode == 2 ? 2 : 3;
    return true;
}

// @func 0x005BA750 Sly2UVClass::SelectTechnique
AglTechnique* Sly2UVClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    return SelectSingleLit(material, item, material->anim != nullptr);
}

class SlyCausticsClass : public AglShaderClass {
public:
    // @func 0x007AB22C SlyCausticsClass::~SlyCausticsClass
    // @func 0x007AEB44 SlyCausticsClass::~SlyCausticsClass (deleting)
    ~SlyCausticsClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyCausticsClass() : AglShaderClass("SlyCaustics") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005BAD8C SlyCausticsClass::CreateTechniques
void SlyCausticsClass::CreateTechniques()
{
    AddShader<SlyCausticsTechnique>("SlyCaustics", "SlyCaustics", kParams_F892A0);
    AddShader<SlyCausticsTechnique>("SlyCaustics_UVX", "SlyCaustics", kParams_F892A0);
    AddVariants<SlyCausticsTechnique>(kVariants_SlyCaustics, 1, 8, true, kParams_F892A0);
}

// @func 0x005BB388 SlyCausticsClass::SelectTechnique
AglTechnique* SlyCausticsClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    return SelectSingleLit(material, item, material->anim != nullptr);
}

class SlyCharacterClass : public AglShaderClass {
public:
    // @func 0x007AB27C SlyCharacterClass::~SlyCharacterClass
    // @func 0x007AEBFC SlyCharacterClass::~SlyCharacterClass (deleting)
    ~SlyCharacterClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyCharacterClass() : AglShaderClass("SlyCharacter") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005BC030 SlyCharacterClass::CreateTechniques
void SlyCharacterClass::CreateTechniques()
{
    AddShader<SlyCharacterTechnique>("SlyCharacter", "SlyCharacter", kParams_F89338);
    AddShader<SlyCharacterTechnique>("SlyCharacter_SK", "SlyCharacter", kParams_F89338);
    AddVariants<SlyCharacterTechnique>(kVariants_SlyCharacter, 1, 24, true, kParams_F89338);
}

// @func 0x005BC800 SlyCharacterClass::SelectTechnique
AglTechnique* SlyCharacterClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    const bool alt = IsCharacterSkinned(item);
    item->lightMask = 0;
    if (!IsLit(item))
        return Pick(material, alt);
    const uint32_t pair = LightPair(item, alt);
    if (material->textureCount < 3) {
        item->lightMask = 1;
        return Pick(material, pair + 0x02);
    }
    if (material->textureCount < 4) {
        item->lightMask = 3;
        return Pick(material, pair + 0x10);
    }
    item->lightMask = 7;
    return Pick(material, pair + 0x20);
}

class SlyCharacter2Class : public AglShaderClass {
public:
    // @func 0x007AB2CC SlyCharacter2Class::~SlyCharacter2Class
    // @func 0x007AECB4 SlyCharacter2Class::~SlyCharacter2Class (deleting)
    ~SlyCharacter2Class() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyCharacter2Class() : AglShaderClass("SlyCharacter2") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005BD7A0 SlyCharacter2Class::CreateTechniques
void SlyCharacter2Class::CreateTechniques()
{
    AddShader<SlyCharacter2Technique>("SlyCharacter2", "SlyCharacter2", kParams_F894D0);
    AddShader<SlyCharacter2Technique>("SlyCharacter2_SK", "SlyCharacter2", kParams_F894D0);
    AddVariants<SlyCharacter2Technique>(kVariants_SlyCharacter2, 1, 17, true, kParams_F894D0);
}

// @func 0x005BDC50 SlyCharacter2Class::SelectTechnique
AglTechnique* SlyCharacter2Class::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    const bool alt = IsCharacterSkinned(item);
    item->lightMask = 0;
    if (!IsLit(item))
        return Pick(material, alt);
    const uint32_t pair = LightPair(item, alt);
    if (material->textureCount < 3) {
        item->lightMask = 3;
        return Pick(material, pair + 0x0A);
    }
    if (material->textureCount < 4) {
        item->lightMask = 7;
        return Pick(material, pair + 0x12);
    }
    item->lightMask = 0xF;
    return Pick(material, pair + 0x1A);
}

class SlyCharacterEyeClass : public AglShaderClass {
public:
    // @func 0x007AB31C SlyCharacterEyeClass::~SlyCharacterEyeClass
    // @func 0x007AED6C SlyCharacterEyeClass::~SlyCharacterEyeClass (deleting)
    ~SlyCharacterEyeClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyCharacterEyeClass() : AglShaderClass("SlyCharacterEye") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005BF148 SlyCharacterEyeClass::CreateTechniques
void SlyCharacterEyeClass::CreateTechniques()
{
    AddShader<SlyCharacterEyeTechnique>("SlyCharacterEye", "SlyCharacterEye", kParams_F8961C);
    AddShader<SlyCharacterEyeTechnique>("SlyCharacterEye_SK", "SlyCharacterEye", kParams_F8961C);
    AddVariants<SlyCharacterEyeTechnique>(kVariants_SlyCharacterEye, 1, 6, true, kParams_F8961C);
}

// @func 0x005BFA7C SlyCharacterEyeClass::SelectTechnique
AglTechnique* SlyCharacterEyeClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    const bool alt = IsCharacterSkinned(item);
    item->lightMask = 0;
    if (IsLit(item) && material->textureCount < 4) {
        item->lightMask = 1;
        return Pick(material, LightPair(item, alt) + 2);
    }
    return Pick(material, alt);
}

class SlyCharacterFurClass : public AglShaderClass {
public:
    // @func 0x007AB36C SlyCharacterFurClass::~SlyCharacterFurClass
    // @func 0x007AEE24 SlyCharacterFurClass::~SlyCharacterFurClass (deleting)
    ~SlyCharacterFurClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyCharacterFurClass() : AglShaderClass("SlyCharacterFur") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005C0C28 SlyCharacterFurClass::CreateTechniques
void SlyCharacterFurClass::CreateTechniques()
{
    AddVariants<SlyCharacterFurTechnique>(kVariants_SlyCharacterFur, 0, 10, true, kParams_F89704);
}

// @func 0x005C12E4 SlyCharacterFurClass::SelectTechnique
AglTechnique* SlyCharacterFurClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    return SelectSingleLit(material, item, IsCharacterSkinned(item));
}

class SlyCharacterFurFinClass : public AglShaderClass {
public:
    // @func 0x007AB3BC SlyCharacterFurFinClass::~SlyCharacterFurFinClass
    // @func 0x007AEEDC SlyCharacterFurFinClass::~SlyCharacterFurFinClass (deleting)
    ~SlyCharacterFurFinClass() override = default;
    SlyCharacterFurFinClass() : AglShaderClass("SlyCharacterFurFin") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005C1A10 SlyCharacterFurFinClass::CreateTechniques
void SlyCharacterFurFinClass::CreateTechniques()
{
    AddVariants<SlyCharacterFurFinTechnique>(kVariants_SlyCharacterFurFin, 0, 5, true, kParams_F897C4);
}

// @func 0x005C1D1C SlyCharacterFurFinClass::SetupMaterial
bool SlyCharacterFurFinClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (MaterialParamsEnabled()) {
        *material->GetConstant(0) = desc->params[0];
        *material->GetConstant(1) = desc->params[1];
    }
    return true;
}

// @func 0x005C1DBC SlyCharacterFurFinClass::SelectTechnique
AglTechnique* SlyCharacterFurFinClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    return SelectSingleLit(material, item, IsCharacterSkinned(item));
}

class SlyCharacterFakeFurClass : public AglShaderClass {
public:
    // @func 0x007AB40C SlyCharacterFakeFurClass::~SlyCharacterFakeFurClass
    // @func 0x007AEF94 SlyCharacterFakeFurClass::~SlyCharacterFakeFurClass (deleting)
    ~SlyCharacterFakeFurClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyCharacterFakeFurClass() : AglShaderClass("SlyCharacterFakeFur") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005C22A8 SlyCharacterFakeFurClass::CreateTechniques
void SlyCharacterFakeFurClass::CreateTechniques()
{
    AddVariants<SlyCharacterFakeFurTechnique>(kVariants_SlyCharacterFakeFur, 0, 13, true, kParams_F89858);
}

// @func 0x005C26E8 SlyCharacterFakeFurClass::SelectTechnique
AglTechnique* SlyCharacterFakeFurClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    const bool alt = IsCharacterSkinned(item);
    item->lightMask = 0;
    if (!IsLit(item))
        return Pick(material, alt);
    const uint32_t pair = LightPair(item, alt);
    if (material->textureCount < 4) {
        item->lightMask = 1;
        return Pick(material, pair + 0x02);
    }
    if (material->textureCount < 5) {
        item->lightMask = 3;
        return Pick(material, pair + 0x0A);
    }
    item->lightMask = 7;
    return Pick(material, pair + 0x12);
}

class SlyCharacterCollectibleClass : public AglShaderClass {
public:
    // @func 0x007AB45C SlyCharacterCollectibleClass::~SlyCharacterCollectibleClass
    // @func 0x007AF04C SlyCharacterCollectibleClass::~SlyCharacterCollectibleClass (deleting)
    ~SlyCharacterCollectibleClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyCharacterCollectibleClass() : AglShaderClass("SlyCharacterCollectible") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005C320C SlyCharacterCollectibleClass::CreateTechniques
void SlyCharacterCollectibleClass::CreateTechniques()
{
    AddVariants<SlyCharacterCollectibleTechnique>(kVariants_SlyCharacterCollectible, 0, 24, true, kParams_F8995C);
}

// @func 0x005C380C SlyCharacterCollectibleClass::SelectTechnique
AglTechnique* SlyCharacterCollectibleClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    // The light variant is not used here: collectibles have one lit set.
    const uint32_t alt = IsCharacterSkinned(item) ? 1 : 0;
    item->lightMask = 0;
    if (!((item->material->flags & AglMaterial::kFlag_Lit) && item->lights))
        return Pick(material, alt);
    if (material->textureCount < 3) {
        item->lightMask = 1;
        return Pick(material, alt + 0x02);
    }
    if (material->textureCount < 4) {
        item->lightMask = 3;
        return Pick(material, alt + 0x10);
    }
    item->lightMask = 7;
    return Pick(material, alt + 0x20);
}

class SlyDecalClass : public AglShaderClass {
public:
    // @func 0x007AB4AC SlyDecalClass::~SlyDecalClass
    // @func 0x007AF108 SlyDecalClass::~SlyDecalClass (deleting)
    ~SlyDecalClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyDecalClass() : AglShaderClass("SlyDecal") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005C4120 SlyDecalClass::CreateTechniques
void SlyDecalClass::CreateTechniques()
{
    AddShader<SlyDecalTechnique>("SlyDecal", "SlyDecal", kParams_F89AB4);
    AddShader<SlyDecalTechnique>("SlyDecal_UVX", "SlyDecal", kParams_F89AB4);
    AddVariants<SlyDecalTechnique>(kVariants_SlyDecal, 1, 24, true, kParams_F89AB4);
}

// @func 0x005C48EC SlyDecalClass::SelectTechnique
AglTechnique* SlyDecalClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    const bool alt = material->anim != nullptr;
    item->lightMask = 0;
    if (!IsLit(item))
        return Pick(material, alt);
    const uint32_t pair = LightPair(item, alt);
    if (material->textureCount < 3) {
        item->lightMask = 1;
        return Pick(material, pair + 0x02);
    }
    if (material->textureCount < 5) {
        item->lightMask = 3;
        return Pick(material, pair + 0x10);
    }
    item->lightMask = 7;
    return Pick(material, pair + 0x20);
}

class SlyDecal4Class : public AglShaderClass {
public:
    // @func 0x007AB4FC SlyDecal4Class::~SlyDecal4Class
    // @func 0x007AF1C4 SlyDecal4Class::~SlyDecal4Class (deleting)
    ~SlyDecal4Class() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyDecal4Class() : AglShaderClass("SlyDecal4") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005C5104 SlyDecal4Class::CreateTechniques
void SlyDecal4Class::CreateTechniques()
{
    AddVariants<SlyDecal4Technique>(kVariants_SlyDecal4, 0, 6, false, kParams_F89C1C);
}

// @func 0x005C5678 SlyDecal4Class::SelectTechnique
AglTechnique* SlyDecal4Class::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    item->lightMask = 0;
    if (!IsLit(item))
        return Pick(material, 0);
    item->lightMask = 1;
    return Pick(material, uint32_t(item->lights->variant) + 1);
}

class SlyEnvMapClass : public AglShaderClass {
public:
    // @func 0x007AB54C SlyEnvMapClass::~SlyEnvMapClass
    // @func 0x007AF280 SlyEnvMapClass::~SlyEnvMapClass (deleting)
    ~SlyEnvMapClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyEnvMapClass() : AglShaderClass("SlyEnvMap") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005C6328 SlyEnvMapClass::CreateTechniques
void SlyEnvMapClass::CreateTechniques()
{
    AddShader<SlyEnvMapTechnique>("SlyEnvMap", "SlyEnvMap", kParams_F89C94);
    AddShader<SlyEnvMapTechnique>("SlyEnvMap_UVX", "SlyEnvMap", kParams_F89C94);
    AddVariants<SlyEnvMapTechnique>(kVariants_SlyEnvMap, 1, 24, true, kParams_F89C94);
}

// @func 0x005C6A6C SlyEnvMapClass::SelectTechnique
AglTechnique* SlyEnvMapClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    return SelectLayeredLit(material, item, material->anim != nullptr, 2, 3);
}

class SlyHeightFogClass : public AglShaderClass {
public:
    // @func 0x007AB59C SlyHeightFogClass::~SlyHeightFogClass
    // @func 0x007AF33C SlyHeightFogClass::~SlyHeightFogClass (deleting)
    ~SlyHeightFogClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyHeightFogClass() : AglShaderClass("SlyHeightFog") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005C7B84 SlyHeightFogClass::CreateTechniques
void SlyHeightFogClass::CreateTechniques()
{
    AddVariants<SlyHeightFogTechnique>(kVariants_SlyHeightFog, 0, 4, true, kParams_F89E2C);
}

// @func 0x005C8204 SlyHeightFogClass::SelectTechnique
AglTechnique* SlyHeightFogClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    // Variants by texture layer count (2 and 3 layers), not by lights.
    uint32_t index = material->anim ? 1 : 0;
    uint8_t mask = 0;
    if (material->textureCount > 1) {
        mask |= 1;
        index += 2;
    }
    if (material->textureCount > 2) {
        mask |= 2;
        index += 4;
    }
    item->lightMask = mask;
    return Pick(material, index);
}

class SlyMirrorClass : public AglShaderClass {
public:
    // @func 0x007AB5EC SlyMirrorClass::~SlyMirrorClass
    // @func 0x007AF3F8 SlyMirrorClass::~SlyMirrorClass (deleting)
    ~SlyMirrorClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyMirrorClass() : AglShaderClass("SlyMirror") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005C914C SlyMirrorClass::CreateTechniques
void SlyMirrorClass::CreateTechniques()
{
    AddShader<SlyMirrorTechnique>("SlyMirror", "SlyMirror", kParams_F89EDC);
    AddShader<SlyMirrorTechnique>("SlyMirror_UVX", "SlyMirror", kParams_F89EDC);
    AddVariants<SlyMirrorTechnique>(kVariants_SlyMirror, 1, 24, true, kParams_F89EDC);
}

// @func 0x005C97D8 SlyMirrorClass::SelectTechnique
AglTechnique* SlyMirrorClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    return SelectLayeredLit(material, item, material->anim != nullptr, 2, 3);
}

class SlyPanoramaClass : public AglShaderClass {
public:
    // @func 0x007AB63C SlyPanoramaClass::~SlyPanoramaClass
    // @func 0x007AF4B4 SlyPanoramaClass::~SlyPanoramaClass (deleting)
    ~SlyPanoramaClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyPanoramaClass() : AglShaderClass("SlyPanorama") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005CA368 SlyPanoramaClass::CreateTechniques
void SlyPanoramaClass::CreateTechniques()
{
    AddShader<SlyPanoramaTechnique>("SlyPanorama", "SlyPanorama", kParams_F8A054);
    AddShader<SlyPanoramaTechnique>("SlyPanorama_UVX", "SlyPanorama", kParams_F8A054);
    AddVariants<SlyPanoramaTechnique>(kVariants_SlyPanorama, 1, 24, true, kParams_F8A054);
}

// @func 0x005CA9D8 SlyPanoramaClass::SelectTechnique
AglTechnique* SlyPanoramaClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    return SelectLayeredLit(material, item, material->anim != nullptr, 1, 2);
}

class SlyProjTxrClass : public AglShaderClass {
public:
    // @func 0x007AB68C SlyProjTxrClass::~SlyProjTxrClass
    // @func 0x007AF574 SlyProjTxrClass::~SlyProjTxrClass (deleting)
    ~SlyProjTxrClass() override = default;
    SlyProjTxrClass() : AglShaderClass("SlyProjTxr") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005CAA98 SlyProjTxrClass::CreateTechniques
void SlyProjTxrClass::CreateTechniques()
{
    AddVariants<SlyProjTxrTechnique>(kVariants_SlyProjTxr, 0, 2, false, kParams_F8A1F4);
}

// @func 0x005CAC3C SlyProjTxrClass::SetupMaterial
bool SlyProjTxrClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    AglTexture* texture = FindTexture(desc->textureHash);
    if (!texture)
        return false;
    texture->SetWrap(1, 1, 1);
    material->AddTexture(texture);
    material->field_78 = desc->wrapMode == 2 ? 2 : 3;
    return true;
}

// @func 0x005CAD00 SlyProjTxrClass::SelectTechnique
AglTechnique* SlyProjTxrClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    const bool lit = (material->flags & AglMaterial::kFlag_10) != 0;
    if (lit)
        item->lightMask = 1;
    return Pick(material, lit ? 1 : 0);
}

class SlyPropClass : public AglShaderClass {
public:
    // @func 0x007AB6DC SlyPropClass::~SlyPropClass
    // @func 0x007AF630 SlyPropClass::~SlyPropClass (deleting)
    ~SlyPropClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyPropClass() : AglShaderClass("SlyProp") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005CB9D8 SlyPropClass::CreateTechniques
void SlyPropClass::CreateTechniques()
{
    AddShader<SlyPropTechnique>("SlyProp", "SlyProp", kParams_F8A234);
    AddShader<SlyPropTechnique>("SlyProp_UVX", "SlyProp", kParams_F8A234);
    AddVariants<SlyPropTechnique>(kVariants_SlyProp, 1, 24, true, kParams_F8A234);
}

// @func 0x005CC0AC SlyPropClass::SelectTechnique
AglTechnique* SlyPropClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    return SelectLayeredLit(material, item, material->anim != nullptr, 1, 2);
}

class SlyTerrainClass : public AglShaderClass {
public:
    // @func 0x007AB81C SlyTerrainClass::~SlyTerrainClass
    // @func 0x007AF6EC SlyTerrainClass::~SlyTerrainClass (deleting)
    ~SlyTerrainClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyTerrainClass() : AglShaderClass("SlyTerrain") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005CCDF0 SlyTerrainClass::CreateTechniques
void SlyTerrainClass::CreateTechniques()
{
    AddVariants<SlyTerrainTechnique>(kVariants_SlyTerrain, 0, 16, true, kParams_F8A38C);
}

// @func 0x005CD310 SlyTerrainClass::SelectTechnique
AglTechnique* SlyTerrainClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    // Far away terrain uses the single light technique; close up the
    // layered one, blending out (mask bit 3) in the fade band.
    item->lightMask = 0;
    uint32_t variant = 0;
    if (IsLit(item)) {
        const int32_t lights = item->lights->variant;
        if (material->textureCount == 1) {
            variant = uint32_t(lights) + 4;
            item->lightMask = 1;
        } else {
            const float* eye = GetViewEyePosition();
            const float dx = eye[0] - item->position[0];
            const float dy = eye[1] - item->position[1];
            const float dz = eye[2] - item->position[2];
            const float distSq = dx * dx + dy * dy + dz * dz;
            const float* c = material->constants;
            const float layerDistance = c ? c[3] : 0.0f;
            if (layerDistance * layerDistance < distSq) {
                variant = uint32_t(lights) + 4;
                item->lightMask = 1;
            } else {
                uint8_t mask;
                if (material->textureCount == 2) {
                    mask = 3;
                    variant = uint32_t(lights) + 8;
                } else {
                    mask = 7;
                    variant = uint32_t(lights) + 0xC;
                }
                item->lightMask = mask;
                const float fade = c ? c[4] : 0.0f;
                if ((layerDistance - fade) * (layerDistance - fade) < distSq)
                    item->lightMask = mask | 8;
            }
        }
    }
    return Pick(material, variant << 1 | (material->anim ? 1 : 0));
}

class SlyScreenWarpClass : public AglShaderClass {
public:
    // @func 0x007AB72C SlyScreenWarpClass::~SlyScreenWarpClass
    // @func 0x007AFB20 SlyScreenWarpClass::~SlyScreenWarpClass (deleting)
    ~SlyScreenWarpClass() override = default;
    SlyScreenWarpClass() : AglShaderClass("SlyScreenWarp") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x007AF7A4 SlyScreenWarpClass::CreateTechniques
void SlyScreenWarpClass::CreateTechniques()
{
    AddShader<SlyScreenWarpTechnique>("SlyScreenWarp", nullptr, kParams_F8A484);
    AddShader<SlyScreenWarpTechnique>("SlyScreenWarp_Lit", nullptr, kParams_F8A484);
}

// @func 0x007AF970 SlyScreenWarpClass::SetupMaterial
bool SlyScreenWarpClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    return SetupWarpMaterial(material, desc);
}

// @func 0x005CDC18 SlyScreenWarpClass::SelectTechnique
AglTechnique* SlyScreenWarpClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    (void)item;
    return Pick(material, 0);
}

class SlyShadowMapClass : public AglShaderClass {
public:
    // @func 0x007AB77C SlyShadowMapClass::~SlyShadowMapClass
    // @func 0x007AFBD8 SlyShadowMapClass::~SlyShadowMapClass (deleting)
    ~SlyShadowMapClass() override = default;
    SlyShadowMapClass() : AglShaderClass("SlyShadowMap") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
};

// @func 0x007AFB94 SlyShadowMapClass::CreateTechniques
void SlyShadowMapClass::CreateTechniques()
{
    // Retired class: its materials are redirected in SetupMaterial.
}

// @func 0x007AFB98 SlyShadowMapClass::SetupMaterial
bool SlyShadowMapClass::SetupMaterial(AglMaterial* /*material*/, const AglMaterialDesc* desc)
{
    // Retired: materials are drawn with the "Debug" class instead.
    desc->replacementClass = ape::HashNameNoCase("Debug");
    return false;
}

class SlyShrubClass : public AglShaderClass {
public:
    // @func 0x007AB7CC SlyShrubClass::~SlyShrubClass
    // @func 0x007AFC94 SlyShrubClass::~SlyShrubClass (deleting)
    ~SlyShrubClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyShrubClass() : AglShaderClass("SlyShrub") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005CE264 SlyShrubClass::CreateTechniques
void SlyShrubClass::CreateTechniques()
{
    AddShader<SlyShrubTechnique>("SlyShrub", "SlyShrub", kParams_F8A4D4);
}

// @func 0x005CE4E8 SlyShrubClass::SelectTechnique
AglTechnique* SlyShrubClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    item->lightMask = 0;
    return Pick(material, 0);
}

class SlyTerrainSnowClass : public AglShaderClass {
public:
    // @func 0x007AB86C SlyTerrainSnowClass::~SlyTerrainSnowClass
    // @func 0x007AFD50 SlyTerrainSnowClass::~SlyTerrainSnowClass (deleting)
    ~SlyTerrainSnowClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyTerrainSnowClass() : AglShaderClass("SlyTerrainSnow") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005CF28C SlyTerrainSnowClass::CreateTechniques
void SlyTerrainSnowClass::CreateTechniques()
{
    AddVariants<SlyTerrainSnowTechnique>(kVariants_SlyTerrainSnow, 0, 11, true, kParams_F8A514);
}

// @func 0x005CF968 SlyTerrainSnowClass::SelectTechnique
AglTechnique* SlyTerrainSnowClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    const float* c = material->constants;
    const bool snow = IsLit(item) || (c && FloatBitsNonZero(c[18]));
    const uint8_t snowBit = snow ? 4 : 0;
    const uint32_t alt = material->anim ? 1 : 0;
    item->lightMask = snowBit | 2;
    if (IsLit(item)) {
        item->lightMask = snowBit | 3;
        return Pick(material, LightPair(item, alt) + 0xE);
    }
    return Pick(material, alt + (snow ? 2 : 0) + 2);
}

class SlyTerrainSnowDecalClass : public AglShaderClass {
public:
    // @func 0x007AB8BC SlyTerrainSnowDecalClass::~SlyTerrainSnowDecalClass
    // @func 0x007AFE0C SlyTerrainSnowDecalClass::~SlyTerrainSnowDecalClass (deleting)
    ~SlyTerrainSnowDecalClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyTerrainSnowDecalClass() : AglShaderClass("SlyTerrainSnowDecal") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005D0974 SlyTerrainSnowDecalClass::CreateTechniques
void SlyTerrainSnowDecalClass::CreateTechniques()
{
    AddVariants<SlyTerrainSnowDecalTechnique>(kVariants_SlyTerrainSnowDecal, 0, 10, true, kParams_F8A5F8);
}

// @func 0x005D124C SlyTerrainSnowDecalClass::SelectTechnique
AglTechnique* SlyTerrainSnowDecalClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    const uint32_t alt = material->anim ? 1 : 0;
    item->lightMask = 2;
    if (IsLit(item)) {
        item->lightMask = 3;
        return Pick(material, LightPair(item, alt) + 0xC);
    }
    return Pick(material, alt + 2);
}

class SlyTextureWarpClass : public AglShaderClass {
public:
    // @func 0x007AB90C SlyTextureWarpClass::~SlyTextureWarpClass
    // @func 0x007AFEC4 SlyTextureWarpClass::~SlyTextureWarpClass (deleting)
    ~SlyTextureWarpClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    bool SetupWarp(AglMaterial* material, const AglMaterialDesc* desc);
    SlyTextureWarpClass() : AglShaderClass("SlyTextureWarp") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005D1EC4 SlyTextureWarpClass::CreateTechniques
void SlyTextureWarpClass::CreateTechniques()
{
    AddShader<SlyTextureWarpTechnique>("SlyTextureWarp", nullptr, kParams_F8A6E8);
    AddShader<SlyTextureWarpTechnique>("SlyTextureWarp_Lit", nullptr, kParams_F8A6E8);
}

// @func 0x005D2090 SlyTextureWarpClass::SelectTechnique
AglTechnique* SlyTextureWarpClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    return Pick(material, IsLit(item) ? 1 : 0);
}

class SlyWaterClass : public AglShaderClass {
public:
    // @func 0x007AB95C SlyWaterClass::~SlyWaterClass
    // @func 0x007AFF7C SlyWaterClass::~SlyWaterClass (deleting)
    ~SlyWaterClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    SlyWaterClass() : AglShaderClass("SlyWater") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005D2C04 SlyWaterClass::CreateTechniques
void SlyWaterClass::CreateTechniques()
{
    AddVariants<SlyWaterTechnique>(kVariants_SlyWater, 0, 4, true, kParams_F8A738);
}

// @func 0x005D30F8 SlyWaterClass::SelectTechnique
AglTechnique* SlyWaterClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    uint32_t index = material->anim ? 1 : 0;
    if (IsLit(item))
        index += 2;
    if (material->textureCount > 2)
        index += 4;
    item->lightMask = static_cast<uint8_t>(index);
    return Pick(material, index);
}

class SlyWater2Class : public AglShaderClass {
public:
    // @func 0x007AB9AC SlyWater2Class::~SlyWater2Class
    // NOTE: the deleting destructor is 0x007B0034.
    ~SlyWater2Class() override = default;
    // TODO(0x005D47A0 SlyWater2Class::SelectTechnique)
    // TODO(0x005D4198 SlyWater2Class::SetupMaterial)
    SlyWater2Class() : AglShaderClass("SlyWater2") {}
    void CreateTechniques() override;
};

// @func 0x005D3F88 SlyWater2Class::CreateTechniques
void SlyWater2Class::CreateTechniques()
{
    AddVariants<SlyWater2Technique>(kVariants_SlyWater2, 0, 64, false, kParams_F8A860);
}

class SlyWater3Class : public AglShaderClass {
public:
    // @func 0x007AB9FC SlyWater3Class::~SlyWater3Class
    // NOTE: the deleting destructor is 0x007B00EC.
    ~SlyWater3Class() override = default;
    // TODO(0x005D62E8 SlyWater3Class::SelectTechnique)
    // TODO(0x005D5C80 SlyWater3Class::SetupMaterial)
    SlyWater3Class() : AglShaderClass("SlyWater3") {}
    void CreateTechniques() override;
};

// @func 0x005D5A6C SlyWater3Class::CreateTechniques
void SlyWater3Class::CreateTechniques()
{
    AddVariants<SlyWater3Technique>(kVariants_SlyWater3, 0, 64, false, kParams_F8AC30);
}

class ShoDefaultClass : public AglShaderClass {
public:
    // @func 0x007ABA4C ShoDefaultClass::~ShoDefaultClass
    // NOTE: the deleting destructor is 0x007B01A8.
    ~ShoDefaultClass() override = default;
    ShoDefaultClass() : AglShaderClass("SHO_Default") {}
    void CreateTechniques() override;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005D6D50 ShoDefaultClass::CreateTechniques
void ShoDefaultClass::CreateTechniques()
{
    AddVariants<ShoDefaultTechnique>(kVariants_SHODefault, 0, 8, true, kParams_F8AF80);
}

// @func 0x005D705C ShoDefaultClass::SetupMaterial
bool ShoDefaultClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    (void)material;
    (void)desc;
    return true;
}

// @func 0x005D7064 ShoDefaultClass::SelectTechnique
AglTechnique* ShoDefaultClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    item->lightMask = 0;
    uint32_t variant = 0;
    if (IsLit(item)) {
        variant = uint32_t(item->lights->variant) + 4;
        item->lightMask = 1;
    }
    return Pick(material, variant << 1 | (material->anim ? 1 : 0));
}

class ShoTextureWarpClass : public AglShaderClass {
public:
    // @func 0x007ABA9C ShoTextureWarpClass::~ShoTextureWarpClass
    // NOTE: the deleting destructor is 0x007B0260.
    ~ShoTextureWarpClass() override = default;
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    bool SetupWarp(AglMaterial* material, const AglMaterialDesc* desc);
    ShoTextureWarpClass() : AglShaderClass("SHO_TextureWarp") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// @func 0x005D7BD0 ShoTextureWarpClass::CreateTechniques
void ShoTextureWarpClass::CreateTechniques()
{
    AddShader<ShoTextureWarpTechnique>("SHO_TextureWarp", nullptr, kParams_F8B010);
    AddShader<ShoTextureWarpTechnique>("SHO_TextureWarp_Lit", nullptr, kParams_F8B010);
}

// @func 0x005D7D9C ShoTextureWarpClass::SelectTechnique
AglTechnique* ShoTextureWarpClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    return Pick(material, IsLit(item) ? 1 : 0);
}

class ShoNightBlendClass : public AglShaderClass {
public:
    // @func 0x007ABAEC ShoNightBlendClass::~ShoNightBlendClass
    // NOTE: the deleting destructor is 0x007B031C.
    ~ShoNightBlendClass() override = default;
    // TODO(0x005D8C6C ShoNightBlendClass::SelectTechnique)
    bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc) override;
    ShoNightBlendClass() : AglShaderClass("SHO_NightBlend") {}
    void CreateTechniques() override;
};

// @func 0x005D8890 ShoNightBlendClass::CreateTechniques
void ShoNightBlendClass::CreateTechniques()
{
    AddVariants<ShoNightBlendTechnique>(kVariants_SHONightBlend, 0, 8, true, kParams_F8B078);
}

// --- class instances and lists ---------------------------------------------------

namespace {

DebugClass g_debug;
PrimTxrClass g_primTxr;
PrimNoTxrClass g_primNoTxr;
SimpleColorTxrClass g_simpleColorTxr;
SimpleColorTxrUVClass g_simpleColorTxrUV;
TextureWarpClass g_textureWarp;
ScreenWarpClass g_screenWarp;
LineOutlineClass g_lineOutline;
OutlineClass g_outline;
ParticleMaterialClass g_particleMaterial;
GeoParticleClass g_geoParticle;
SprParticleClass g_sprParticle;
ShadowClass g_shadow;
ProjTxrClass g_projTxr;
ProjTxrDeferredClass g_projTxrDeferred;
FluidSimClass g_fluidSim;
ZPassClass g_zPass;
DiffuseClass g_diffuse;
DecalClass g_decal;
EnvMapClass g_envMap;
SelfLightClass g_selfLight;
AngleIndexClass g_angleIndex;
WaterClass g_water;
SacSkinnedClass g_sacSkinned;
SacEnvironmentClass g_sacEnvironment;
SacPropClass g_sacProp;
SacLitPropClass g_sacLitProp;
SacFastPropClass g_sacFastProp;
SacEnvMapClass g_sacEnvMap;
SacPanoramaClass g_sacPanorama;
SacParticleClass g_sacParticle;
Sly2UVClass g_sly2UV;
SlyCausticsClass g_slyCaustics;
SlyCharacterClass g_slyCharacter;
SlyCharacter2Class g_slyCharacter2;
SlyCharacterEyeClass g_slyCharacterEye;
SlyCharacterFurClass g_slyCharacterFur;
SlyCharacterFurFinClass g_slyCharacterFurFin;
SlyCharacterFakeFurClass g_slyCharacterFakeFur;
SlyCharacterCollectibleClass g_slyCharacterCollectible;
SlyDecalClass g_slyDecal;
SlyDecal4Class g_slyDecal4;
SlyEnvMapClass g_slyEnvMap;
SlyHeightFogClass g_slyHeightFog;
SlyMirrorClass g_slyMirror;
SlyPanoramaClass g_slyPanorama;
SlyProjTxrClass g_slyProjTxr;
SlyPropClass g_slyProp;
SlyTerrainClass g_slyTerrain;
SlyScreenWarpClass g_slyScreenWarp;
SlyShadowMapClass g_slyShadowMap;
SlyShrubClass g_slyShrub;
SlyTerrainSnowClass g_slyTerrainSnow;
SlyTerrainSnowDecalClass g_slyTerrainSnowDecal;
SlyTextureWarpClass g_slyTextureWarp;
SlyWaterClass g_slyWater;
SlyWater2Class g_slyWater2;
SlyWater3Class g_slyWater3;
ShoDefaultClass g_shoDefault;
ShoTextureWarpClass g_shoTextureWarp;
ShoNightBlendClass g_shoNightBlend;

AglShaderClass* const kCoreClasses[] = {
    &g_debug,
    &g_primTxr,
    &g_primNoTxr,
    &g_simpleColorTxr,
    &g_simpleColorTxrUV,
    &g_zPass,
    &g_particleMaterial,
    &g_geoParticle,
    &g_sprParticle,
    &g_outline,
    &g_lineOutline,
    &g_shadow,
    &g_textureWarp,
    &g_screenWarp,
    &g_projTxr,
    &g_projTxrDeferred,
    nullptr,
};

AglShaderClass* const kBasicClasses[] = {
    &g_diffuse,
    &g_decal,
    &g_envMap,
    &g_selfLight,
    &g_water,
    &g_angleIndex,
    nullptr,
};

AglShaderClass* const kSacClasses[] = {
    &g_sacSkinned,
    &g_sacEnvironment,
    &g_sacProp,
    &g_sacLitProp,
    &g_sacFastProp,
    &g_sacEnvMap,
    &g_sacPanorama,
    &g_sacParticle,
    nullptr,
};

AglShaderClass* const kSlyClasses[] = {
    &g_sly2UV,
    &g_slyCaustics,
    &g_slyCharacter,
    &g_slyCharacter2,
    &g_slyCharacterEye,
    &g_slyCharacterFur,
    &g_slyCharacterFurFin,
    &g_slyCharacterFakeFur,
    &g_slyCharacterCollectible,
    &g_slyDecal,
    &g_slyDecal4,
    &g_slyEnvMap,
    &g_slyHeightFog,
    &g_slyMirror,
    &g_slyPanorama,
    &g_slyProjTxr,
    &g_slyProp,
    &g_slyScreenWarp,
    &g_slyShadowMap,
    &g_slyShrub,
    &g_slyTerrain,
    &g_slyTerrainSnow,
    &g_slyTerrainSnowDecal,
    &g_slyTextureWarp,
    &g_slyWater,
    &g_slyWater2,
    &g_slyWater3,
    nullptr,
};

// Classes the console defined but did not put in a start-up list.
AglShaderClass* const kUnlistedClasses[] = {
    &g_fluidSim,
    &g_shoDefault,
    &g_shoTextureWarp,
    &g_shoNightBlend,
    nullptr,
};

} // namespace

AglShaderClass* const* GetCoreShaderClasses()
{
    return kCoreClasses;
}

AglShaderClass* const* GetBasicShaderClasses()
{
    return kBasicClasses;
}

AglShaderClass* const* GetSacShaderClasses()
{
    return kSacClasses;
}

AglShaderClass* const* GetSlyShaderClasses()
{
    return kSlyClasses;
}

AglShaderClass* const* GetUnlistedShaderClasses()
{
    return kUnlistedClasses;
}

// --- SetupMaterial helpers -------------------------------------------------------

// Second render-quality bit of the material flag byte: no second detail layer.
static bool SecondDetailLayerDisabled()
{
    return (uint8_t(g_materialParamsFlag) & 0x40) != 0;
}

// Adds `texture` as the next layer, wrapping like the material's first layer.
static void AddLayerLikeFirst(AglMaterial* material, AglTexture* texture)
{
    material->AddTexture(texture);
    const AglTexture* first = material->textures[0];
    texture->SetWrap(first->sampler[3], first->sampler[4], 1);
}

// Optional detail layers shared by many classes: the detail texture with its
// tint (params 0..2 -> constants 0..2) and a second detail texture. Returns
// whether the detail texture was added.
static bool AddDetailLayers(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (!MaterialParamsEnabled() || desc->paramCount == 0)
        return false;
    AglTexture* detail = FindTexture(desc->detailHash);
    if (!detail)
        return false;
    AddLayerLikeFirst(material, detail);
    for (uint32_t i = 0; i < 3; ++i)
        *material->GetConstant(i) = desc->params[i];
    if (!SecondDetailLayerDisabled())
        if (AglTexture* detail2 = FindTexture(desc->detail2Hash))
            AddLayerLikeFirst(material, detail2);
    return true;
}

// Environment map base layer (wrap s, clamp t) and the material word 0x78
// switch from the desc wrap mode. False without the texture.
static bool AddEnvMapLayer(AglMaterial* material, const AglMaterialDesc* desc)
{
    AglTexture* envMap = FindTexture(desc->diffuseHash);
    if (!envMap)
        return false;
    envMap->SetWrap(1, 3, 1);
    material->AddTexture(envMap);
    material->field_78 = desc->wrapMode == 2 ? 2 : 3;
    return true;
}

// @func 0x005B2DF0 DiffuseClass::SetupMaterial
bool DiffuseClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (!MaterialParamsEnabled() || desc->paramCount == 0)
        return true;
    AddDetailLayers(material, desc);
    // Constant 3: integer alpha reference (1 when not exported).
    int32_t alphaRef = 1;
    if (desc->paramCount > 2)
        std::memcpy(&alphaRef, &desc->params[3], sizeof(alphaRef));
    std::memcpy(material->GetConstant(3), &alphaRef, sizeof(alphaRef));
    return true;
}

// @func 0x005B44B4 EnvMapClass::SetupMaterial
bool EnvMapClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (!AddEnvMapLayer(material, desc))
        return false;
    AddDetailLayers(material, desc);
    return true;
}

// @func 0x005B9A1C SacEnvMapClass::SetupMaterial
bool SacEnvMapClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (!AddEnvMapLayer(material, desc))
        return false;
    AddDetailLayers(material, desc);
    return true;
}

// @func 0x005C67E0 SlyEnvMapClass::SetupMaterial
bool SlyEnvMapClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (!AddEnvMapLayer(material, desc))
        return false;
    AddDetailLayers(material, desc);
    // Constant 3 switches the reflection mask on when param 3 is set.
    int32_t mask = 0;
    if (desc->paramCount >= 4)
        std::memcpy(&mask, &desc->params[3], sizeof(mask));
    *material->GetConstant(3) = mask != 0 ? 1.0f : 0.0f;
    return true;
}

// @func 0x005C95FC SlyMirrorClass::SetupMaterial
bool SlyMirrorClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    // The reflection itself is bound at draw time; layer 0 is a placeholder.
    material->AddTexture(GetDefaultTexture(5));
    AddDetailLayers(material, desc);
    return true;
}

// @func 0x005CA818 SlyPanoramaClass::SetupMaterial
bool SlyPanoramaClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    AddDetailLayers(material, desc);
    return true;
}

// @func 0x005CD100 SlyTerrainClass::SetupMaterial
bool SlyTerrainClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (!MaterialParamsEnabled() || desc->paramCount == 0)
        return true;
    AglTexture* detail = FindTexture(desc->detailHash);
    if (!detail)
        return true;
    AddLayerLikeFirst(material, detail);
    for (uint32_t i = 0; i < 3; ++i)
        *material->GetConstant(i) = desc->params[i];
    // Constants 3/4: detail fade distance (param 3, default 30) and range.
    *material->GetConstant(3) = desc->paramCount > 3 ? desc->params[3] : 30.0f;
    *material->GetConstant(4) = 5.0f;
    if (!SecondDetailLayerDisabled())
        if (AglTexture* detail2 = FindTexture(desc->detail2Hash))
            AddLayerLikeFirst(material, detail2);
    return true;
}

// Main texture repeated in both directions plus the word 0x78 switch from
// the desc wrap mode. False without the texture.
static bool AddRepeatedMainLayer(AglMaterial* material, const AglMaterialDesc* desc)
{
    AglTexture* texture = FindTexture(desc->textureHash);
    if (!texture)
        return false;
    texture->SetWrap(1, 1, 1);
    material->AddTexture(texture);
    material->field_78 = desc->wrapMode == 2 ? 2 : 3;
    return true;
}

// @func 0x005BB23C SlyCausticsClass::SetupMaterial
bool SlyCausticsClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (!AddRepeatedMainLayer(material, desc))
        return false;
    for (uint32_t i = 0; i < 4; ++i)
        *material->GetConstant(i) = desc->params[i];
    return true;
}

// @func 0x005C45D0 SlyDecalClass::SetupMaterial
bool SlyDecalClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (desc->paramCount == 0)
        return true;
    if (!AddRepeatedMainLayer(material, desc))
        return false;
    *material->GetConstant(0) = desc->params[0];
    *material->GetConstant(1) = desc->params[1];
    if (!MaterialParamsEnabled())
        return true;
    // Detail and colour layers; a missing one is replaced by default texture 4.
    AglTexture* detail = FindTexture(desc->detailHash);
    AglTexture* color = FindTexture(desc->diffuseHash);
    if (!detail && !color)
        return true;
    AddLayerLikeFirst(material, detail ? detail : GetDefaultTexture(4));
    AddLayerLikeFirst(material, color ? color : GetDefaultTexture(4));
    for (uint32_t i = 2; i < 5; ++i)
        *material->GetConstant(i) = desc->params[i];
    if (!SecondDetailLayerDisabled())
        if (AglTexture* detail2 = FindTexture(desc->detail2Hash))
            AddLayerLikeFirst(material, detail2);
    return true;
}

// @func 0x005CE368 SlyShrubClass::SetupMaterial
bool SlyShrubClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    // Constant 0 holds an integer (0..255) read from the raw bits of param 0.
    uint32_t level = 0;
    if (desc->paramCount != 0) {
        std::memcpy(&level, &desc->params[0], sizeof(level));
        if (level >= 0x100)
            level = 0xFF;
    }
    std::memcpy(material->GetConstant(0), &level, sizeof(level));
    for (uint32_t i = 1; i < 7; ++i)
        *material->GetConstant(i) = desc->paramCount > i ? desc->params[i] : 0.0f;
    return true;
}

// @func 0x005BC4F0 SlyCharacterClass::SetupMaterial
bool SlyCharacterClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (desc->paramCount == 0)
        return true;
    // Environment layer; default texture 5 when the character has none.
    if (!AddEnvMapLayer(material, desc)) {
        material->AddTexture(GetDefaultTexture(5));
        material->field_78 = 3;
    }
    // NOTE: when the detail texture is missing the console looks up the
    // projected texture and fetches its name without using it (stripped
    // debug output); nothing is done here.
    AddDetailLayers(material, desc);
    // Constant 4: rim width (param 4, default 15, never negative); 7 cleared.
    const float rim = desc->paramCount > 4 ? desc->params[4] : 15.0f;
    *material->GetConstant(4) = rim > 0.0f ? rim : 0.0f;
    *material->GetConstant(7) = 0.0f;
    return true;
}

// Same as the character class, except that the second detail layer ignores
// the quality flag and constant 7 is left alone.
// @func 0x005C3528 SlyCharacterCollectibleClass::SetupMaterial
bool SlyCharacterCollectibleClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (desc->paramCount == 0)
        return true;
    if (!AddEnvMapLayer(material, desc)) {
        material->AddTexture(GetDefaultTexture(5));
        material->field_78 = 3;
    }
    if (MaterialParamsEnabled()) {
        if (AglTexture* detail = FindTexture(desc->detailHash)) {
            AddLayerLikeFirst(material, detail);
            for (uint32_t i = 0; i < 3; ++i)
                *material->GetConstant(i) = desc->params[i];
            if (AglTexture* detail2 = FindTexture(desc->detail2Hash))
                AddLayerLikeFirst(material, detail2);
        }
    }
    const float rim = desc->paramCount > 4 ? desc->params[4] : 15.0f;
    *material->GetConstant(4) = rim > 0.0f ? rim : 0.0f;
    return true;
}

// Param bits holding a colour with red and blue swapped into the order the
// shaders expect.
static uint32_t SwapRedBlue(uint32_t color)
{
    return ((color >> 16) & 0xFFu) | ((color << 16) & 0xFF0000u) | (color & 0xFF00FF00u);
}

static bool SetupCharacter2Material(AglMaterial* material, const AglMaterialDesc* desc);

// @func 0x005BDD30 SlyCharacter2Class::SetupMaterial
bool SlyCharacter2Class::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    return SetupCharacter2Material(material, desc);
}

static bool SetupCharacter2Material(AglMaterial* material, const AglMaterialDesc* desc)
{
    const uint32_t count = desc->paramCount;
    if (count == 0)
        return true;
    AglTexture* ramp = FindTexture(desc->textureHash);
    if (!ramp)
        ramp = GetDefaultTexture(3);
    material->AddTexture(ramp);
    ramp->SetWrap(3, 3, 1);
    AddDetailLayers(material, desc);

    const float rim = count > 4 ? desc->params[4] : 15.0f;
    *material->GetConstant(4) = rim > 0.0f ? rim : 0.0f;
    const float* extra = desc->params;
    *material->GetConstant(7) = count > 7 ? extra[7] : 2.5f;
    *material->GetConstant(9) = count > 9 ? extra[9] : 2.0f;
    *material->GetConstant(10) = count > 10 ? extra[10] : 1.0f;
    *material->GetConstant(12) = count > 12 ? extra[12] : 1.0f;
    uint32_t color8 = 0xFFFFFFFFu, color11 = 0xFFFFFFFFu;
    if (count > 8)
        std::memcpy(&color8, &extra[8], sizeof(color8));
    if (count > 11)
        std::memcpy(&color11, &extra[11], sizeof(color11));
    color8 = SwapRedBlue(color8);
    color11 = SwapRedBlue(color11);
    std::memcpy(material->GetConstant(8), &color8, sizeof(color8));
    std::memcpy(material->GetConstant(11), &color11, sizeof(color11));
    return true;
}

// @func 0x005D8B9C ShoNightBlendClass::SetupMaterial
bool ShoNightBlendClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (!MaterialParamsEnabled())
        return true;
    AglTexture* night = FindTexture(desc->diffuseHash);
    if (!night)
        night = GetDefaultTexture(0);
    AddLayerLikeFirst(material, night);
    return true;
}

// Exported parameter `index`, or `fallback` when fewer were exported.
static float ParamOr(const AglMaterialDesc* desc, uint32_t index, float fallback)
{
    return desc->paramCount > index ? desc->params[index] : fallback;
}

static void AddRepeated(AglMaterial* material, AglTexture* texture)
{
    texture->SetWrap(1, 1, 1);
    material->AddTexture(texture);
}

constexpr float kDegToRad = 0.017453292f;

// Four blended decal layers, each with a scale pair and a rotation.
// @func 0x005C5300 SlyDecal4Class::SetupMaterial
bool SlyDecal4Class::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (desc->paramCount == 0)
        return true;
    if (AglTexture* base = FindTexture(desc->diffuseHash))
        AddRepeated(material, base);
    AglTexture* layer1 = FindTexture(desc->detailHash);
    if (!layer1)
        return false;
    AddRepeated(material, layer1);
    *material->GetConstant(0) = ParamOr(desc, 0, 1.0f);
    *material->GetConstant(1) = ParamOr(desc, 1, 1.0f);
    *material->GetConstant(6) = ParamOr(desc, 6, 0.0f) * kDegToRad;
    AglTexture* layer2 = FindTexture(desc->detail2Hash);
    if (!layer2)
        return false;
    AddRepeated(material, layer2);
    *material->GetConstant(2) = ParamOr(desc, 2, 1.0f);
    *material->GetConstant(3) = ParamOr(desc, 3, 1.0f);
    *material->GetConstant(7) = ParamOr(desc, 7, 0.0f) * kDegToRad;
    AglTexture* layer3 = FindTexture(desc->textureHash);
    if (!layer3)
        return false;
    AddRepeated(material, layer3);
    *material->GetConstant(4) = ParamOr(desc, 4, 1.0f);
    *material->GetConstant(5) = ParamOr(desc, 5, 1.0f);
    *material->GetConstant(8) = ParamOr(desc, 8, 0.0f) * kDegToRad;
    return true;
}

// @func 0x005C7DE8 SlyHeightFogClass::SetupMaterial
bool SlyHeightFogClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (AglTexture* base = FindTexture(desc->diffuseHash))
        AddRepeated(material, base);
    // The fog ramp is sampled clamped with default filtering; its mip count
    // goes to constant 17 (as an integer).
    if (AglTexture* ramp = FindTexture(desc->detailHash)) {
        ramp->sampler[0] = 0;
        ramp->sampler[1] = 0;
        ramp->sampler[2] = 0;
        ramp->sampler[6] |= 1;
        ramp->SetWrap(3, 3, 3);
        material->AddTexture(ramp);
        const uint32_t mips = ramp->image.MipCount();
        std::memcpy(material->GetConstant(17), &mips, sizeof(mips));
    }
    // NOTE: the console converts params 2 and 8 from degrees in place in
    // the record; the converted values are only used here.
    *material->GetConstant(0) = desc->params[0];
    *material->GetConstant(1) = desc->params[1];
    *material->GetConstant(2) = desc->params[2] * kDegToRad;
    *material->GetConstant(3) = ParamOr(desc, 3, 0.0f);
    *material->GetConstant(4) = ParamOr(desc, 4, 1.0f);
    *material->GetConstant(5) = ParamOr(desc, 5, 0.0f);
    *material->GetConstant(6) = desc->params[6];
    *material->GetConstant(7) = desc->params[7];
    *material->GetConstant(8) = desc->params[8] * kDegToRad;
    *material->GetConstant(9) = ParamOr(desc, 9, 0.0f);
    *material->GetConstant(10) = ParamOr(desc, 10, 1.0f);
    *material->GetConstant(11) = ParamOr(desc, 11, 0.0f);
    *material->GetConstant(12) = ParamOr(desc, 12, 1.0f);
    *material->GetConstant(13) = ParamOr(desc, 13, 1.0f);
    *material->GetConstant(14) = ParamOr(desc, 14, 0.5f);
    *material->GetConstant(15) = ParamOr(desc, 15, 5.0f);
    *material->GetConstant(16) = ParamOr(desc, 16, 0.2f);
    return true;
}

// @func 0x005BF608 SlyCharacterEyeClass::SetupMaterial
bool SlyCharacterEyeClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (desc->paramCount == 0)
        return true;
    material->textures[0]->SetWrap(3, 3, 1);
    AglTexture* highlight = FindTexture(desc->textureHash);
    if (!highlight)
        highlight = GetDefaultTexture(0);
    material->AddTexture(highlight);
    highlight->SetWrap(3, 3, 1);
    AglTexture* reflection = FindTexture(desc->detail2Hash);
    if (!reflection)
        reflection = GetDefaultTexture(5);
    material->AddTexture(reflection);
    reflection->SetWrap(3, 3, 1);

    *material->GetConstant(0) = ParamOr(desc, 0, 1.0f);
    // Constant 1: pupil angle in radians (param 1 in degrees, default 9.75).
    const float angle = ParamOr(desc, 1, 9.75f);
    *material->GetConstant(1) = (angle > 0.0f ? angle : 9.75f) * kDegToRad;
    // Fixed shading parameters of the eye shader (constants 2..31).
    static const float kEyeDefaults[30] = {
        1.0f, 0.0f, 0.0f, -0.1f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, -0.1f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.1f,
        2.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.5f,
    };
    for (uint32_t i = 0; i < 30; ++i)
        *material->GetConstant(2 + i) = kEyeDefaults[i];
    return true;
}

// @func 0x005CBE90 SlyPropClass::SetupMaterial
bool SlyPropClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    int32_t usesProp = 0;
    if (desc->paramCount >= 4)
        std::memcpy(&usesProp, &desc->params[3], sizeof(usesProp));
    if (usesProp == 0) {
        // Props without their switch are drawn as terrain: the record is
        // rewritten for SlyTerrain (fade distance, range) and rejected.
        const float fade = desc->paramCount > 4 ? desc->params[4] : 30.0f;
        desc->params[3] = fade;
        desc->params[4] = 5.0f;
        desc->paramCount = 4;
        desc->replacementClass = ape::HashNameNoCase("SlyTerrain");
        return false;
    }
    if (!MaterialParamsEnabled())
        return true;
    AddDetailLayers(material, desc);
    return true;
}

// Raw bits of parameter `index`, or `fallback` when fewer were exported.
static uint32_t ParamBitsOr(const AglMaterialDesc* desc, uint32_t index, uint32_t fallback)
{
    uint32_t bits = fallback;
    if (desc->paramCount > index)
        std::memcpy(&bits, &desc->params[index], sizeof(bits));
    return bits;
}

static void SetConstantBits(AglMaterial* material, uint32_t index, uint32_t bits)
{
    std::memcpy(material->GetConstant(index), &bits, sizeof(bits));
}

// @func 0x005C0F50 SlyCharacterFurClass::SetupMaterial
bool SlyCharacterFurClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (!MaterialParamsEnabled())
        return true;
    AglTexture* fur = FindTexture(desc->diffuseHash);
    if (!fur)
        fur = GetDefaultTexture(0);
    AddRepeated(material, fur);
    for (uint32_t i = 0; i < 3; ++i)
        *material->GetConstant(i) = desc->params[i];
    for (uint32_t i = 7; i <= 14; ++i)
        *material->GetConstant(i) = desc->params[i];
    const float rim = ParamOr(desc, 4, 15.0f);
    *material->GetConstant(4) = rim > 0.0f ? rim : 0.0f;
    *material->GetConstant(15) = ParamOr(desc, 15, 1.0f);
    *material->GetConstant(17) = ParamOr(desc, 17, 1.0f);
    SetConstantBits(material, 16, SwapRedBlue(ParamBitsOr(desc, 16, 0)));
    SetConstantBits(material, 18, SwapRedBlue(ParamBitsOr(desc, 18, 0)));
    // Constant 19: shell count as an integer, at most 14.
    SetConstantBits(material, 19, std::min<uint32_t>(ParamBitsOr(desc, 19, 14), 14));
    return true;
}

// @func 0x005CF5B0 SlyTerrainSnowClass::SetupMaterial
bool SlyTerrainSnowClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (!MaterialParamsEnabled() || desc->paramCount == 0)
        return true;
    AglTexture* snow = FindTexture(desc->textureHash);
    if (!snow)
        return false;
    material->AddTexture(snow);
    snow->SetWrap(1, 1, 1);
    SetConstantBits(material, 3, SwapRedBlue(ParamBitsOr(desc, 3, 0xFFFFFFFFu)));
    SetConstantBits(material, 4, SwapRedBlue(ParamBitsOr(desc, 4, 0xFF661919u)));
    *material->GetConstant(5) = ParamOr(desc, 5, 0.0001f);
    *material->GetConstant(6) = ParamOr(desc, 6, 0.0001f);
    *material->GetConstant(7) = ParamOr(desc, 7, 1.0f);
    *material->GetConstant(8) = ParamOr(desc, 8, 0.3f);
    *material->GetConstant(9) = ParamOr(desc, 9, 0.5f);
    *material->GetConstant(13) = ParamOr(desc, 13, 0.0001f);
    *material->GetConstant(14) = ParamOr(desc, 14, 0.0001f);
    // Constant 10: sparkle colour; constant 18 (integer) tells whether it has
    // any red, green or blue.
    const uint32_t sparkle = ParamBitsOr(desc, 10, 0xFFFFFFFFu);
    SetConstantBits(material, 10, SwapRedBlue(sparkle));
    SetConstantBits(material, 18, (sparkle & 0x00FFFFFFu) != 0 ? 1u : 0u);
    *material->GetConstant(11) = ParamOr(desc, 11, 30.0f);
    *material->GetConstant(12) = ParamOr(desc, 12, 15.0f);
    *material->GetConstant(15) = 0.0f;
    *material->GetConstant(16) = 0.0f;
    return true;
}

// Fake fur: its own fur texture layer and constant 13, then the regular
// second generation character set-up.
// @func 0x005C25B8 SlyCharacterFakeFurClass::SetupMaterial
bool SlyCharacterFakeFurClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (desc->paramCount != 0) {
        AglTexture* fur = FindTexture(desc->diffuseHash);
        if (fur) {
            fur->SetWrap(1, 1, 1);
            fur->sampler[0] = 1;
            fur->sampler[1] = 1;
            fur->sampler[2] = 1;
            fur->sampler[6] |= 1;
        } else {
            fur = GetDefaultTexture(0);
        }
        material->AddTexture(fur);
        *material->GetConstant(13) = ParamOr(desc, 13, 1.0f);
    }
    return SetupCharacter2Material(material, desc);
}

// @func 0x005D0CA0 SlyTerrainSnowDecalClass::SetupMaterial
bool SlyTerrainSnowDecalClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (!MaterialParamsEnabled() || desc->paramCount == 0)
        return true;
    // Snow, base and two decal layers; every one of them is required.
    AglTexture* snow = FindTexture(desc->textureHash);
    if (!snow)
        return false;
    material->AddTexture(snow);
    snow->SetWrap(1, 1, 1);
    AglTexture* base = FindTexture(desc->diffuseHash);
    if (!base)
        return false;
    AddRepeated(material, base);
    AglTexture* decal1 = FindTexture(desc->detailHash);
    if (!decal1)
        return false;
    AddRepeated(material, decal1);
    *material->GetConstant(8) = ParamOr(desc, 8, 1.0f);
    *material->GetConstant(9) = ParamOr(desc, 9, 1.0f);
    *material->GetConstant(12) = ParamOr(desc, 12, 0.0f) * kDegToRad;
    AglTexture* decal2 = FindTexture(desc->detail2Hash);
    if (!decal2)
        return false;
    AddRepeated(material, decal2);
    *material->GetConstant(10) = ParamOr(desc, 10, 1.0f);
    *material->GetConstant(11) = ParamOr(desc, 11, 1.0f);
    *material->GetConstant(13) = ParamOr(desc, 13, 0.0f) * kDegToRad;

    SetConstantBits(material, 0, SwapRedBlue(ParamBitsOr(desc, 0, 0xFFFFFFFFu)));
    SetConstantBits(material, 1, SwapRedBlue(ParamBitsOr(desc, 1, 0xFF661919u)));
    *material->GetConstant(2) = ParamOr(desc, 2, 0.0001f);
    *material->GetConstant(3) = ParamOr(desc, 3, 0.0001f);
    *material->GetConstant(4) = ParamOr(desc, 4, 1.0f);
    *material->GetConstant(5) = ParamOr(desc, 5, 0.3f);
    *material->GetConstant(6) = ParamOr(desc, 6, 0.5f);
    *material->GetConstant(16) = ParamOr(desc, 16, 0.0001f);
    *material->GetConstant(17) = ParamOr(desc, 17, 0.0001f);
    SetConstantBits(material, 7, SwapRedBlue(ParamBitsOr(desc, 7, 0xFFFFFFFFu)));
    *material->GetConstant(14) = ParamOr(desc, 14, 30.0f);
    *material->GetConstant(15) = ParamOr(desc, 15, 15.0f);
    *material->GetConstant(18) = 0.0f;
    *material->GetConstant(19) = 0.0f;
    return true;
}

// Shared body of the texture warp classes: the warped texture (repeated)
// with eight parameters and two rotations (degrees, params 8 and 9).
// NOTE: the console converts params 8/9 to radians inside the record.
static bool ApplyTextureWarpSetup(AglMaterial* material, const AglMaterialDesc* desc)
{
    AglTexture* texture = FindTexture(desc->diffuseHash);
    if (!texture)
        return false;
    AddRepeated(material, texture);
    for (uint32_t i = 0; i < 8; ++i)
        *material->GetConstant(i) = desc->params[i];
    if (desc->paramCount < 9) {
        *material->GetConstant(8) = 0.0f;
        *material->GetConstant(9) = 0.0f;
    } else {
        *material->GetConstant(8) = desc->params[8] * kDegToRad;
        *material->GetConstant(9) = desc->params[9] * kDegToRad;
    }
    return true;
}

// @func 0x005AF298 TextureWarpClass::SetupWarp
bool TextureWarpClass::SetupWarp(AglMaterial* material, const AglMaterialDesc* desc)
{
    return ApplyTextureWarpSetup(material, desc);
}

// @func 0x005AF4EC TextureWarpClass::SetupMaterial
bool TextureWarpClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    return SetupWarp(material, desc);
}

// @func 0x005D20E0 SlyTextureWarpClass::SetupWarp
bool SlyTextureWarpClass::SetupWarp(AglMaterial* material, const AglMaterialDesc* desc)
{
    return ApplyTextureWarpSetup(material, desc);
}

// @func 0x005D2334 SlyTextureWarpClass::SetupMaterial
bool SlyTextureWarpClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    return SetupWarp(material, desc);
}

// @func 0x005D7DE8 ShoTextureWarpClass::SetupWarp
bool ShoTextureWarpClass::SetupWarp(AglMaterial* material, const AglMaterialDesc* desc)
{
    return ApplyTextureWarpSetup(material, desc);
}

// @func 0x005D803C ShoTextureWarpClass::SetupMaterial
bool ShoTextureWarpClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    return SetupWarp(material, desc);
}

// Water: the texture warp set-up, then wave parameters 10..20.
// @func 0x005B6528 WaterClass::SetupMaterial
bool WaterClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    // The console runs the shared TextureWarp class's set-up (0x01196B20).
    if (!ApplyTextureWarpSetup(material, desc))
        return false;
    SetConstantBits(material, 10, ParamBitsOr(desc, 10, 1));
    for (uint32_t i = 11; i <= 20; ++i)
        *material->GetConstant(i) = ParamOr(desc, i, 0.0f);
    return true;
}

// @func 0x005D2F20 SlyWaterClass::SetupMaterial
bool SlyWaterClass::SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc)
{
    if (!ApplyTextureWarpSetup(material, desc))
        return false;
    *material->GetConstant(10) = ParamOr(desc, 10, 16.0f);
    *material->GetConstant(11) = ParamOr(desc, 11, 4.0f);
    *material->GetConstant(12) = ParamOr(desc, 12, 1.0f);
    *material->GetConstant(13) = ParamOr(desc, 13, 1.0f);
    *material->GetConstant(14) = ParamOr(desc, 14, 1.0f);
    SetConstantBits(material, 15, ParamBitsOr(desc, 15, 1));
    if (AglTexture* foam = FindTexture(desc->detailHash))
        AddRepeated(material, foam);
    return true;
}

// @func 0x005AE0B0 SimpleColorTxrUVTechnique::BeginMaterial
void SimpleColorTxrUVTechnique::BeginMaterial(AglMaterial* material)
{
    UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
}

// @func 0x005D8060 ShoNightBlendTechnique::BeginMaterial
void ShoNightBlendTechnique::BeginMaterial(AglMaterial* material)
{
    if (material->anim)
        UploadUVTransform(material->anim, 4, 1.0f, 1.0f);
    // NOTE: vertex register 6 always receives (1, 1, 1, 1) (a static that is
    // never changed on the console).
    static const float kBlend[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    Cmd().SetVertexProgramConstants(6, 4, kBlend);
}

} // namespace agl
