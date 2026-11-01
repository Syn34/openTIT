// Function discovery and control-flow recovery for the PPU executable.
//
// Strategy (recursive descent, no linear sweep - the text section contains
// inline jump tables that must never be decoded as code):
//   1. seeds = every .opd descriptor + every import stub + the entry point
//      (+ any user-supplied extra entry points)
//   2. walk each function from its entry, following fallthrough, local
//      branches and recovered jump tables; every `bl` target becomes a new
//      function seed; iterate to a fixed point
//   3. a branch or fallthrough that reaches another function's entry is a
//      tail call; code shared between functions is emitted once per owner
#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "common/types.h"
#include "loader/elf.h"

namespace opentit::analysis {

struct JumpTable {
    u32 bctr = 0;    // address of the bctr instruction
    u32 table = 0;   // table base
    std::vector<u32> targets;
};

struct Function {
    u32 start = 0;
    u32 toc = 0;
    std::vector<u32> insns;           // reachable instruction addresses, sorted
    std::set<u32> labels;             // addresses that need a label (branch targets)
    std::map<u32, JumpTable> jump_tables;  // keyed by bctr address
    std::set<u32> unresolved_bctr;    // bctr we could not resolve (runtime dispatch)
    std::set<u32> invalid;            // reachable words that did not decode
    std::set<u32> callees;            // direct call / tail call targets
};

struct Options {
    std::vector<u32> extra_entries;   // from --entries file
    bool verbose = false;
};

struct Program {
    u32 toc = 0;
    std::map<u32, Function> functions;   // keyed by start address
    std::map<u32, std::string> import_stubs;  // stub trampoline address -> "lib::name"

    std::size_t total_insns() const;
};

Program analyze(const loader::Elf& elf, const Options& opts);

// Reads a whitespace / newline separated list of hex addresses ("0x..." or
// bare hex, '#' comments allowed).
std::vector<u32> read_address_list(const std::string& path, std::string* error);

}  // namespace opentit::analysis
