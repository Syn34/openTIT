// openTIT - C runtime <stdlib.h> routines: searching and sorting, number
// parsing, atexit, the heap front end, abort and assertion failure reporting.
#include "crt/libc/Libc.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace crt {

// @func 0x0071B054 Abs
// abs().
int Abs(int value)
{
    return std::abs(value);
}

// @func 0x0071B458 BSearch
// bsearch(): binary search of a sorted array; the comparator is called as
// compare(key, element).
void* BSearch(const void* key, const void* base, size_t count, size_t size, CompareFunc compare)
{
    return std::bsearch(key, base, count, size, compare);
}

// ---------------------------------------------------------------------------
// qsort: introsort (median-of-three / median-of-nine pivot, three-way
// partition, heap sort fallback when recursion gets too deep, insertion
// sort for short ranges), operating on opaque elements of `size` bytes.
// ---------------------------------------------------------------------------
namespace {

using Byte = unsigned char;

// Ranges at or below this many elements are finished with insertion sort.
constexpr size_t kInsertionSortThreshold = 32;
// Above this many elements the pivot is a median of nine instead of three.
constexpr size_t kNintherThreshold = 40;

struct SortContext {
    size_t size;
    CompareFunc compare;

    Byte* At(Byte* base, size_t index) const { return base + index * size; }
    int Compare(const Byte* a, const Byte* b) const { return compare(a, b); }
};

// @func 0x0071E0D4 SwapElements
// Exchanges two elements.
void SwapElements(Byte* a, Byte* b, size_t size)
{
    std::swap_ranges(a, a + size, b);
}

// @func 0x0071E540 MoveElementDown
// Moves the element at `from` down to `to` (to < from), shifting the elements
// in [to, from) up by one slot. Used by the insertion sort.
void MoveElementDown(Byte* to, Byte* from, size_t size)
{
    std::rotate(to, from, from + size);
}

// @func 0x0071E3E8 SortThreeElements
// Orders three elements in place so that *a <= *b <= *c.
void SortThreeElements(Byte* a, Byte* b, Byte* c, const SortContext& ctx)
{
    if (ctx.Compare(b, a) < 0)
        SwapElements(a, b, ctx.size);
    if (ctx.Compare(c, b) < 0) {
        SwapElements(b, c, ctx.size);
        if (ctx.Compare(b, a) < 0)
            SwapElements(a, b, ctx.size);
    }
}

// @func 0x0071E218 SiftDown
// Restores the max-heap property below `hole` in a heap of `count` elements.
void SiftDown(Byte* base, size_t hole, size_t count, const SortContext& ctx)
{
    for (;;) {
        size_t child = 2 * hole + 1;
        if (child >= count)
            return;
        if (child + 1 < count && ctx.Compare(ctx.At(base, child), ctx.At(base, child + 1)) < 0)
            ++child;
        if (ctx.Compare(ctx.At(base, hole), ctx.At(base, child)) >= 0)
            return;
        SwapElements(ctx.At(base, hole), ctx.At(base, child), ctx.size);
        hole = child;
    }
}

void HeapSort(Byte* base, size_t count, const SortContext& ctx)
{
    for (size_t i = count / 2; i-- > 0;)
        SiftDown(base, i, count, ctx);
    for (size_t end = count; end > 1;) {
        --end;
        SwapElements(base, ctx.At(base, end), ctx.size);
        SiftDown(base, 0, end, ctx);
    }
}

void InsertionSort(Byte* base, size_t count, const SortContext& ctx)
{
    Byte* const end = ctx.At(base, count);
    for (Byte* next = base + ctx.size; next < end; next += ctx.size) {
        Byte* slot = next;
        while (slot > base && ctx.Compare(next, slot - ctx.size) < 0)
            slot -= ctx.size;
        if (slot != next)
            MoveElementDown(slot, next, ctx.size);
    }
}

// Picks a pivot and moves it to the front of the range.
void ChoosePivot(Byte* base, size_t count, const SortContext& ctx)
{
    Byte* first = base;
    Byte* mid = ctx.At(base, count / 2);
    Byte* last = ctx.At(base, count - 1);
    if (count > kNintherThreshold) {
        const size_t step = (count / 8 + 1) * ctx.size;
        SortThreeElements(first, first + step, first + 2 * step, ctx);
        SortThreeElements(mid - step, mid, mid + step, ctx);
        SortThreeElements(last - 2 * step, last - step, last, ctx);
        SortThreeElements(first + step, mid, last - step, ctx);
    } else {
        SortThreeElements(first, mid, last, ctx);
    }
    SwapElements(first, mid, ctx.size);
}

// @func 0x0071E63C IntroSortLoop
// Sorts `count` elements. `depthBudget` shrinks by a quarter on every
// partitioning step; when it runs out the range is heap sorted instead.
void IntroSortLoop(Byte* base, size_t count, size_t depthBudget, const SortContext& ctx)
{
    while (count > kInsertionSortThreshold) {
        if (depthBudget == 0) {
            HeapSort(base, count, ctx);
            return;
        }
        ChoosePivot(base, count, ctx);

        // Three-way partition: [base, lt) < pivot, [lt, gt) == pivot,
        // [gt, end) > pivot. A copy of the pivot value always sits at lt.
        Byte* lt = base;
        Byte* i = base + ctx.size;
        Byte* gt = ctx.At(base, count);
        while (i < gt) {
            const int order = ctx.Compare(i, lt);
            if (order < 0) {
                SwapElements(lt, i, ctx.size);
                lt += ctx.size;
                i += ctx.size;
            } else if (order > 0) {
                gt -= ctx.size;
                SwapElements(i, gt, ctx.size);
            } else {
                i += ctx.size;
            }
        }

        depthBudget = depthBudget / 2 + depthBudget / 4;
        const size_t lowCount = (size_t)(lt - base) / ctx.size;
        const size_t highCount = count - (size_t)(gt - base) / ctx.size;
        // Recurse into the smaller side, iterate on the larger one.
        if (lowCount < highCount) {
            IntroSortLoop(base, lowCount, depthBudget, ctx);
            base = gt;
            count = highCount;
        } else {
            IntroSortLoop(gt, highCount, depthBudget, ctx);
            count = lowCount;
        }
    }
    if (count > 1)
        InsertionSort(base, count, ctx);
}

} // namespace

