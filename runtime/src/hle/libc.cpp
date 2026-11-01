// sysPrxForUser libc subset (_sys_mem*, _sys_str*, _sys_*printf) used by
// firmware modules such as libsre.
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

#include "../hle.h"
#include "../log.h"

using namespace opentit;
using namespace opentit::rtm;

namespace {

// Variadic arguments on PPC64 ELFv1: argument k lives in r(3+k) for k < 8,
// otherwise in the caller's parameter save area at r1 + 48 + 8*k. A va_list
// is a 32-bit pointer to consecutive 8-byte slots.
struct ArgSource {
    PPUContext* ctx = nullptr;
    int next = 0;       // register-based: next argument index
    u32 va = 0;         // memory-based: next slot address
    u64 get() {
        if (!ctx) {
            const u64 v = mem::r64(va);
            va += 8;
            return v;
        }
        const int k = next++;
        if (k < 8) return ctx->gpr[3 + k];
        return mem::r64(static_cast<u32>(ctx->gpr[1]) + 48 + 8 * static_cast<u32>(k));
    }
};

std::string format(u32 fmt_addr, ArgSource args) {
    const std::string fmt = mem::read_string(fmt_addr, 4096);
    std::string out;
    for (std::size_t i = 0; i < fmt.size(); ++i) {
        if (fmt[i] != '%') {
            out.push_back(fmt[i]);
            continue;
        }
        std::string spec = "%";
        ++i;
        while (i < fmt.size() && std::strchr("-+ #0", fmt[i])) spec.push_back(fmt[i++]);
        auto number = [&] {
            if (i < fmt.size() && fmt[i] == '*') {
                spec += std::to_string(static_cast<s32>(args.get()));
                ++i;
                return;
            }
            while (i < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[i]))) spec.push_back(fmt[i++]);
        };
        number();
        if (i < fmt.size() && fmt[i] == '.') {
            spec.push_back(fmt[i++]);
            number();
        }
        int longs = 0;
        bool half = false, hh = false;
        while (i < fmt.size() && std::strchr("hlLqjzt", fmt[i])) {
            if (fmt[i] == 'h') { hh = half; half = true; }
            else if (fmt[i] == 'l' || fmt[i] == 'q' || fmt[i] == 'L' || fmt[i] == 'j') ++longs;
            ++i;
        }
        if (i >= fmt.size()) break;
        const char conv = fmt[i];
        char buf[512];
        switch (conv) {
        case 'd': case 'i': {
            s64 v = static_cast<s64>(args.get());
            if (hh) v = static_cast<s8>(v);
            else if (half) v = static_cast<s16>(v);
            else if (!longs) v = static_cast<s32>(v);
            std::snprintf(buf, sizeof(buf), (spec + "lld").c_str(), static_cast<long long>(v));
            out += buf;
            break;
        }
        case 'u': case 'x': case 'X': case 'o': {
            u64 v = args.get();
            if (hh) v = static_cast<u8>(v);
            else if (half) v = static_cast<u16>(v);
            else if (!longs) v = static_cast<u32>(v);
            std::snprintf(buf, sizeof(buf), (spec + "ll" + conv).c_str(), static_cast<unsigned long long>(v));
            out += buf;
            break;
        }
        case 'p':
            std::snprintf(buf, sizeof(buf), "0x%08x", static_cast<u32>(args.get()));
            out += buf;
            break;
        case 'c':
            std::snprintf(buf, sizeof(buf), (spec + "c").c_str(), static_cast<int>(args.get() & 0xFF));
            out += buf;
            break;
        case 's': {
            const u32 p = static_cast<u32>(args.get());
            const std::string str = p ? mem::read_string(p, 4096) : "(null)";
            std::snprintf(buf, sizeof(buf), (spec + "s").c_str(), str.c_str());
            out += buf;
            break;
        }
        case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': case 'a': case 'A': {
            const u64 bits = args.get();
            f64 d;
            std::memcpy(&d, &bits, 8);
            std::snprintf(buf, sizeof(buf), (spec + conv).c_str(), d);
            out += buf;
            break;
        }
        case '%': out.push_back('%'); break;
        case 'n': args.get(); break;
        default: out += spec + conv; break;
        }
    }
    return out;
}

