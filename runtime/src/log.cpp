#include "log.h"

#include <atomic>
#include <cstdio>
#include <mutex>

namespace opentit::log {

namespace {
std::atomic<Level> g_level{Level::Info};
std::mutex g_mutex;
const char* tag(Level l) {
    switch (l) {
    case Level::Trace: return "T";
    case Level::Debug: return "D";
    case Level::Info: return "I";
    case Level::Warn: return "W";
    default: return "E";
    }
}
}  // namespace

void set_level(Level l) { g_level = l; }
Level level() { return g_level; }

void write(Level l, const char* channel, const char* fmt, ...) {
    if (l < g_level) return;
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    std::lock_guard<std::mutex> lk(g_mutex);
    std::fprintf(stderr, "[%s][%s] %s\n", tag(l), channel, buf);
}

}  // namespace opentit::log
