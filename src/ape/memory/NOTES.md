# ape/memory notes (range 0x0054E000-0x00558000)

## Layout
- `ApeMem.*` - memory classes, pools, `SysAlloc`/`SysFree`/`SysRealloc`
  (0x0056C514/0x0056C730/0x0056C89C, outside the range but untagged elsewhere and
  part of this system), `MemAlloc`/`MemFree`, zlib callbacks, `StrDup`, stats.
- `ApeMemStack.*` - global 512 KiB scratch stack (`GetMemStack`).
- `ApeAllocator.*` - allocator interface + default/bucket/stack singletons.
- `ApeTArray.*` - growable POD array used by engine containers.
- `ApeTFreeList.*` - pooled element allocator, `ApeTListAllocator<T>`.
- `ApeBucketAlloc.*` - `ApeMem::_BucketAlloc` size-class pools.

## Not memory, but in this address range
These were implemented here because the range was assigned to this directory.
Suggest moving them (the lead decides):
- `ApeMatrixOps.*`, `ApeBounds.*` -> `src/ape/math/` (they use `ape/math/ApeMath.h` types).
- `ApeDebugPrim.*` -> the ApeDebug module when one exists.

## Left untagged on purpose
- 0x0054EE9C, 0x0054F6FC, 0x0054F7A8, 0x0054FDF4, 0x005504F4, 0x00550690:
  statically linked dlmalloc 2.8 (mspace_free, malloc-failure/mparams init,
  tmalloc_large, mspace_malloc, init_user_mstate, internal_memalign). The PC
  build replaces them with the host allocator (same treatment as the SN libc).
  Their callers 0x00565BD0/0x00565C50 (heap wrappers for memory classes 4/5)
  are outside this range; SysAlloc/SysFree cover their effect.
- 0x00551540, 0x00555CF4, 0x00555D48, 0x00555D70, 0x00555DE8, 0x00555EB0,
  0x00555FCC, 0x00555FF0, 0x00556014, 0x005565A0, 0x005565F0, 0x005566A8,
  0x005566CC, 0x005566FC, 0x0055672C: thin C-string wrappers (sprintf, snprintf,
  vsnprintf, stricmp, ASCII stricmp/strnicmp returning -1/0/1, strstr, strchr,
  strrchr, strlwr, strpbrk-like, strcat, atoi, atof, strtol). The owner of
  `ape/core/ApeString.cpp` is already tagging neighbours (0x00555D94, 0x00555DB8),
  so they are left to that file to avoid duplicate tags.
- 0x0054E120, 0x0054E30C, 0x0054E7D8: static init/teardown of renderer
  post-processing globals (not memory).
- 0x0054EC94: empty destructor of a second compact array type (u16 count/elem
  size, initialised by 0x00558F38, outside the range).
- 0x005541F0: AltiVec Matrix34 multiply (in-place, uses Cell lvlx loads plus
  stack reads I could not account for) - not done.
- 0x00557A90: AltiVec sphere/frustum style test, no callers - not done.
- 0x00557C80 / 0x00557CFC: ref-counted packed parameter block (create/release);
  the type alignment table at 0x00F88714 belongs to the renderer.

## Uncertain identifications
- Memory class meaning: only 0 (default), 3 (scratch stack buffer) and 4/5
  (the two dlmalloc mspaces at 0x0116B340 / 0x0116B328) behave differently.
- `ApeArrayBase::m_flags` bit 2 and a flag bit in ApeFreeList (+0x36, not
  reproduced) are set/cleared but never read in this range.
- `ApeMemStack::m_upperLimit` (field +0x10) is only written by Reset().
- MemAlloc (0x0054ED38) sets two bytes at 0x011648A0/0x011648A1 on first use; their
  meaning is unknown and they are not reproduced.
- 0x00553FF0 vs 0x00555370: identical 3x3 products, the first also zeroes the
  translation row (names are mine).

## Behaviour differences (intentional)
- Pools have no fixed capacity on PC; `SetPoolBudget` gives the stats a budget.
- `ApeArrayBase::SetCapacity` clamps a shrink request to the element count
  before sizing the new block (the console sized it from the raw request).
- `ApeFreeList` passes the real block size to `ApeAllocator::Free` (console: 0).
- `BucketFree(nullptr, n)` is a no-op (console would crash).
- Global operator new/delete (0x007A3DD8 etc.) is not overridden; engine objects
  that need engine memory derive from `ape::ApeMemObject`.
