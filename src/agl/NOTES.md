# Agl graphics layer: notes for integration

## Layout

* `gpu/` is the backend boundary. `CommandList` records typed commands (the libgcm
  helpers 0x005DF950-0x005E5000 became its methods), `Device` executes them, and
  `NullDevice` is the only backend: it validates and counts, and draws nothing.
  `CgProgram` parses the shipped Cg binary images (.vpo/.fpo).
* `AglTypes.h` / `AglSurface.cpp`: GPU memory blocks, the format and AA tables,
  render surfaces (`AglSurface`, the 0x3C console struct), texture images.
* `AglState.*`: the render state shadow (globals 0x010F1020-0x010F1180) and the
  state functions that use it.
* `AglShader.*`, `AglPostProcShaders.*`: shader loading, caching, fragment constant patching,
  and the shader sets of the post-process passes.
* `AglTexture.*`, `AglResources.*`, `AglDeferred.*`: pooled, reference-counted resources and
  deferred destruction.

## Facts for other modules (please merge where it fits)

* Memory classes 4 and 5 (`ape::kMemClass_HeapA/HeapB`) are the RSX **local** (video)
  memory and the **IO-mapped main** memory. Agl allocates every GPU block with
  `SysAlloc(align, size, 4|5, category)`. The NOTE in `src/ape/memory/ApeMem.h` can be
  updated to say this. (Source: AglMemBlock::Alloc 0x0052F198, AglMemBlock::Free 0x0052EFE0.)
* 0x01164CD0 is a global resource id counter that textures (0x0050A2F0) and render
  surfaces (0x0051AB74) share. Agl keeps its own counters for now.
* 0x0050AFDC (vtable store, 57 callers) is in an exact group with 0x007A0EB4,
  0x007ABB3C... and is not Agl specific, so it is not tagged here.

## Functions outside the Agl range that Agl needs

Untagged equivalents live in `gpu/CgProgram.cpp`, so the owner of these addresses
can tag them or point them at that file:

* 0x005E5E28 GetUCode, 0x005E5E54 GetNamedParameter, 0x005E5F10 GetParameterType,
  0x005E5F20 GetParameterResourceIndex, 0x005E5F28 InitProgram, 0x005E5F38/0x005E5F9C
  Get/SetRegisterCount, 0x005E5FF0 (vertex register count), 0x005E60DC
  GetEmbeddedConstCount, 0x005E610C GetEmbeddedConstOffset (all cellGcmCg* runtime).
* 0x005E5CB0 is called with (ctx, register, floatCount, values). It behaves like
  SetVertexProgramConstants, which is how Agl uses it (`Begin2DPass`).
* 0x005E5914 (records the vertex program load into a sub-buffer) and 0x005E51AC (called at
  the end of the commit-draw-state function with (ctx, 9, stride, table)) are not
  identified. `CommitDrawState` binds vertex streams relative to
  `AglRenderState::vertexBuffer` instead.

## Not done / uncertain

* 0x005E3FA4 (RSX method 0x1E98), 0x005E403C (method 0x1D84), 0x005E4014 and
  0x005E4B24/0x005E4C2C (scaled-transfer setup) are not tagged because their methods are
  not identified. `ResetRenderState` leaves the 0x1E98 write out, with a NOTE.
* 0x005E1814 (parameter-default upload by Cg type, jump table) is implemented by type
  family (float and half vectors and matrices). The jump table was not decoded.
* `AglTopology/AglRenderTarget` contents are still placeholders; only
  Create/Destroy/Release are reimplemented. `AglMaterial` and `AglSubMesh` now have
  their console layout (see below).
* `AglTexture::sampler[7]` defaults are copied from the constructor. Their meaning is unknown.
* Console-only mechanisms were dropped or turned into bookkeeping: memory tiles and
  z-cull (`SurfaceBindTile/BindZcull` keep the region indices only), jump-patching of
  command buffers (0x00537D5C/0x00537DD4), and the command ring buffer (0x00537F28,
  0x0052F670, 0x0052F780).
* `EndFrame` (0x005124F4) is implemented. The engine services it calls outside Agl
  (0x0056B4E8, 0x0056E038, 0x0056E1F0, 0x0056B2B8: debug draw and SPU job dispatch)
  are local empty functions in `AglRenderThread.cpp`; the Agl ones it calls
  (0x00548938, 0x00548A10, 0x00511F8C) are TODO stubs.

