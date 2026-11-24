// openTIT - Ape engine string helpers.
#include "ape/core/ApeString.h"
#include "ape/core/ApeThread.h"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ape {

// @func 0x00553638 StrNCopy
char* StrNCopy(char* dst, const char* src, size_t count)
{
    if (count == 0)
        return nullptr;
    return std::strncpy(dst, src, count);
}

// @func 0x00553684 StrCopyBounded
char* StrCopyBounded(char* dst, const char* src, size_t dstSize)
{
    if (dstSize > 1)
        std::strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
    return dst;
}

// @func 0x00555FA4 StrLen
int StrLen(const char* s)
{
    return (int)std::strlen(s);
}

// @func 0x00555F80 StrCopy
char* StrCopy(char* dst, const char* src)
{
    return std::strcpy(dst, src);
}

// @func 0x00555D94 StrCmp
int StrCmp(const char* a, const char* b)
{
    return std::strcmp(a, b);
}

// @func 0x00555DB8 StrNCmp
int StrNCmp(const char* a, const char* b, size_t count)
{
    return std::strncmp(a, b, count);
}

// @func 0x00555D70 StrICmp
int StrICmp(const char* a, const char* b)
{
    for (;; ++a, ++b) {
        int ca = (unsigned char)*a, cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
        if (ca != cb || ca == 0)
            return ca - cb;
    }
}

namespace {
inline int FoldLower(int c)
{
    return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}
} // namespace

// @func 0x00555DE8 StrCaseCompare
int StrCaseCompare(const char* a, const char* b)
{
    for (int i = 0;; ++i) {
        if (a[i] == '\0' && b[i] == '\0')
            return 0;
        const int ca = FoldLower((signed char)a[i]);
        const int cb = FoldLower((signed char)b[i]);
        if (ca < cb)
            return -1;
        if (ca > cb)
            return 1;
    }
}

// @func 0x00555EB0 StrNCaseCompare
int StrNCaseCompare(const char* a, const char* b, int count)
{
    for (int i = 0; i < count; ++i) {
        if (a[i] == '\0' && b[i] == '\0')
            return 0;
        const int ca = FoldLower((signed char)a[i]);
        const int cb = FoldLower((signed char)b[i]);
        if (ca < cb)
            return -1;
        if (ca > cb)
            return 1;
    }
    return 0;
}

// @func 0x00555CF4 StrPrintf
int StrPrintf(char* buffer, size_t size, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int written = std::vsnprintf(buffer, size, fmt, args);
    va_end(args);
    return written;
}

// @func 0x00555D48 StrVPrintf
int StrVPrintf(char* buffer, size_t size, const char* fmt, va_list args)
{
    return std::vsnprintf(buffer, size, fmt, args);
}

// @func 0x00555FCC StrStr
char* StrStr(const char* s, const char* sub)
{
    return const_cast<char*>(std::strstr(s, sub));
}

// @func 0x00555FF0 StrChr
char* StrChr(const char* s, int c)
{
    return const_cast<char*>(std::strchr(s, c));
}

// @func 0x00556014 StrRChr
char* StrRChr(const char* s, int c)
{
    return const_cast<char*>(std::strrchr(s, c));
}

// @func 0x005566A8 StrCat
char* StrCat(char* dst, const char* src)
{
    return std::strcat(dst, src);
}

// @func 0x005565A0 StrLower
void StrLower(char* s)
{
    for (; *s; ++s)
        *s = (char)FoldLower((signed char)*s);
}

// @func 0x005565F0 StrFindAny
char* StrFindAny(const char* s, const char* set)
{
    const size_t length = std::strlen(s);
    const size_t setLength = std::strlen(set);
    for (size_t i = 0; i < length; ++i)
        for (size_t j = 0; j < setLength; ++j)
            if (s[i] == set[j])
                return const_cast<char*>(s + i);
    return nullptr;
}

// @func 0x005566CC StrToUInt
uint32_t StrToUInt(const char* s)
{
    return (uint32_t)std::strtoul(s, nullptr, 10);
}

// @func 0x005566FC StrToFloat
float StrToFloat(const char* s)
{
    return (float)std::strtod(s, nullptr);
}

// @func 0x0055672C StrToInt
long StrToInt(const char* s, int base)
{
    char* end = nullptr;
    return std::strtol(s, &end, base);
}

// @func 0x0056A710 HashNameNoCase
uint32_t HashNameNoCase(const char* name)
{
    if (!name)
        return 0;
    uint32_t hash = 0;
    for (const char* p = name; *p; ++p) {
        int c = (signed char)*p;
        if (c >= 'a' && c <= 'z')
            c -= 'a' - 'A';
        hash = hash * 31u + (uint32_t)c;
    }
    return hash;
}

// @func 0x00324A48 HashNameListSum
uint32_t HashNameListSum(const char* list)
{
    if (!list || StrLen(list) >= 0xff)
        return 0;
    uint32_t sum = 0;
    const char* p = list;
    while (*p) {
        const char* end = std::strchr(p, ';');
        size_t len = end ? (size_t)(end - p) : std::strlen(p);
        if (len > 0) {
            char token[0x100];
            std::memcpy(token, p, len);
            token[len] = '\0';
            sum += HashNameNoCase(token);
        }
        if (!end)
            break;
        p = end + 1;
    }
    return sum;
}

namespace {
constexpr int kTempSlots = 8;
constexpr size_t kTempSize = 0x400;

// One ring of scratch strings per thread class. The main thread owns its
// ring exclusively; the render ring and the shared "any other thread" ring
// are advanced atomically because several threads may format at once.
struct TempRing {
    char slots[kTempSlots][kTempSize];
};
TempRing g_mainRing, g_renderRing, g_otherRing;
uint32_t g_mainCursor;
std::atomic<uint32_t> g_renderCursor{0}, g_otherCursor{0};
}

// @func 0x005534D0 TempFormat
const char* TempFormat(const char* fmt, ...)
{
    TempRing* ring;
    uint32_t slot;
    if (IsMainThread()) {
        ring = &g_mainRing;
        slot = ++g_mainCursor;
    } else if (IsRenderThread()) {
        ring = &g_renderRing;
        slot = g_renderCursor.fetch_add(1);
    } else {
        ring = &g_otherRing;
        slot = g_otherCursor.fetch_add(1);
    }
    char* buf = ring->slots[slot % kTempSlots];

    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, kTempSize, fmt, args);
    va_end(args);
    return buf;
}

// @func 0x0056A6E0 TempHexString
const char* TempHexString(uint32_t value)
{
    return TempFormat("0x%x", value);
}

} // namespace ape