// @func 0x0071EC28 QSort
// qsort().
void QSort(void* base, size_t count, size_t size, CompareFunc compare)
{
    if (count < 2 || size == 0)
        return;
    const SortContext ctx{size, compare};
    IntroSortLoop(static_cast<Byte*>(base), count, count, ctx);
}

// ---------------------------------------------------------------------------
// Number parsing
// ---------------------------------------------------------------------------
namespace {

constexpr char kDigitChars[] = "0123456789abcdefghijklmnopqrstuvwxyz";

// The unsigned conversion shared by strtoul and strtoull (_Stoulx/_Stoullx):
// optional white space and sign, base 0 (auto-detect 0x / 0 prefixes) or
// 2..36, digits in either case. A '-' negates the result modulo 2^N. On
// overflow the result is the maximum value, errno is ERANGE and *error
// (when given) is set to 1. Without any digits nothing is consumed.
template <typename U>
U ParseUnsigned(const char* s, char** end, int base, int* error)
{
    if (error)
        *error = 0;

    const char* p = s;
    while (std::isspace((unsigned char)*p))
        ++p;
    char sign = '+';
    if (*p == '-' || *p == '+')
        sign = *p++;

    if (base < 0 || base == 1 || base > 36) {
        if (end)
            *end = const_cast<char*>(s);
        return 0;
    }
    const bool hexPrefix = p[0] == '0' && (p[1] == 'x' || p[1] == 'X');
    if (base == 0)
        base = hexPrefix ? 16 : (p[0] == '0' ? 8 : 10);
    if (base == 16 && hexPrefix)
        p += 2;

    const char* digitsStart = p;
    const U maxValue = (U)~(U)0;
    U value = 0;
    bool overflow = false;
    for (;; ++p) {
        const char* found = std::strchr(kDigitChars, std::tolower((unsigned char)*p));
        if (*p == '\0' || !found || found - kDigitChars >= base)
            break;
        const U digit = (U)(found - kDigitChars);
        if (value > (maxValue - digit) / (U)base)
            overflow = true;
        else
            value = value * (U)base + digit;
    }

    if (p == digitsStart) {
        if (end)
            *end = const_cast<char*>(s);
        return 0;
    }
    if (end)
        *end = const_cast<char*>(p);
    if (overflow) {
        errno = ERANGE;
        if (error)
            *error = 1;
        return maxValue;
    }
    return sign == '-' ? (U)(0 - value) : value;
}

// The signed conversion behind strtol and strtoll (_Stolx/_Stollx): the sign
// is taken off here, the digits are converted unsigned, and the magnitude is
// then checked against the signed range (ERANGE and clamping when outside).
template <typename S, typename U>
S ParseSigned(const char* s, char** end, int base, U (*parseUnsigned)(const char*, char**, int, int*))
{
    const char* p = s;
    while (std::isspace((unsigned char)*p))
        ++p;
    char sign = '+';
    if (*p == '-' || *p == '+')
        sign = *p++;

    char* digitsEnd = nullptr;
    int overflow = 0;
    const U magnitude = parseUnsigned(p, &digitsEnd, base, &overflow);
    if (digitsEnd == p) {
        if (end)
            *end = const_cast<char*>(s);
        return 0;
    }
    if (end)
        *end = digitsEnd;

    const U positiveLimit = (U)((U)~(U)0 >> 1);
    const U limit = sign == '-' ? (U)(positiveLimit + 1) : positiveLimit;
    if (overflow || magnitude > limit) {
        errno = ERANGE;
        return sign == '-' ? (S)(positiveLimit + 1) : (S)positiveLimit;
    }
    return sign == '-' ? (S)(0 - magnitude) : (S)magnitude;
}

} // namespace

