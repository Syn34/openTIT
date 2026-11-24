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

#include <cstring>

namespace agl {

namespace {

// --- parameter and variant tables -------------------------------------------------

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

} // namespace

// --- techniques -------------------------------------------------------------------

class ParticleMaterialTechnique : public AglTechnique {
    // TODO(0x005B0388 ParticleMaterialTechnique::BeginMaterial)
    // TODO(0x005B0440 ParticleMaterialTechnique::Draw)
};

class GeoParticleTechnique : public AglTechnique {
    // TODO(0x005B06B0 GeoParticleTechnique::BeginMaterial)
    // TODO(0x005B095C GeoParticleTechnique::Draw)
public:
    void End(AglMaterial* material) override;
};

class ProjTxrTechnique : public AglTechnique {
    // TODO(0x005B0C78 ProjTxrTechnique::BeginMaterial)
    // TODO(0x005B0FB0 ProjTxrTechnique::Draw)
};

class ProjTxrDeferredTechnique : public AglTechnique {
    // TODO(0x005B1830 ProjTxrDeferredTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
};

class FluidSimTechnique : public AglTechnique {
public:
    void BeginMaterial(AglMaterial* material) override;
    void Draw(AglRenderItem* item) override;
};

class ZPassTechnique : public AglTechnique {
    // TODO(0x005B1AE8 ZPassTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
};

class DiffuseTechnique : public AglTechnique {
    // TODO(0x005B1E88 DiffuseTechnique::BeginMaterial)
    // TODO(0x005B1FA0 DiffuseTechnique::Draw)
public:
    void End(AglMaterial* material) override;
};

class DecalTechnique : public AglTechnique {
    // TODO(0x005B30A0 DecalTechnique::BeginMaterial)
    // TODO(0x005B3148 DecalTechnique::Draw)
};

class EnvMapTechnique : public AglTechnique {
    // TODO(0x005B34C0 EnvMapTechnique::BeginMaterial)
    // TODO(0x005B3590 EnvMapTechnique::Draw)
};

class SelfLightTechnique : public AglTechnique {
    // TODO(0x005B48E8 SelfLightTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
};

class AngleIndexTechnique : public AglTechnique {
    // TODO(0x005B50A0 AngleIndexTechnique::BeginMaterial)
    // TODO(0x005B5150 AngleIndexTechnique::Draw)
};

class WaterTechnique : public AglTechnique {
    // TODO(0x005B5990 WaterTechnique::BeginMaterial)
    // TODO(0x005B5E00 WaterTechnique::Draw)
public:
    void End(AglMaterial* material) override;
};

class SacSkinnedTechnique : public AglTechnique {
    // TODO(0x005B67C8 SacSkinnedTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
};

class SacEnvironmentTechnique : public AglTechnique {
    // TODO(0x005B7260 SacEnvironmentTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
};

class SacPropTechnique : public AglTechnique {
    // TODO(0x005B78A8 SacPropTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
};

class SacLitPropTechnique : public AglTechnique {
    // TODO(0x005B7F18 SacLitPropTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
};

class SacFastPropTechnique : public AglTechnique {
    // TODO(0x005B8640 SacFastPropTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
};

class SacEnvMapTechnique : public AglTechnique {
    // TODO(0x005B8DF0 SacEnvMapTechnique::BeginMaterial)
    // TODO(0x005B8EC0 SacEnvMapTechnique::Draw)
};

class Sly2UVTechnique : public AglTechnique {
    // TODO(0x005B9C80 Sly2UVTechnique::BeginMaterial)
    // TODO(0x005B9D0C Sly2UVTechnique::Draw)
};

class SlyCausticsTechnique : public AglTechnique {
    // TODO(0x005BA7D0 SlyCausticsTechnique::BeginMaterial)
    // TODO(0x005BA8BC SlyCausticsTechnique::Draw)
};

class SlyCharacterTechnique : public AglTechnique {
    // TODO(0x005BB408 SlyCharacterTechnique::BeginMaterial)
    // TODO(0x005BB4D8 SlyCharacterTechnique::Draw)
public:
    void End(AglMaterial* material) override;
};

class SlyCharacter2Technique : public AglTechnique {
    // TODO(0x005BD298 SlyCharacter2Technique::BeginMaterial)
    // TODO(0x005BD3B0 SlyCharacter2Technique::Draw)
public:
    void End(AglMaterial* material) override;
};

class SlyCharacterEyeTechnique : public AglTechnique {
    // TODO(0x005BE100 SlyCharacterEyeTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    void End(AglMaterial* material) override;
};

class SlyCharacterFurTechnique : public AglTechnique {
    // TODO(0x005BFD20 SlyCharacterFurTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
    void End(AglMaterial* material) override;
};

class SlyCharacterFurFinTechnique : public AglTechnique {
    // TODO(0x005C1368 SlyCharacterFurFinTechnique::BeginMaterial)
    // TODO(0x005C13DC SlyCharacterFurFinTechnique::Draw)
public:
    void End(AglMaterial* material) override;
};

class SlyCharacterFakeFurTechnique : public AglTechnique {
    // TODO(0x005C1E40 SlyCharacterFakeFurTechnique::BeginMaterial)
    // TODO(0x005C1EB8 SlyCharacterFakeFurTechnique::Draw)
public:
    void End(AglMaterial* material) override;
};

class SlyCharacterCollectibleTechnique : public AglTechnique {
    // TODO(0x005C27B0 SlyCharacterCollectibleTechnique::BeginMaterial)
    // TODO(0x005C2880 SlyCharacterCollectibleTechnique::Draw)
public:
    void End(AglMaterial* material) override;
};

class SlyDecalTechnique : public AglTechnique {
    // TODO(0x005C38C0 SlyDecalTechnique::BeginMaterial)
    // TODO(0x005C39C8 SlyDecalTechnique::Draw)
};

class SlyDecal4Technique : public AglTechnique {
    // TODO(0x005C49C0 SlyDecal4Technique::BeginMaterial)
    // TODO(0x005C4C3C SlyDecal4Technique::Draw)
};

class SlyEnvMapTechnique : public AglTechnique {
    // TODO(0x005C56E0 SlyEnvMapTechnique::BeginMaterial)
    // TODO(0x005C57B0 SlyEnvMapTechnique::Draw)
};

class SlyHeightFogTechnique : public AglTechnique {
    // TODO(0x005C6D38 SlyHeightFogTechnique::BeginMaterial)
    // TODO(0x005C7210 SlyHeightFogTechnique::Draw)
};

class SlyMirrorTechnique : public AglTechnique {
    // TODO(0x005C8268 SlyMirrorTechnique::BeginMaterial)
    // TODO(0x005C8338 SlyMirrorTechnique::Draw)
};

class SlyPanoramaTechnique : public AglTechnique {
    // TODO(0x005C9898 SlyPanoramaTechnique::BeginMaterial)
    // TODO(0x005C9948 SlyPanoramaTechnique::Draw)
};

class SlyProjTxrTechnique : public AglTechnique {
public:
    void BeginMaterial(AglMaterial* material) override;
};

class SlyPropTechnique : public AglTechnique {
    // TODO(0x005CAD48 SlyPropTechnique::BeginMaterial)
    // TODO(0x005CAE18 SlyPropTechnique::Draw)
};

class SlyScreenWarpTechnique : public AglTechnique {
    // TODO(0x005CD480 SlyScreenWarpTechnique::BeginMaterial)
    // TODO(0x005CD6F0 SlyScreenWarpTechnique::Draw)
public:
    void End(AglMaterial* material) override;
};

class SlyShrubTechnique : public AglTechnique {
    // TODO(0x005CDC38 SlyShrubTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
};

class SlyTerrainTechnique : public AglTechnique {
    // TODO(0x005CC170 SlyTerrainTechnique::BeginMaterial)
    // TODO(0x005CC240 SlyTerrainTechnique::Draw)
};

class SlyTerrainSnowTechnique : public AglTechnique {
    // TODO(0x005CE508 SlyTerrainSnowTechnique::BeginMaterial)
    // TODO(0x005CE6A0 SlyTerrainSnowTechnique::Draw)
};

class SlyTerrainSnowDecalTechnique : public AglTechnique {
    // TODO(0x005CFA68 SlyTerrainSnowDecalTechnique::BeginMaterial)
    // TODO(0x005CFDC0 SlyTerrainSnowDecalTechnique::Draw)
};

class SlyTextureWarpTechnique : public AglTechnique {
    // TODO(0x005D1628 SlyTextureWarpTechnique::BeginMaterial)
    // TODO(0x005D17E8 SlyTextureWarpTechnique::Draw)
public:
    void End(AglMaterial* material) override;
};

class SlyWaterTechnique : public AglTechnique {
    // TODO(0x005D2360 SlyWaterTechnique::BeginMaterial)
    // TODO(0x005D2614 SlyWaterTechnique::Draw)
public:
    void End(AglMaterial* material) override;
};

class SlyWater2Technique : public AglTechnique {
    // TODO(0x005D33C8 SlyWater2Technique::BeginMaterial)
    // TODO(0x005D3750 SlyWater2Technique::Draw)
public:
    void End(AglMaterial* material) override;
};

class SlyWater3Technique : public AglTechnique {
    // TODO(0x005D4C80 SlyWater3Technique::BeginMaterial)
    // TODO(0x005D5220 SlyWater3Technique::Draw)
public:
    void End(AglMaterial* material) override;
};

class ShoDefaultTechnique : public AglTechnique {
    // TODO(0x005D65C0 ShoDefaultTechnique::Draw)
public:
    void BeginMaterial(AglMaterial* material) override;
};

class ShoTextureWarpTechnique : public AglTechnique {
    // TODO(0x005D7438 ShoTextureWarpTechnique::BeginMaterial)
    // TODO(0x005D7598 ShoTextureWarpTechnique::Draw)
public:
    void End(AglMaterial* material) override;
};

class ShoNightBlendTechnique : public AglTechnique {
    // TODO(0x005D8060 ShoNightBlendTechnique::BeginMaterial)
    // TODO(0x005D8100 ShoNightBlendTechnique::Draw)
};

// @func 0x005B0AF0 GeoParticleTechnique::End
void GeoParticleTechnique::End(AglMaterial* material)
{
    // Materials with a depth bias turned polygon offset on.
    const float bias = (material && material->constants) ? material->constants[4] : 0.0f;
    if (bias != 0.0f)
        Cmd().SetPolygonOffset(false, 0.0f, 0.0f);
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
    DebugClass() : AglShaderClass("Debug") {}
    void CreateTechniques() override;
};

// TODO(0x007ABEF4 DebugClass::CreateTechniques)
void DebugClass::CreateTechniques()
{
}

class PrimTxrClass : public AglShaderClass {
public:
    PrimTxrClass() : AglShaderClass("PrimTxr") {}
    void CreateTechniques() override;
};

// TODO(0x007AC178 PrimTxrClass::CreateTechniques)
void PrimTxrClass::CreateTechniques()
{
}

class PrimNoTxrClass : public AglShaderClass {
public:
    PrimNoTxrClass() : AglShaderClass("PrimNoTxr") {}
    void CreateTechniques() override;
};

// TODO(0x007AC2E4 PrimNoTxrClass::CreateTechniques)
void PrimNoTxrClass::CreateTechniques()
{
}

class SimpleColorTxrClass : public AglShaderClass {
public:
    SimpleColorTxrClass() : AglShaderClass("SimpleColorTxr") {}
    void CreateTechniques() override;
};

// TODO(0x007AC498 SimpleColorTxrClass::CreateTechniques)
void SimpleColorTxrClass::CreateTechniques()
{
}

class SimpleColorTxrUVClass : public AglShaderClass {
public:
    SimpleColorTxrUVClass() : AglShaderClass("SimpleColorTxrUV") {}
    void CreateTechniques() override;
};

// TODO(0x007AC64C SimpleColorTxrUVClass::CreateTechniques)
void SimpleColorTxrUVClass::CreateTechniques()
{
}

class TextureWarpClass : public AglShaderClass {
public:
    TextureWarpClass() : AglShaderClass("TextureWarp") {}
    void CreateTechniques() override;
};

// TODO(0x005AF080 TextureWarpClass::CreateTechniques)
void TextureWarpClass::CreateTechniques()
{
}

class ScreenWarpClass : public AglShaderClass {
public:
    ScreenWarpClass() : AglShaderClass("ScreenWarp") {}
    void CreateTechniques() override;
};

// TODO(0x007AC8B8 ScreenWarpClass::CreateTechniques)
void ScreenWarpClass::CreateTechniques()
{
}

class LineOutlineClass : public AglShaderClass {
public:
    LineOutlineClass() : AglShaderClass("LineOutline") {}
    void CreateTechniques() override;
};

// TODO(0x005AFAD4 LineOutlineClass::CreateTechniques)
void LineOutlineClass::CreateTechniques()
{
}

class OutlineClass : public AglShaderClass {
public:
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
    const float width = desc->hasParams ? desc->params[0] : 30.0f;
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
    ParticleMaterialClass() : AglShaderClass("ParticleMaterial") {}
    void CreateTechniques() override;
};

// TODO(0x007ACE20 ParticleMaterialClass::CreateTechniques)
void ParticleMaterialClass::CreateTechniques()
{
}

class GeoParticleClass : public AglShaderClass {
public:
    GeoParticleClass() : AglShaderClass("GeoParticle") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// TODO(0x007AD05C GeoParticleClass::CreateTechniques)
void GeoParticleClass::CreateTechniques()
{
}

// @func 0x005B068C GeoParticleClass::SelectTechnique
AglTechnique* GeoParticleClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    (void)item;
    return Pick(material, 0);
}

class SprParticleClass : public AglShaderClass {
public:
    SprParticleClass() : AglShaderClass("SprParticle") {}
    void CreateTechniques() override;
};

// TODO(0x007AD210 SprParticleClass::CreateTechniques)
void SprParticleClass::CreateTechniques()
{
}

class ShadowClass : public AglShaderClass {
public:
    ShadowClass() : AglShaderClass("Shadow") {}
    void CreateTechniques() override;
};

// TODO(0x007AD484 ShadowClass::CreateTechniques)
void ShadowClass::CreateTechniques()
{
}

class ProjTxrClass : public AglShaderClass {
public:
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
    ProjTxrDeferredClass() : AglShaderClass("ProjTxr_Deferred") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// TODO(0x007AD614 ProjTxrDeferredClass::CreateTechniques)
void ProjTxrDeferredClass::CreateTechniques()
{
}

// @func 0x005B19B4 ProjTxrDeferredClass::SelectTechnique
AglTechnique* ProjTxrDeferredClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    (void)item;
    return Pick(material, 0);
}

class FluidSimClass : public AglShaderClass {
public:
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
    // TODO(0x005B2DF0 DiffuseClass::SetupMaterial)
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
    DecalClass() : AglShaderClass("Decal") {}
    void CreateTechniques() override;
};

// TODO(0x007ADAD0 DecalClass::CreateTechniques)
void DecalClass::CreateTechniques()
{
}

class EnvMapClass : public AglShaderClass {
public:
    // TODO(0x005B44B4 EnvMapClass::SetupMaterial)
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
    // TODO(0x005B6528 WaterClass::SetupMaterial)
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
        if (desc->hasParams)
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
    // TODO(0x005B9A1C SacEnvMapClass::SetupMaterial)
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
    SacPanoramaClass() : AglShaderClass("SAC_Panorama") {}
    void CreateTechniques() override;
};

// TODO(0x007AE8D0 SacPanoramaClass::CreateTechniques)
void SacPanoramaClass::CreateTechniques()
{
}

class SacParticleClass : public AglShaderClass {
public:
    SacParticleClass() : AglShaderClass("SAC_Particle") {}
    void CreateTechniques() override;
};

// TODO(0x007AE9AC SacParticleClass::CreateTechniques)
void SacParticleClass::CreateTechniques()
{
}

class Sly2UVClass : public AglShaderClass {
public:
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
    // TODO(0x005BB23C SlyCausticsClass::SetupMaterial)
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
    // TODO(0x005BC4F0 SlyCharacterClass::SetupMaterial)
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
    // TODO(0x005BDD30 SlyCharacter2Class::SetupMaterial)
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
    // TODO(0x005BF608 SlyCharacterEyeClass::SetupMaterial)
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
    // TODO(0x005C0F50 SlyCharacterFurClass::SetupMaterial)
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
    // TODO(0x005C25B8 SlyCharacterFakeFurClass::SetupMaterial)
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
    // TODO(0x005C3528 SlyCharacterCollectibleClass::SetupMaterial)
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
    // TODO(0x005C45D0 SlyDecalClass::SetupMaterial)
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
    // TODO(0x005C5300 SlyDecal4Class::SetupMaterial)
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
    // TODO(0x005C67E0 SlyEnvMapClass::SetupMaterial)
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
    // TODO(0x005C7DE8 SlyHeightFogClass::SetupMaterial)
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
    // TODO(0x005C95FC SlyMirrorClass::SetupMaterial)
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
    // TODO(0x005CA818 SlyPanoramaClass::SetupMaterial)
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
    // TODO(0x005CBE90 SlyPropClass::SetupMaterial)
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
    // TODO(0x005CD100 SlyTerrainClass::SetupMaterial)
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
    SlyScreenWarpClass() : AglShaderClass("SlyScreenWarp") {}
    void CreateTechniques() override;
    AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item) override;
};

