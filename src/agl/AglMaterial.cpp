// openTIT - Agl materials: construction, textures and the constant block.
#include "agl/AglResources.h"

#include "agl/AglShaderClass.h"
#include "agl/AglState.h"
#include "agl/AglTexture.h"

#include "ape/memory/ApeBucketAlloc.h"
#include "ape/memory/ApeTFreeList.h"

#include <cstring>

namespace agl {

// UV animation attached to a material (0x98 bytes on the console).
// NOTE: contents not reconstructed yet.
struct AglMaterialAnim {
    uint8_t data[0x98] = {};
};

namespace {

uint32_t g_nextMaterialId = 0;       // shared resource id counter (0x01164CD0)
uint16_t g_nextMaterialSerial = 0;   // 0x010DD5C0
// Default material color (0x01164BB0).
const float kDefaultColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

// Drops a reference held in 'slot' and stores 'texture' (with a new reference).
void AssignTexture(AglTexture*& slot, AglTexture* texture)
{
    AglTexture* previous = slot;
    if (previous == texture)
        return;
    slot = texture;
    if (texture)
        texture->AddRef();
    if (previous)
        previous->Release();
}

} // namespace

// @func 0x004FF0D8 AglMaterial::AglMaterial
AglMaterial::AglMaterial()
{
    refCount = 1;
    resourceFlags = 0x1000;
    id = ++g_nextMaterialId;
    sortKey = g_nextMaterialSerial++;
    std::memcpy(color0, kDefaultColor, sizeof(color0));
    std::memcpy(color1, kDefaultColor, sizeof(color1));
}

// @func 0x0050C118 AglMaterial::~AglMaterial
// @func 0x0050C350 AglMaterial::~AglMaterial (deleting)
AglMaterial::~AglMaterial()
{
    if (anim) {
        ape::ApeTListAllocator<AglMaterialAnim>::GetAllocator().Delete(anim);
        anim = nullptr;
    }
    if (constants) {
        ape::BucketFree(constants, size_t(constantCapacity) * sizeof(float));
        constants = nullptr;
    }
    for (int i = 0; i < textureCount; ++i) {
        if (textures[i]) {
            AglTexture* texture = textures[i];
            textures[i] = nullptr;
            texture->Release();
        }
        if (i == 0)
            textureSortKey = 0;
    }
    for (int i = kMaxMaterialTextures - 1; i >= 0; --i) {
        if (textures[i])
            textures[i]->Release();
    }
}

// @func 0x004FF2E8 AglMaterial::Create
AglMaterial* AglMaterial::Create()
{
    return ape::ApeTListAllocator<AglMaterial>::GetAllocator().New();
}

// @func 0x004FF43C AglMaterial::CreateWithClass
AglMaterial* AglMaterial::CreateWithClass(AglShaderClass* cls)
{
    AglMaterial* material = Create();
    material->shaderClass = cls;
    return material;
}

// @func 0x004FF46C AglMaterial::CreateForClassHash
AglMaterial* AglMaterial::CreateForClassHash(uint32_t classHash)
{
    AglShaderClass* cls = FindShaderClass(classHash);
    if (!cls)
        return nullptr;
    AglMaterial* material = Create();
    material->shaderClass = cls;
    return material;
}

// @func 0x004FEF48 AglMaterial::SetTexture
void AglMaterial::SetTexture(AglTexture* texture, int index)
{
    AssignTexture(textures[index], texture);
    if (index == 0)
        textureSortKey = texture ? static_cast<uint16_t>(texture->serial) : 0;
}

// @func 0x004FF008 AglMaterial::AddTexture
void AglMaterial::AddTexture(AglTexture* texture)
{
    const int index = textureCount++;
    AssignTexture(textures[index], texture);
    if (index == 0)
        textureSortKey = texture ? static_cast<uint16_t>(texture->serial) : 0;
}

// @func 0x0050C018 AglMaterial::GetConstant
float* AglMaterial::GetConstant(uint32_t index)
{
    const uint32_t needed = index + 1;
    if (!constants) {
        const uint32_t capacity = needed < 8 ? 8 : needed;
        constantCapacity = static_cast<uint8_t>(capacity);
        constants = static_cast<float*>(ape::BucketAlloc(size_t(constantCapacity) * sizeof(float)));
    } else if (constantCapacity < needed) {
        // Grow to 16, then 32, or straight to what is needed.
        const uint32_t step = constantCapacity == 16 ? 32 : 16;
        const uint32_t capacity = needed < step ? step : needed;
        auto* grown = static_cast<float*>(ape::BucketAlloc(size_t(capacity) * sizeof(float)));
        std::memcpy(grown, constants, size_t(constantCapacity) * sizeof(float));
        ape::BucketFree(constants, size_t(constantCapacity) * sizeof(float));
        constants = grown;
        constantCapacity = static_cast<uint8_t>(capacity);
    }
    return constants + index;
}

// @func 0x00509090 AglMaterial::CopyFrom
void AglMaterial::CopyFrom(const AglMaterial& source)
{
    for (int i = 0; i < source.textureCount; ++i)
        AddTexture(source.textures[i]);
    nameHash = source.nameHash;
    field_2D = source.field_2D;
    std::memcpy(color0, source.color0, sizeof(color0));
    std::memcpy(color1, source.color1, sizeof(color1));
    field_60 = source.field_60;
    // Flag bits 3..7 are material settings; the low bits are bookkeeping.
    flags = static_cast<uint8_t>((flags & 0x07) | (source.flags & 0xF8));
    blendEnable = source.blendEnable;
    blendSrc = source.blendSrc;
    field_68 = source.field_68;
    blendEquation = source.blendEquation;
    blendDst = source.blendDst;
    depthTest = source.depthTest;
    depthFunc = source.depthFunc;
    field_70 = source.field_70;
    cullFrontFace = source.cullFrontFace;
    field_78 = source.field_78;
    if (source.constants && source.constantCapacity != 0) {
        GetConstant(source.constantCapacity - 1u);
        std::memcpy(constants, source.constants, size_t(source.constantCapacity) * sizeof(float));
    }
    if (source.anim) {
        if (!anim)
            anim = ape::ApeTListAllocator<AglMaterialAnim>::GetAllocator().New();
        *anim = *source.anim;
    }
}

// @func 0x0050956C AglMaterial::Clone
AglMaterial* AglMaterial::Clone(const AglMaterial& source)
{
    AglMaterial* material = Create();
    material->shaderClass = source.shaderClass;
    material->CopyFrom(source);
    return material;
}

// @func 0x0050061C AglMaterial::GetBlendMode
AglMaterial::BlendMode AglMaterial::GetBlendMode() const
{
    if (!blendEnable)
        return kBlend_None;
    using gpu::BlendEquation;
    using gpu::BlendFactor;
    if (blendSrc == BlendFactor::One && blendDst == BlendFactor::Zero && blendEquation == BlendEquation::Add)
        return kBlend_None;
    if (blendSrc != BlendFactor::SrcAlpha)
        return kBlend_None;
    if (blendDst == BlendFactor::OneMinusSrcAlpha && blendEquation == BlendEquation::Add)
        return kBlend_Alpha;
    if (blendDst == BlendFactor::One && blendEquation == BlendEquation::Add)
        return kBlend_Additive;
    if (blendDst == BlendFactor::One && blendEquation == BlendEquation::ReverseSubtract)
        return kBlend_Subtractive;
    return kBlend_None;
}

// --- built-in materials -------------------------------------------------------------

namespace {
AglMaterial* g_builtinMaterials[6] = {};   // 0x010F0C60
}

// @func 0x0050F0F0 GetBuiltinMaterial
AglMaterial* GetBuiltinMaterial(int index)
{
    AglMaterial*& slot = g_builtinMaterials[index];
    if (slot)
        return slot;
    // Class name hashes of the debug/primitive shader classes.
    constexpr uint32_t kTexturedClass = 0x1808E1E8;
    constexpr uint32_t kClass3 = 0x38FF7607;
    constexpr uint32_t kClass2 = 0x05199F6B;
    constexpr uint32_t kColorClass = 0x03DE9E33;
    AglTexture* texture = nullptr;
    uint32_t classHash = kColorClass;
    uint16_t cull = static_cast<uint16_t>(gpu::FrontFace::CW);
    switch (index) {
    case 5: classHash = kTexturedClass; texture = GetDefaultTexture(kAglTex_White2); break;
    case 4: classHash = kTexturedClass; texture = GetDefaultTexture(kAglTex_White); break;
    case 3: classHash = kClass3; break;
    case 2: classHash = kClass2; break;
    case 1: cull = 0; break;   // double sided
    default: break;
    }
    AglMaterial* material = AglMaterial::Create();
    material->shaderClass = FindShaderClass(classHash);
    slot = material;
    if (texture)
        material->AddTexture(texture);
    // Alpha blended.
    material->blendEquation = gpu::BlendEquation::Add;
    material->blendDst = gpu::BlendFactor::OneMinusSrcAlpha;
    material->blendEnable = true;
    material->blendSrc = gpu::BlendFactor::SrcAlpha;
    material->cullFrontFace = cull;
    return material;
}

// @func 0x00523B7C CreateSpriteMaterial
AglMaterial* CreateSpriteMaterial(AglTexture* texture)
{
    // Same shader classes as the textured / plain built-in materials.
    constexpr uint32_t kTexturedClass = 0x1808E1E8;
    constexpr uint32_t kUntexturedClass = 0x38FF7607;
    AglMaterial* material = AglMaterial::Create();
    if (!texture) {
        material->shaderClass = FindShaderClass(kUntexturedClass);
        return material;
    }
    material->shaderClass = FindShaderClass(kTexturedClass);
    material->AddTexture(texture);
    return material;
}

// --- material animation --------------------------------------------------------------

// TODO(0x0050B980 AglMaterialAnim::GetUVMatrix)
// Current 3x4 texture coordinate matrix of a material animation (rows of
// four floats: u axis, v axis, translation).
static const float* GetMaterialAnimMatrix(AglMaterialAnim* /*anim*/)
{
    static const float kIdentity[12] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
    };
    return kIdentity;
}

