// openTIT - Start-up of the world object module: object pools, name maps and
// the level chunk handlers of every definition and instance type.
#include "ape/world/ApeWorldInit.h"

#include "ape/core/ApeSystem.h"
#include "ape/memory/ApeTFreeList.h"
#include "ape/sys/ApeIff.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeRegions.h"
#include "ape/world/ApeScene.h"
#include "ape/world/ApeSkeleton.h"

namespace ape::world {

namespace {

using iff::MakeId;
using iff::RegisterChunkHandler;

// Chunks whose handler runs for every nesting level (flag 0x10 of the
// handler table, used by the spline and quad chunks).
constexpr uint32_t kChunkFlag_10 = 0x10;

// Pool size: the configured value, or the module default when unset.
uint32_t PoolSize(uint16_t configured, uint32_t defaultSize)
{
    return configured != 0 ? configured : defaultSize;
}

template <class T>
void SizeDefPool(uint16_t configured, uint32_t defaultSize)
{
    // NOTE: the console also creates the type's name map here with the same
    // capacity; GetDefMap creates it on first use.
    ApeTListAllocator<T>::SetInitialCount(PoolSize(configured, defaultSize));
    (void)ApeTDefList<T>::Map();
}

template <class T>
void SizeInstancePool(uint16_t configured, uint32_t defaultSize)
{
    ApeTListAllocator<T>::SetInitialCount(PoolSize(configured, defaultSize));
    (void)GetInstanceMap(T::kInstanceType);
}

// TODO(0x0059DB28 LoadEmitterDefChunk) the emitter definition chunk builds
// the particle system definition, which is not ported; the chunk is skipped.
int LoadEmitterDefChunk(iff::IffReader*, void*)
{
    return 1;
}

// TODO(0x00586DB4 InitWorldDebugMaterials) debug materials of the world
// module (shader classes 0x6DC54CB1/0x3A45928D) and their vertex layout.
bool InitWorldDebugMaterials()
{
    return false;
}

// TODO(0x00586BF4 RecordWorldDebugPrograms) pre-recorded program blocks of
// the debug materials.
void RecordWorldDebugPrograms()
{
}

// TODO(0x00586B3C InitWorldShadowMaterial) clone of built-in material 0
// used for shadow volumes.
void InitWorldShadowMaterial()
{
}

// TODO(0x005868D0 InitWorldShadowMaps) shadow map render target and
// cascades (installed as the render hook when none is set).
void InitWorldShadowMaps()
{
}

} // namespace

// @func 0x00588478 InitGeomDefs
void InitGeomDefs()
{
    SizeDefPool<ApeGeomDef>(g_engineConfig.geomDefCapacity, 0x400);
    RegisterChunkHandler(MakeId('G', 'E', 'O', 'B'), &LoadGeomDefChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('G', 'E', 'O', 'H'), &LoadGeomDefRecordChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('G', 'L', 'O', 'D'), &LoadGeomDefLodChunk, 0, nullptr, 0);
}

// @func 0x00588660 InitLightDefs
void InitLightDefs()
{
    SizeDefPool<ApeLightDef>(g_engineConfig.lightDefCapacity, 0x80);
    RegisterChunkHandler(MakeId('L', 'I', 'T', 'B'), &LoadLightDefChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('L', 'I', 'T', 'H'), &LoadLightDefRecordChunk, 0, nullptr, 0);
}

// @func 0x00588240 InitEmitterDefs
void InitEmitterDefs()
{
    SizeDefPool<ApeEmitterDef>(g_engineConfig.emitterDefCapacity, 0x140);
    RegisterChunkHandler(MakeId('E', 'M', 'I', 'T'), &LoadEmitterDefChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('E', 'M', 'I', 'H'), &LoadEmitterDefRecordChunk, 0, nullptr, 0);
}

// @func 0x00588078 InitCameraDefs
void InitCameraDefs()
{
    SizeDefPool<ApeCameraDef>(g_engineConfig.cameraDefCapacity, 0x20);
    RegisterChunkHandler(MakeId('C', 'A', 'M', 'C'), &LoadCameraDefChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('C', 'A', 'M', 'H'), &LoadCameraDefRecordChunk, 0, nullptr, 0);
}

// @func 0x00587E90 InitSplineDefs
void InitSplineDefs()
{
    SizeDefPool<ApeSplineDef>(g_engineConfig.splineDefCapacity, 0x40);
    RegisterChunkHandler(MakeId('S', 'P', 'D', 'S'), &LoadSplineDefChunk, 0, nullptr, kChunkFlag_10);
    RegisterChunkHandler(MakeId('S', 'P', 'D', 'H'), &LoadSplineDefHeaderChunk, 0, nullptr, kChunkFlag_10);
    RegisterChunkHandler(MakeId('S', 'P', 'D', 'P'), &LoadSplineDefPointsChunk, 0, nullptr, kChunkFlag_10);
}

// @func 0x005873AC InitPointDefs
void InitPointDefs()
{
    SizeDefPool<ApePointDef>(g_engineConfig.pointDefCapacity, 0x40);
    RegisterChunkHandler(MakeId('P', 'N', 'D', 'C'), &LoadPointDefChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('P', 'N', 'D', 'H'), &LoadPointDefRecordChunk, 0, nullptr, 0);
}

// @func 0x00587A40 InitGeomInstances
// Geometry instances also bring the skeleton chunks.
void InitGeomInstances()
{
    SizeInstancePool<ApeGeomInstance>(g_engineConfig.geomInstanceCapacity, 0x400);
    RegisterChunkHandler(MakeId('G', 'I', 'N', 'S'), &LoadGeomInstanceChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('G', 'I', 'N', 'H'), &LoadGeomInstanceRecordChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('S', 'K', 'E', 'L'), &LoadSkeletonContainerChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('S', 'K', 'H', 'D'), &LoadSkeletonDefChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('B', 'O', 'N', 'S'), &LoadSkeletonBonesChunk, 0, nullptr, 0);
}

// @func 0x00587750 InitLightInstances
void InitLightInstances()
{
    SizeInstancePool<ApeLightInstance>(g_engineConfig.lightInstanceCapacity, 0x80);
    RegisterChunkHandler(MakeId('L', 'I', 'N', 'S'), &LoadLightInstanceChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('L', 'I', 'N', 'H'), &LoadRenderInstanceRecordChunk, 0, nullptr, 0);
}

// @func 0x00587574 InitEmitterInstances
void InitEmitterInstances()
{
    SizeInstancePool<ApeEmitterInstance>(g_engineConfig.emitterInstanceCapacity, 0x200);
    RegisterChunkHandler(MakeId('E', 'I', 'N', 'S'), &LoadEmitterInstanceChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('E', 'I', 'N', 'H'), &LoadRenderInstanceRecordChunk, 0, nullptr, 0);
}

// @func 0x00587CC8 InitCameraInstances
void InitCameraInstances()
{
    SizeInstancePool<ApeCameraInstance>(g_engineConfig.cameraInstanceCapacity, 0x20);
    RegisterChunkHandler(MakeId('C', 'A', 'M', 'I'), &LoadCameraInstanceChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('C', 'M', 'I', 'H'), &LoadCameraInstanceRecordChunk, 0, nullptr, 0);
}

// @func 0x005871E4 InitPointInstances
void InitPointInstances()
{
    SizeInstancePool<ApePointInstance>(g_engineConfig.field_16E, 0x40);
    RegisterChunkHandler(MakeId('P', 'N', 'T', 'C'), &LoadPointInstanceChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('P', 'N', 'T', 'H'), &LoadPointInstanceRecordChunk, 0, nullptr, 0);
}

// @func 0x00586FEC InitSplineInstances
void InitSplineInstances()
{
    SizeInstancePool<ApeSplineInstance>(g_engineConfig.splineInstanceCapacity, 0x40);
    RegisterChunkHandler(MakeId('S', 'P', 'I', 'S'), &LoadSplineInstanceChunk, 0, nullptr, kChunkFlag_10);
    RegisterChunkHandler(MakeId('S', 'P', 'I', 'H'), &LoadSplineInstanceRecordChunk, 0, nullptr, kChunkFlag_10);
}

// @func 0x00588828 InitWorld
bool InitWorld()
{
    // NOTE: the console also sizes the dirty root list (0x800 entries) and
    // allocates the draw record table (0x01193004, EngineConfig +0x154
    // entries of 0x80 bytes, default 0x1000); the port's containers grow on
    // demand and the record table is created on first use (ApeVisTable.h).
    InitLightDefs();
    InitGeomDefs();
    InitEmitterDefs();
    InitCameraDefs();
    InitSplineDefs();
    InitCameraInstances();
    InitGeomInstances();
    InitLightInstances();
    InitEmitterInstances();
    InitPointDefs();
    InitPointInstances();
    InitSplineInstances();

    // Visibility regions ("PVS") and occluder quads ("OCC").
    RegisterChunkHandler(MakeId('P', 'V', 'S', ' '), &ParseNestedChunks, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('P', 'V', 'S', 'H'), &LoadRegionSetChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('P', 'V', 'S', 'R'), &LoadRegionChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('O', 'C', 'C', 'P'), &ParseNestedChunks, 0, nullptr, kChunkFlag_10);
    RegisterChunkHandler(MakeId('O', 'C', 'C', 'H'), &LoadQuadListChunk, 0, nullptr, kChunkFlag_10);
    RegisterChunkHandler(MakeId('O', 'C', 'C', 'V'), &LoadQuadCornersChunk, 0, nullptr, kChunkFlag_10);
    RegisterChunkHandler(MakeId('O', 'C', 'H', 'T'), &LoadQuadNameHashesChunk, 0, nullptr, kChunkFlag_10);
    RegisterChunkHandler(MakeId('O', 'C', 'F', 'T'), &LoadQuadFlagsChunk, 0, nullptr, kChunkFlag_10);
    RegisterChunkHandler(MakeId('O', 'C', 'N', 'T'), &LoadQuadNamesChunk, 0, nullptr, kChunkFlag_10);

    InstallSceneObjectHook();

    if (InitWorldDebugMaterials())
        RecordWorldDebugPrograms();

    // Definition locators.
    RegisterChunkHandler(MakeId('L', 'O', 'C', 'A'), &ParseGeomDefChunks, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('L', 'C', 'R', 'H'), &LoadLocatorListChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('L', 'C', 'R', 'D'), &LoadLocatorChunk, 0, nullptr, 0);

    InitWorldShadowMaterial();
    InitWorldShadowMaps();
    return true;
}

} // namespace ape::world
