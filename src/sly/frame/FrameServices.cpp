// openTIT - Per-frame services (end-of-frame flips and callbacks).
#include "sly/frame/FrameServices.h"

#include <vector>

namespace sly {

void FrameBuffer::Reset()
{
    cursor = begin;
    limit = end;
    usedA = 0;
    usedB = 0;
    ++frame;
    flags &= 0x3F;
}

void FrameBufferPair::Swap()
{
    previous = current;
    current = (current == first) ? second : first;
    current->Reset();
}

namespace {

struct PendingCallback {
    EndOfFrameFn fn;
    void* arg;
};

std::vector<PendingCallback> g_endOfFrameCallbacks;   // 0x010BB854 (count at 0x010BB85C)
FrameBufferPair* g_gameFrameBuffers = nullptr;        // 0x00F65C90
FrameBufferPair* g_debugFrameBuffers = nullptr;       // 0x010AA614
bool g_debugSwapRequested = false;                    // bit 4 of 0x010AA1B0
uint32_t g_frameCounters[2] = {};                     // 0x01195090
uint32_t g_frameCounterIndex = 0;                     // 0x01195098

} // namespace

void QueueEndOfFrameCallback(EndOfFrameFn fn, void* arg)
{
    g_endOfFrameCallbacks.push_back({ fn, arg });
}

void RunEndOfFrameCallbacks()
{
    // Callbacks may queue more work for the next frame, so run a snapshot.
    std::vector<PendingCallback> pending;
    pending.swap(g_endOfFrameCallbacks);
    for (const PendingCallback& cb : pending) {
        if (cb.fn)
            cb.fn(cb.arg);
    }
}

FrameBufferPair* GetGameFrameBuffers() { return g_gameFrameBuffers; }
void SetGameFrameBuffers(FrameBufferPair* pair) { g_gameFrameBuffers = pair; }

// @func 0x005AC60C FlipFrameCounters
void FlipFrameCounters()
{
    g_frameCounterIndex ^= 1;
    g_frameCounters[g_frameCounterIndex] = 0;
}

uint32_t& CurrentFrameCounter() { return g_frameCounters[g_frameCounterIndex]; }

void RequestDebugBufferSwap() { g_debugSwapRequested = true; }
void SetDebugFrameBuffers(FrameBufferPair* pair) { g_debugFrameBuffers = pair; }

// @func 0x004291C8 SwapDebugBuffersIfRequested
void SwapDebugBuffersIfRequested()
{
    if (g_debugSwapRequested && g_debugFrameBuffers)
        g_debugFrameBuffers->Swap();
    g_debugSwapRequested = false;
}

// @func 0x0042AD98 EndFrameHook
// Empty in the shipped game (a debug hook compiled out).
void EndFrameHook()
{
}

} // namespace sly