// @func 0x007219D0 ParseULong
// _Stoulx(): 32-bit unsigned conversion (see ParseUnsigned).
uint32_t ParseULong(const char* s, char** end, int base, int* error)
{
    return ParseUnsigned<uint32_t>(s, end, base, error);
}

// @func 0x00721D20 ParseULongLong
// _Stoullx(): 64-bit unsigned conversion (see ParseUnsigned).
unsigned long long ParseULongLong(const char* s, char** end, int base, int* error)
{
    return ParseUnsigned<unsigned long long>(s, end, base, error);
}

// @func 0x0071F284 StrToL
// strtol() for the console's 32-bit long: out-of-range values clamp to
// INT32_MAX / INT32_MIN and set errno = ERANGE.
// (Its worker, entered at 0x0071F088, lies inside the range listed for StrTok.)
int32_t StrToL(const char* s, char** end, int base)
{
    return ParseSigned<int32_t, uint32_t>(s, end, base, &ParseULong);
}

// @func 0x00721D18 StrToUL
// strtoul() for the console's 32-bit unsigned long: a leading '-' negates the
// result modulo 2^32; values above UINT32_MAX give UINT32_MAX and ERANGE.
uint32_t StrToUL(const char* s, char** end, int base)
{
    return ParseULong(s, end, base, nullptr);
}

// @func 0x00721738 StrToLL
// strtoll().
long long StrToLL(const char* s, char** end, int base)
{
    return ParseSigned<long long, unsigned long long>(s, end, base, &ParseULongLong);
}

// @func 0x00722074 StrToULL
// strtoull().
unsigned long long StrToULL(const char* s, char** end, int base)
{
    return ParseULongLong(s, end, base, nullptr);
}

// @func 0x00721044 StrToD
// strtod(). (The original converts the digits itself with extended-precision
// scaling helpers; the host conversion is correctly rounded.)
double StrToD(const char* s, char** end)
{
    return std::strtod(s, end);
}

