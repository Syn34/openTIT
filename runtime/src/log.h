#pragma once

#include <cstdarg>

namespace opentit::log {

enum class Level { Trace = 0, Debug, Info, Warn, Error };

void set_level(Level l);
Level level();
void write(Level l, const char* channel, const char* fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 3, 4)))
#endif
    ;

}  // namespace opentit::log

#define LOG_TRACE(ch, ...) ::opentit::log::write(::opentit::log::Level::Trace, ch, __VA_ARGS__)
#define LOG_DEBUG(ch, ...) ::opentit::log::write(::opentit::log::Level::Debug, ch, __VA_ARGS__)
#define LOG_INFO(ch, ...) ::opentit::log::write(::opentit::log::Level::Info, ch, __VA_ARGS__)
#define LOG_WARN(ch, ...) ::opentit::log::write(::opentit::log::Level::Warn, ch, __VA_ARGS__)
#define LOG_ERROR(ch, ...) ::opentit::log::write(::opentit::log::Level::Error, ch, __VA_ARGS__)