## Build state seen while working

The full link was failing because of files outside `src/agl`: missing
`ape::world::Matrix34Mul/Matrix34FromBasis`, and `ape::FileData` in
`sly/game/AudioSystem.cpp`. All `src/agl` sources build with no warnings
(`cmake --build ... -- -k`).

## Second pass (materials, shader classes, Cg runtime)

### Files

* `AglShaderClass.h/.cpp`: the material type system. `AglShaderClass` (console
  object 0x20 bytes: vtable, name, name hash, technique array) owns techniques
  (`AglTechnique`, an `AglShader` with BeginMaterial/Draw/End). Registry 0x010F0BA8,
  `RegisterShaderClass` 0x0050AFEC, `RegisterShaderClasses` 0x0050B138 (four static
  lists at 0x00F88A08/0x00F88A4C/0x00F88A68/0x00F88A8C, the last three gated by the
  bytes 0x010C5ABC..0x010C5ABE), `FindShaderClass` 0x004FE990 (with four alias hashes).
* `AglShaderClasses.cpp`: the 63 built-in classes (statics 0x01196A80..0x01197230).
  Program names, the extra parameter tables and the variant tables were read from
  the binary. Every class's `CreateTechniques` in 0x005B0000-0x005DF950 is done, most
  `SelectTechnique`s, the simple `SetupMaterial`s and the empty technique methods.
  The big technique `Draw` functions (VMX matrix uploads plus constant patching,
  300-4000 bytes each) and the larger `BeginMaterial`/`SetupMaterial` ones are
  listed as `TODO(...)` comments inside the class declarations (the base class does
  nothing). `SlyWater2/3` and `SHO_NightBlend` selection are also TODO.
* `AglMaterial.cpp`: `AglMaterial` (0x90 bytes) construction/destruction, textures,
  the float constant block (0x0050C018) and the UV transform upload (0x0050BE5C,
  0x0050D124). The material animation matrix (0x0050B980) is a TODO stub returning
  identity.

### Facts for other modules

* Technique vtable slots: 2 = BeginMaterial(material) (called by the material
  apply function 0x0079CD0C), 3 = Draw(renderItem), 4 = End(material or null).
  Class vtable: 0 = CreateTechniques, 1 = SetupMaterial(material, desc), 2/3 dtors,
  4 = SelectTechnique(material, renderItem). The renderer loop is 0x00512C30.
* Material words decoded from 0x0079CD0C: 0x68 bit 31 = blend enable, bits 15..30 =
  source factor; 0x6C = blend equation; 0x6E = destination factor; 0x70 bit 31 =
  depth test, bits 15..30 = depth function; 0x74 = culled front face (0 = no culling).
* Render items: +0x0C submesh, +0x1C material, +0x2C light set, +0x30 position,
  +0x45 light mask written by SelectTechnique.
* Functions outside the Agl ranges that this code needs (not tagged here):
  0x007A0E98 (AglTextureBase::SetWrap, implemented untagged in `AglTexture.cpp`),
  the `CreateTechniques` of Debug/Prim*/SimpleColor*/TextureWarp/ScreenWarp/
  LineOutline/Outline (0x005AD...-0x005AFF0C, in the world range) and of the classes
  whose code is at 0x007A.... (stubs with TODO in `AglShaderClasses.cpp`), and the
  base class defaults 0x007A349C/0x007A3518.
* 0x010DDAF4 is the texture name map (`FindTexture` 0x0050C398), not a mutex as the
  first pass assumed; `RegisterTextureName` fills it (its console writer is unknown).
* 0x005E6154 (floorf) and 0x005E61D4 (_FDint) are SN libc, not Agl: they belong in
  `src/crt`.
* 0x005D9810 (6240 bytes) and 0x005DB070..0x005DF294 at the end of range 1 are not
  Agl material code (static initialisers and zlib inflate); not touched.

### Uncertain

* Technique class names for vtables whose class code is outside the range
  (0x00B25220 ParticleMaterial, 0x00B25358 ProjTxrDeferred, 0x00B254B0 Decal,
  0x00B25DC8 SlyScreenWarp) are guesses from their position in the binary.
* `AglMaterialDesc` field names (0x04 diffuse hash, 0x10 texture hash, 0x4C wrap
  mode, 0x54/0x58 parameters) come from the few SetupMaterial functions done.
