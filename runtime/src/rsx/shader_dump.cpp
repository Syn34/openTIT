#include "shader_dump.h"

#include <cinttypes>
#include <cstdio>
#include <filesystem>

#include "../log.h"
#include "fragment_program.h"
#include "opentit/memory.h"
#include "vertex_program.h"

namespace opentit::rsx {

u64 fnv1a(const void* data, std::size_t size, u64 h) {
    const u8* p = static_cast<const u8*>(data);
    for (std::size_t k = 0; k < size; ++k) h = (h ^ p[k]) * 0x100000001B3ull;
    return h;
}

ShaderDumpBackend::ShaderDumpBackend(std::string dir, std::function<u32(u32, bool)> to_ea)
    : dir_(std::move(dir)), to_ea_(std::move(to_ea)) {
    std::error_code ec;
    std::filesystem::create_directories(dir_, ec);
}

void ShaderDumpBackend::draw(const State& s, const DrawCall&) {
    const vp::Program v = vp::analyze(s.vp_ucode.data(), s.vp_start());
    u64 vh = 0;
    for (const auto& i : v.insns) vh = fnv1a(i.d, sizeof i.d, vh ? vh : 0xCBF29CE484222325ull);

    const u32 fp_ea = to_ea_ ? to_ea_(s.fp_offset(), s.fp_local()) : 0;
    fp::Program f;
    u64 fh = 0;
    if (fp_ea) {
        f = fp::parse(mem::ptr(fp_ea), 0x10000);
        fh = fnv1a(mem::ptr(fp_ea + f.start), f.size);
    }

    std::lock_guard<std::mutex> lk(m_);
    if (vps_.insert(vh).second) {
        char name[64];
        std::snprintf(name, sizeof name, "/vp_%016" PRIX64 ".txt", vh);
        if (FILE* o = std::fopen((dir_ + name).c_str(), "w")) {
            std::fprintf(o, "# start %u, %zu instructions, inputs 0x%04X, outputs 0x%04X%s%s\n", v.start,
                         v.insns.size(), v.inputs_read, v.outputs_written, v.has_end ? "" : ", NO END",
                         v.indexed_constants ? ", indexed constants" : "");
            for (std::size_t k = 0; k < v.insns.size(); ++k)
                std::fprintf(o, "%3zu: %08X %08X %08X %08X  %s\n", v.start + k, v.insns[k].d[0], v.insns[k].d[1],
                             v.insns[k].d[2], v.insns[k].d[3], vp::disasm(v.insns[k]).c_str());
            std::fclose(o);
        }
        LOG_INFO("rsx", "new vertex program %016" PRIX64 " (%zu instructions)", vh, v.insns.size());
    }
    if (fp_ea && fps_.insert(fh).second) {
        char name[64];
        std::snprintf(name, sizeof name, "/fp_%016" PRIX64 ".txt", fh);
        if (FILE* o = std::fopen((dir_ + name).c_str(), "w")) {
            std::fprintf(o, "# offset 0x%08X (%s), %u bytes, textures 0x%X, inputs 0x%X%s%s%s\n", s.fp_offset(),
                         s.fp_local() ? "local" : "main", f.size, f.textures, f.inputs,
                         f.has_branches ? ", branches" : "", f.has_kill ? ", kill" : "",
                         f.truncated ? ", TRUNCATED" : "");
            for (const auto& i : f.insns)
                std::fprintf(o, "%4u: %08X %08X %08X %08X  %s\n", i.offset, i.d[0], i.d[1], i.d[2], i.d[3],
                             fp::disasm(i).c_str());
            std::fclose(o);
        }
        LOG_INFO("rsx", "new fragment program %016" PRIX64 " (%zu instructions)", fh, f.insns.size());
    }
}

}  // namespace opentit::rsx
