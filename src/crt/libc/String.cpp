// openTIT - C runtime <string.h> routines.
//
// The console versions are hand-scheduled PPU routines (word-at-a-time zero
// byte detection, dcbt prefetching, alignment special cases). None of that is
// meaningful on the PC: each routine is the plain standard function, provided
// by the host C library.
#include "crt/libc/Libc.h"

#include <cctype>
#include <cerrno>
#include <cstring>
#include <cwchar>

namespace crt {

// @func 0x0071C2A4 MemChr
// memchr(): first occurrence of byte `c` in the first `n` bytes of `s`.
void* MemChr(const void* s, int c, size_t n)
{
    return const_cast<void*>(std::memchr(s, c, n));
}

// @func 0x0071C3F0 MemCpy
// memcpy(): non-overlapping block copy.
void* MemCpy(void* dst, const void* src, size_t n)
{
    return std::memcpy(dst, src, n);
}

// @func 0x0071C9B4 MemMove
// memmove(): block copy that handles overlapping ranges (the original copies
// backwards when dst lies inside [src, src + n)).
void* MemMove(void* dst, const void* src, size_t n)
{
    return std::memmove(dst, src, n);
}

// @func 0x00726E54 MemSet
// memset(): fill `n` bytes with the byte value `c`.
void* MemSet(void* dst, int c, size_t n)
{
    return std::memset(dst, c, n);
}

// @func 0x00726F28 MemCmp
// memcmp(): compares as unsigned bytes. The original returns exactly -1/0/1.
int MemCmp(const void* a, const void* b, size_t n)
{
    const int result = std::memcmp(a, b, n);
    return (result > 0) - (result < 0);
}

// @func 0x0071ED08 StrLen
// strlen().
size_t StrLen(const char* s)
{
    return std::strlen(s);
}

// @func 0x007239D8 StrCpy
// strcpy().
char* StrCpy(char* dst, const char* src)
{
    return std::strcpy(dst, src);
}

// @func 0x00723C88 StrNCpy
// strncpy(): copies at most `n` characters and zero-pads the remainder.
char* StrNCpy(char* dst, const char* src, size_t n)
{
    return std::strncpy(dst, src, n);
}

// @func 0x0072354C StrCat
// strcat().
char* StrCat(char* dst, const char* src)
{
    return std::strcat(dst, src);
}

// @func 0x00723884 StrCmp
// strcmp(): unsigned character comparison, returns exactly -1/0/1 like the original.
int StrCmp(const char* a, const char* b)
{
    const int result = std::strcmp(a, b);
    return (result > 0) - (result < 0);
}

// @func 0x00723AC4 StrNCmp
// strncmp(): returns exactly -1/0/1 like the original.
int StrNCmp(const char* a, const char* b, size_t n)
{
    const int result = std::strncmp(a, b, n);
    return (result > 0) - (result < 0);
}

// @func 0x00723F2C StrCaseCmp
// strcasecmp() / stricmp(): compares after folding upper case letters to
// lower case (the original consults the ctype table's upper-case bit).
// Not every host spells this function the same way, so it is written out.
int StrCaseCmp(const char* a, const char* b)
{
    for (;; ++a, ++b) {
        const int ca = std::tolower((unsigned char)*a);
        const int cb = std::tolower((unsigned char)*b);
        if (ca != cb)
            return ca < cb ? -1 : 1;
        if (ca == '\0')
            return 0;
    }
}

// @func 0x00723678 StrChr
// strchr().
char* StrChr(const char* s, int c)
{
    return const_cast<char*>(std::strchr(s, c));
}

// @func 0x0071EDAC StrRChr
// strrchr().
char* StrRChr(const char* s, int c)
{
    return const_cast<char*>(std::strrchr(s, c));
}

// @func 0x0071EEF8 StrStr
// strstr(): an empty needle matches at the start of the haystack.
char* StrStr(const char* haystack, const char* needle)
{
    return const_cast<char*>(std::strstr(haystack, needle));
}

// @func 0x0072AD24 StrSpn
// strspn(): length of the leading run made only of characters from `accept`.
size_t StrSpn(const char* s, const char* accept)
{
    return std::strspn(s, accept);
}

// @func 0x0072ACB4 StrCSpn
// strcspn(): length of the leading run containing no character from `reject`.
size_t StrCSpn(const char* s, const char* reject)
{
    return std::strcspn(s, reject);
}

// @func 0x0071EFA4 StrTok
// strtok(). The original keeps its continuation pointer in thread-local
// storage, so each thread tokenises independently; host strtok does not
// guarantee that everywhere, hence the explicit implementation.
// NOTE: the address range listed for this function also contains the body of
// the strtol worker (entered at 0x0071F088 from StrToL).
char* StrTok(char* s, const char* delimiters)
{
    static thread_local char* s_next = nullptr;
    static char s_empty[1] = {'\0'};

    char* p = s ? s : s_next;
    if (!p)
        return nullptr;
    p += std::strspn(p, delimiters);
    if (*p == '\0') {
        s_next = s_empty;
        return nullptr;
    }
    char* tokenEnd = p + std::strcspn(p, delimiters);
    if (*tokenEnd != '\0')
        *tokenEnd++ = '\0';
    s_next = tokenEnd;
    return p;
}

// @func 0x007225FC StrErrorToBuffer
// _Strerror(): message text for an error code. Known codes map to fixed
// strings; unknown codes are formatted as "error 0x%08x" into `buffer` (or a
// per-thread static buffer when `buffer` is null). Error codes on the PC are
// host errno values, so the host's message table is used for them.
char* StrErrorToBuffer(int errnum, char* buffer)
{
    static char s_noError[] = "no error";
    if (errnum == 0)
        return s_noError;
    char* message = std::strerror(errnum);
    if (message)
        return message;
    // "error 0x" + 8 hex digits + terminator: the size the original writes.
    constexpr size_t kUnknownLength = 17;
    static thread_local char s_unknown[kUnknownLength];
    char* out = buffer ? buffer : s_unknown;
    std::snprintf(out, kUnknownLength, "error 0x%08x", (unsigned)errnum);
    return out;
}

// @func 0x0071ECDC StrError
// strerror().
char* StrError(int errnum)
{
    return StrErrorToBuffer(errnum, nullptr);
}

// @func 0x007231D8 WcToB
// wctob(): the single-byte form of a wide character, or EOF if it has none.
int WcToB(wint_t wc)
{
    return std::wctob(wc);
}

// @func 0x00723518 WcToMb
// _Wctomb(): converts one wide character into its multibyte sequence in the
// current locale (the original walks the locale's conversion state tables and
// falls back to UTF-8 style encoding). Returns the byte count, or -1 with
// errno = EILSEQ for an unrepresentable character.
int WcToMb(char* dst, wchar_t wc, std::mbstate_t* state)
{
    std::mbstate_t localState{};
    const size_t count = std::wcrtomb(dst, wc, state ? state : &localState);
    if (count == (size_t)-1) {
        errno = EILSEQ;
        return -1;
    }
    return (int)count;
}

} // namespace crt
