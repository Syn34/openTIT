#include "loader/sfo.h"

#include <algorithm>
#include <cstring>
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

bool write_sfo(const std::string& path, const std::map<std::string, SfoValue>& values,
               const std::map<std::string, u32>& max_len) {
    auto put16 = [](std::vector<u8>& d, std::size_t o, u16 v) { std::memcpy(d.data() + o, &v, 2); };
    auto put32 = [](std::vector<u8>& d, std::size_t o, u32 v) { std::memcpy(d.data() + o, &v, 4); };
    const u32 count = static_cast<u32>(values.size());
    std::vector<u8> keys, data;
    std::vector<u8> index(static_cast<std::size_t>(count) * 16);
    u32 i = 0;
    for (const auto& [k, v] : values) {  // std::map keeps keys sorted, as the format requires
        const std::size_t e = static_cast<std::size_t>(i++) * 16;
        put16(index, e, static_cast<u16>(keys.size()));
        keys.insert(keys.end(), k.begin(), k.end());
        keys.push_back(0);
        const u32 off = static_cast<u32>(data.size());
        if (const u32* n = std::get_if<u32>(&v)) {
            put16(index, e + 2, 0x0404);
            put32(index, e + 4, 4);
            put32(index, e + 8, 4);
            data.resize(data.size() + 4);
            std::memcpy(data.data() + off, n, 4);
        } else {
            const std::string& str = std::get<std::string>(v);
            const bool binary = k == "PARAMS" || k == "PARAMS2";
            const u32 len = static_cast<u32>(str.size()) + (binary ? 0 : 1);
            auto it = max_len.find(k);
            u32 max = it != max_len.end() ? std::max(it->second, len) : (len + 3) & ~3u;
            put16(index, e + 2, binary ? 0x0004 : 0x0204);
            put32(index, e + 4, len);
            put32(index, e + 8, max);
            data.resize(data.size() + max, 0);
            std::memcpy(data.data() + off, str.data(), str.size());
        }
        put32(index, e + 12, off);
    }
    while (keys.size() % 4) keys.push_back(0);
    std::vector<u8> out(20);
    std::memcpy(out.data(), "\0PSF", 4);
    put32(out, 4, 0x00000101);
    put32(out, 8, static_cast<u32>(20 + index.size()));
    put32(out, 12, static_cast<u32>(20 + index.size() + keys.size()));
    put32(out, 16, count);
    out.insert(out.end(), index.begin(), index.end());
    out.insert(out.end(), keys.begin(), keys.end());
    out.insert(out.end(), data.begin(), data.end());
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(out.data()), static_cast<std::streamsize>(out.size()));
    return static_cast<bool>(f);
}

}  // namespace opentit::loader