* The flag at 0x010C5AB8 (sign bit disables material parameters) is unexplained.
* `AglLightSet::variant` is a 0/1 selector of every lit technique pair.

## Third pass (agl3: render targets, post-process chain, draw list, queries, loaders)

### New files

* `AglRenderTarget.h/.cpp`: `AglRenderTarget` (pool object 0x730, the game's "view"):
  camera/view matrices, projection types (ortho / perspective / oblique), viewport,
  fog layers, render callbacks, post-process chain, the render copy used by the
  render thread (`CopyToRenderCopy` 0x005109B0, `UpdateAll` 0x00511F8C, now called
  by `EndFrame`), `UpdateMatrices` 0x005115F0, frustum planes 0x004FDEA0, view
  constant block (0x010DD780). Replaces the placeholder in `AglResources.h`.
  Vtable: 0 dtor, 1 deleting dtor, 2 Release, 4 OnProjectionUpdated (stereo,
  0x005483EC), 5 ApplyViewport, 6 SetOrthoProjection, 7 RunPostProcess.
* `AglPostProc.h/.cpp`: the ten-effect chain (0x0051F850) and the effect classes
  whose constructors/setup are known (screen warp grid, motion trail, thermal vision,
  fading effects, god rays / motion trail tuning). The pass bodies (`Apply`, e.g.
  0x0053DF98 bloom, 0x00540D1C DoF, 0x00545A38 warp, 0x005437F0 god rays, 0x00544870
  motion trail, 0x0054664C thermal vision) are still TODO. `AglPostProcShaders` gained
  the thermal vision, DoF, bloom noise and dark screen shader sets.
* `AglDrawList.h/.cpp`: the packet stream at 0x010DDBB4 (set projection/view/world,
  material, texture, scissor, state byte, callback, immediate primitive). The packet
  reader (render thread replay) was not found; the PC format is its own.
* `AglOcclusion.h/.cpp`: the 16 occlusion queries (0x010DFAD0) incl. the EndFrame
  helpers 0x00548938 and 0x00548A10. Reports use `Device::GetReport`.
* `AglPrimitives.h/.cpp`: vertex declarations (`AglVertexDecl`, 0x80 bytes), sphere
  mesh, `CreateSubMesh` 0x0052374C, debug meshes.
* `AglTexturePaths.cpp`, `AglLoaders.cpp`: texture search paths, loose texture files,
  IFF chunk handlers (texture header/file/default, topology header, include, index
  buffer, bone table).
* `AglStencilShadow.h/.cpp`, `AglShadowMap.h/.cpp`: stencil shadow geometry/settings,
  shadow cascade settings and the shadow map base object (fields mostly unnamed).

### Changes to earlier files

* `AglSubMesh::Lock` (0x004FCE70) was a misreading: 0x6D is the vertex stream count;
  renamed `AddVertexStream`. Submesh fields 0x28/0x40/0x44 named (vertex count, index
  count, vertex declaration). `AglTopology` got its mesh list, bounds, LOD and name.
* `AglRenderSurface` now has a destructor (0x0051AD90). NOTE: the console releases the
  surfaces of texture wrappers too; here texture wrappers skip that because the
  texture (or the caller, for shared depth buffers) owns those surfaces.
* `AglState`: `InvalidateRenderState` (0x0051279C) and render-loop caches;
  `BindRenderSurface` (0x0053B668) is a simplified untagged version (stereo eye
  buffer selection not done).
* `AglDisplay`: display settings struct, stereo settings (0x00536958..0x00536A38).
  0x00536290 (stereo projection offset, a utility routine with its own parameter
  checks) is approximated untagged.
* `AglMaterial`: CopyFrom 0x00509090, Clone 0x0050956C, built-in materials 0x0050F0F0,
  blend mode classification 0x0050061C.

### Facts for other modules

* The shared resource id counter 0x01164CD0 is used by render targets too.
* IFF reader flag bit 2 (console reader +0xC & 4) marks a "resolve only" pass: texture
  chunks look textures up instead of creating them. Bit 1 means the index data is
  byte swapped. `ape::iff::IffReaderFlags` could name both.
* 0x0055E940 (plane from a matrix, used for the oblique projection) and 0x00536290 are
  outside/unfinished; `UpdateMatrices` uses a stand-in for the oblique clip plane.
* 0x00F8F118 (+8 function, +0xC user data) is a "texture created" hook.

### Not done / uncertain

* BeginRender 0x00521D20 / EndRender 0x00522140 (draw buckets, job lists), the draw
  list reader, 0x00527708 (query sphere draw, listing truncated), the particle system
  (0x004FF4C0.., 0x005019B8.., property chunk loaders 0x005286BC..0x0052A7C0: their
  field names are in the strings, the consumer objects are not modelled), mesh loaders
  0x00524B68 / 0x0051C704 / 0x0051CD48, display/video mode set-up 0x0052FD2C /
  0x0053AC38, and the post-process pass bodies.
* Names guessed: `AglShadowSettings::depthBias`, the stereo layout values,
  `SetRenderSettingInt/Float` (0x00527650/0x0052765C, globals 0x00F66FC8/0x00F66FCC),
  `AglRenderHook` (0x00F66FD0).
* Build: `src/agl` compiles warning-free. The full link currently fails only because
  `src/sly/levels/script/CommandsL.cpp` (another agent's work) does not compile.

## agent1 additions
* World-range shader classes: SelectTechnique of Debug (skinned -> "_SK"),
  SimpleColorTxr(UV), TextureWarp (lit pair), ScreenWarp, LineOutline, and
  CreateTechniques of TextureWarp, LineOutline and Outline (four variants:
  Outline, Outline_SK, OutlineNS, OutlineNS_SK). The technique Draw bodies
  (0x005ADA18, 0x005ADD54, 0x005ADEE4, 0x005AE0F0, 0x005AE9A0/0x005AEB00,
  0x005AF518/0x005AF730, 0x005AFBF8) are still open.
* "TBLK" (0x00542630, slot 0) and "MBLK" (0x00548C00, slot 1) chunks copy raw
  data into memory class 4; base class ApeLoadedData (vtable 0x00B24110) is
  in ape/sys/ApeIff.h. The TBLK registration lives in 0x00542704 together
  with the not yet ported TGTF/DXTN (DDS) handlers 0x005399CC/0x00539C7C.
* AglDrawList gained SetWorldMatrix(Matrix34) and EndPrimitive (0x00523748).

## agent1 pass 3
* TGTF (0x005399CC) and DXTN (0x00539C7C) texture image chunks done. The registration
  function 0x00542704 (also the texture system init 0x0051DF3C and two textures from the
  display images 0x010F0E74/0x010F0E84) is not ported. NOTE: the console DXTN handler uses the
  chunk argument as the texture, the other texture chunks read it through the argument.
* `FlushRendering` 0x005126E4 (render thread wait, frame pacing 0x0053BF70 stub, drain of the
  deferred destruction queue). Material create hook 0x00F8F114 (+8/+0xC) added
  (`SetMaterialCreateHook`); nothing runs it until the material chunk handlers exist.
* Technique Draw functions: the render item starts with the world matrix; its unused fourth
  lanes hold the submesh (+0x0C), material (+0x1C), light set (+0x2C); +0x3C is the bone
  palette (0x30 bytes per bone) and +0x40 an optional RGBA colour. A typical Draw (Debug,
  0x005ADA18) multiplies the matrix by the view-projection (0x010DD7C0), uploads it
  transposed to vertex constants 0..3, sets constant 4 and calls the submesh's virtual Draw
  (platform submesh vtable 0x00B23EC0 slot 4 = 0x00538B44: bone upload to register 0x40,
  vertex attributes, fragment program commit, draw arrays/indexed). None are ported yet.
* Done since: `AglSubMesh::Draw` (0x00538B44; the platform path 0x005382C0 is a stub),
  technique Draws of Debug, PrimTxr, SimpleColorTxr, GeoParticle, SprParticle,
  ProjTxrDeferred, ZPass and Outline (outline params at item +0x40: RGBA + width), the draw
  fog helpers 0x0050CF50/0x0050D0E0 (vertex constant 0x20, fog colour fragment constant).
  `AglRenderState` cache fields renamed: 0x010F10A8 boundVertexBuffer, 0x010F10B0
  boundVertexDecl, 0x010F10B4 vertexProgram (the bound technique's AglVertexProgram),
  0x010F10C0 meshAttributes, 0x010F1178 fogLayer, 0x010F117C fogEnabled[2]. The material
  pointer at +0x7C is the constants block (GeoParticle reads constants[3]).
