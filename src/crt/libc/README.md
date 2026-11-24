# Statically linked C runtime (`src/crt/libc`)

Sly Cooper: Thieves in Time links its C library statically: a Dinkumware
derived libc as shipped in the PS3 SDK (SN Systems toolchain), with Sony's
PPU-optimised string routines, a dlmalloc `mspace` heap, a small file
descriptor layer over `cellFs`/TTY, and the SPU printf server. It occupies
roughly `0x0071A200`-`0x0072E000` of the executable (and continues past it).

On the PC the host C/C++ runtime is the equivalent of this library. Every
positively identified routine therefore has a small named function in
namespace `crt` (declared in `Libc.h`):

* standard functions forward to the host standard library;
* internal helpers (locks, printf output sinks, scanf input sources, the
  qsort introsort pieces, calendar arithmetic, number parsing, the fd
  layer) have clean implementations of their own, built on the host
  runtime where that subsumes them.

Behavioural notes:

* The PPU ABI is ILP32, so `long` is 32 bits on the console. `StrToL` and
  `StrToUL` return `int32_t`/`uint32_t` with 32-bit range checks.
* Error codes are host `errno` values. The console uses Sony's
  `0x8001xxxx` codes (e.g. `ERANGE` = `0x8001001C`, `EBADF` = `0x8001002A`).
* Memory from `crt::Malloc`/`MemAlign`/`Realloc` must be released with
  `crt::Free` (blocks carry a header so aligned blocks can be freed too).
* `FOpen` and `Open` take game paths and translate them via `plat::ToHostPath`.
* fd 0/1/2 are the console (stdin/stdout/stderr); other descriptors are
  `plat::File` objects.

## Address map

