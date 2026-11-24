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
* `ApeBase` (0x00558A48/0x00558B84 dtors, 0x00558F34 empty Release, 0x00558E24 tag share) is
  defined here because the sys module has not ported it; its tag block release (0x0056C730)
  and the global tag hook (0x01164CD4) are not reproduced. Move/merge when tags are ported.
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
