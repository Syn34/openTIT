// Guest path -> host path translation.
//   /dev_bdvd/...  -> <game_root>/...              (your disc dump, read-only)
//   /app_home/...  -> <game_root>/PS3_GAME/USRDIR/...
//   /dev_hdd0/...  -> <hdd0>/...                   (saves, caches; created on demand)
//   /dev_hdd1/...  -> <hdd0>/../dev_hdd1/...
#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace opentit::vfs {

std::optional<std::filesystem::path> translate(const std::string& guest_path);

}  // namespace opentit::vfs