// ---------------------------------------------------------------------------
// atexit
// ---------------------------------------------------------------------------
namespace {

// One registered exit handler. Plain atexit() handlers take no argument;
// the library also supports handlers registered together with an argument
// (used for C++ static destructors).
struct ExitHandler {
    void (*plain)() = nullptr;
    void (*withArg)(void*) = nullptr;
    void* arg = nullptr;
};

constexpr size_t kInitialExitHandlerCapacity = 32;

std::vector<ExitHandler>& GetExitHandlers()
{
    static std::vector<ExitHandler> handlers;
    return handlers;
}

// @func 0x0071B068 AppendExitHandler
// Records one handler (with its argument, if it has one) at the end of the
// handler table. Capacity has been checked by the caller.
void AppendExitHandler(std::vector<ExitHandler>& handlers, const ExitHandler& handler)
{
    handlers.push_back(handler);
}

// @func 0x0071B260 GrowExitHandlerTable
// Makes room for `extra` more handlers. Returns true on failure.
bool GrowExitHandlerTable(std::vector<ExitHandler>& handlers, size_t extra)
{
    try {
        handlers.reserve(handlers.capacity() + extra);
    } catch (const std::bad_alloc&) {
        return true;
    }
    return false;
}

void RunAtExitHandlersAtProcessExit()
{
    RunAtExitHandlers();
}

} // namespace

// @func 0x0071B44C AtExit
// atexit(): registers `func` to run at exit, in reverse registration order.
// The table starts with room for 32 entries and grows by half when full.
// Returns 0 on success and 1 if no memory was available.
int AtExit(void (*func)())
{
    static std::atomic<bool> s_hookedHostExit{false};

    LockSysLock(kSysLock_AtExit);
    std::vector<ExitHandler>& handlers = GetExitHandlers();
    if (handlers.size() == handlers.capacity()) {
        const size_t extra = handlers.empty() ? kInitialExitHandlerCapacity : handlers.capacity() / 2;
        if (GrowExitHandlerTable(handlers, extra)) {
            UnlockSysLock(kSysLock_AtExit);
            return 1;
        }
    }
    ExitHandler handler;
    handler.plain = func;
    AppendExitHandler(handlers, handler);

    // exit() on the PC goes through the host runtime, so make sure the host
    // runs this table when the process terminates normally.
    if (!s_hookedHostExit.exchange(true))
        std::atexit(&RunAtExitHandlersAtProcessExit);
    UnlockSysLock(kSysLock_AtExit);
    return 0;
}

// @func 0x0071B094 RunAtExitHandlers
// Called by exit(): pops and runs every registered handler, newest first.
// Handlers may register further handlers; those run as well.
void RunAtExitHandlers()
{
    LockSysLock(kSysLock_AtExit);
    std::vector<ExitHandler>& handlers = GetExitHandlers();
    while (!handlers.empty()) {
        const ExitHandler handler = handlers.back();
        handlers.pop_back();
        if (handler.withArg)
            handler.withArg(handler.arg);
        else if (handler.plain)
            handler.plain();
    }
    UnlockSysLock(kSysLock_AtExit);
}

// ---------------------------------------------------------------------------
// Heap front end
//
// The console heap is a dlmalloc mspace that hands out 16-byte aligned
// blocks. On the PC the host allocator does the real work. Blocks carry a
// small header so that aligned and ordinary allocations can both be released
// with Free(); memory from these functions must be released with crt::Free.
// ---------------------------------------------------------------------------
namespace {

constexpr size_t kMinAlignment = 16;

struct BlockHeader {
    void* hostBlock;
    size_t size;
};

size_t RoundUpToPowerOfTwo(size_t value)
{
    size_t result = kMinAlignment;
    while (result < value)
        result <<= 1;
    return result;
}

void* AllocateBlock(size_t alignment, size_t size)
{
    alignment = RoundUpToPowerOfTwo(alignment);
    const size_t overhead = alignment + sizeof(BlockHeader);
    if (size > SIZE_MAX - overhead)
        return nullptr;
    void* hostBlock = std::malloc(size + overhead);
    if (!hostBlock)
        return nullptr;
    uintptr_t user = (uintptr_t)hostBlock + sizeof(BlockHeader);
    user = (user + alignment - 1) & ~(uintptr_t)(alignment - 1);
    BlockHeader* header = reinterpret_cast<BlockHeader*>(user) - 1;
    header->hostBlock = hostBlock;
    header->size = size;
    return reinterpret_cast<void*>(user);
}

BlockHeader* HeaderOf(void* ptr)
{
    return static_cast<BlockHeader*>(ptr) - 1;
}

} // namespace