// @func 0x0050BE5C AglMaterialAnim::BuildUVTransform
const float* BuildUVTransform(AglMaterialAnim* anim, float scaleU, float scaleV)
{
    // Two constant rows: (u.x, u.y, t.x, t.y) and (v.x, v.y, 0, 0) (0x010C5C80).
    static float rows[8];
    const float* m = GetMaterialAnimMatrix(anim);
    rows[0] = m[0] * scaleU;
    rows[1] = m[1] * scaleU;
    rows[2] = m[8];
    rows[3] = m[9];
    rows[4] = m[4] * scaleV;
    rows[5] = m[5] * scaleV;
    rows[6] = 0.0f;
    rows[7] = 0.0f;
    return rows;
}

// @func 0x0050D124 AglMaterialAnim::UploadUVTransform
void UploadUVTransform(AglMaterialAnim* anim, uint32_t reg, float scaleU, float scaleV)
{
    if (anim) {
        Cmd().SetVertexProgramConstants(reg, 8, BuildUVTransform(anim, scaleU, scaleV));
        return;
    }
    const float rows[8] = { scaleU, 0.0f, 0.0f, 0.0f, 0.0f, scaleV, 0.0f, 0.0f };
    Cmd().SetVertexProgramConstants(reg, 8, rows);
}

// --- view --------------------------------------------------------------------------

namespace {
float g_viewEyePosition[3] = {};   // 0x010DD8B0
}

const float* GetViewEyePosition()
{
    return g_viewEyePosition;
}

void SetViewEyePosition(const float position[3])
{
    std::memcpy(g_viewEyePosition, position, sizeof(g_viewEyePosition));
}

} // namespace agl
