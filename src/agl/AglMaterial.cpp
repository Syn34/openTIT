// openTIT - Agl materials: construction, textures and the constant block.
#include "agl/AglResources.h"

#include "agl/AglShaderClass.h"
#include "agl/AglState.h"
#include "agl/AglTexture.h"

#include "ape/memory/ApeBucketAlloc.h"
#include "ape/memory/ApeTFreeList.h"

#include "ape/math/ApeMath.h"
#include "ape/sys/ApeGeometry.h"
#include "ape/sys/ApeIff.h"
#include "ape/world/ApeInstances.h"

#include <cmath>
#include <cstring>

namespace agl {

// UV animation attached to a material (0x98 bytes on the console).
// NOTE: contents not reconstructed yet.

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
        anim->CopyFrom(*source.anim);
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

// Inline on the console wherever a material is looked up by name (the
// pool's live objects are walked).
AglMaterial* FindLiveMaterial(uint32_t nameHash)
{
    AglMaterial* found = nullptr;
    ape::ApeTListAllocator<AglMaterial>::GetAllocator().ForEach([&](AglMaterial* material) {
        if (!found && material->nameHash == nameHash)
            found = material;
    });
    return found && found->refCount >= 1 ? found : nullptr;
}

namespace {

// Texture wrap mode by stored clamp mode (0x00F66EDC).
constexpr uint16_t kClampModeWrap[3] = { 1, 3, 2 };
// Culled front face by stored cull mode (0x00F66ED0); 2 draws both sides.
constexpr uint16_t kCullModeFace[3] = {
    static_cast<uint16_t>(gpu::FrontFace::CW), static_cast<uint16_t>(gpu::FrontFace::CW), 0,
};
constexpr uint32_t kDefaultClassHash = 0x03DE9E33u;   // 0x00F66FB4

// Draw order group by the stored sort layer value.
uint8_t SortGroup(uint8_t layer)
{
    switch (layer) {
    case 0x20: return 0;
    case 0x28: return 1;
    case 0x2C: return 2;
    case 0x2F: return 3;
    case 0x30: return 4;
    case 0x38: return 5;
    case 0x3C: return 6;
    case 0x2E: return 10;
    case 0x40: case 0x50: case 0x60: return 9;
    case 0x66: return 12;
    default: return 0;
    }
}

} // namespace

// @func 0x0050C3D8 AglMaterial::Setup
void AglMaterial::Setup(AglMaterialDesc* desc)
{
    nameHash = desc->nameHash;
    AglTexture* texture = FindTexture(desc->field_0);
    if (!texture)
        texture = GetDefaultTexture(kAglTex_White);
    AddTexture(texture);

    for (int i = 0; i < 4; ++i) {
        color0[i] = desc->diffuseColor[i] / 255.0f;
        color1[i] = desc->specColor[i] / 255.0f;
    }
    field_60 = desc->specPower;

    // NOTE: the console traps on blend/cull/clamp modes outside its tables;
    // they fall back to the first entry here.
    blendEquation = gpu::BlendEquation::Add;
    blendEnable = true;
    blendSrc = gpu::BlendFactor::SrcAlpha;
    switch (desc->blendMode < 5 ? desc->blendMode : 0) {
    case 4:   // subtractive
        blendEquation = gpu::BlendEquation::ReverseSubtract;
        blendDst = gpu::BlendFactor::One;
        break;
    case 3:   // additive
        blendDst = gpu::BlendFactor::One;
        break;
    case 2:   // alpha blended
        blendDst = gpu::BlendFactor::OneMinusSrcAlpha;
        break;
    case 1:   // blending on, but replacing
        blendSrc = gpu::BlendFactor::One;
        blendDst = gpu::BlendFactor::Zero;
        break;
    default:  // opaque
        blendEnable = false;
        blendSrc = gpu::BlendFactor::One;
        blendDst = gpu::BlendFactor::Zero;
        break;
    }
    cullFrontFace = kCullModeFace[desc->cullMode < 3 ? desc->cullMode : 0];
    texture->SetWrap(kClampModeWrap[desc->clampModeU < 3 ? desc->clampModeU : 0],
                     kClampModeWrap[desc->clampModeV < 3 ? desc->clampModeV : 0], 1);

    // Stored flags: 0x01 lit, 0x02 -> material flag 0x20, 0x04 -> 0x40,
    // 0x20 -> 0x10, 0x10 depth test, 0x08 depth compare (else always pass).
    const uint8_t stored = desc->flags;
    flags = static_cast<uint8_t>((flags & 0x0F) | ((stored & 0x01) ? kFlag_Lit : 0) | ((stored & 0x04) ? kFlag_40 : 0) |
                                 ((stored & 0x02) ? kFlag_20 : 0) | ((stored & 0x20) ? kFlag_10 : 0));
    depthTest = (stored & 0x10) != 0;
    field_65 = (flags & kFlag_10) ? 8 : 0;
    depthFunc = (stored & 0x08) ? gpu::CompareFunc::LEqual : gpu::CompareFunc::Always;
    field_2D = SortGroup(desc->sortLayer);

    // The named class sets the material up; a class may refuse and name a
    // replacement, and unknown classes fall back to the default one.
    AglShaderClass* cls = nullptr;
    bool accepted = false;
    do {
        const uint32_t requested = desc->replacementClass;
        cls = FindShaderClass(requested);
        if (!cls)
            cls = FindShaderClass(kDefaultClassHash);
        accepted = cls->SetupMaterial(this, desc);
        if (!accepted && requested == desc->replacementClass)
            desc->replacementClass = kDefaultClassHash;
    } while (!accepted);
    shaderClass = cls;
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

namespace {

// Value quantised to steps of 1/step (truncated towards zero).
float Quantise(float value, float step)
{
    return step == 0.0f ? value : float(int64_t(value * step)) / step;
}

// The engine's parabolic sine (x in [-pi, pi]).
float ParabolicSin(float x)
{
    const float y = x * 1.2732395f + x * -0.40528473f * std::fabs(x);
    return (y * std::fabs(y) - y) * 0.225f + y;
}

constexpr float kPi = 3.1415927f;
constexpr float kTwoPi = 6.2831855f;

} // namespace

// Channels start active, unscaled (range 1) and pivot about the centre.
AglMaterialAnim::AglMaterialAnim()
{
    for (Channel* channel : { &u, &v, &rotation }) {
        channel->range = 1.0f;
        channel->pivot[0] = channel->pivot[1] = 0.5f;
        channel->flags = (channel->flags & 0xFFF) | (uint32_t(Channel::kPhase_Active) << Channel::kPhaseShift);
    }
}

// @func 0x00508FBC AglMaterialAnim::CopyFrom
// Copies settings and state; each channel keeps the low 12 bits of its own
// flag word.
void AglMaterialAnim::CopyFrom(const AglMaterialAnim& source)
{
    frameStamp = source.frameStamp;
    wrapU = source.wrapU;
    Channel* to[3] = { &u, &v, &rotation };
    const Channel* from[3] = { &source.u, &source.v, &source.rotation };
    for (int i = 0; i < 3; ++i) {
        const uint32_t ownBits = to[i]->flags & 0xFFF;
        *to[i] = *from[i];
        to[i]->flags = (from[i]->flags & ~0xFFFu) | ownBits;
    }
}

// @func 0x0050B6D8 AglMaterialAnim::Configure
void AglMaterialAnim::Configure(const ChannelParams& params)
{
    Channel* channels[3] = { &u, &v, &rotation };
    if (params.channel < 3) {
        Channel& channel = *channels[params.channel];
        const uint32_t keep = channel.flags & 0xFFFFF;   // phase and low bits
        if (params.speed == 0.0f) {
            channel.flags = (channel.flags & ~Channel::kModeMask) | (Channel::kMode_Off << Channel::kModeShift);
        } else {
            channel.speed = params.speed;
            channel.step = params.steps;
            channel.range = params.limit;
            channel.waitMin = params.delayMin;
            channel.waitMax = params.delayMax;
            channel.activeTime = params.playTime;
            if (params.channel == 2) {
                channel.pivot[0] = params.pivot[0];
                channel.pivot[1] = params.pivot[1];
            } else {
                channel.pivot[0] = channel.pivot[1] = 0.0f;
            }
            channel.flags = (channel.flags & ~Channel::kModeMask) | (uint32_t(params.mode) << Channel::kModeShift);
            // An unstepped ping-pong follows a sine.
            if (params.mode == Channel::kMode_PingPong && params.steps == 0.0f)
                channel.flags = keep | (uint32_t(params.mode) << Channel::kModeShift) | Channel::kFlag_Sine;
        }
    }
    if (u.Mode() != Channel::kMode_Off && v.Mode() != Channel::kMode_Off && u.step > 0.0f && v.step > 0.0f &&
        v.speed < u.speed) {
        wrapU = 1;
        u.flags = (u.flags & ~Channel::kModeMask) | (Channel::kMode_Off << Channel::kModeShift);
    }
}

// @func 0x00501410 AglMaterialAnim::Channel::SetPhase
void AglMaterialAnim::Channel::SetPhase(int phase)
{
    switch (phase) {
    case kPhase_Waiting:
        timer = ape::RandomRange(waitMin, waitMax, ape::g_gameRandom);
        flags = (flags & ~kPhaseMask) | (uint32_t(kPhase_Waiting) << kPhaseShift);
        break;
    case kPhase_Active:
        timer = activeTime;
        flags = (flags & ~kPhaseMask) | (uint32_t(kPhase_Active) << kPhaseShift);
        break;
    case kPhase_Stopped:
        flags = (flags & ~kPhaseMask) | (uint32_t(kPhase_Stopped) << kPhaseShift);
        break;
    default:
        break;
    }
    sineTime = 0.0f;
}

// @func 0x00501520 AglMaterialAnim::Channel::Advance
// A waiting channel only counts its pause down. An active one moves (for
// activeTime when set, then pauses again): wrapping through [0, range),
// bouncing between 0 and range, or following a sine of amplitude range
// whose period is range / speed.
void AglMaterialAnim::Channel::Advance(float dt)
{
    if (Mode() == kMode_Off)
        return;
    const uint32_t phase = Phase();
    if (phase == kPhase_Waiting) {
        timer -= dt;
        if (timer <= 0.0f)
            SetPhase(kPhase_Active);
        return;
    }
    if (phase != kPhase_Active)
        return;
    if (activeTime > 0.0f) {
        timer -= dt;
        if (timer <= 0.0f) {
            dt += timer;   // only the part of the step before the burst ended
            SetPhase(kPhase_Waiting);
        }
    }
    const uint32_t mode = Mode();
    if (mode == kMode_Wrap) {
        value += speed * dt;
        if (value < 0.0f)
            value += range;
        if (range <= value)
            value -= range;
    } else if (mode == kMode_PingPong) {
        if (!(flags & kFlag_Sine)) {
            float velocity = speed;
            value += velocity * dt;
            if (value < 0.0f) {
                velocity = -velocity;
                value = 0.0f;
                speed = velocity;
            }
            if (range <= value) {
                speed = -velocity;
                value = range - 1e-5f;
            }
        } else {
            const float period = range / speed;
            sineTime += dt;
            if (period < sineTime)
                sineTime -= period;
            // Angle wrapped into [-pi, pi].
            const float angle = sineTime / period * kTwoPi;
            const float bias = angle < 0.0f ? -kPi : kPi;
            const float shifted = bias + angle;
            const float wrapped = shifted - float(int64_t(shifted / kTwoPi)) * kTwoPi - bias;
            value = range * ParabolicSin(wrapped);
        }
    }
}

// @func 0x00501780 AglMaterialAnim::Update
void AglMaterialAnim::Update()
{
    if (frameStamp == ape::world::g_renderFrameCounter)
        return;
    frameStamp = ape::world::g_renderFrameCounter;
    const AglAnimationClock& clock = GetAnimationClock();
    const float dt = clock.lastStep * clock.field_C;
    u.Advance(dt);
    v.Advance(dt);
    rotation.Advance(dt);
}

// @func 0x0050BF48 AglMaterialAnim::GetOffset
// With wrapU set, U steps through the fractional part of the V steps
// (flip book pages laid out in rows).
void AglMaterialAnim::GetOffset(float out[2]) const
{
    float offsetU = Quantise(u.value, u.step);
    const float offsetV = Quantise(v.value, v.step);
    if (wrapU) {
        const float scaled = v.value * v.step;
        offsetU = float(int64_t((scaled - float(int64_t(scaled))) * u.step)) / u.step;
    }
    out[0] = offsetU;
    out[1] = offsetV;
}

// @func 0x0050B980 AglMaterialAnim::GetUVMatrix
// Current texture coordinate matrix of a material animation (rows of four
// floats: u axis, v axis, translation (z = 1), w row): the rotation about
// the rotation channel's pivot, then the offset.
static const float* GetMaterialAnimMatrix(AglMaterialAnim* anim)
{
    static float rows[16];   // 0x010C5C40
    float offset[2];
    anim->GetOffset(offset);
    const float turns = Quantise(anim->rotation.value, anim->rotation.step);
    float ax = 1.0f, ay = 0.0f, bx = 0.0f, by = 1.0f;
    float tx = offset[0], ty = offset[1];
    if (turns != 0.0f) {
        ape::Matrix34 r;
        ape::MatrixAxisAngle(turns * kTwoPi, r, ape::Vec3{ 0.0f, 0.0f, 1.0f, 0.0f });
        ax = r.xAxis.x;
        ay = r.xAxis.y;
        bx = r.yAxis.x;
        by = r.yAxis.y;
        const float px = anim->rotation.pivot[0];
        const float py = anim->rotation.pivot[1];
        tx += px - (px * ax + py * bx);
        ty += py - (px * ay + py * by);
    }
    const float m[16] = { ax, ay, 0.0f, 0.0f,
                          bx, by, 0.0f, 0.0f,
                          tx, ty, 1.0f, 0.0f,
                          0.0f, 0.0f, 0.0f, 1.0f };
    std::memcpy(rows, m, sizeof(rows));
    return rows;
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

// @func 0x0050D1E0 AglMaterialAnim::UploadUVOffset
// Vertex constant `reg` = (U offset, V offset, 0, 0), zero without an animation.
void UploadUVOffset(const AglMaterialAnim* anim, uint32_t reg)
{
    float value[4] = {};
    if (anim)
        anim->GetOffset(value);
    Cmd().SetVertexProgramConstants(reg, 4, value);
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

// @func 0x007A0F08 AglMaterial::GetName
const char* AglMaterial::GetName() const
{
    return textures[0] ? textures[0]->GetName() : nullptr;
}

// @func 0x0051C640 MaterialChunkHandler
// A material record: a new material, announced to the create hook before
// and after its children (header, animation) are read.
int MaterialChunkHandler(ape::iff::IffReader* reader, void* /*arg*/)
{
    AglMaterial* material = AglMaterial::Create();
    RunMaterialCreateHook(material, 0);
    reader->ParseChunks(material);
    RunMaterialCreateHook(material, 1);
    return 1;
}

// @func 0x0051C704 MaterialHeaderChunkHandler
// The material settings. Versions 2-4 store the same fields (version 2 in
// another order, version 4 adds the sort layer); class parameters follow.
int MaterialHeaderChunkHandler(ape::iff::IffReader* reader, void* arg)
{
    AglMaterial* material = static_cast<AglMaterial*>(arg);
    AglMaterialDesc desc;
    uint8_t version = 4;
    reader->ReadU8(&version);
    auto readTextures = [&]() {
        reader->ReadU32(&desc.field_0);
        reader->ReadU32(&desc.diffuseHash);
        reader->ReadU32(&desc.detailHash);
        reader->ReadU32(&desc.detail2Hash);
        reader->ReadU32(&desc.textureHash);
    };
    auto readSurface = [&]() {
        reader->ReadU32(&desc.nameHash);
        reader->ReadRaw32(desc.name);
        reader->ReadU8(&desc.blendMode);
        reader->ReadU8(&desc.cullMode);
        reader->ReadU8(&desc.flags);
        reader->ReadRaw4(desc.diffuseColor);
        reader->ReadRaw4(desc.specColor);
        reader->ReadFloat(&desc.specPower);
    };
    if (version == 4 || version == 3) {
        readTextures();
        readSurface();
        reader->ReadU8(&desc.clampModeU);
        reader->ReadU8(&desc.clampModeV);
        if (version == 4)
            reader->ReadU8(&desc.sortLayer);
        reader->ReadU8(&desc.envMapType);
        reader->ReadU8(&desc.wrapMode);
        reader->ReadU32(&desc.replacementClass);
    } else if (version == 2) {
        readSurface();
        reader->ReadU32(&desc.field_0);
        reader->ReadU8(&desc.clampModeU);
        reader->ReadU8(&desc.clampModeV);
        reader->ReadU32(&desc.diffuseHash);
        reader->ReadU8(&desc.envMapType);
        reader->ReadU8(&desc.wrapMode);
        reader->ReadU32(&desc.detailHash);
        reader->ReadU32(&desc.detail2Hash);
        reader->ReadU32(&desc.textureHash);
    }
    desc.paramCount = static_cast<uint8_t>(reader->GetChunkRemaining() / 4);
    for (uint8_t i = 0; i < desc.paramCount && i < 24; ++i)
        reader->ReadU32(reinterpret_cast<uint32_t*>(&desc.params[i]));
    material->Setup(&desc);
    return 1;
}

// @func 0x0051CD48 MaterialClassChunkHandler
// One UV animation channel of the material (the animation is created with
// the first channel).
int MaterialClassChunkHandler(ape::iff::IffReader* reader, void* arg)
{
    AglMaterial* material = static_cast<AglMaterial*>(arg);
    uint8_t version = 0;
    AglMaterialAnim::ChannelParams params;
    reader->ReadU8(&version);
    reader->ReadFloat(&params.speed);
    reader->ReadFloat(&params.steps);
    reader->ReadFloat(&params.limit);
    reader->ReadFloat(&params.delayMin);
    reader->ReadFloat(&params.delayMax);
    reader->ReadFloat(&params.playTime);
    reader->ReadFloat(&params.pivot[0]);
    reader->ReadFloat(&params.pivot[1]);
    reader->ReadU8(&params.mode);
    reader->ReadU8(&params.channel);
    reader->ReadU8(&params.flags);
    if (!material->anim)
        material->anim = ape::ApeTListAllocator<AglMaterialAnim>::GetAllocator().New();
    material->anim->Configure(params);
    return 1;
}

} // namespace agl
