// openTIT - Skeletons: bone hierarchy definitions and posed skeleton instances.
#include "ape/world/ApeSkeleton.h"

#include <vector>

#include "ape/core/ApeString.h"
#include "ape/memory/ApeBucketAlloc.h"
#include "ape/memory/ApeMem.h"
#include "ape/memory/ApeTFreeList.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/sys/ApeIff.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeObj.h"

#include <cstring>
#include <new>

namespace ape::world {

ApeSkeletonQueue g_skeletonQueue;
uint8_t g_skeletonBoundsInterval = 0;

namespace {

// Round-robin start value for the skeleton bounds refresh counters, so that
// skeletons created together do not all refresh on the same frame (0x01194594).
uint8_t s_nextLodPhase = 0;

// Pools sized like the console's ApeTMemClassFL<T, size> instances.
ApeTFreeList<ApeSkeleton>& SkeletonPool()
{
    static ApeTFreeList<ApeSkeleton>* s_pool = nullptr;
    if (!s_pool) {
        s_pool = new ApeTFreeList<ApeSkeleton>();
        s_pool->SetName("ApeTMemClassFL<ApeSkeleton, 128>");
        s_pool->Init(128);
    }
    return *s_pool;
}

ApeTFreeList<ApeSkeletonDef>& SkeletonDefPool()
{
    static ApeTFreeList<ApeSkeletonDef>* s_pool = nullptr;
    if (!s_pool) {
        s_pool = new ApeTFreeList<ApeSkeletonDef>();
        s_pool->SetName("ApeTMemClassFL<ApeSkeletonDef, 64>");
        s_pool->Init(64);
    }
    return *s_pool;
}

size_t AlignUp(size_t value, size_t align)
{
    return (value + align - 1) & ~(align - 1);
}

// Copies the rotation and translation lanes of a matrix.
void CopyAffine(Matrix34& out, const Matrix34& m)
{
    Matrix34Copy(out, m);
}

// Inverse of a rigid transform (orthonormal rotation): transposed rotation
// and the translation moved back through it.
void InvertRigid(Matrix34& out, const Matrix34& m)
{
    const Vec3 x = m.xAxis, y = m.yAxis, z = m.zAxis, t = m.translation;
    out.xAxis = {x.x, y.x, z.x, 0.0f};
    out.yAxis = {x.y, y.y, z.y, 0.0f};
    out.zAxis = {x.z, y.z, z.z, 0.0f};
    out.translation = {-(t.x * x.x + t.y * x.y + t.z * x.z),
                       -(t.x * y.x + t.y * y.y + t.z * y.z),
                       -(t.x * z.x + t.y * z.y + t.z * z.z), 0.0f};
}

// p * rotation(m) + translation(m) for a row vector p.
Vec3 TransformPoint(const Vec3& p, const Matrix34& m)
{
    return {p.x * m.xAxis.x + p.y * m.yAxis.x + p.z * m.zAxis.x + m.translation.x,
            p.x * m.xAxis.y + p.y * m.yAxis.y + p.z * m.zAxis.y + m.translation.y,
            p.x * m.xAxis.z + p.y * m.yAxis.z + p.z * m.zAxis.z + m.translation.z, 0.0f};
}

// Nesting level of the bone tree dump (0x00F889F4).
int s_dumpLevel = 0;

// @func 0x0055498C PrintSkeletonBone
// NOTE: the console's debug text output was compiled out.
void PrintSkeletonBone(const ApeSkeletonDefBone* /*bone*/)
{
}

// Sets bit `level` of the tree-line mask when a later bone shares `parent`.
void MarkLaterSibling(const ApeSkeleton& skeleton, uint32_t after, int16_t parent, int level, uint32_t* lines)
{
    uint32_t& word = lines[level >> 5];
    const uint32_t bit = 1u << (level & 31);
    word &= ~bit;
    for (uint32_t i = after + 1; i < skeleton.numBones; ++i) {
        if (skeleton.bones[i].def->parent == parent) {
            word |= bit;
            return;
        }
    }
}

// @func 0x00583FBC DumpSkeletonBone
void DumpSkeletonBone(const ApeSkeleton& skeleton, uint32_t bone, int print, uint32_t* lines)
{
    ++s_dumpLevel;
    if (print)
        PrintSkeletonBone(skeleton.bones[bone].def);
    for (uint32_t child = 0; child < skeleton.numBones; ++child) {
        if (skeleton.bones[child].def->parent != int16_t(bone))
            continue;
        MarkLaterSibling(skeleton, child, int16_t(bone), s_dumpLevel, lines);
        DumpSkeletonBone(skeleton, child, print, lines);
    }
    --s_dumpLevel;
}

} // namespace

// @func 0x00584160 DumpSkeleton
void DumpSkeleton(const ApeSkeleton* skeleton, int depth)
{
    if (depth < 1 || skeleton->numBones == 0)
        return;
    std::vector<uint32_t> lines((skeleton->numBones + 31) / 32, 0u);
    for (uint32_t root = 0; root < skeleton->numBones; ++root) {
        if (skeleton->bones[root].def->parent != -1)
            continue;
        MarkLaterSibling(*skeleton, root, -1, 0, lines.data());
        DumpSkeletonBone(*skeleton, root, depth - 1, lines.data());
    }
}

// --- ApeSkeletonDef --------------------------------------------------------------

// @func 0x00584FD4 ApeSkeletonDef::Create
ApeSkeletonDef* ApeSkeletonDef::Create(uint32_t numBones)
{
    ApeSkeletonDef* def = SkeletonDefPool().AllocT();
    if (!def)
        return nullptr;
    def->refCount = 1;
    def->numBones = uint16_t(numBones);
    def->bones = static_cast<ApeSkeletonDefBone*>(MemAlloc(numBones * sizeof(ApeSkeletonDefBone)));
    if (def->bones) {
        for (uint32_t i = 0; i < numBones; ++i) {
            ApeSkeletonDefBone& bone = def->bones[i];
            Matrix34Copy(bone.inverseBind, kWorldIdentity);
            bone.nameHash = 0;
            bone.name[0] = '\0';
            bone.parent = -1;
        }
    }
    return def;
}

// @func 0x00584DFC ApeSkeletonDef::Release
void ApeSkeletonDef::Release()
{
    if (--refCount >= 1)
        return;
    if (bones) {
        MemFree(bones);
        bones = nullptr;
    }
    SkeletonDefPool().Free(this);
}

// @func 0x00584C70 ApeSkeletonDef::SetBone
void ApeSkeletonDef::SetBone(ApeSkeletonDefBone& bone, uint32_t nameHash, const float xBasis[3],
                             const float zBasis[3], const float position[3], const char* name)
{
    StrCopyBounded(bone.name, name, sizeof(bone.name));
    bone.nameHash = nameHash;
    Matrix34 bind;
    Matrix34FromBasis(bind, xBasis, zBasis, position);
    InvertRigid(bone.inverseBind, bind);
    bone.parent = -1;
}

// @func 0x00584888 ApeSkeletonDef::FindBone
int ApeSkeletonDef::FindBone(uint32_t nameHash) const
{
    for (uint32_t i = 0; i < numBones; ++i)
        if (bones[i].nameHash == nameHash)
            return int(i);
    return -1;
}

// --- ApeSkeleton ---------------------------------------------------------------------

// @func 0x0058142C ApeSkeleton::SetBuffer
void ApeSkeleton::SetBuffer(void* block, size_t size)
{
    const size_t n = numBones;
    uint8_t* base = static_cast<uint8_t*>(block);
    buffer = block;
    bufferSize = size;
    local = reinterpret_cast<Matrix34*>(base);
    model = local + n;
    skin = model + n;
    scales = reinterpret_cast<Vec3*>(skin + n);
    parents = reinterpret_cast<int16_t*>(scales + n);
    bones = reinterpret_cast<ApeSkeletonBone*>(reinterpret_cast<uint8_t*>(parents) + AlignUp(n * sizeof(int16_t), 16));
}

// Bone view setup: both the local and the model matrix start out as the bind
// pose in model space (the inverse of the inverse bind matrix).
// @func 0x005812F0 ApeSkeleton::InitBone
static void InitBone(ApeSkeletonBone& bone, const ApeSkeletonDefBone& def, Matrix34* local, Matrix34* model, Vec3* scale)
{
    bone.def = &def;
    bone.local = local;
    bone.model = model;
    bone.scale = scale;
    Matrix34 bind;
    InvertRigid(bind, def.inverseBind);
    CopyAffine(*local, bind);
    CopyAffine(*model, bind);
}

// @func 0x005814A0 ApeSkeleton::Init
void ApeSkeleton::Init(ApeSkeletonDef* skeletonDef)
{
    def = skeletonDef;
    buffer = nullptr;
    bufferSize = 0;
    numBones = skeletonDef->numBones;

    const uint32_t interval = g_skeletonBoundsInterval > 1 ? g_skeletonBoundsInterval : 1;
    lodCounter = uint8_t(s_nextLodPhase % interval);
    ++s_nextLodPhase;
    flags &= 0x1F;

    const size_t n = numBones;
    const size_t size = n * sizeof(Matrix34) * 3 + n * sizeof(Vec3) + AlignUp(n * sizeof(int16_t), 16) +
                        n * sizeof(ApeSkeletonBone);
    SetBuffer(BucketAlloc(size), size);

    for (uint32_t i = 0; i < numBones; ++i) {
        const ApeSkeletonDefBone& defBone = skeletonDef->bones[i];
        InitBone(bones[i], defBone, &local[i], &model[i], &scales[i]);
        parents[i] = defBone.parent;
        scales[i] = kWorldUnitScale;
    }
    ComputeSkinMatrices(skin);
}

// @func 0x005816F0 ApeSkeleton::Create
ApeSkeleton* ApeSkeleton::Create(ApeSkeletonDef* skeletonDef)
{
    ApeSkeleton* skeleton = SkeletonPool().AllocT();
    if (!skeleton)
        return nullptr;
    skeleton->Init(skeletonDef);

    // The definition gives model space bind poses; turn the local matrix of
    // every child bone into a transform relative to its parent.
    for (uint32_t i = 0; i < skeleton->numBones; ++i) {
        const int16_t parent = skeleton->bones[i].def->parent;
        if (parent == -1)
            continue;
        Matrix34 parentInverse;
        InvertRigid(parentInverse, *skeleton->bones[parent].model);
        Matrix34 relative;
        Matrix34Mul(relative, *skeleton->bones[i].local, parentInverse);
        CopyAffine(*skeleton->bones[i].local, relative);
    }
    return skeleton;
}

// @func 0x005807D8 ApeSkeleton::Destroy
void ApeSkeleton::Destroy(ApeSkeleton* skeleton)
{
    if (!skeleton)
        return;
    if (skeleton->flags & kSkelFlag_Queued) {
        for (uint32_t i = 0; i < g_skeletonQueue.count; ++i) {
            if (g_skeletonQueue.items[i] == skeleton) {
                g_skeletonQueue.items[i] = nullptr;
                break;
            }
        }
    }
    if (skeleton->buffer) {
        BucketFree(skeleton->buffer, skeleton->bufferSize);
        skeleton->buffer = nullptr;
    }
    SkeletonPool().Free(skeleton);
}

// @func 0x0057E8D0 ApeSkeleton::UpdateModel
void ApeSkeleton::UpdateModel()
{
    if (!(flags & kSkelFlag_Dirty))
        return;
    flags &= ~kSkelFlag_Dirty;

    Enqueue();

    CopyAffine(model[0], local[0]);
    const bool scaled = (flags & kSkelFlag_HasScale) != 0;
    for (uint32_t i = 1; i < numBones; ++i) {
        const Matrix34& parentModel = model[parents[i]];
        Matrix34 result;
        Matrix34Mul(result, local[i], parentModel);
        if (scaled) {
            // The parent's scale stretches the offset of the child bone but
            // not its orientation.
            const Vec3& s = scales[parents[i]];
            const Vec3 offset = {local[i].translation.x * s.x, local[i].translation.y * s.y,
                                 local[i].translation.z * s.z, 0.0f};
            result.translation = TransformPoint(offset, parentModel);
        }
        CopyAffine(model[i], result);
    }
}

// @func 0x00580EA0 ApeSkeleton::ComputeSkinMatrices
void ApeSkeleton::ComputeSkinMatrices(Matrix34* out) const
{
    const bool scaled = (flags & kSkelFlag_HasScale) != 0;
    for (uint32_t i = 0; i < numBones; ++i) {
        Matrix34 boneModel = model[i];
        if (scaled)
            Matrix34ScaleAxes(boneModel, scales[i]);
        Matrix34Mul(out[i], def->bones[i].inverseBind, boneModel);
    }
}

// @func 0x00580D70 ApeSkeleton::CopyPose
void ApeSkeleton::CopyPose(const ApeSkeleton& other)
{
    for (uint32_t i = 0; i < numBones; ++i) {
        CopyAffine(local[i], other.local[i]);
        CopyAffine(model[i], other.model[i]);
        bones[i].local = &local[i];
        bones[i].model = &model[i];
    }
}

// @func 0x005A9378 ApeSkeleton::Enqueue
void ApeSkeleton::Enqueue()
{
    if (!(flags & kSkelFlag_Queued) && g_skeletonQueue.count < g_skeletonQueue.capacity) {
        g_skeletonQueue.items[g_skeletonQueue.count++] = this;
        flags |= kSkelFlag_Queued;
    }
}

// @func 0x005A93D8 ApeSkeleton::FindBone
int ApeSkeleton::FindBone(uint32_t nameHash) const
{
    for (uint32_t i = 0; i < numBones; ++i)
        if (bones[i].def->nameHash == nameHash)
            return int(i);
    return -1;
}

// --- chunk handlers ---------------------------------------------------------------------

// The skeleton definition chunk: version byte and bone count. The new
// definition is returned through the handler argument.
// @func 0x005851E8 LoadSkeletonDefChunk
int LoadSkeletonDefChunk(iff::IffReader* reader, void* arg)
{
    uint8_t version = 0;
    uint32_t numBones = 0;
    reader->ReadU8(&version);
    reader->ReadU32(&numBones);
    *static_cast<ApeSkeletonDef**>(arg) = ApeSkeletonDef::Create(numBones);
    return 1;
}

// One record per bone; parents are referenced by name hash and must come
// before their children.
// @func 0x00585268 LoadSkeletonBonesChunk
int LoadSkeletonBonesChunk(iff::IffReader* reader, void* arg)
{
    ApeSkeletonDef* def = *static_cast<ApeSkeletonDef**>(arg);
    for (uint32_t i = 0; i < def->numBones; ++i) {
        uint32_t nameHash = 0, parentNameHash = 0;
        float xBasis[3] = {}, zBasis[3] = {}, position[3] = {};
        char name[32] = {};
        reader->ReadU32(&nameHash);
        reader->ReadU32(&parentNameHash);
        reader->ReadVec3(xBasis);
        reader->ReadVec3(zBasis);
        reader->ReadVec3(position);
        reader->ReadRaw32(name);

        ApeSkeletonDefBone& bone = def->bones[i];
        ApeSkeletonDef::SetBone(bone, nameHash, xBasis, zBasis, position, name);
        for (uint32_t j = 0; j < i; ++j) {
            if (def->bones[j].nameHash == parentNameHash) {
                bone.parent = int16_t(j);
                break;
            }
        }
    }
    return 1;
}

// Container of the skeleton chunks of a geometry definition; the definition
// built by the nested chunks replaces the target's skeleton (offset 0xA4).
// @func 0x00584F54 LoadSkeletonContainerChunk
int LoadSkeletonContainerChunk(iff::IffReader* reader, void* arg)
{
    ApeSkeletonDef* loaded = nullptr;
    reader->ParseChunks(&loaded);

    ApeSkeletonDef*& slot = static_cast<ApeGeomDef*>(arg)->m_skeletonDef;
    if (slot != loaded) {
        ApeSkeletonDef* old = slot;
        slot = loaded;
        if (loaded)
            ++loaded->refCount;
        if (old)
            old->Release();
    }
    // NOTE: the console releases the loader's reference without a null check.
    if (loaded)
        loaded->Release();
    return 1;
}

} // namespace ape::world
