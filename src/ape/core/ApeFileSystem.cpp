// openTIT - Ape engine file system front end.
#include "ape/core/ApeFileSystem.h"
#include "platform/Platform.h"

namespace ape {

// TODO(0x0055D78C FileExists): the original also searches mounted archives.
bool FileExists(const char* path)
{
    return plat::FileExists(path);
}

// TODO(0x00562944 MountArchive)
bool MountArchive(const char* path, int priority, int arg2, int arg3)
{
    return false;
}

// TODO(0x00560B90 OpenArchive)
Archive* OpenArchive(const char* path, void* outInfo)
{
    return nullptr;
}

// TODO(0x005611B0 CloseArchive)
void CloseArchive(Archive* archive)
{
}

// TODO(0x00567034 CreateRamDisk)
RamDisk* CreateRamDisk(Archive* source, int flags, const char* deviceName)
{
    return nullptr;
}

// Released through the RAM disk's virtual destructor (vtable slot 2) on PS3.
void DestroyRamDisk(RamDisk* disk)
{
}

} // namespace ape