void write_bounded(u32 dst, u32 size, const std::string& s) {
    if (!size) return;
    const u32 n = std::min<u32>(static_cast<u32>(s.size()), size - 1);
    mem::write_bytes(dst, s.data(), n);
    mem::w8(dst + n, 0);
}

ArgSource regs(PPUContext& ctx, int first) {
    ArgSource a;
    a.ctx = &ctx;
    a.next = first;
    return a;
}
ArgSource valist(u32 va) {
    ArgSource a;
    a.va = va;
    return a;
}

}  // namespace

// ---------------------------------------------------------------- printf family
OPENTIT_HLE(sysPrxForUser, _sys_printf) {
    const std::string s = format(arg32(ctx, 0), regs(ctx, 1));
    std::fwrite(s.data(), 1, s.size(), stdout);
    std::fflush(stdout);
    ret(ctx, s.size());
}
OPENTIT_HLE(sysPrxForUser, _sys_vprintf) {
    const std::string s = format(arg32(ctx, 0), valist(arg32(ctx, 1)));
    std::fwrite(s.data(), 1, s.size(), stdout);
    std::fflush(stdout);
    ret(ctx, s.size());
}
OPENTIT_HLE(sysPrxForUser, _sys_sprintf) {
    const std::string s = format(arg32(ctx, 1), regs(ctx, 2));
    write_bounded(arg32(ctx, 0), 0x7FFFFFFF, s);
    ret(ctx, s.size());
}
OPENTIT_HLE(sysPrxForUser, _sys_snprintf) {
    const std::string s = format(arg32(ctx, 2), regs(ctx, 3));
    write_bounded(arg32(ctx, 0), arg32(ctx, 1), s);
    ret(ctx, s.size());
}
OPENTIT_HLE(sysPrxForUser, _sys_vsprintf) {
    const std::string s = format(arg32(ctx, 1), valist(arg32(ctx, 2)));
    write_bounded(arg32(ctx, 0), 0x7FFFFFFF, s);
    ret(ctx, s.size());
}
OPENTIT_HLE(sysPrxForUser, _sys_vsnprintf) {
    const std::string s = format(arg32(ctx, 2), valist(arg32(ctx, 3)));
    write_bounded(arg32(ctx, 0), arg32(ctx, 1), s);
    ret(ctx, s.size());
}

// ---------------------------------------------------------------- memory
OPENTIT_HLE(sysPrxForUser, _sys_memset) {
    std::memset(mem::ptr(arg32(ctx, 0)), static_cast<int>(arg32(ctx, 1) & 0xFF), arg32(ctx, 2));
    ret(ctx, arg32(ctx, 0));
}
OPENTIT_HLE(sysPrxForUser, _sys_memcpy) {
    std::memmove(mem::ptr(arg32(ctx, 0)), mem::ptr(arg32(ctx, 1)), arg32(ctx, 2));
    ret(ctx, arg32(ctx, 0));
}
OPENTIT_HLE(sysPrxForUser, _sys_memmove) {
    std::memmove(mem::ptr(arg32(ctx, 0)), mem::ptr(arg32(ctx, 1)), arg32(ctx, 2));
    ret(ctx, arg32(ctx, 0));
}
OPENTIT_HLE(sysPrxForUser, _sys_memcmp) {
    const int r = std::memcmp(mem::ptr(arg32(ctx, 0)), mem::ptr(arg32(ctx, 1)), arg32(ctx, 2));
    ret(ctx, static_cast<u64>(static_cast<s64>(r < 0 ? -1 : r > 0 ? 1 : 0)));
}
OPENTIT_HLE(sysPrxForUser, _sys_memchr) {
    const u32 p = arg32(ctx, 0), n = arg32(ctx, 2);
    const void* hit = std::memchr(mem::ptr(p), static_cast<int>(arg32(ctx, 1) & 0xFF), n);
    ret(ctx, hit ? p + static_cast<u32>(static_cast<const u8*>(hit) - mem::ptr(p)) : 0);
}

// ---------------------------------------------------------------- strings
static const char* gstr(u32 a) { return reinterpret_cast<const char*>(mem::ptr(a)); }
static char* gstrw(u32 a) { return reinterpret_cast<char*>(mem::ptr(a)); }

