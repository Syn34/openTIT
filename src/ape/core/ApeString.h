// openTIT - Ape engine string helpers.
#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>

namespace ape {

// strncpy that tolerates a zero length (no write at all).
char* StrNCopy(char* dst, const char* src, size_t count);

// Bounded copy that always NUL-terminates; dstSize is the full buffer size.
char* StrCopyBounded(char* dst, const char* src, size_t dstSize);

// Thin wrappers over the C library kept by the engine's string module.
int StrLen(const char* s);
char* StrCopy(char* dst, const char* src);
int StrCmp(const char* a, const char* b);
int StrNCmp(const char* a, const char* b, size_t count);
// Case-insensitive ASCII compare (stricmp).
int StrICmp(const char* a, const char* b);
// The engine's own case-insensitive compares (letters folded to lower case);
// they return -1, 0 or 1. The bounded one stops after `count` characters.
int StrCaseCompare(const char* a, const char* b);
int StrNCaseCompare(const char* a, const char* b, int count);

// More C library wrappers.
// sprintf with no size limit.
int StrSPrintf(char* buffer, const char* fmt, ...);
int StrPrintf(char* buffer, size_t size, const char* fmt, ...);
int StrVPrintf(char* buffer, size_t size, const char* fmt, va_list args);
char* StrStr(const char* s, const char* sub);
char* StrChr(const char* s, int c);
char* StrRChr(const char* s, int c);
char* StrCat(char* dst, const char* src);
// Lower-cases the ASCII letters of `s` in place.
void StrLower(char* s);
// First character of `s` that also occurs in `set` (strpbrk), or null.
char* StrFindAny(const char* s, const char* set);
// Number parsing (no error reporting).
uint32_t StrToUInt(const char* s);
float StrToFloat(const char* s);
long StrToInt(const char* s, int base);

// Case-insensitive 31-multiplier string hash used for all engine name lookups
// (entity names, tuning keys, asset names). Lower-case ASCII letters are
// folded to upper case before hashing. Null or empty strings hash to 0.
uint32_t HashNameNoCase(const char* name);

// Splits a ';'-separated list and returns the sum of HashNameNoCase() of
// every token. Lists of 255 characters or more are rejected (return 0).
uint32_t HashNameListSum(const char* list);

// printf into one of several rotating 1 KiB scratch buffers and return it.
// The result stays valid until the same thread has formatted 8 more strings.
const char* TempFormat(const char* fmt, ...);

// Formats a value as "0x%x" using TempFormat.
const char* TempHexString(uint32_t value);

} // namespace ape
