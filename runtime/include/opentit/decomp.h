// API for hand-decompiled game functions.
//
// A decompiled function replaces the recompiled one at the same address:
//
//     DECOMP(004FC7B4, main) {
//         const u32 argc = arg<u32>(ctx, 0);
//         ...
//         ret(ctx, 0);
//     }
//
// It keeps the PPU calling convention (arguments in r3-r10 / f1-f13, result
// in r3 or f1), so recompiled callers and callees keep working around it.
// The recompiled original stays available as f_XXXXXXXX_recomp and is used
// instead when the function is switched off (opentit --no-decomp, or
// --decomp-off XXXXXXXX,...), which makes A/B comparisons easy.
//
// Guest memory is big endian: describe guest structures with be<T> fields
// and view them in place with ref<T>(ea).
#pragma once

#include <cstring>
#include <initializer_list>
#include <type_traits>

#include "opentit/recomp.h"

namespace opentit::decomp {

// ---------------------------------------------------------------- switches
bool enabled(u32 addr);  // false under --no-decomp / --decomp-off
void disable_all();
void disable(u32 addr);

// ---------------------------------------------------------------- big-endian values in guest memory
template <typename T>
struct be {
    static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T>, "be<T> holds a number");
    u8 raw[sizeof(T)];

    T get() const {
        T v;
        u8 tmp[sizeof(T)];
        for (std::size_t k = 0; k < sizeof(T); ++k) tmp[k] = raw[sizeof(T) - 1 - k];
        std::memcpy(&v, tmp, sizeof(T));
        return v;
    }
    void set(T v) {
        u8 tmp[sizeof(T)];
        std::memcpy(tmp, &v, sizeof(T));
        for (std::size_t k = 0; k < sizeof(T); ++k) raw[k] = tmp[sizeof(T) - 1 - k];
    }
    operator T() const { return get(); }
    be& operator=(T v) {
        set(v);
        return *this;
    }
    be& operator+=(T v) { return *this = static_cast<T>(get() + v); }
    be& operator-=(T v) { return *this = static_cast<T>(get() - v); }
    be& operator|=(T v) { return *this = static_cast<T>(get() | v); }
    be& operator&=(T v) { return *this = static_cast<T>(get() & v); }
};
using be_u8 = u8;
using be_s8 = s8;
using be_u16 = be<u16>;
using be_s16 = be<s16>;
using be_u32 = be<u32>;
using be_s32 = be<s32>;
using be_u64 = be<u64>;
using be_s64 = be<s64>;
using be_f32 = be<f32>;
using be_f64 = be<f64>;
using gptr = be<u32>;  // a 32-bit guest pointer stored in guest memory

// View guest memory at `ea` as T (a struct of be<> fields, laid out as on the PS3).
template <typename T>
T& ref(u32 ea) {
    return *reinterpret_cast<T*>(mem::ptr(ea));
}
// A field of a guest structure whose layout isn't written down yet:
//     field<u32>(obj, 0x144) = 0x1F4000;
template <typename T>
be<T>& field(u32 base, u32 offset) {
    return ref<be<T>>(base + offset);
}
template <typename T>
T load(u32 ea) {
    return ref<be<T>>(ea).get();
}
template <typename T>
void store(u32 ea, T v) {
    ref<be<T>>(ea).set(v);
}

// ---------------------------------------------------------------- arguments and results
// Integer/pointer argument n (r3 + n). Narrow types take the low bits.
template <typename T = u32>
T arg(const PPUContext& ctx, int n) {
    return static_cast<T>(ctx.gpr[3 + n]);
}
// Floating-point argument n (f1 + n).
inline f64 farg(const PPUContext& ctx, int n) { return ctx.fpr[1 + n]; }

template <typename T>
void ret(PPUContext& ctx, T v) {
    if constexpr (std::is_floating_point_v<T>) {
        ctx.fpr[1] = static_cast<f64>(v);
    } else if constexpr (std::is_signed_v<T>) {
        ctx.gpr[3] = static_cast<u64>(static_cast<s64>(v));  // sign-extended like PPU code does
    } else {
        ctx.gpr[3] = static_cast<u64>(v);
    }
}

// ---------------------------------------------------------------- calling other guest functions
// Call the function at `addr` (decompiled or recompiled) with integer
// arguments in r3.. and return r3. Volatile registers are clobbered, as for
// any PPU call.
u64 call(PPUContext& ctx, u32 addr, std::initializer_list<u64> args = {});
// Same, with floating-point arguments in f1.. as well; returns f1.
f64 callf(PPUContext& ctx, u32 addr, std::initializer_list<u64> args, std::initializer_list<f64> fargs);
// Call through a function descriptor (OPD) such as a function pointer read from guest memory.
u64 call_ptr(PPUContext& ctx, u32 opd, std::initializer_list<u64> args = {});
// LV2 system call `num` (the `sc` instruction with r11 = num); returns r3.
u64 syscall(PPUContext& ctx, u32 num, std::initializer_list<u64> args = {});

// Guest stack space for the lifetime of a scope, e.g. for out-parameters
// passed to guest functions. Keeps the PPU ABI's 16-byte alignment and
// back-chain.
class Stack {
public:
    Stack(PPUContext& ctx, u32 bytes);
    ~Stack();
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;
    u32 addr() const { return base_; }

private:
    PPUContext& ctx_;
    u64 saved_r1_;
    u32 base_;
};

}  // namespace opentit::decomp

// DECOMP(XXXXXXXX, name) { body }   -- body sees `ctx`
#define DECOMP(addr, name)                                                                       \
    void f_##addr##_recomp(::opentit::PPUContext& ctx);                                         \
    static void decomp_##name(::opentit::PPUContext& ctx);                                      \
    void f_##addr(::opentit::PPUContext& ctx) {                                                 \
        if (!::opentit::decomp::enabled(0x##addr##u)) return f_##addr##_recomp(ctx);            \
        decomp_##name(ctx);                                                                     \
    }                                                                                           \
    static void decomp_##name([[maybe_unused]] ::opentit::PPUContext& ctx)
