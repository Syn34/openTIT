// openTIT - Ape engine thread identity helpers.
#include "ape/core/ApeThread.h"

#include <atomic>
#include <thread>

namespace ape {

namespace {
// A default-constructed std::thread::id means "not registered".
std::thread::id g_mainThread;
std::thread::id g_renderThread;

bool IsCurrentOrUnset(const std::thread::id& id)
{
    return id == std::thread::id() || id == std::this_thread::get_id();
}
}

// Part of 0x005DB1D0 (ApeSystemInit): the main thread records its id first thing.
void RegisterMainThread()
{
    g_mainThread = std::this_thread::get_id();
}

void RegisterRenderThread()
{
    g_renderThread = std::this_thread::get_id();
}

void UnregisterRenderThread()
{
    g_renderThread = std::thread::id();
}

// @func 0x005DB0A8 IsMainThread
bool IsMainThread()
{
    return IsCurrentOrUnset(g_mainThread);
}

// @func 0x005DB13C IsRenderThread
// NOTE: the second registered thread id is assumed to be the render thread;
// it is written by the renderer start-up code (not yet reimplemented).
bool IsRenderThread()
{
    return IsCurrentOrUnset(g_renderThread);
}

} // namespace ape
