// openTIT - Skeletons: bone hierarchy definitions and posed skeleton instances.
//
// An ApeSkeletonDef lists the bones of a model: each bone stores its inverse
// bind matrix (model space -> bone space), name and parent index. It is
// loaded from the "numBones" chunk and its bone sub-chunks and shared by
// reference count.
//
// An ApeSkeleton is a posed copy: per bone a local matrix (relative to the
// parent bone), a model space matrix (UpdateModel), a skinning matrix
// (inverse bind * model, ComputeSkinMatrices) and a scale. All arrays live
// in one block from the bucket allocator.
#pragma once

#include "ape/math/ApeBounds.h"
#include "ape/math/ApeMath.h"

#include <cstddef>
#include <cstdint>

namespace ape::iff {
struct IffReader;
}

namespace ape::world {

struct alignas(16) ApeSkeletonDefBone {
    Matrix34 inverseBind;   // 0x00 model space -> bone space
    uint32_t nameHash;      // 0x40
    char name[32];          // 0x44
    int16_t parent;         // 0x64 index of the parent bone, -1 for a root
    uint8_t pad[10];
};

struct ApeSkeletonDef {
    int16_t refCount;
    uint16_t numBones;
    ApeSkeletonDefBone* bones;

    // Allocates a definition with `numBones` bones (identity bind, no name,
    // no parent) and one reference.
    static ApeSkeletonDef* Create(uint32_t numBones);
    // Drops a reference; frees the bones and the definition when unused.
    void Release();

    // Sets a bone from its bind pose axes (y = z x x) and position.
    static void SetBone(ApeSkeletonDefBone& bone, uint32_t nameHash, const float xBasis[3],
                        const float zBasis[3], const float position[3], const char* name);
    // Index of the bone with the given name hash, or -1.
    int FindBone(uint32_t nameHash) const;
};

// Per-bone view into the skeleton arrays.
struct ApeSkeletonBone {
    const ApeSkeletonDefBone* def;
    Matrix34* local;
    Matrix34* model;
    Vec3* scale;
};

enum ApeSkeletonFlags : uint8_t {
    kSkelFlag_Queued   = 0x20,   // in the per-frame skinning queue
    kSkelFlag_HasScale = 0x40,   // some bone scale differs from 1
    kSkelFlag_Dirty    = 0x80,   // model matrices need recomputing
};

struct ApeSkeleton {
    Sphere bounds;                 // 0x00 model space bounds of the bone positions
    ApeSkeletonDef* def;           // 0x10
    void* buffer;                  // 0x14
    size_t bufferSize;             // 0x18
    uint16_t numBones;             // 0x1C
    uint8_t lodCounter;            // 0x1E frames left before the bounds are refreshed
    uint8_t flags;                 // 0x1F ApeSkeletonFlags
    ApeSkeletonBone* bones;        // 0x20
    Matrix34* local;               // 0x24
    Matrix34* model;               // 0x28
    Matrix34* skin;                // 0x2C
    Vec3* scales;                  // 0x30
    int16_t* parents;              // 0x34

    // Creates a posed skeleton in the bind pose of `def`.
    static ApeSkeleton* Create(ApeSkeletonDef* def);
    // Removes the skeleton from the skinning queue and frees it.
    static void Destroy(ApeSkeleton* skeleton);

    // Recomputes the model matrices from the local ones when dirty and queues
    // the skeleton for skinning.
    void UpdateModel();
    // out[i] = inverse bind of bone i * (scaled) model matrix of bone i.
    void ComputeSkinMatrices(Matrix34* out) const;
    // Copies the local and model matrices of another skeleton of the same
    // definition.
    void CopyPose(const ApeSkeleton& other);
    // Index of the bone with the given name hash, or -1.
    int FindBone(uint32_t nameHash) const;
    // Puts the skeleton in the skinning queue (if there is room).
    void Enqueue();

private:
    void Init(ApeSkeletonDef* def);
    void SetBuffer(void* block, size_t size);
};

// Skeletons whose skinning matrices must be rebuilt this frame (0x01194590).
struct ApeSkeletonQueue {
    ApeSkeleton** items = nullptr;
    uint32_t count = 0;
    uint32_t capacity = 0;
};
extern ApeSkeletonQueue g_skeletonQueue;

// Number of frames between skeleton bounds refreshes (engine config byte
// 0x010C5ABA); 0 refreshes every frame.
extern uint8_t g_skeletonBoundsInterval;

// Chunk handlers of the skeleton definition chunks.
int LoadSkeletonDefChunk(iff::IffReader* reader, void* arg);      // "numBones"
int LoadSkeletonBonesChunk(iff::IffReader* reader, void* arg);    // bone list
int LoadSkeletonContainerChunk(iff::IffReader* reader, void* arg);

} // namespace ape::world
