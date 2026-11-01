#include "vfs.h"

#include <system_error>

#include "log.h"
#include "runtime.h"

namespace opentit::vfs {

namespace fs = std::filesystem;

namespace {

bool starts_with(const std::string& s, const char* prefix, std::string* rest) {
    const std::size_t n = std::char_traits<char>::length(prefix);
    if (s.compare(0, n, prefix) != 0) return false;
    if (s.size() > n && s[n] != '/') return false;
    *rest = s.size() > n ? s.substr(n + 1) : std::string();
    return true;
}

// Case-insensitive resolution so a dump extracted on a case-sensitive file
// system still works when the game asks for different capitalisation.
fs::path resolve_case(const fs::path& base, const std::string& rel) {
    fs::path cur = base;
    std::size_t pos = 0;
    while (pos <= rel.size()) {
        const std::size_t slash = rel.find('/', pos);
        const std::string part = rel.substr(pos, slash == std::string::npos ? std::string::npos : slash - pos);
        pos = slash == std::string::npos ? rel.size() + 1 : slash + 1;
        if (part.empty() || part == ".") continue;
        fs::path next = cur / part;
        std::error_code ec;
        if (!fs::exists(next, ec) && fs::is_directory(cur, ec)) {
            for (const auto& e : fs::directory_iterator(cur, ec)) {
                const std::string name = e.path().filename().string();
                if (name.size() == part.size() &&
                    std::equal(name.begin(), name.end(), part.begin(), [](char a, char b) {
                        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
                    })) {
                    next = e.path();
                    break;
                }
            }
        }
        cur = next;
    }
    return cur;
}

}  // namespace

std::optional<fs::path> translate(const std::string& guest) {
    const auto& cfg = rtm::config();
    std::string rest;
    if (starts_with(guest, "/dev_bdvd", &rest)) return resolve_case(cfg.game_root, rest);
    if (starts_with(guest, "/app_home", &rest)) return resolve_case(fs::path(cfg.game_root) / "PS3_GAME" / "USRDIR", rest);
    if (starts_with(guest, "/dev_hdd0", &rest)) {
        std::error_code ec;
        fs::create_directories(cfg.hdd0, ec);
        return resolve_case(cfg.hdd0, rest);
    }
    if (starts_with(guest, "/dev_hdd1", &rest)) {
        const fs::path hdd1 = fs::path(cfg.hdd0).parent_path() / "dev_hdd1";
        std::error_code ec;
        fs::create_directories(hdd1, ec);
        return resolve_case(hdd1, rest);
    }
    LOG_WARN("vfs", "unmapped guest path '%s'", guest.c_str());
    return std::nullopt;
}

}  // namespace opentit::vfs
