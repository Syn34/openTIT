// openTIT - Ape engine string helpers.
#pragma once

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
