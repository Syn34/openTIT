// openTIT - Sly Cooper: Thieves in Time, PC reimplementation.
//
// Platform abstraction layer.
//
// The PS3 build talks to the LV2 kernel and Sony system libraries
// (sys_lwmutex, sys_ppu_thread, cellSysutil, cellFs, ...). Game and engine
// code in this project never calls an OS API directly; it goes through the
// small interface below, which has a portable implementation in
// Platform.cpp built on the C++ standard library.
#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace plat {

// ---------------------------------------------------------------------------
// Logging (replaces the TTY output the PS3 build sent through sys_tty_write)
// ---------------------------------------------------------------------------
enum class LogLevel { Info, Warning, Error };
void Log(LogLevel level, const char* fmt, ...);
void LogV(LogLevel level, const char* fmt, va_list args);

// ---------------------------------------------------------------------------
// Time
// ---------------------------------------------------------------------------
// Microseconds since an arbitrary fixed point (sys_time_get_system_time).
uint64_t GetSystemTimeUsec();
// High resolution tick counter (the PPU time base register) and its rate.
uint64_t GetTimeBase();
uint64_t GetTimeBaseFrequency();
void SleepUsec(uint64_t usec);

// ---------------------------------------------------------------------------
// Synchronisation
// ---------------------------------------------------------------------------
// The engine uses lightweight recursive mutexes everywhere (sys_lwmutex with
// SYS_SYNC_RECURSIVE). std::recursive_mutex has the same semantics.
using Mutex = std::recursive_mutex;
using ScopedLock = std::lock_guard<std::recursive_mutex>;

// ---------------------------------------------------------------------------
// System events (cellSysutil replacement)
// ---------------------------------------------------------------------------
// Event codes match the console values so the game-side handlers can keep
// their original meaning.
enum SystemEvent : uint32_t {
    kSystemEvent_RequestExitGame = 0x0101,
    kSystemEvent_DrawingBegin    = 0x0121,
    kSystemEvent_DrawingEnd      = 0x0122,
    kSystemEvent_SystemMenuOpen  = 0x0131,
    kSystemEvent_SystemMenuClose = 0x0132,
};

using SystemEventCallback = void (*)(uint32_t event, uint32_t param, void* userdata);

// Four callback slots exist on the console; we keep the same limit.
constexpr int kNumSystemCallbackSlots = 4;
bool RegisterSystemCallback(int slot, SystemEventCallback callback, void* userdata);
bool UnregisterSystemCallback(int slot);
// Dispatches queued system events to the registered callbacks.
void DispatchSystemCallbacks();
// Queue an event (used by the windowing backend, e.g. when the window closes).
void PostSystemEvent(uint32_t event, uint32_t param = 0);

// ---------------------------------------------------------------------------
// Process
// ---------------------------------------------------------------------------
[[noreturn]] void ExitProcess(int code);

// ---------------------------------------------------------------------------
// Files
// ---------------------------------------------------------------------------
// Root directory that holds the extracted game data (the contents of
// PS3_GAME/USRDIR). Defaults to the current working directory, can be
// overridden with the OPENTIT_DATA environment variable.
const std::string& GetDataRoot();
// Translate a game path ("System\\Shaders.Zip", "/dev_hdd0/...") to a host path.
std::string ToHostPath(const char* gamePath);

class File {
public:
    enum class Mode { Read, Write, Append };
    static std::unique_ptr<File> Open(const char* gamePath, Mode mode);
    virtual ~File() = default;
    virtual size_t Read(void* dst, size_t bytes) = 0;
    virtual size_t Write(const void* src, size_t bytes) = 0;
    virtual bool Seek(int64_t offset, int whence) = 0;
    virtual int64_t Tell() const = 0;
    virtual int64_t Size() const = 0;
};
bool FileExists(const char* gamePath);

} // namespace plat