// TODO(0x007AF7A4 SlyScreenWarpClass::CreateTechniques)
void SlyScreenWarpClass::CreateTechniques()
{
}

// @func 0x005CDC18 SlyScreenWarpClass::SelectTechnique
AglTechnique* SlyScreenWarpClass::SelectTechnique(AglMaterial* material, AglRenderItem* item)
{
    (void)item;
    return Pick(material, 0);
}

class SlyShadowMapClass : public AglShaderClass {
public:
    SlyShadowMapClass() : AglShaderClass("SlyShadowMap") {}
    void CreateTechniques() override;
};

// TODO(0x007AFB94 SlyShadowMapClass::CreateTechniques)
void SlyShadowMapClass::CreateTechniques()
{
}

class SlyShrubClass : public AglShaderClass {
public:
    // TODO(0x005CE368 SlyShrubClass::SetupMaterial)
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
    // TODO(0x005CF5B0 SlyTerrainSnowClass::SetupMaterial)
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
    // TODO(0x005D0CA0 SlyTerrainSnowDecalClass::SetupMaterial)
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
    // TODO(0x005D2334 SlyTextureWarpClass::SetupMaterial)
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
    // TODO(0x005D2F20 SlyWaterClass::SetupMaterial)
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
    // TODO(0x005D803C ShoTextureWarpClass::SetupMaterial)
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
    // TODO(0x005D8C6C ShoNightBlendClass::SelectTechnique)
    // TODO(0x005D8B9C ShoNightBlendClass::SetupMaterial)
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

} // namespace agl
