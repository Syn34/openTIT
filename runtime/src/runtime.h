// Internal runtime interfaces shared by the kernel, HLE modules and boot code.
#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "common/types.h"
#include "loader/elf.h"
#include "opentit/ppu_context.h"
#include "opentit/recomp.h"

namespace opentit::rtm {

struct Config {
    std::string game_root;  // folder containing PS3_GAME/ (the disc dump)
    std::string eboot;      // decrypted EBOOT.elf
    std::string hdd0;       // host folder backing /dev_hdd0
    std::string firmware;   // folder with decrypted firmware PRX modules (default: <game_root>/firmware)
    bool strict = false;    // abort on unimplemented HLE functions
    std::string shader_dump;  // write vertex/fragment program disassembly here (debug)
    std::string renderer = "auto";  // auto | vulkan | null
    std::string spu_record;   // record unknown SPU code here
    bool headless = false;    // no window
    int scale = 2;            // window size in half-720p steps (2 = 1280x720)
};

Config& config();
const loader::Elf& elf();
void set_elf(const loader::Elf* e);

// ---- dispatcher
void init_dispatch(const FunctionEntry* table, std::size_t count);
GuestFn lookup(u32 code_addr);
const std::vector<ModuleTable>& module_tables();

// Run a guest function described by an OPD (sets r2 from the descriptor).
void call_opd(PPUContext& ctx, u32 opd_addr);
// Run guest code at `code_addr` with TOC `toc`.
void call_code(PPUContext& ctx, u32 code_addr, u32 toc);

// ---- HLE
// Bind every import of `elf`. `resolve` returns the address of a real
// (LLE) export for an import, or 0 to fall back to the HLE implementation.
void hle_bind_imports(const loader::Elf& elf, const std::function<u32(const loader::Import&)>& resolve);
bool hle_try_call(PPUContext& ctx, u32 code_addr);

// ---- threads
struct ThreadExit {
    u64 code;
};
PPUContext* current_ctx();
void set_current_ctx(PPUContext* ctx);
[[noreturn]] void exit_process(int code);
void install_crash_handler();
// --dump-mem: write the given guest ranges to `path` when the process exits or
// crashes, for comparing runs (e.g. with and without decompiled functions).
void set_exit_dump(const std::string& path, std::vector<std::pair<u32, u32>> ranges);
void write_exit_dump();

// ---- kernel
void kernel_init();
u32 alloc_guest(u32 size, u32 align);   // general purpose guest allocator (kernel-owned memory)
void free_guest(u32 addr);
u32 alloc_stack(u32 size);
u32 alloc_tls();                         // TLS block for a new thread; returns base, r13 = base + 0x7030

// Error codes (CELL_*)
inline constexpr u32 CELL_OK = 0;
inline constexpr u32 CELL_EAGAIN = 0x80010001;
inline constexpr u32 CELL_EINVAL = 0x80010002;
inline constexpr u32 CELL_ENOSYS = 0x80010003;
inline constexpr u32 CELL_ENOMEM = 0x80010004;
inline constexpr u32 CELL_ESRCH = 0x80010005;
inline constexpr u32 CELL_ENOENT = 0x80010006;
inline constexpr u32 CELL_EDEADLK = 0x80010008;
inline constexpr u32 CELL_EPERM = 0x80010009;
inline constexpr u32 CELL_EBUSY = 0x8001000A;
inline constexpr u32 CELL_ETIMEDOUT = 0x8001000B;
inline constexpr u32 CELL_EFAULT = 0x8001000D;
inline constexpr u32 CELL_ESTAT = 0x8001000F;
inline constexpr u32 CELL_ECANCELED = 0x80010013;
inline constexpr u32 CELL_EEXIST = 0x80010014;
inline constexpr u32 CELL_EISDIR = 0x80010012;
inline constexpr u32 CELL_ENOTDIR = 0x80010011;
inline constexpr u32 CELL_EBADF = 0x8001002A;
inline constexpr u32 CELL_EIO = 0x8001002B;

}  // namespace opentit::rtm
