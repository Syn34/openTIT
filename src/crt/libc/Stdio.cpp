// openTIT - C runtime <stdio.h> routines.
//
// The console library is Dinkumware's stdio on top of a cellFs file
// descriptor layer: FILE objects with their own buffers and read/write
// "prepare" steps, a list of open streams, and a printf/scanf engine that
// talks to its destination through small output/input callbacks. On the PC
// the host stdio provides streams, buffering and formatting; this file keeps
// the original entry points, the open-stream bookkeeping needed for
// CloseAllFiles(), and clean versions of the callback helpers.
#include "crt/libc/Libc.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <vector>

namespace crt {

namespace {

// Streams opened through FOpen, so that CloseAllFiles() can find them
// (the original links every dynamically allocated FILE into a list).
std::vector<FILE*>& GetOpenStreams()
{
    static std::vector<FILE*> streams;
    return streams;
}

void RegisterStream(FILE* stream)
{
    LockSysLock(kSysLock_Streams);
    GetOpenStreams().push_back(stream);
    UnlockSysLock(kSysLock_Streams);
}

void UnregisterStream(FILE* stream)
{
    LockSysLock(kSysLock_Streams);
    std::vector<FILE*>& streams = GetOpenStreams();
    streams.erase(std::remove(streams.begin(), streams.end(), stream), streams.end());
    UnlockSysLock(kSysLock_Streams);
}

} // namespace

// @func 0x0071BB1C FOpen
// fopen(): opens a stream. Game paths are translated to host paths by the
// platform layer. Modes are the usual "r", "w", "a" with optional '+'/'b'.
FILE* FOpen(const char* path, const char* mode)
{
    const std::string hostPath = plat::ToHostPath(path);
    FILE* stream = std::fopen(hostPath.c_str(), mode);
    if (stream)
        RegisterStream(stream);
    return stream;
}

// @func 0x0071B5B8 FClose
// fclose(): flushes and closes a stream. Returns 0, or EOF on failure.
int FClose(FILE* stream)
{
    if (!stream)
        return EOF;
    UnregisterStream(stream);
    return std::fclose(stream) == 0 ? 0 : EOF;
}

// @func 0x0071B774 FFlush
// fflush(): writes out buffered output. A null stream flushes every stream.
int FFlush(FILE* stream)
{
    return std::fflush(stream) == 0 ? 0 : EOF;
}

// @func 0x0071B704 CloseAllFiles
// Shutdown helper: closes every open stream, then the standard streams.
// NOTE: the host's standard streams are only flushed here, not closed, since
// the host runtime (and our own logging) may still use them afterwards.
int CloseAllFiles()
{
    LockSysLock(kSysLock_Streams);
    std::vector<FILE*> streams;
    streams.swap(GetOpenStreams());
    UnlockSysLock(kSysLock_Streams);

    int result = 0;
    for (FILE* stream : streams) {
        if (std::fclose(stream) != 0)
            result = EOF;
    }
    if (std::fflush(stdout) != 0 || std::fflush(stderr) != 0)
        result = EOF;
    return result;
}

// @func 0x0071BB90 FRead
// fread(): returns the number of complete elements read.
size_t FRead(void* dst, size_t size, size_t count, FILE* stream)
{
    return std::fread(dst, size, count, stream);
}

// @func 0x0071BF58 FWrite
// fwrite(): returns the number of complete elements written. Line-buffered
// streams flush on newline, unbuffered streams after every call.
size_t FWrite(const void* src, size_t size, size_t count, FILE* stream)
{
    return std::fwrite(src, size, count, stream);
}

// @func 0x0071BE88 FSeek
// fseek().
int FSeek(FILE* stream, long offset, int whence)
{
    return std::fseek(stream, offset, whence);
}

// @func 0x0071BEFC FTell
// ftell().
long FTell(FILE* stream)
{
    return std::ftell(stream);
}

// @func 0x0071B998 FGetC
// fgetc(): next byte as an unsigned char, or EOF.
int FGetC(FILE* stream)
{
    return std::fgetc(stream);
}

// @func 0x0071F28C UngetC
// ungetc(): pushes one character back onto an input stream; EOF is refused.
int UngetC(int c, FILE* stream)
{
    return std::ungetc(c, stream);
}

// @func 0x0072A9A4 FPutC
// fputc(): returns the character written, or EOF.
int FPutC(int c, FILE* stream)
{
    return std::fputc(c, stream);
}

// @func 0x0072AA98 FPutS
// fputs(): returns 0 on success, EOF on failure.
int FPutS(const char* s, FILE* stream)
{
    return std::fputs(s, stream) >= 0 ? 0 : EOF;
}

// @func 0x0071DE8C PutS
// puts(): writes the string and a newline to stdout. Returns 0 or EOF.
int PutS(const char* s)
{
    LockFile(stdout);
    const bool ok = std::fputs(s, stdout) >= 0 && std::fputc('\n', stdout) != EOF;
    UnlockFile(stdout);
    return ok ? 0 : EOF;
}

// @func 0x0071F770 BytesBetween
// Number of bytes from `begin` up to `end`, or 0 if `end` is not past
// `begin`. The stream position code uses it to count buffered bytes.
ptrdiff_t BytesBetween(const void* begin, const void* end)
{
    const char* b = static_cast<const char*>(begin);
    const char* e = static_cast<const char*>(end);
    return e > b ? e - b : 0;
}

// @func 0x0071F788 FileGetPos
// _Fgpos(): the current position of a stream. With `pos` the position is
// stored there and 0 is returned (fgetpos); without it the offset itself is
// returned (ftell). Returns -1 on failure.
long FileGetPos(FILE* stream, std::fpos_t* pos)
{
    if (pos)
        return std::fgetpos(stream, pos) == 0 ? 0 : -1;
    return std::ftell(stream);
}

// @func 0x00720114 FileSetPos
// _Fspos(): repositions a stream, either to a saved position plus `offset`
// (fsetpos) or by `offset` relative to `whence` (fseek). Buffered input and
// pushed-back characters are discarded. Returns 0, or -1 on failure.
int FileSetPos(FILE* stream, const std::fpos_t* pos, long offset, int whence)
{
    if (pos) {
        if (std::fsetpos(stream, pos) != 0)
            return -1;
        if (offset != 0 && std::fseek(stream, offset, SEEK_CUR) != 0)
            return -1;
        return 0;
    }
    return std::fseek(stream, offset, whence) == 0 ? 0 : -1;
}

// @func 0x0071FDA0 FileReadPrepare
// _Frprep(): makes input available for reading. Returns 1 if there is data,
// 0 at end of file and -1 on a read error.
int FileReadPrepare(FILE* stream)
{
    const int c = std::getc(stream);
    if (c == EOF)
        return std::ferror(stream) ? -1 : 0;
    std::ungetc(c, stream);
    return 1;
}

// ---------------------------------------------------------------------------
// printf family
// ---------------------------------------------------------------------------

// @func 0x007240F4 Printf
// printf().
int Printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int result = std::vprintf(fmt, args);
    va_end(args);
    return result;
}