| Address | crt:: function | Standard function / role | File |
|---|---|---|---|
| 0x0071AECC | FormatHex64 | (abort helper: 64-bit value to 16 hex digits) | Stdlib.cpp |
| 0x0071AF0C | Abort | abort | Stdlib.cpp |
| 0x0071B054 | Abs | abs | Stdlib.cpp |
| 0x0072F5F4, 0x00730614 | Div | div (two copies) | Stdlib.cpp |
| 0x0071B068 | AppendExitHandler | (atexit table: append entry) | Stdlib.cpp |
| 0x0071B094 | RunAtExitHandlers | (exit: run atexit handlers) | Stdlib.cpp |
| 0x0071B260 | GrowExitHandlerTable | (atexit table: grow) | Stdlib.cpp |
| 0x0071B44C | AtExit | atexit | Stdlib.cpp |
| 0x0071B458 | BSearch | bsearch | Stdlib.cpp |
| 0x0071B560 | ErrnoLocation | errno location (_Geterrno) | Locks.cpp |
| 0x0071B5B8 | FClose | fclose | Stdio.cpp |
| 0x0071B704 | CloseAllFiles | (close all streams at shutdown) | Stdio.cpp |
| 0x0071B774 | FFlush | fflush | Stdio.cpp |
| 0x0071B998 | FGetC | fgetc | Stdio.cpp |
| 0x0071BB1C | FOpen | fopen | Stdio.cpp |
| 0x0071BB90 | FRead | fread | Stdio.cpp |
| 0x0071BD94 | FScanf | fscanf | Stdio.cpp |
| 0x0071BE2C | ScanFromStream | (fscanf input callback) | Stdio.cpp |
| 0x0071BE88 | FSeek | fseek | Stdio.cpp |
| 0x0071BEFC | FTell | ftell | Stdio.cpp |
| 0x0071BF58 | FWrite | fwrite | Stdio.cpp |
| 0x0071C1D0 | GmTime | gmtime | Time.cpp |
| 0x0071C220 | LocalTime | localtime | Time.cpp |
| 0x0071C2A4 | MemChr | memchr | String.cpp |
| 0x0071C3F0 | MemCpy | memcpy | String.cpp |
| 0x0071C9B4 | MemMove | memmove | String.cpp |
| 0x0071DC88 | MkTime | mktime | Time.cpp |
| 0x0071DE8C | PutS | puts | Stdio.cpp |
| 0x0071E0D4 | SwapElements | (qsort: swap elements) | Stdlib.cpp |
| 0x0071E218 | SiftDown | (qsort: heap sift-down) | Stdlib.cpp |
| 0x0071E3E8 | SortThreeElements | (qsort: median of three) | Stdlib.cpp |
| 0x0071E540 | MoveElementDown | (qsort: insertion-sort rotate) | Stdlib.cpp |
| 0x0071E63C | IntroSortLoop | (qsort: introsort loop) | Stdlib.cpp |
| 0x0071EC28 | QSort | qsort | Stdlib.cpp |
| 0x0071EC38 | ScanFromString | (sscanf input callback) | Stdio.cpp |
| 0x0071EC78 | SScanf | sscanf | Stdio.cpp |
| 0x0071ECDC | StrError | strerror | String.cpp |
| 0x0071ED08 | StrLen | strlen | String.cpp |
| 0x0071EDAC | StrRChr | strrchr | String.cpp |
| 0x0071EEF8 | StrStr | strstr | String.cpp |
| 0x0071EFA4 | StrTok | strtok | String.cpp |
| 0x0071F284 | StrToL | strtol | Stdlib.cpp |
| 0x0071F28C | UngetC | ungetc | Stdio.cpp |
| 0x0071F36C | VSNPrintf | vsnprintf | Stdio.cpp |
| 0x0071F3DC | WriteToBoundedBuffer | (snprintf/vsnprintf output sink) | Stdio.cpp |
| 0x0071F490 | FloatScale | _FDscale (float ldexp core) | Math.cpp |
| 0x0071F668 | FloatUnscale | _FDunscale (float frexp core) | Math.cpp |
| 0x0071F770 | BytesBetween | (stream position helper) | Stdio.cpp |
| 0x0071F788 | FileGetPos | _Fgpos (ftell/fgetpos core) | Stdio.cpp |
| 0x0071FDA0 | FileReadPrepare | _Frprep (stream read prepare) | Stdio.cpp |
| 0x00720114 | FileSetPos | _Fspos (fseek/fsetpos core) | Stdio.cpp |
| 0x00721044 | StrToD | strtod | Stdlib.cpp |
| 0x00721738 | StrToLL | strtoll | Stdlib.cpp |
| 0x007219D0 | ParseULong | _Stoulx (strtoul core) | Stdlib.cpp |
| 0x00721D18 | StrToUL | strtoul | Stdlib.cpp |
| 0x00721D20 | ParseULongLong | _Stoullx (strtoull core) | Stdlib.cpp |
| 0x00722074 | StrToULL | strtoull | Stdlib.cpp |
| 0x007225FC | StrErrorToBuffer | _Strerror | String.cpp |
| 0x00722C70 | UnlockSysLock | _Unlocksyslock | Locks.cpp |
| 0x00722CBC | UnlockFile | _Unlockfilelock | Locks.cpp |
| 0x00722CEC | LockSysLock | _Locksyslock | Locks.cpp |
| 0x00722D38 | LockFile | _Lockfilelock | Locks.cpp |
| 0x00722D68 | InitSysLocks | _Initlocks | Locks.cpp |
| 0x00722E60 | DestroySysLocks | (destroy library locks) | Locks.cpp |
| 0x00722EBC | DaysToMonth | _Daysto | Time.cpp |
| 0x00722F4C | SecondsToTm | _Ttotm | Time.cpp |
| 0x007231D8 | WcToB | wctob | String.cpp |
| 0x00723518 | WcToMb | _Wctomb / wcrtomb | String.cpp |
| 0x0072354C | StrCat | strcat | String.cpp |
| 0x00723678 | StrChr | strchr | String.cpp |
| 0x00723884 | StrCmp | strcmp | String.cpp |
| 0x007239D8 | StrCpy | strcpy | String.cpp |
| 0x00723AC4 | StrNCmp | strncmp | String.cpp |
| 0x00723C88 | StrNCpy | strncpy | String.cpp |
| 0x00723F2C | StrCaseCmp | strcasecmp / stricmp | String.cpp |
| 0x00723FEC | FPrintf | fprintf | Stdio.cpp |
| 0x00724084 | WriteToStream | (stream printf output sink) | Stdio.cpp |
| 0x007240F4 | Printf | printf | Stdio.cpp |
| 0x00724184 | WriteToStream | (stream printf output sink) | Stdio.cpp |
| 0x007241F4 | SNPrintf | snprintf | Stdio.cpp |
| 0x00724280 | WriteToBoundedBuffer | (snprintf/vsnprintf output sink) | Stdio.cpp |
| 0x00724304 | SPrintf | sprintf | Stdio.cpp |
| 0x00724380 | WriteToBuffer | (sprintf/vsprintf output sink) | Stdio.cpp |
| 0x007243BC | VFPrintf | vfprintf | Stdio.cpp |
| 0x00724430 | WriteToStream | (stream printf output sink) | Stdio.cpp |
| 0x007244A0 | VPrintf | vprintf | Stdio.cpp |
| 0x00724518 | WriteToStream | (stream printf output sink) | Stdio.cpp |
| 0x00724588 | VSPrintf | vsprintf | Stdio.cpp |
| 0x007245E4 | WriteToBuffer | (sprintf/vsprintf output sink) | Stdio.cpp |
| 0x00724620 | FormatToSink | _Printf (printf engine front end) | Stdio.cpp |
| 0x00725E14 | Time | time | Time.cpp |
| 0x00725E88 | PrintAssertFailure | (assertion failure message) | Stdlib.cpp |
| 0x00725F84 | AssertFailTrap | (assertion failure + trap) | Stdlib.cpp |
| 0x00725FA8 | AssertFailAbort | (assertion failure + abort) | Stdlib.cpp |
| 0x00726878 | LSeek64 | lseek (64-bit) | FileDesc.cpp |
| 0x00726900 | Close | close | FileDesc.cpp |
| 0x00726964 | Open | open | FileDesc.cpp |
| 0x007269FC | LSeek | lseek | FileDesc.cpp |
| 0x00726A88 | Read | read | FileDesc.cpp |
| 0x00726CB0 | Write | write | FileDesc.cpp |
| 0x00726E54 | MemSet | memset | String.cpp |
| 0x00726F28 | MemCmp | memcmp | String.cpp |
| 0x00727080 | MutexUnlock | _Mtxunlock | Locks.cpp |
| 0x007270C4 | MutexLock | _Mtxlock | Locks.cpp |
| 0x0072716C | MutexDestroy | _Mtxdst | Locks.cpp |
| 0x007271C0 | MutexInit | _Mtxinit | Locks.cpp |
| 0x0072A88C | MemAlign | memalign | Stdlib.cpp |
| 0x0072A8C4 | Realloc | realloc | Stdlib.cpp |
| 0x0072A8FC | Free | free | Stdlib.cpp |
| 0x0072A928 | Malloc | malloc | Stdlib.cpp |
| 0x0072A9A4 | FPutC | fputc | Stdio.cpp |
| 0x0072AA98 | FPutS | fputs | Stdio.cpp |
| 0x0072ACB4 | StrCSpn | strcspn | String.cpp |
| 0x0072AD24 | StrSpn | strspn | String.cpp |
| 0x0072AD98 | DoubleScale | _Dscale (double ldexp core) | Math.cpp |
| 0x0072B878 | OpenForStream | _Fopen (open file for a new stream) | FileDesc.cpp |
| 0x0072D458 | ScanSetContains | (scanf %[ set membership) | Stdio.cpp |
| 0x0072DB48 | LongDoubleScale | _LDscale | Math.cpp |

Several printf output sinks exist as identical copies in the original (one
per printf variant); each copy is tagged on the shared C++ function.

## Identified but not reimplemented

| Address | Identity | Why not |
|---|---|---|
| 0x0071DF48 | qsort helper: rotate three elements | Only used by the original's partition scheme; ours does not need it. |
| 0x0071F978 / 0x0071FA10 / 0x0071FAF8 | `_Fofind` / `_Fofree` / `_Foprep` (allocate, free, open a FILE) | Host `fopen`/`fclose` own FILE objects. |
| 0x00720544 | `_Fwprep` (prepare stream for writing) | No host equivalent short of writing. |
| 0x007206E4 | `_Scanf` engine head (also holds the `strtod` worker at 0x00720CAC) | Host `vfscanf`/`vsscanf`/`strtod`. |
| 0x0072104C / 0x00721740 / 0x0072207C | `strtod` digit scanners (decimal/hex, also hold the `strtoll` worker at 0x00721548) | Host `strtod`. |
| 0x007231A8 | va_list copy helper | Pure ABI glue (`va_copy`). |
| 0x00724CE4 | `_Putfld` (printf conversion dispatcher) | Host printf. |
| 0x00725FC0-0x007265F0 | SPU printf server (attach/detach group/thread, init, finalize, service thread) | SPU support, no PC equivalent. |
| 0x00726DEC | init of a 256-entry descriptor table | Purpose not established. |
| 0x00726E50 | empty function called during start-up/shutdown | Identity not established. |
| 0x00727198 / 0x00727214 | heap lock destroy / create | Host heap is thread safe. |
| 0x00727250-0x0072A744 | dlmalloc mspace internals (`segment_holding`, `init_top`, `init_bins`, malloc/free/realloc/memalign bodies, `create_mspace`) | Host allocator. |
| 0x0072A958 / 0x0072A97C | heap shutdown / initialisation | Host allocator. |
| 0x0072AFE4, 0x0072B094-0x0072B750 | extended-precision helpers for `strtod` (unpack, multiply, add, subtract, scale by 10^n, pack) and `_FDnorm` | Host `strtod`. |
| 0x0072B8E8 / 0x0072BCF8 / 0x0072CD10 / 0x0072D504 | scanf field converters (`_Getfld`, `_Getfloat`, `_Getint`, `_Getstr`) | Host scanf. |
| 0x0072DB70 | printf/scanf floating-point helper (exact identity unconfirmed) | Host printf/scanf. |
| 0x0071A080-0x0071A214 | WwsJob named-object registry, SPU job image accessors and the job exception handler printout | SPU job manager support, not libc. |

## C++ runtime support (`CxxRuntime.cpp`)

| Address | Name | Original | Notes |
|---|---|---|---|
| 0x007315EC | OperatorDelete | `operator delete` | `crt::Free`, null ignored. |
| 0x00731620 | OperatorNewArrayAligned | `operator new[](size, align)` | Forwards to the scalar version. |
| 0x00731648 | OperatorNewAligned | `operator new(size, align)` | `crt::MemAlign` + new-handler loop; reports bad_alloc and aborts. |
| 0x00731800 | ReportUncaughtException | uncaught exception report | "exception: <what>" then abort. |
| 0x007318A4 | PureVirtualCalled | `__cxa_pure_virtual` | Calls Terminate. |
| 0x00732BCC | MemCpyAligned | aligned block copy | `memcpy`. |
| 0x00732D44 | Terminate | `std::terminate` | Handler, then abort. |
| 0x00732D84 | DefaultTerminateHandler | default terminate handler | Abort. |

The std::exception / bad_alloc classes and the `__cxxabiv1` type_info classes
(0x00731850-0x00732AE4, vtables 0x00F2D1B8-0x00F2D2A0) are the host runtime's
own business and are not reproduced.
