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
* The `AglSubMesh/AglTopology/AglMaterial/AglRenderTarget` contents are
  placeholders; only Create/Destroy/Release are reimplemented.
* `AglTexture::sampler[7]` defaults are copied from the constructor. Their meaning is unknown.
* Console-only mechanisms were dropped or turned into bookkeeping: memory tiles and
  z-cull (`SurfaceBindTile/BindZcull` keep the region indices only), jump-patching of
  command buffers (0x00537D5C/0x00537DD4), and the command ring buffer (0x00537F28,
  0x0052F670, 0x0052F780).
* `AglRenderThread.cpp` (EndFrame, 0x005124F4) was not changed. EndFrame calls
  `ProcessDeferredDestruction` (0x00512148), which is now in `AglDeferred.cpp`.

## Build state seen while working

The full link was failing because of files outside `src/agl`: missing
`ape::world::Matrix34Mul/Matrix34FromBasis`, and `ape::FileData` in
`sly/game/AudioSystem.cpp`. All `src/agl` sources build with no warnings
(`cmake --build ... -- -k`).
