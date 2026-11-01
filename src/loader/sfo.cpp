#include "loader/sfo.h"

#include <fstream>
#include <iterator>
#include <vector>

namespace opentit::loader {

namespace {
template <typename T>
T le(const std::vector<u8>& d, std::size_t o) {
    T v{};
    std::memcpy(&v, d.data() + o, sizeof(T));
    return v;  // host is assumed little endian (x86-64 / arm64)
}
}  // namespace

std::optional<std::map<std::string, SfoValue>> read_sfo(const std::string& path, std::string* error) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        if (error) *error = "cannot open " + path;
        return std::nullopt;
    }
    std::vector<u8> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (d.size() < 20 || std::memcmp(d.data(), "\0PSF", 4) != 0) {
        if (error) *error = "not a PARAM.SFO";
        return std::nullopt;
    }
    const u32 key_table = le<u32>(d, 8);
    const u32 data_table = le<u32>(d, 12);
    const u32 count = le<u32>(d, 16);
    std::map<std::string, SfoValue> out;
    for (u32 i = 0; i < count; ++i) {
        const std::size_t e = 20 + std::size_t(i) * 16;
        if (e + 16 > d.size()) break;
        const u16 key_off = le<u16>(d, e);
        const u16 fmt = le<u16>(d, e + 2);
        const u32 len = le<u32>(d, e + 4);
        const u32 data_off = le<u32>(d, e + 12);
        const std::size_t k = key_table + key_off;
        const std::size_t v = data_table + data_off;
        if (k >= d.size() || v + len > d.size()) continue;
        std::string key(reinterpret_cast<const char*>(d.data() + k));
        if (fmt == 0x0404) {
            out[key] = le<u32>(d, v);
        } else {
            std::string s(reinterpret_cast<const char*>(d.data() + v), len);
            while (!s.empty() && s.back() == '\0') s.pop_back();
            out[key] = s;
        }
    }
    return out;
}

}  // namespace opentit::loader
