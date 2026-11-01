// PPU -> C++ code generator.
#pragma once

#include <map>
#include <string>

#include "analysis/functions.h"
#include "loader/elf.h"
#include "ppu/ppu_decoder.h"

namespace opentit::recomp {

struct EmitOptions {
    std::string out_dir;
    std::size_t insns_per_file = 12000;
    bool annotate = true;  // emit the disassembly as a comment above each instruction
    // For PRX modules: name + load base. The function table then registers
    // itself with the runtime instead of defining g_recompiled_functions.
    std::string module_name;
    u32 module_base = 0;
};

struct EmitStats {
    std::size_t files = 0;
    std::size_t functions = 0;
    std::size_t instructions = 0;
    std::map<ppu::Op, std::size_t> fallback_ops;  // ops emitted as runtime fallbacks
};

// Generates the C++ body for a single function (exposed for tests).
std::string emit_function(const loader::Elf& elf, const analysis::Program& prog, const analysis::Function& fn,
                          bool annotate, std::map<u32, bool>* referenced, EmitStats* stats);

bool emit_program(const loader::Elf& elf, const analysis::Program& prog, const EmitOptions& opts, EmitStats* stats,
                  std::string* error);

}  // namespace opentit::recomp
