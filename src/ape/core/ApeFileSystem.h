// openTIT - Ape engine file system front end (archives, RAM disk).
#pragma once

namespace ape {

struct Archive;   // opened .zip/.psarc container (not yet reconstructed)
class RamDisk;    // in-memory file device fed from an archive

bool FileExists(const char* path);
bool MountArchive(const char* path, int priority, int arg2, int arg3);
Archive* OpenArchive(const char* path, void* outInfo);
void CloseArchive(Archive* archive);
RamDisk* CreateRamDisk(Archive* source, int flags, const char* deviceName);
void DestroyRamDisk(RamDisk* disk);

} // namespace ape
