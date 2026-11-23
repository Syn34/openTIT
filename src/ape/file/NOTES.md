# ape/file: notes for the lead

## Switching the stubs in `src/ape/core/ApeFileSystem.cpp`

The stubs there can now forward to this module (`#include "ape/file/ApeFile.h"`,
`"ape/file/AsyncFile.h"`). Some of the names in `ApeFileSystem.h` are misleading. Here is
what the original functions actually do:

| stub | address | real function | forward to |
|---|---|---|---|
| `FileExists(path)` | 0x0055D78C | quiet open "rb" + Close | `ape::file::FileExists(path)` |
| `MountArchive(path, 1, 0, 0)` | 0x00562944 | opens the .psarc on the FIOS device (which mounts it) | `ape::file::MountDeviceArchive(path, params) != nullptr`, where `params.flags` is the high word of arg 2 (0 here) and `params.urgent = (low word == 1)` |
| `OpenArchive(path, &info)` | 0x00560B90 | **LoadFile**: reads the whole file into a new buffer, `info` gets the size (u32) | `ape::file::LoadFile(path, &size)` |
| `CloseArchive(p)` | 0x005611B0 | frees that buffer | `ape::file::FreeFileData(p)` |
| `CreateRamDisk(buf, size, "RamDisk")` | 0x00567034 | **CreateMemoryFile(data, size, name)**: returns null when data is null or size is 0 | `ape::file::CreateMemoryFile(buf, size, name)` |
| `DestroyRamDisk(f)` | (vslot 2) | `File::Close()`, which also deletes the object | `f->Close()` |

**Bug in `SlyMain.cpp`:** it calls `CreateRamDisk(shaders, 0, "RamDisk")`. The disassembly
at 0x00463A54 loads r4 from the `&info` stack slot that 0x00560B90 filled in, so the second
argument is the file size. Ghidra lost it. With 0, the function returns null.

`ape::Archive` / `ape::RamDisk` are really "file data buffer" (`void*`) and
`ape::file::File`.

Not wired anywhere yet: `ape::file::MountImageArchive()` (0x0055D830). The original calls it
from the file system init at 0x0055DC18, which I did not reconstruct (see below).

## Layout of the module

* `ApeFile.*`: `File` base class (vtable: Close, Read, Write, Seek, GetSize), `MemoryFile`,
  `OpenFile`/`FileExists`/`LoadFile`, path helpers, quiet-probe scope, Image.zip mount.
* `ZipArchive.*`: the engine's zip reader (128-bucket name index, case-insensitive lookups,
  stored and deflate entries, ".sca" archives with an XOR-scrambled EOCD signature) and
  `ArchiveFile`.
* `HostFile.*`: device files with a 2 x 32 KiB read-ahead ring and a 32 KiB write buffer.
* `AsyncFile.*`: the APE_ASYNC_FILE_CMD_* request layer. On PS3 it wraps Sony FIOS. Here a
  single I/O worker thread runs the requests against `plat::File` or mounted PSARC entries.
  Error codes keep the CELL_FIOS values.
* `Psarc.*`, `FileStream.h`: PSARC reader (zlib blocks, MD5-matched manifest) and the
  positional streams used by the worker.
* `Inflate.*`, `Md5.*`: self-contained DEFLATE decoder and MD5. Neither is tagged: zlib sits
  at 0x5D8000+, outside this range.

## Deliberate deviations (marked NOTE in the code)

* Opens are looked up in mounted PSARCs before the host file system (newest mount first).
  FIOS precedence is unknown.
* `ArchiveFile::Close` deletes the object. The original leaked it.
* A backward seek in an `ArchiveFile` restarts decoding. The original kept reading from the
  decoder position and returned wrong data.
* `ArchiveFile::GetSize` uses the entry's own size, not the archive's "current entry".
* `ZipArchive::Open` fails cleanly when the archive file cannot be opened. The original
  dereferenced a null stream.
* `LoadFile` closes the file when the allocation fails. The original leaked it.
* `SEEK_END` computes `size - offset` in every file type. This matches the original and is
  kept as is.
* Debug prints whose formatting survives in the binary ("%s - Open", "%s - Seek ...") are not
  emitted. Request errors and archive writes are logged with `plat::Log(Warning)`.
* Buffers use `malloc`/`free`, not the engine heaps (0x0056C514 / 0x0056C730, memory agent).

## Not done (in range, file related)

* 0x0055DC18 file system / resource init (registers FourCC handlers through 0x0055902C, then
  calls 0x0055DA00, 0x00570E08 and 0x0055D830).
* 0x00570E08 FIOS stack init (media, cache, dearchiver). Here the scheduler starts lazily.
* 0x0055BE90 controller.bin record/playback (mode 1 = "wb", 2 = "rb"). It also forces the
  frame timer to 1/60 s, and that object lives elsewhere.
* 0x0055DA00 ("WaitStart"/"WaitComplete" objects), 0x005721B4 (returns a FIOS global).
* 0x0056C8E4 cellGameBootCheck (content root used by 0x00570604 and 0x005706FC). Probably
  belongs with sysutil.
* Outside the range but implemented untagged: 0x007A5840 (read-cache read), 0x007A5684
  (read-ahead), 0x007A5B58 (buffered write), 0x007A4DF0 (MemoryFile::GetSize), the zlib
  inflate and crc32 routines.
