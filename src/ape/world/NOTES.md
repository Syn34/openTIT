# ape/world notes (world object layer, range 0x00578000-0x005B0000)

## Layout
* `ApeBase.h/.cpp`: shared base of definitions and instances (console vtable 0x00B242B8):
  16-bit reference count, 6-bit type, unique id (counter 0x01164CD0), tag block pointer.
  Also the world constants (unit scale, zero, empty sphere, identity).
* `ApeDef.h/.cpp`: `ApeDef` (vtable 0x00B246B0), `ApeTDefList<T>` (per-type pool +
  load callback + name map, the static blocks at 0x00F8F12C..0x00F8F140), `FindDef`,
  `FindInstance`/`FindObject`/`FindGeomInstance`, deferred destruction hook.
* `ApeDefTypes.h/.cpp`: geometry (0x00B246D8), light (0x00B24700), camera (0x00B247B8),
  emitter (0x00B247E0, partial), point (0x00B24808) and spline (0x00B24830) definitions and
  their chunk handlers.
* `ApeObj.h/.cpp`: `ApeObj` (vtable 0x00B24610): transforms, dirty-root list (0x0116BD00),
  hierarchy, world update, bounds.
* `ApeInstances.h/.cpp`: `ApeRenderObj` (0x00B24660) and the instance types (geometry
  0x00B24858, light 0x00B248A8, emitter 0x00B248F8, camera 0x00B24948, point 0x00B24998,
  spline 0x00B249E8), `ApeTInstanceList<T>` (0x00F8F144..0x00F8F158), instance chunk handlers.
* `ApeSkeleton.h/.cpp`: skeleton definitions ("numBones") and posed skeletons.
* `ApeLocators.cpp`: definition locators ("numLocators"); the definition field at 0x6C.
* `ApeRegions.h/.cpp`: visibility regions ("numRegions") and quads ("numQuads").
* `ApeRenderNode.h/.cpp`: the per-object node table at 0x01194598.
* `ApeWorldMath.h/.cpp`, `ApeWorldHooks.h`: small geometry queries; renderer hooks.

## Identifications / uncertainties
* The table at 0x01194598 is created by `ApeProceduralMatrixAnimMgr::Create` (0x0059541C:
  512 nodes of 0xA0 bytes + 512 bounds entries of 0x20, plus a 0x70-byte element pool used for
  the node blocks at +0x84/+0x88). It is kept under the neutral name "render node table"
  (`ApeRenderNode`), because nodes also carry the culling sphere. It is most likely the ProcAnim
  manager's per-object animation state; rename once ProcAnim is ported.
* Object flag 0x4000 (`kObjFlag_4000`): when set, UpdateWorld uses the inverse of the local
  matrix (camera style). Flag 0x0100 selects the animated-bones bounds path.
* Transform byte 0x0A9: 0x04 inherit parent scale, 0x08 scale translation + normalise parent,
  0x10 follow parent position only (decoded from the disassembly; Ghidra truncated it).
* `ApeObj::UpdateWorld` (0x0057EE00) and `UpdateWorldFromParent` (0x0057E260) were decoded
  from the disassembly (Ghidra stopped at Cell `lvlx` instructions).
* Type 0x13 (`kObjType_HierarchyRoot`): AttachChild stops climbing at it; class unknown.
* Reader flag 0x04 is treated as "reload" (quad list / spline definitions keep existing data).
* `LoadSplineDefPointsChunk` reproduces a console quirk: re-initialising a closed spline passes
  count + 1 to `ApeSpline::Init`, which adds the closing point again.
* `LoadRegionChunk` keeps the read nameHash (the console computes the name hash and overwrites it).
* Region test uses `AabbContainsBox` (0x00557430) as named by the math module.

## Requests for other modules
* `ApeBase` (0x00558A48/0x00558B84/0x00558CE8 dtors, 0x00558F34 empty Release, 0x00558E24 tag
  share) releases its tag block; tags live in `ApeTags.h/.cpp` (TAG2 chunks, ApeTagData,
  ApeTagBlock, the tag cache). The older 'ATAG' form (0x00563808/0x00563368) is not ported.
* `ApeRegionSet` is now an ApeBase class (type 0x12, 0x0057711C..0x00577520) with the object
  mask and camera visibility queries; `BuildRegionVolume`/`BuildBoxPlanes` (0x00576F28/
  0x00576BD8) build oriented regions (plane order of the vector routine is a guess).
* `ApeScene.h/.cpp`: scene slot table, construction/destruction (0x00586440..0x00586744) and
  `CreateScene` (0x005903F4); the scene virtuals (0x00586xxx-0x0059AF54) are still open.
* Region volume builder 0x00576F28 (scene module) is a local stub (`BuildRegionVolume`) that only
  fills the bounds; region lookup 0x00577338 is not called.
* Renderer: mesh lookups/refs, light links, particle systems and the deferred destruction queue
  (0x005082F4) are reached through `g_worldRenderHooks` / `g_deferredDestroy`.
* The spline definitions use `ape/spline/ApeSpline.h` (Init, ComputeLength, GetBounds,
  Evaluate*, FindClosestParam, GetFlag/IsClosed/GetPoint).
* Duplicates outside the range tagged here because they are the same class: 0x007A9964
  (ApeObj::OnBoundsChanged copy). Not tagged: 0x007A9F38 (identical to ApeObj dtor, used by
  the point instance vtable), 0x007A9A1C (render object GetSkeleton), 0x007A99F8.

## Not done in the range
* 0x005784E8.. area: 0x005786C0 (segment vs plane set clip, initial values look odd),
  0x00578868-0x0057CE08 (render node/vis-slot maintenance, ProcAnim node setup 0x0057BEE0).
