// Debug backend: writes the disassembly of every distinct vertex and
// fragment program the game draws with (opentit --dump-shaders <dir>).
#pragma once

#include <functional>
#include <mutex>
#include <set>
#include <string>

#include "rsx.h"

namespace opentit::rsx {

class ShaderDumpBackend : public Backend {
public:
    ShaderDumpBackend(std::string dir, std::function<u32(u32 offset, bool local)> to_ea);
    void draw(const State& s, const DrawCall& c) override;

    std::size_t vertex_programs() const { return vps_.size(); }
    std::size_t fragment_programs() const { return fps_.size(); }

private:
    std::string dir_;
    std::function<u32(u32, bool)> to_ea_;
    std::mutex m_;
    std::set<u64> vps_, fps_;
};

u64 fnv1a(const void* data, std::size_t size, u64 h = 0xCBF29CE484222325ull);

}  // namespace opentit::rsx