// @func 0x00723FEC FPrintf
// fprintf().
// NOTE: the original takes stdout's lock around the call, not the lock of
// `stream`; host streams lock themselves.
int FPrintf(FILE* stream, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int result = std::vfprintf(stream, fmt, args);
    va_end(args);
    return result;
}

// @func 0x00724304 SPrintf
// sprintf().
int SPrintf(char* dst, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int result = std::vsprintf(dst, fmt, args);
    va_end(args);
    return result;
}

// @func 0x007241F4 SNPrintf
// snprintf(): writes at most size-1 characters plus a terminator and returns
// the length the full output would have had.
int SNPrintf(char* dst, size_t size, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int result = std::vsnprintf(dst, size, fmt, args);
    va_end(args);
    return result;
}

// @func 0x007244A0 VPrintf
// vprintf().
int VPrintf(const char* fmt, va_list args)
{
    return std::vprintf(fmt, args);
}

// @func 0x007243BC VFPrintf
// vfprintf().
int VFPrintf(FILE* stream, const char* fmt, va_list args)
{
    return std::vfprintf(stream, fmt, args);
}

// @func 0x00724588 VSPrintf
// vsprintf().
int VSPrintf(char* dst, const char* fmt, va_list args)
{
    return std::vsprintf(dst, fmt, args);
}

// @func 0x0071F36C VSNPrintf
// vsnprintf().
int VSNPrintf(char* dst, size_t size, const char* fmt, va_list args)
{
    return std::vsnprintf(dst, size, fmt, args);
}