* ProcAnim proper (0x005922A8-0x00598E60): parameter parser 0x00592DA8 reads the tag blocks
  ("%sPosPingPong", "%sPosAmplitude", ...), the build 0x005947D0 and update code are large
  vector routines. Needs the tag block port first.
* Level/scene orchestration 0x0058AC70-0x00591A30, 0x005964B4.. (load sequence calling the
  per-type setup functions), light list 0x01194EC0 (0x005965BC), light linking
  0x00581D48/0x00582848/0x00582A84, vis table 0x01193004 (0x0057A1D8, 0x0057A46C, 0x00582F3C).
* Emitter definition particle setup 0x0059D360/0x0059D578/0x0059D6BC/0x0059DB28/0x0058E140,
  light instance construction 0x005A2528/0x005A2BA8, instance clone functions (0x005A089C...),
  light/emitter/camera instance load handlers (implemented generically, untagged).
* 0x005A9914/0x005A9AA8 (ProcAnim candidate collection), point instance table 0x0119458C
  (0x005A8BA8, 0x005A8BE4, 0x005A8C30), 0x005A9B48.. to the end of the range (render passes:
  "Reflection", "TextureWarp", "Outline", "LineOutline").
* Skeleton debug dump 0x00584160/0x00583FBC (stripped prints only).

## Scene loading and world start-up (agent1, pass 3)
* `ApeScene.cpp`: LoadSceneFile (0x005904EC), FinishSceneLoad (0x005964B4), DestroyScene
  (0x00596E3C), the load callback every object list runs while a scene file is parsed
  (0x00588E70: phase 0 marks the object with the scene slot, phase 1 adds it to the scene's
  object list) and the ApeBase destruction hook 0x00588F44 (removes a destroyed object from its
  scene). The former `ApeBase::m_tagFlags` is the scene slot (bits 6..9 of the flags word);
  textures and materials carry the same field (`AglRefCounted::sceneSlot`). The scene keeps
  `ApeSceneObject {object, release}` entries because the port has no common base for
  ApeBase and the Agl resources (on the console they share one: same unique id counter, same
  flags word layout).
* CreateScene disables Agl deferred destruction (0x010DDC90) and FinishSceneLoad enables it
  again. 0x0116BD05 is set while DestroyScene releases objects (readers not ported).
* `ApeSceneSetup.cpp`: the FinishSceneLoad passes. Done: world transform updates (0x00588FFC,
  0x0058F118), light grid (0x0058FD50, grid = 128x128 around the local lights' centre, at
  least covering them), global ambient/directional light (0x00590DA0, `g_sceneLighting`,
  EngineConfig block 0x010C5AD0 on the console), dynamic objects (0x00590FEC, 0x00591178),
  empty geometry hiding (0x0058F8C4, through two new mesh hooks), object lighting
  (0x0058FADC). TODO stubs: render nodes 0x0058EA70, instance batching 0x00595968 (geometry
  instances sharing a definition merged into batches; table 0x0116BF00, 0x110-byte records),
  ProcAnim manager create/destroy/update (0x0059541C, 0x00596770, 0x00596A8C), light list
  0x00591A30/0x005965BC, light links 0x00583518/0x005832FC, views 0x005908A0, shadow bounds
  0x0058DBE8.
* `ApeLightInstance::m_lightType` (+0x170): 0 ambient, 1 directional, 2 local (inferred).
* Instance creation/cloning (ApeInstances.cpp): CreateGeomInstance 0x005A05E8,
  CreateCameraInstance 0x005A59AC, CreatePointInstance 0x005A68EC and `ApeObj::Clone` (console
  slot 14) for geometry 0x005A089C, camera 0x005A5B28 and point 0x005A6B28 instances. Clone
  flags: 1 children, 2 own definition copy, 4 (emitters), 8 no ProcAnim registration, 0x10
  (lights) skip light construction. Light 0x005A2E50 / emitter 0x005A42D0 / spline
  0x005A7E80 clones and their creators (0x005A2BA8, 0x005A3FFC, 0x005A7CB4) are open; the
  light construction 0x005A2528 is the light animation set-up (tags "IntensityOffset",
  "IntensityAmplitude", "IntensityPeriod", "IntensityRandomStart", parsed by 0x00591410).
* `ApeWorldInit.cpp`: InitWorld 0x00588828 and the twelve per-type start-up functions (pool
  sizes from EngineConfig +0x168..+0x182, new 16-bit fields; chunk ids GEOB/GEOH/GLOD,
  LITB/LITH, EMIT/EMIH, CAMC/CAMH, SPDS/SPDH/SPDP, PNDC/PNDH, GINS/GINH + SKEL/SKHD/BONS,
  LINS/LINH, EINS/EINH, CAMI/CMIH, PNTC/PNTH, SPIS/SPIH, regions "PVS "/PVSH/PVSR, occluder
  quads OCCP/OCCH/OCCV/OCHT/OCFT/OCNT, locators LOCA/LCRH/LCRD). SystemInit flag 2 runs it
  (flag 4 is the Agl init 0x0050F520). The emitter definition chunk 0x0059DB28 is a skipping
  stub (particle systems); the GPU parts of InitWorld (0x00586DB4, 0x00586BF4, 0x00586B3C,
  0x005868D0) are stubs.
* The scene virtuals (vtables 0x00B24728/0x00B24770) are the SPU render job set-up
  (0x005894F8 copies the view into the job block 0x0117E280, 0x00589BC8 walks the
  visibility table, 0x00599804/0x00599FA8 wait on SPU job counters): console specific.
