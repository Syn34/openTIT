# ape/input, ape/spline, ape/sys: notes for the lead (range 0x00558000-0x00578000)

## Layout
- `ape/input/PadDevice.h` - the pad backend interface (cellPad replacement). Data layout
  follows CellPadData / CellPadInfo2 so the engine's conversion code is unchanged. Default
  backend: no controller connected. A frontend installs its own with `SetPadBackend()`.
- `ape/input/ApeInputDevice.*` - ApeInputDevice (vtable 0x00B24310), ApePadDevice
  (0x00B244C0), rumble, controller.bin record/playback (written big-endian like the console).
- `ape/input/ApeInputMgr.*` - device pool, CreatePad / Update / Shutdown, PadSystemInit/Shutdown.
- `ape/spline/ApeSpline.*` - cubic Bezier splines (64-byte points, arc-length parameter).
- `ape/sys/ApeFrameClock.*` - minimal stand-in for the global frame clock at 0x011DE094
  (only `deltaTime` +0x04 and `fixedStep` +0x08). Whoever reconstructs that object should
  replace it.
- `ape/sys/ApePlatform.*` - platform init/shutdown/update, system language, sysmodule
  loading, heap A/B budgets, file system shutdown hooks.
- `ape/sys/ApeGameContent.*` - cellGameBootCheck equivalent; reads PARAM.SFO from the data
  root's parent (or the data root) when present.
- `ape/sys/ApeWorker.*` - "ApeWorkerThread_1" job thread (start/complete event flags).
- `ape/sys/ApeIff.*` - IFF chunk handler registry, chunk reader/parser, typed reads,
  include-file handler, loaded-object list.
- `ape/sys/ApeCallBuffer.*` - deferred (func, arg) call buffer.
- `ape/sys/ApeStringPool.*`, `ApeLinearAllocator.*`, `ApeUtil.*` (hash, key sort),
  `ApeJobs.*` (job queue lock, JobMemcpy), `ApeGeometry.*` (planes, camera matrices,
  axis normalisation, frustum tests).

## Wiring suggestions (not done, other owners' files)
- `ape/core/ApeSystem.cpp` SystemInit: the core group should call `PlatformSystemInit()`
  (0x0056D87C); the rest of the core init (0x0055DC18) is listed below.
- `ape/core/ApeSysUtil.cpp`: 0x0056C274 is `SysUtilUnregisterCallback(slot)` =
  lock the sysutil lock, `plat::UnregisterSystemCallback(slot)`, unlock. It needs the
  lock that is private to that file, so I did not implement it.
- `ape/core/ApeSystem.h` EngineConfig: the console stores the game language at
  EngineConfig+0x28 (0x010C5A48); here it is `ape::g_gameLanguage`.
- `ape/file`: 0x0055E350 is `UnmountZipArchive(handle)` (1-based handle into the mounted
  zip list, frees the 128-bucket index and the archive). The Image.zip handle
  (`g_fs.imageArchiveCount`, 0x01164A9C) is unmounted by the core shutdown 0x0055E7C0.

## Not done in the range
- 0x0055DC18 CoreSystemInit: memstack init, mutex 0x01164CD8, `CreateWorkerThreads`,
  FIOS init 0x00570E08, `MountImageArchive`, `PadSystemInit`, `SetControllerRecordMode(0)`,
  registers 'FORM','PLAT','PROF','PRFE','PRFN' with handlers at 0x007A50B4..0x007A5284
  (flags 0x18, outside the range), clears `SetUnknownChunkHandler`/`g_iffReaderFlag`, then
  0x00559310. Everything except the FORM.. handlers and 0x00559310 now exists.
- 0x0055E7C0 CoreSystemShutdown: `ApeInputMgr::Shutdown`, UnmountZipArchive(image
  handle), `PlatformFileSystemShutdown`, `AsyncFileSystemShutdown`, then waits for and
  destroys the workers (`DestroyWorkerThreads` does that part).
- 0x00559310 registers 'ATAG' -> 0x00563808, 'TAG2' -> 0x00564428, 'TAGG' ->
  `TaggChunkHandler` and creates a compact array (cap 32, 8-byte elements). The tag
  handlers (0x00563808, 0x00564428, helper 0x00563368), the ref-counted packed parameter
  block they build (0x00558480 init, 0x00558A48/0x00558B84/0x00558CE8 dtors, 0x00558E24
  share, 0x00577200/0x00577230/0x00577288/0x0057711C subclass) and the tag cleanup
  0x00564C64 are not done.
- SPURS/job chains: 0x0056D3B0, 0x0056D630 (init), 0x0056DC64, 0x0056E048, 0x0056E088,
  0x0056E1F0 (dispatcher ring), 0x0056E4C0..0x0056F430 (SPU job descriptor builders, each
  runs an embedded SPU program; would need CPU ports of the SPU kernels), 0x0056E038,
  0x00572228/0x005721C4 (static init/teardown), 0x0056D0E0.
- Debug draw/font: 0x0056A944, 0x0056AC78, 0x0056AD18, 0x0056AEFC, 0x0056B2B8,
  0x0056B4E8, 0x0056B57C, 0x0056B6A8, 0x0056B93C, 0x00565410, 0x00565550, 0x00569CDC,
  0x00569EC8 (double-buffered ApeDebug::Prim pools at 0x01164E50, renderer dependent).
- dlmalloc heap wrappers 0x00565A88/0x00565AA8/0x00565BD0/0x00565C50 (memory module
  covers their effect through SysAlloc), 0x0056C2E4 (memory module).
- Renderer/scene code 0x00572420..0x00577E40 (except 0x00576E28 and 0x00577680).
- 0x0055E940 and 0x00577D8C: Ghidra could not decode them (bad instruction data).
- 0x00564F10 (ctor of a resource base class, vtable 0x00B24110, dtor outside the range),
  0x0056A70C (empty hook of unknown purpose).

## Uncertain identifications / deviations (marked NOTE in the code)
- Spline evaluation outside the spline's parameter range: the console evaluates
  "segment -1" (reads before the array); the nearer end point is returned instead. The
  reverse search wraps to the last real segment (console: one past it), and a stale
  start segment is clamped.
- `kSplineDefaultTangent` (0x01164B60) is set by a static initialiser I did not find;
  unit Y is a guess.
- ApePadDevice::UpdateRumble: the large motor only runs on the frames where the small
  motor's duty-cycle phase wraps (console behaviour, kept).
- ApeStringPool::Grow with altHeap: the console allocates the old size (overflow); fixed.
- `IsPad()`, `VirtualSlot5/6/10/11` of the input device: empty/constant slots, names guessed.
- System module ids are kept as numbers; only 0x0E (FS), 0x10 (GCM), 0x16 (SYSUTIL_NP)
  are certain.
- GameBootCheck on PC reports DISC type, no attributes, free space of the data root.
- `IffReader` chunk size is the header size minus 4 (as on the console); chunk ids and
  payload integers are little-endian.
- `PlaneIntersectsBox` (0x00577680) is described literally; the caller's convention for
  `center` is not checked.
- 0x005642AC and 0x005698F4 are two separate LE 32-bit reads in the original (integer and
  float destination); they are `ReadU32` and `ReadFloat` here.
