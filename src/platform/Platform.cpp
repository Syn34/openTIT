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

namespace { void DispatchDialogAnswer(); }

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
    DispatchDialogAnswer();
}

// ---------------------------------------------------------------------------
// System message dialog
// ---------------------------------------------------------------------------
namespace {
struct MessageDialogState {
    bool open = false;
    bool answerPending = false;
    int answer = kDialog_Other;
    MessageDialogCallback callback = nullptr;
    void* userdata = nullptr;
};
MessageDialogState g_dialog;

bool OpenDialog(DialogButtons buttons, MessageDialogCallback callback, void* userdata)
{
    std::lock_guard<std::mutex> lock(g_eventMutex);
    if (g_dialog.open)
        return false;
    g_dialog.open = true;
    g_dialog.callback = callback;
    g_dialog.userdata = userdata;
    g_dialog.answerPending = buttons != DialogButtons::None;
    g_dialog.answer = kDialog_Yes;
    return true;
}

// Runs the answer of the open dialog, if there is one waiting.
void DispatchDialogAnswer()
{
    MessageDialogCallback callback = nullptr;
    void* userdata = nullptr;
    int answer = kDialog_Other;
    {
        std::lock_guard<std::mutex> lock(g_eventMutex);
        if (!g_dialog.open || !g_dialog.answerPending)
            return;
        g_dialog.answerPending = false;
        g_dialog.open = false;
        callback = g_dialog.callback;
        userdata = g_dialog.userdata;
        answer = g_dialog.answer;
    }
    if (callback)
        callback(answer, userdata);
}
} // namespace

bool OpenMessageDialog(const char* text, const MessageDialogStyle& style, MessageDialogCallback callback,
                       void* userdata)
{
    if (!OpenDialog(style.buttons, callback, userdata))
        return false;
    Log(LogLevel::Info, "[dialog] %s", text ? text : "");
    return true;
}

bool OpenErrorDialog(uint32_t errorCode, MessageDialogCallback callback, void* userdata)
{
    if (!OpenDialog(DialogButtons::Ok, callback, userdata))
        return false;
    Log(LogLevel::Info, "[dialog] system error 0x%08X", errorCode);
    return true;
}

void CloseMessageDialog()
{
    std::lock_guard<std::mutex> lock(g_eventMutex);
    g_dialog = MessageDialogState{};
}

void AbortMessageDialog()
{
    std::lock_guard<std::mutex> lock(g_eventMutex);
    g_dialog = MessageDialogState{};
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
