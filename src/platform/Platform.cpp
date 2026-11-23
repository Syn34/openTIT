// openTIT - portable implementation of the platform layer (C++17 standard library only).
#include "platform/Platform.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <thread>

namespace plat {

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------
void LogV(LogLevel level, const char* fmt, va_list args)
{
    static std::mutex s_logMutex;
    std::lock_guard<std::mutex> lock(s_logMutex);
    FILE* out = (level == LogLevel::Info) ? stdout : stderr;
    if (level == LogLevel::Warning) std::fputs("[warn] ", out);
    if (level == LogLevel::Error)   std::fputs("[error] ", out);
    std::vfprintf(out, fmt, args);
    std::fflush(out);
}

void Log(LogLevel level, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogV(level, fmt, args);
    va_end(args);
}

// ---------------------------------------------------------------------------
// Time
// ---------------------------------------------------------------------------
namespace {
const auto g_clockStart = std::chrono::steady_clock::now();
}

uint64_t GetSystemTimeUsec()
{
    return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - g_clockStart).count();
}

uint64_t GetTimeBase()
{
    return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - g_clockStart).count();
}

uint64_t GetTimeBaseFrequency()
{
    return 1000000000ull;
}

void SleepUsec(uint64_t usec)
{
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
}

// ---------------------------------------------------------------------------
// System events
// ---------------------------------------------------------------------------
namespace {
struct CallbackSlot {
    SystemEventCallback callback = nullptr;
    void* userdata = nullptr;
};
struct PendingEvent {
    uint32_t event;
    uint32_t param;
};
std::mutex g_eventMutex;
CallbackSlot g_callbackSlots[kNumSystemCallbackSlots];
std::deque<PendingEvent> g_pendingEvents;
}

bool RegisterSystemCallback(int slot, SystemEventCallback callback, void* userdata)
{
    if (slot < 0 || slot >= kNumSystemCallbackSlots || !callback)
        return false;
    std::lock_guard<std::mutex> lock(g_eventMutex);
    g_callbackSlots[slot] = { callback, userdata };
    return true;
}

bool UnregisterSystemCallback(int slot)
{
    if (slot < 0 || slot >= kNumSystemCallbackSlots)
        return false;
    std::lock_guard<std::mutex> lock(g_eventMutex);
    g_callbackSlots[slot] = {};
    return true;
}

void PostSystemEvent(uint32_t event, uint32_t param)
{
    std::lock_guard<std::mutex> lock(g_eventMutex);
    g_pendingEvents.push_back({ event, param });
}

void DispatchSystemCallbacks()
{
    std::deque<PendingEvent> events;
    CallbackSlot slots[kNumSystemCallbackSlots];
    {
        std::lock_guard<std::mutex> lock(g_eventMutex);
        events.swap(g_pendingEvents);
        for (int i = 0; i < kNumSystemCallbackSlots; ++i)
            slots[i] = g_callbackSlots[i];
    }
    for (const PendingEvent& e : events)
        for (const CallbackSlot& s : slots)
            if (s.callback)
                s.callback(e.event, e.param, s.userdata);
}

// ---------------------------------------------------------------------------
// Process
// ---------------------------------------------------------------------------
void ExitProcess(int code)
{
    std::fflush(nullptr);
    std::exit(code);
}

// ---------------------------------------------------------------------------
// Files
// ---------------------------------------------------------------------------
const std::string& GetDataRoot()
{
    static const std::string root = [] {
        const char* env = std::getenv("OPENTIT_DATA");
        std::string r = env ? env : ".";
        if (!r.empty() && r.back() != '/' && r.back() != '\\')
            r += '/';
        return r;
    }();
    return root;
}

std::string ToHostPath(const char* gamePath)
{
    std::string p = gamePath ? gamePath : "";
    for (char& c : p)
        if (c == '\\') c = '/';
    // Strip console mount points; everything lives under the data root.
    static const char* const kMounts[] = { "/app_home/", "/dev_bdvd/PS3_GAME/USRDIR/", "/dev_hdd0/game/" };
    for (const char* m : kMounts) {
        size_t n = std::char_traits<char>::length(m);
        if (p.compare(0, n, m) == 0) { p.erase(0, n); break; }
    }
    while (!p.empty() && p.front() == '/')
        p.erase(0, 1);
    return GetDataRoot() + p;
}

namespace {
class StdFile final : public File {
public:
    explicit StdFile(FILE* f) : m_file(f) {}
    ~StdFile() override { if (m_file) std::fclose(m_file); }
    size_t Read(void* dst, size_t bytes) override { return std::fread(dst, 1, bytes, m_file); }
    size_t Write(const void* src, size_t bytes) override { return std::fwrite(src, 1, bytes, m_file); }
    bool Seek(int64_t offset, int whence) override
    {
#if defined(_WIN32)
        return _fseeki64(m_file, offset, whence) == 0;
#else
        return fseeko(m_file, (off_t)offset, whence) == 0;
#endif
    }
    int64_t Tell() const override
    {
#if defined(_WIN32)
        return _ftelli64(m_file);
#else
        return (int64_t)ftello(m_file);
#endif
    }
    int64_t Size() const override
    {
        int64_t here = Tell();
        const_cast<StdFile*>(this)->Seek(0, SEEK_END);
        int64_t size = Tell();
        const_cast<StdFile*>(this)->Seek(here, SEEK_SET);
        return size;
    }
private:
    FILE* m_file;
};
}

std::unique_ptr<File> File::Open(const char* gamePath, Mode mode)
{
    const std::string host = ToHostPath(gamePath);
    const char* fmode = mode == Mode::Read ? "rb" : mode == Mode::Write ? "wb" : "ab";
    FILE* f = std::fopen(host.c_str(), fmode);
    if (!f)
        return nullptr;
    return std::make_unique<StdFile>(f);
}

bool FileExists(const char* gamePath)
{
    std::error_code ec;
    return std::filesystem::is_regular_file(ToHostPath(gamePath), ec);
}

} // namespace plat
