#include "loader/nid.h"

#include <cstdio>
#include <mutex>
#include <unordered_map>

#include "common/sha1.h"

namespace opentit::loader {

namespace {
struct NameEntry {
    const char* library;
    const char* name;
};

constexpr NameEntry kNames[] = {
#define X(lib, fn) {lib, fn},
#include "loader/nid_names.inc"
#undef X
};

const std::unordered_map<u32, const char*>& table() {
    static std::unordered_map<u32, const char*> map;
    static std::once_flag once;
    std::call_once(once, [] {
        for (const auto& e : kNames) map.emplace(compute_nid(e.name), e.name);
    });
    return map;
}
}  // namespace

u32 compute_nid(std::string_view name) {
    static constexpr u8 kSuffix[16] = {0x67, 0x59, 0x65, 0x99, 0x04, 0x25, 0x04, 0x90,
                                       0x56, 0x64, 0x27, 0x49, 0x94, 0x89, 0x74, 0x1A};
    std::string buf(name);
    buf.append(reinterpret_cast<const char*>(kSuffix), sizeof(kSuffix));
    const auto h = sha1(buf.data(), buf.size());
    return u32(h[0]) | (u32(h[1]) << 8) | (u32(h[2]) << 16) | (u32(h[3]) << 24);
}

const char* nid_name(u32 nid) {
    const auto& t = table();
    auto it = t.find(nid);
    return it == t.end() ? nullptr : it->second;
}

std::string nid_display_name(std::string_view library, u32 nid) {
    if (const char* n = nid_name(nid)) return n;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "_%08X", nid);
    return std::string(library) + buf;
}

}  // namespace opentit::loader
