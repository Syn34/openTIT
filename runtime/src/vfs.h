// Guest path -> host path translation.
//   /dev_bdvd/...  -> <game_root>/...              (your disc dump, read-only)
//   /app_home/...  -> <game_root>/PS3_GAME/USRDIR/...
//   /dev_hdd0/...  -> <hdd0>/...                   (saves, caches; created on demand)
//   /dev_hdd1/...  -> <hdd0>/../dev_hdd1/caches/<TITLE_ID>_<cache id>/... (after cellSysCacheMount)
#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace opentit::vfs {

std::optional<std::filesystem::path> translate(const std::string& guest_path);

// Point /dev_hdd1 at caches/<dir> (cellSysCacheMount). Returns true if it already existed.
bool mount_hdd1(const std::string& dir);

}  // namespace opentit::vfs
