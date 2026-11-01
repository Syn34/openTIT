// High-level emulation of PS3 system libraries (the PRX imports).
//
// Each implemented import is a plain function registered by name; its NID is
// computed from the name at startup, so registration is typo-proof: a name
// that doesn't hash to an imported NID simply never binds.
#pragma once

#include <initializer_list>
#include <string>

#include "opentit/memory.h"
#include "opentit/ppu_context.h"
#include "runtime.h"

namespace opentit::hle {

using HleFn = void (*)(PPUContext&);

struct Registrar {
    Registrar(const char* library, const char* name, HleFn fn);
};

// The implementation registered under `name` (tests, internal calls).
HleFn find(const char* name);

// Log an unimplemented-but-harmless call once per function.
void todo(const char* name);

// Call a guest function (by OPD) from inside an HLE function, e.g. a
// callback. Volatile registers are clobbered as per the PPC64 ABI; r2 and
// the link register are preserved for the caller.
u64 call_guest(PPUContext& ctx, u32 opd, std::initializer_list<u64> args);

// Create a callable HLE function that is not an import (e.g. the GCM
// command-buffer callback). Returns the address of its function descriptor.
u32 make_internal_function(const char* name, HleFn fn);
void commit_descriptors();  // make the HLE descriptor region usable (done on first use)

}  // namespace opentit::hle

#define OPENTIT_HLE(lib, name)                                                         \
    static void hle_##name(::opentit::PPUContext& ctx);                               \
    static const ::opentit::hle::Registrar hle_reg_##name(#lib, #name, &hle_##name);  \
    static void hle_##name([[maybe_unused]] ::opentit::PPUContext& ctx)

// Register a function that only needs to return CELL_OK.
#define OPENTIT_HLE_OK(lib, name) \
    OPENTIT_HLE(lib, name) { ::opentit::ret(ctx, 0); }