// @func 0x0071F3DC WriteToBoundedBuffer
// @func 0x00724280 WriteToBoundedBuffer
// @func 0x0072F570 WriteToBoundedBuffer (copy used by the SPU printf server)
// Output sink of snprintf/vsnprintf: copies as much of `text` as still fits
// and advances the buffer. Never fails, so formatting carries on counting.
void* WriteToBoundedBuffer(void* state, const char* text, size_t length)
{
    BoundedBuffer* buffer = static_cast<BoundedBuffer*>(state);
    const size_t count = std::min(length, buffer->room);
    if (count != 0) {
        std::memcpy(buffer->next, text, count);
        buffer->next += count;
        buffer->room -= count;
    }
    return buffer;
}

// @func 0x00724380 WriteToBuffer
// @func 0x007245E4 WriteToBuffer
// Output sink of sprintf/vsprintf: appends `text` and returns the new end.
void* WriteToBuffer(void* state, const char* text, size_t length)
{
    char* dst = static_cast<char*>(state);
    std::memcpy(dst, text, length);
    return dst + length;
}

// @func 0x00724084 WriteToStream
// @func 0x00724184 WriteToStream
// @func 0x00724430 WriteToStream
// @func 0x00724518 WriteToStream
// Output sink of the stream printf variants: writes `text` to the FILE and
// returns it, or null if not everything could be written.
void* WriteToStream(void* state, const char* text, size_t length)
{
    FILE* stream = static_cast<FILE*>(state);
    return std::fwrite(text, 1, length, stream) == length ? stream : nullptr;
}

// @func 0x00724620 FormatToSink
// _Printf(): formats `fmt` with `args` and passes the text to `sink`.
// Returns the number of characters produced, or -1 if formatting failed or
// the sink reported an error.
int FormatToSink(PrintSink sink, void* state, const char* fmt, va_list args)
{
    char small[512];
    va_list measure;
    va_copy(measure, args);
    const int length = std::vsnprintf(small, sizeof(small), fmt, measure);
    va_end(measure);
    if (length < 0)
        return -1;

    const char* text = small;
    std::vector<char> large;
    if ((size_t)length >= sizeof(small)) {
        large.resize((size_t)length + 1);
        std::vsnprintf(large.data(), large.size(), fmt, args);
        text = large.data();
    }
    if (length > 0 && !sink(state, text, (size_t)length))
        return -1;
    return length;
}

// ---------------------------------------------------------------------------
// scanf family
// ---------------------------------------------------------------------------

// @func 0x0071BD94 FScanf
// fscanf().
// NOTE: like fprintf, the original holds stdin's lock rather than `stream`'s.
int FScanf(FILE* stream, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int result = std::vfscanf(stream, fmt, args);
    va_end(args);
    return result;
}

// @func 0x0071EC78 SScanf
// sscanf().
int SScanf(const char* src, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int result = std::vsscanf(src, fmt, args);
    va_end(args);
    return result;
}

// @func 0x0071BE2C ScanFromStream
// Input source of fscanf: `state` is the FILE.
int ScanFromStream(void* state, int c, bool get)
{
    FILE* stream = static_cast<FILE*>(state);
    return get ? std::fgetc(stream) : std::ungetc(c, stream);
}

// @func 0x0071EC38 ScanFromString
// Input source of sscanf: `state` points at the read cursor into the source
// string. The terminator reads as EOF; pushing back just steps the cursor back.
int ScanFromString(void* state, int c, bool get)
{
    const char** cursor = static_cast<const char**>(state);
    if (!get) {
        --*cursor;
        return c;
    }
    const unsigned char next = (unsigned char)**cursor;
    if (next == '\0')
        return EOF;
    ++*cursor;
    return next;
}

// @func 0x0072D458 ScanSetContains
// Membership test for a scanf "%[...]" scan set of `length` characters.
// "a-z" style ranges are honoured wherever a '-' has characters on both
// sides; a '-' at either end is an ordinary member.
bool ScanSetContains(const char* set, unsigned char c, size_t length)
{
    const unsigned char* p = reinterpret_cast<const unsigned char*>(set);
    while (length >= 3) {
        if (p[1] == '-') {
            if (p[0] <= c && c <= p[2])
                return true;
            p += 3;
            length -= 3;
        } else {
            if (p[0] == c)
                return true;
            ++p;
            --length;
        }
    }
    for (; length != 0; ++p, --length) {
        if (*p == c)
            return true;
    }
    return false;
}

} // namespace crt
