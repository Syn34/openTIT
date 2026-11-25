// openTIT - Agl built-in geometry: vertex declarations and procedurally built meshes.
#pragma once

#include "agl/AglState.h"

#include <cstdint>

namespace agl {

class AglMaterial;
class AglSubMesh;

// A vertex layout: up to ten attribute streams terminated by an entry of
// size 0, the vertex stride and the mask of attributes it provides
// (console object 0x80 bytes).
struct AglVertexDecl {
    static constexpr int kMaxStreams = 10;
    AglVertexStream streams[kMaxStreams];
    uint32_t stride = 0;              // 0x78
    uint16_t flags = 0;               // 0x7C (bit 15: initialised)
    uint16_t attributeMask = 0;       // 0x7E
};

// Builds a declaration from a stream table (terminated by size 0).
void VertexDeclInit(AglVertexDecl* decl, const AglVertexStream* streams, uint32_t stride);

// Layout of the debug/primitive meshes: position (float3), color (ubyte4),
// texture coordinate (float2); 24 bytes. 'alternate' selects the second,
// identical declaration the console keeps for the skinned path.
const AglVertexDecl* GetPrimitiveVertexDecl(bool alternate);

// Primitive types by the index the game uses (0x00F66F1C).
gpu::PrimitiveType PrimitiveFromIndex(int index);

// A new submesh drawn with 'material' with room for 'vertexCount'
// vertices of 'decl' and 'indexCount' 16-bit indices (main memory).
AglSubMesh* CreateSubMesh(AglMaterial* material, int primitiveIndex, uint32_t vertexCount,
                          uint32_t indexCount, const AglVertexDecl* decl);

// Declarations of the debug meshes (0x010DC740 and 0x010DC7C0).
// NOTE: they are filled outside Agl on the console; here they use the
// primitive layout.
const AglVertexDecl* GetDebugVertexDecl(bool alternate);

// A debug mesh handle (the console allocates a 4-byte box around the submesh).
struct AglDebugMesh {
    AglSubMesh* mesh = nullptr;
};
// Material of debug meshes: textured (1) or vertex colored (anything else).
AglMaterial* GetDebugMaterial(int textured);
AglDebugMesh* CreateDebugMesh(AglMaterial* material, int primitiveIndex, uint32_t vertexCount,
                              uint32_t indexCount);
void DestroyDebugMesh(AglDebugMesh* mesh);
// Vertex memory of a debug mesh, marked as being written by the CPU.
void* LockDebugMeshVertices(AglDebugMesh* mesh);
// Ends writing the vertices (nothing to do here).
void UnlockDebugMeshVertices(AglDebugMesh* mesh);

// A unit sphere as one triangle strip of 'rings' x 'segments' quads.
AglSubMesh* CreateSphereMesh(AglMaterial* material, int rings, int segments);
// A unit sphere with one vertex per cell centre, drawn as indexed strips.
AglSubMesh* CreateIndexedSphereMesh(AglMaterial* material, int rings, int segments);
// A unit cube centred on the origin: twelve edges, or twelve triangles when 'solid'.
AglSubMesh* CreateBoxMesh(AglMaterial* material, bool solid);

} // namespace agl