// @func 0x0072A928 Malloc
// malloc().
void* Malloc(size_t size)
{
    return AllocateBlock(kMinAlignment, size);
}

// @func 0x0072A8FC Free
// free(): null is ignored.
void Free(void* ptr)
{
    if (ptr)
        std::free(HeaderOf(ptr)->hostBlock);
}

// @func 0x0072A8C4 Realloc
// realloc(): null behaves like malloc. The contents are preserved up to the
// smaller of the old and new sizes; on failure the old block is untouched.
void* Realloc(void* ptr, size_t size)
{
    if (!ptr)
        return Malloc(size);
    void* resized = Malloc(size);
    if (!resized)
        return nullptr;
    std::memcpy(resized, ptr, std::min(size, HeaderOf(ptr)->size));
    Free(ptr);
    return resized;
}

// @func 0x0072A88C MemAlign
// memalign(): alignments of 16 or less give an ordinary block; other values
// are rounded up to a power of two.
void* MemAlign(size_t alignment, size_t size)
{
    return AllocateBlock(alignment, size);
}

// ---------------------------------------------------------------------------
// abort / assert
// ---------------------------------------------------------------------------

// @func 0x0071AECC FormatHex64
// Writes `value` as 16 lower-case hex digits (no terminator) and returns `out16`.
char* FormatHex64(uint64_t value, char* out16)
{
    static const char kDigits[] = "0123456789abcdef";
    for (int i = 15; i >= 0; --i) {
        out16[i] = kDigits[value & 0xF];
        value >>= 4;
    }
    return out16;
}

// @func 0x0071AF0C Abort
// abort(): reports where it was called from and terminates the process with
// exit code 1, without running exit handlers. The console version walks the
// stack back-chain and prints every return address; only the immediate
// caller is portable to obtain here.
[[noreturn]] void Abort()
{
#if defined(_MSC_VER)
    const uint64_t caller = (uint64_t)(uintptr_t)_ReturnAddress();
#elif defined(__GNUC__) || defined(__clang__)
    const uint64_t caller = (uint64_t)(uintptr_t)__builtin_return_address(0);
#else
    const uint64_t caller = 0;
#endif
    char hex[17];
    FormatHex64(caller, hex);
    hex[16] = '\0';
    std::fputs("abort() is called from 0x", stderr);
    std::fputs(hex, stderr);
    std::fputs("\n\n\nContinue... (Lv-2 is still running.)\n", stderr);
    std::fflush(stderr);
    std::_Exit(1);
}

// @func 0x00725E88 PrintAssertFailure
// Flushes all streams and prints "In function <function> -- <expression>
// -- assertion failed" (the function part only when known) to the console.
void PrintAssertFailure(const char* expression, const char* function)
{
    std::fflush(nullptr);
    if (function) {
        std::fputs("In function ", stderr);
        std::fputs(function, stderr);
        std::fputs(" -- ", stderr);
    }
    std::fputs(expression, stderr);
    std::fputs(" -- assertion failed\n", stderr);
    std::fflush(nullptr);
}

// @func 0x00725F84 AssertFailTrap
// Assertion failure that ends in a trap instruction (a debugger break on the
// console, otherwise fatal). The PC equivalent of an unhandled trap is abort().
[[noreturn]] void AssertFailTrap(const char* expression, const char* function)
{
    PrintAssertFailure(expression, function);
    std::abort();
}

// @func 0x00725FA8 AssertFailAbort
// Assertion failure that ends in abort().
[[noreturn]] void AssertFailAbort(const char* expression, const char* function)
{
    PrintAssertFailure(expression, function);
    Abort();
}

} // namespace crt
