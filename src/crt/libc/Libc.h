// openTIT - Statically linked C runtime (SN Systems / Sony libc), PC equivalents.
//
// The PS3 executable carries its own copy of the C library (a Dinkumware
// derived libc with Sony's PPU-optimised string routines, a dlmalloc heap and
// a small cellFs backed file-descriptor layer). On the PC the host C/C++
// runtime provides all of this, so most entry points below are thin, named
// wrappers around the host standard library. They exist so that ported game
// code has one documented place that corresponds to each original routine.
//
// Internal helpers of the original library (stream locks, printf output
// sinks, time conversion, the qsort introsort pieces, ...) are given clean
// implementations of their own where that makes sense on the host.
//
// Notes on types: the PPU ABI used by the game is ILP32, so `long` is 32 bits
// on the console. Functions whose behaviour depends on that width (strtol,
// strtoul) take/return int32_t/uint32_t here.
#pragma once

#include "platform/Platform.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cwchar>
#include <exception>
#include <memory>

namespace crt {

// ---------------------------------------------------------------------------
// errno, locks
// ---------------------------------------------------------------------------
int* ErrnoLocation();

// System-wide library locks (_Locksyslock / _Unlocksyslock). Lock indices used
// by the original: 1 = atexit table, 2 = stream list.
enum SysLock : int { kSysLock_Malloc = 0, kSysLock_AtExit = 1, kSysLock_Streams = 2 };
void InitSysLocks();
void DestroySysLocks();
void LockSysLock(int index);
void UnlockSysLock(int index);
void LockFile(FILE* stream);
void UnlockFile(FILE* stream);

// Recursive mutex used by the library (_Mtxinit/_Mtxdst/_Mtxlock/_Mtxunlock).
// On the console this is a lock count + owner thread id + sys_lwmutex; the
// platform mutex is already recursive, so it only needs the mutex itself.
struct LibMutex {
    std::unique_ptr<plat::Mutex> impl;
};
bool MutexInit(LibMutex* mutex);
void MutexDestroy(LibMutex* mutex);
void MutexLock(LibMutex* mutex);
void MutexUnlock(LibMutex* mutex);

// ---------------------------------------------------------------------------
// <string.h>
// ---------------------------------------------------------------------------
void* MemChr(const void* s, int c, size_t n);
void* MemCpy(void* dst, const void* src, size_t n);
void* MemMove(void* dst, const void* src, size_t n);
void* MemSet(void* dst, int c, size_t n);
int MemCmp(const void* a, const void* b, size_t n);
size_t StrLen(const char* s);
char* StrCpy(char* dst, const char* src);
char* StrNCpy(char* dst, const char* src, size_t n);
char* StrCat(char* dst, const char* src);
int StrCmp(const char* a, const char* b);
int StrNCmp(const char* a, const char* b, size_t n);
int StrCaseCmp(const char* a, const char* b);
char* StrChr(const char* s, int c);
char* StrRChr(const char* s, int c);
char* StrStr(const char* haystack, const char* needle);
size_t StrSpn(const char* s, const char* accept);
size_t StrCSpn(const char* s, const char* reject);
char* StrTok(char* s, const char* delimiters);
char* StrError(int errnum);
char* StrErrorToBuffer(int errnum, char* buffer);
int WcToB(wint_t wc);
int WcToMb(char* dst, wchar_t wc, std::mbstate_t* state);

// ---------------------------------------------------------------------------
// <stdlib.h>
// ---------------------------------------------------------------------------
int Abs(int value);
std::div_t Div(int numerator, int denominator);
using CompareFunc = int (*)(const void*, const void*);
void* BSearch(const void* key, const void* base, size_t count, size_t size, CompareFunc compare);
void QSort(void* base, size_t count, size_t size, CompareFunc compare);
uint32_t ParseULong(const char* s, char** end, int base, int* error);
unsigned long long ParseULongLong(const char* s, char** end, int base, int* error);
int32_t StrToL(const char* s, char** end, int base);
uint32_t StrToUL(const char* s, char** end, int base);
long long StrToLL(const char* s, char** end, int base);
unsigned long long StrToULL(const char* s, char** end, int base);
double StrToD(const char* s, char** end);

int AtExit(void (*func)());
void RunAtExitHandlers();

void* Malloc(size_t size);
void Free(void* ptr);
void* Realloc(void* ptr, size_t size);
void* MemAlign(size_t alignment, size_t size);

[[noreturn]] void Abort();
char* FormatHex64(uint64_t value, char* out16);
void PrintAssertFailure(const char* expression, const char* function);
[[noreturn]] void AssertFailTrap(const char* expression, const char* function);
[[noreturn]] void AssertFailAbort(const char* expression, const char* function);

// ---------------------------------------------------------------------------
// <stdio.h>
// ---------------------------------------------------------------------------
FILE* FOpen(const char* path, const char* mode);
int FClose(FILE* stream);
int FFlush(FILE* stream);
int CloseAllFiles();
size_t FRead(void* dst, size_t size, size_t count, FILE* stream);
size_t FWrite(const void* src, size_t size, size_t count, FILE* stream);
int FSeek(FILE* stream, long offset, int whence);
long FTell(FILE* stream);
int FGetC(FILE* stream);
int UngetC(int c, FILE* stream);
int FPutC(int c, FILE* stream);
int FPutS(const char* s, FILE* stream);
int PutS(const char* s);

long FileGetPos(FILE* stream, std::fpos_t* pos);
int FileSetPos(FILE* stream, const std::fpos_t* pos, long offset, int whence);
int FileReadPrepare(FILE* stream);
ptrdiff_t BytesBetween(const void* begin, const void* end);

int Printf(const char* fmt, ...);
int FPrintf(FILE* stream, const char* fmt, ...);
int SPrintf(char* dst, const char* fmt, ...);
int SNPrintf(char* dst, size_t size, const char* fmt, ...);
int VPrintf(const char* fmt, va_list args);
int VFPrintf(FILE* stream, const char* fmt, va_list args);
int VSPrintf(char* dst, const char* fmt, va_list args);
int VSNPrintf(char* dst, size_t size, const char* fmt, va_list args);
int FScanf(FILE* stream, const char* fmt, ...);
int SScanf(const char* src, const char* fmt, ...);

// printf output sinks: return the (possibly advanced) sink state, or null on failure.
using PrintSink = void* (*)(void* state, const char* text, size_t length);
struct BoundedBuffer {
    char* next;
    size_t room;
};
void* WriteToBoundedBuffer(void* state, const char* text, size_t length);
void* WriteToBuffer(void* state, const char* text, size_t length);
void* WriteToStream(void* state, const char* text, size_t length);
int FormatToSink(PrintSink sink, void* state, const char* fmt, va_list args);

// scanf input sources: get the next character (get == true, EOF at the end)
// or push `c` back (get == false).
int ScanFromStream(void* state, int c, bool get);
int ScanFromString(void* state, int c, bool get);
bool ScanSetContains(const char* set, unsigned char c, size_t length);

// ---------------------------------------------------------------------------
// File descriptors (the cellFs layer under stdio)
// ---------------------------------------------------------------------------
enum OpenFlags : int {
    kOpen_ReadOnly = 0x000,
    kOpen_WriteOnly = 0x001,
    kOpen_ReadWrite = 0x002,
    kOpen_AccessMask = 0x003,
    kOpen_Create = 0x040,
    kOpen_Truncate = 0x200,
    kOpen_Append = 0x400,
};
int Open(const char* path, int flags, int mode);
int Close(int fd);
int Read(int fd, void* dst, size_t bytes);
int Write(int fd, const void* src, size_t bytes);
int32_t LSeek(int fd, int32_t offset, int whence);
int64_t LSeek64(int fd, int64_t offset, int whence);
int OpenForStream(const char* path, unsigned streamMode);

// ---------------------------------------------------------------------------
// <time.h>
// ---------------------------------------------------------------------------
std::time_t Time(std::time_t* out);
std::tm* GmTime(const std::time_t* t);
std::tm* LocalTime(const std::time_t* t);
std::time_t MkTime(std::tm* t);
int DaysToMonth(int year, int month);
std::tm* SecondsToTm(std::tm* out, int64_t secondsSince1970, int isDst);

// ---------------------------------------------------------------------------
// <math.h> internals
// ---------------------------------------------------------------------------
// Classification codes returned by the scale helpers (Dinkumware convention).
enum FloatClass : short { kFloat_Finite = -1, kFloat_Zero = 0, kFloat_Inf = 1, kFloat_NaN = 2 };
short FloatScale(float* value, long exponent);
short FloatUnscale(short* exponent, float* value);
short DoubleScale(double* value, long exponent);
short LongDoubleScale(double* value, long exponent);

// single precision libm (FloatMath.cpp)
float SqrtF(float x);
float FModF(float x, float y);
float TanF(float x);
float ASinF(float x);
float ACosF(float x);
float ATanF(float x);
float ATan2F(float y, float x);
float ATanKernelF(float x, uint32_t octant);
int FloatClassify(const float* value);

// ---------------------------------------------------------------------------
// C++ runtime support (CxxRuntime.cpp)
// ---------------------------------------------------------------------------
[[noreturn]] void ReportUncaughtException(const std::exception& exception);
void* OperatorNewAligned(size_t size, size_t alignment);
void* OperatorNewArrayAligned(size_t size, size_t alignment);
void OperatorDelete(void* block);
[[noreturn]] void DefaultTerminateHandler();
std::terminate_handler SetTerminateHandler(std::terminate_handler handler);
[[noreturn]] void Terminate();
[[noreturn]] void PureVirtualCalled();
void* MemCpyAligned(void* dst, const void* src, size_t bytes);

} // namespace crt
