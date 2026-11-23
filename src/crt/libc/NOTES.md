# Notes for integration (src/crt/libc)

* `docs/functions.tsv` boundaries that hide separate functions (tags were put
  on the listed addresses only):
  * 0x0071EFA4 (strtok) also contains the strtol worker entered at 0x0071F088.
  * 0x0071B260 (atexit table growth) also contains the atexit/`__cxa_atexit`
    core at 0x0071B2EC (0x0071B44C atexit branches there).
  * 0x007206E4 (`_Scanf`) also contains the strtod worker at 0x00720CAC
    (0x00721044 strtod branches there).
  * 0x0072104C also contains the strtoll worker at 0x00721548.
  * 0x00728608 (mspace_malloc) also contains mspace_memalign at 0x0072A178;
    0x0072A470 is only `b 0x0072A178`, followed by the realloc core at 0x0072A474.
* Start-up / shutdown hooks that `src/crt/Startup.cpp` may want to call:
  `crt::InitSysLocks()` (0x00722D68, called from 0x004FC8BC on the console;
  until then the library locks are no-ops, as on the console),
  `crt::CloseAllFiles()` (0x0071B704) and `crt::DestroySysLocks()` (0x00722E60).
  `crt::AtExit` registers `crt::RunAtExitHandlers` with the host `std::atexit`
  on first use, so handlers run when `plat::ExitProcess` calls `std::exit`.
* `plat::File` has no read-write mode, so `crt::Open` maps O_RDWR without
  O_CREAT/O_TRUNC/O_APPEND ("r+") to read-only. A `Mode::ReadWrite` in the
  platform layer would fix that.
* `fprintf` (0x00723FEC) and `fscanf` (0x0071BD94) lock stdout's / stdin's
  stream lock rather than the stream they are given. That looks like a quirk of the
  console library; host streams lock themselves.
