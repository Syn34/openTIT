#include "hle.h"

#include <cstdlib>
#include <mutex>
#include <set>
#include <unordered_map>
#include <deque>

#include "loader/nid.h"
#include "log.h"

namespace opentit::hle {

namespace {

struct Registered {
    const char* library;
    const char* name;
    HleFn fn;
};

std::unordered_map<u32, Registered>& registry() {
    static std::unordered_map<u32, Registered> r;
    return r;
}

struct Bound {
    std::string library;
    std::string name;
    u32 nid;
    HleFn fn;
    bool warned = false;
};

std::deque<Bound> g_bound;  // deque: references stay valid as internal functions are added
std::mutex g_bind_mutex;
std::mutex g_warn_mutex;

constexpr u32 kOpdBase = mem::kHleBase;               // fake function descriptors
constexpr u32 kCodeBase = mem::kHleBase + 0x00800000; // fake code addresses (never executed)
constexpr u32 kVarBase = mem::kHleBase + 0x00400000;  // storage for imported variables

}  // namespace

Registrar::Registrar(const char* library, const char* name, HleFn fn) {
    registry()[loader::compute_nid(name)] = {library, name, fn};
}

u64 call_guest(PPUContext& ctx, u32 opd, std::initializer_list<u64> args) {
    const u64 saved_r2 = ctx.gpr[2];
    const u64 saved_lr = ctx.lr;
    const u64 saved_r1 = ctx.gpr[1];
    int k = 3;
    for (u64 a : args) ctx.gpr[k++] = a;
    // Leave a red zone + linkage area so the callee can't clobber our frame.
    ctx.gpr[1] = (saved_r1 - 0x200) & ~0xFull;
    rtm::call_opd(ctx, opd);
    const u64 result = ctx.gpr[3];
    ctx.gpr[1] = saved_r1;
    ctx.gpr[2] = saved_r2;
    ctx.lr = saved_lr;
    return result;
}

void todo(const char* name) {
    static std::set<std::string> seen;
    std::lock_guard<std::mutex> lk(g_warn_mutex);
    if (seen.insert(name).second) LOG_WARN("hle", "%s: partial stub", name);
}

u32 make_internal_function(const char* name, HleFn fn) {
    std::lock_guard<std::mutex> lk(g_bind_mutex);
    const u32 index = static_cast<u32>(g_bound.size());
    const u32 opd = kOpdBase + index * 8;
    mem::w32(opd, kCodeBase + index * 4);
    mem::w32(opd + 4, 0);
    Bound b;
    b.library = "internal";
    b.name = name;
    b.nid = 0;
    b.fn = fn;
    g_bound.push_back(std::move(b));
    return opd;
}

}  // namespace opentit::hle

namespace opentit::rtm {

void hle_bind_imports(const loader::Elf& elf) {
    using namespace hle;
    mem::commit(kOpdBase, 0x00800000);
    std::size_t implemented = 0;
    u32 var_cursor = kVarBase;
    for (const auto& im : elf.imports()) {
        if (im.is_variable) {
            // Give each imported variable 4 KiB of zeroed storage.
            mem::w32(im.slot, var_cursor);
            LOG_WARN("hle", "imported variable %s 0x%08X bound to empty storage at 0x%08X", im.library.c_str(), im.nid,
                     var_cursor);
            var_cursor += 0x1000;
            continue;
        }
        const u32 index = static_cast<u32>(g_bound.size());
        const u32 opd = kOpdBase + index * 8;
        mem::w32(opd, kCodeBase + index * 4);
        mem::w32(opd + 4, 0);
        mem::w32(im.slot, opd);

        Bound b;
        b.library = im.library;
        b.name = loader::nid_display_name(im.library, im.nid);
        b.nid = im.nid;
        auto it = registry().find(im.nid);
        b.fn = it != registry().end() ? it->second.fn : nullptr;
        implemented += b.fn != nullptr;
        g_bound.push_back(std::move(b));
    }
    LOG_INFO("hle", "bound %zu imports (%zu implemented, %zu default stubs)", g_bound.size(), implemented,
             g_bound.size() - implemented);
}

bool hle_try_call(PPUContext& ctx, u32 code_addr) {
    using namespace hle;
    if (code_addr < kCodeBase || code_addr >= kCodeBase + g_bound.size() * 4) return false;
    Bound& b = g_bound[(code_addr - kCodeBase) / 4];
    ctx.hle_last_nid = b.nid;
    if (b.fn) {
        LOG_TRACE("hle", "%s(0x%llX, 0x%llX, 0x%llX)", b.name.c_str(), (unsigned long long)ctx.gpr[3],
                  (unsigned long long)ctx.gpr[4], (unsigned long long)ctx.gpr[5]);
        b.fn(ctx);
        return true;
    }
    {
        std::lock_guard<std::mutex> lk(g_warn_mutex);
        if (!b.warned) {
            b.warned = true;
            LOG_WARN("hle", "unimplemented %s::%s (r3=0x%llX) -> CELL_OK", b.library.c_str(), b.name.c_str(),
                     (unsigned long long)ctx.gpr[3]);
        }
    }
    if (config().strict) std::abort();
    ctx.gpr[3] = 0;
    return true;
}

}  // namespace opentit::rtm