OPENTIT_HLE(sysPrxForUser, _sys_strlen) { ret(ctx, std::strlen(gstr(arg32(ctx, 0)))); }
OPENTIT_HLE(sysPrxForUser, _sys_strcmp) {
    const int r = std::strcmp(gstr(arg32(ctx, 0)), gstr(arg32(ctx, 1)));
    ret(ctx, static_cast<u64>(static_cast<s64>(r < 0 ? -1 : r > 0 ? 1 : 0)));
}
OPENTIT_HLE(sysPrxForUser, _sys_strncmp) {
    const int r = std::strncmp(gstr(arg32(ctx, 0)), gstr(arg32(ctx, 1)), arg32(ctx, 2));
    ret(ctx, static_cast<u64>(static_cast<s64>(r < 0 ? -1 : r > 0 ? 1 : 0)));
}
OPENTIT_HLE(sysPrxForUser, _sys_strncasecmp) {
    const char *a = gstr(arg32(ctx, 0)), *b = gstr(arg32(ctx, 1));
    int r = 0;
    for (u32 i = 0; i < arg32(ctx, 2); ++i) {
        const int x = std::tolower(static_cast<unsigned char>(a[i])), y = std::tolower(static_cast<unsigned char>(b[i]));
        if (x != y || !x) { r = x - y; break; }
    }
    ret(ctx, static_cast<u64>(static_cast<s64>(r < 0 ? -1 : r > 0 ? 1 : 0)));
}
OPENTIT_HLE(sysPrxForUser, _sys_strcpy) {
    std::strcpy(gstrw(arg32(ctx, 0)), gstr(arg32(ctx, 1)));
    ret(ctx, arg32(ctx, 0));
}
OPENTIT_HLE(sysPrxForUser, _sys_strncpy) {
    std::strncpy(gstrw(arg32(ctx, 0)), gstr(arg32(ctx, 1)), arg32(ctx, 2));
    ret(ctx, arg32(ctx, 0));
}
OPENTIT_HLE(sysPrxForUser, _sys_strcat) {
    std::strcat(gstrw(arg32(ctx, 0)), gstr(arg32(ctx, 1)));
    ret(ctx, arg32(ctx, 0));
}
OPENTIT_HLE(sysPrxForUser, _sys_strncat) {
    std::strncat(gstrw(arg32(ctx, 0)), gstr(arg32(ctx, 1)), arg32(ctx, 2));
    ret(ctx, arg32(ctx, 0));
}
OPENTIT_HLE(sysPrxForUser, _sys_strchr) {
    const u32 p = arg32(ctx, 0);
    const char* hit = std::strchr(gstr(p), static_cast<int>(arg32(ctx, 1) & 0xFF));
    ret(ctx, hit ? p + static_cast<u32>(hit - gstr(p)) : 0);
}
OPENTIT_HLE(sysPrxForUser, _sys_strrchr) {
    const u32 p = arg32(ctx, 0);
    const char* hit = std::strrchr(gstr(p), static_cast<int>(arg32(ctx, 1) & 0xFF));
    ret(ctx, hit ? p + static_cast<u32>(hit - gstr(p)) : 0);
}
OPENTIT_HLE(sysPrxForUser, _sys_tolower) { ret(ctx, static_cast<u64>(std::tolower(static_cast<int>(arg32(ctx, 0) & 0xFF)))); }
OPENTIT_HLE(sysPrxForUser, _sys_toupper) { ret(ctx, static_cast<u64>(std::toupper(static_cast<int>(arg32(ctx, 0) & 0xFF)))); }

// ---------------------------------------------------------------- process / prx queries used by libsre
OPENTIT_HLE(sysPrxForUser, sys_prx_get_module_id_by_name) {
    LOG_DEBUG("prx", "sys_prx_get_module_id_by_name(\"%s\") -> not loaded", mem::read_string(arg32(ctx, 0)).c_str());
    ret(ctx, 0x8001112E);  // CELL_PRX_ERROR_UNKNOWN_MODULE
}
OPENTIT_HLE(sysPrxForUser, sys_process_get_paramsfo) {
    std::memset(mem::ptr(arg32(ctx, 0)), 0, 0x40);
    ret(ctx, CELL_OK);
}
