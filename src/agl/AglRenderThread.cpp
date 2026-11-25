// openTIT - Agl render thread control.
#include "ape/memory/ApeDebugPrim.h"
#include "agl/AglRenderThread.h"

#include "agl/AglDeferred.h"
#include "agl/AglOcclusion.h"
#include "agl/AglRenderTarget.h"

#include "ape/core/ApeThread.h"

#include <condition_variable>
#include <mutex>
#include <thread>

namespace agl {

namespace {

// Auto-reset event (the console uses LV2 event flags for this).
class Event {
public:
    void Set()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_signaled = true;
        m_cond.notify_one();
    }
    void Wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [this] { return m_signaled; });
        m_signaled = false;
    }
private:
    std::mutex m_mutex;
    std::condition_variable m_cond;
    bool m_signaled = false;
};

struct RenderThread {
    std::thread thread;
    Event renderStart;      // "WaitRenderStart"
    Event renderComplete;   // "WaitRenderComplete"
    RenderCallback callback = nullptr;
    void* userdata = nullptr;
    // An alternative job can replace the frame callback for one kick
    // (used by loading code on the console; kept for completeness).
    RenderCallback overrideJob = nullptr;
    void* overrideArg = nullptr;
    bool frameInFlight = false;
};

RenderThread* g_renderThread = nullptr;   // 0x010C5A10
void* g_deferredRelease = nullptr;        // 0x010C5A08
bool g_deferredReleasePending = false;    // 0x010C5A0C

// @func 0x00527F2C RenderThreadMain
void RenderThreadMain(RenderThread* rt)
{
    ape::RegisterRenderThread();
    for (;;) {
        rt->renderStart.Wait();
        if (!rt->overrideArg) {
            if (rt->callback)
                rt->callback(rt->userdata);
        } else if (rt->overrideJob) {
            rt->overrideJob(rt->overrideArg);
        }
        rt->renderComplete.Set();
    }
}

} // namespace

// @func 0x005129BC RenderThreadCreate
void RenderThreadCreate(RenderCallback callback, void* userdata)
{
    RenderThread* rt = new RenderThread();
    rt->callback = callback;
    rt->userdata = userdata;
    rt->thread = std::thread(RenderThreadMain, rt);
    // The thread never exits (as on the console); the process tears it down.
    rt->thread.detach();
    g_renderThread = rt;
}

// @func 0x00512BE0 RenderThreadKick
void RenderThreadKick()
{
    if (!g_renderThread)
        return;
    g_renderThread->renderStart.Set();
    g_renderThread->frameInFlight = true;
}

// @func 0x0051265C RenderThreadWait
void RenderThreadWait()
{
    if (!g_renderThread || !g_renderThread->frameInFlight)
        return;
    g_renderThread->renderComplete.Wait();
    g_renderThread->frameInFlight = false;
}

// @func 0x0051299C ScheduleDeferredRelease
void ScheduleDeferredRelease(void* object)
{
    g_deferredRelease = object;
    g_deferredReleasePending = true;
}

void* TakeDeferredRelease()
{
    if (!g_deferredReleasePending)
        return nullptr;
    g_deferredReleasePending = false;
    void* object = g_deferredRelease;
    g_deferredRelease = nullptr;
    return object;
}

namespace {

// Frame numbering shared by the main and render threads (0x010DDA78..0x010DDA88).
struct FrameCounters {
    uint32_t frame = 0;              // advanced by the main thread
    uint32_t renderFrame = 0;        // frame the render thread is drawing
    uint32_t pendingFrame = 0;       // frame handed over at the last end of frame
    uint32_t bufferIndex = 0;        // double buffered per-frame data being written
    uint32_t renderBufferIndex = 0;  // ... and being read by the render thread
};
FrameCounters g_frames;

// Queries whose release waits for the end of the frame (0x010E05D4, count at 0x010E0614).
constexpr int kMaxEndOfFrameReleases = 16;
AglOcclusionQuery* g_endOfFrameReleases[kMaxEndOfFrameReleases] = {};
uint32_t g_endOfFrameReleaseCount = 0;

// Engine services called at the end of a frame that live outside Agl:
// the debug primitive pools (0x0056B4E8 swap, 0x0056B2B8 free) and the SPU
// job dispatcher (0x0056E038, 0x0056E1F0, not connected).
void FlushDebugDraw() { ape::ApeDebug::SwapBuffers(); }
void KickFrameJobs() {}
void DispatchJobs() {}
void ResetDebugDraw() { ape::ApeDebug::FreeCurrent(); }

} // namespace

// @func 0x00527A48 QueueEndOfFrameRelease
void QueueEndOfFrameRelease(AglOcclusionQuery* query)
{
    if (g_endOfFrameReleaseCount < kMaxEndOfFrameReleases)
        g_endOfFrameReleases[g_endOfFrameReleaseCount++] = query;
}

void ResetEndOfFrameReleases()
{
    g_endOfFrameReleaseCount = 0;
}

uint32_t GetRenderFrame()
{
    return g_frames.renderFrame;
}

void AdvanceFrame()
{
    ++g_frames.frame;
}

// TODO(0x0053BF70 WaitForDisplayFlip) frame pacing: waits on the time base
// until the queued flips have been displayed.
static void WaitForDisplayFlip()
{
}

// @func 0x005126E4 FlushRendering
void FlushRendering()
{
    RenderThreadWait();
    WaitForDisplayFlip();
    // Each call advances the destruction frame, so this ends once every
    // queued object's delay has passed.
    while (!ProcessDeferredDestruction()) {
    }
}

// @func 0x005124F4 EndFrame
void EndFrame(int phase)
{
    // Phases: 0 = main thread part, 1 = render thread part, 2 = both
    // (single threaded), 3 = while loading.
    const bool mainPart = phase == 0 || phase == 2;
    const bool renderPart = phase == 1 || phase == 2;

    if (mainPart && g_endOfFrameReleaseCount != 0) {
        for (uint32_t i = 0; i < g_endOfFrameReleaseCount; ++i)
            ReleaseOcclusionQuery(g_endOfFrameReleases[i]);
        g_endOfFrameReleaseCount = 0;
    }
    if (renderPart)
        UpdateOcclusionQueries();
    if (mainPart) {
        AglRenderTarget::UpdateAll();
        FlushDebugDraw();
    }
    if (renderPart) {
        KickFrameJobs();
        DispatchJobs();
        ProcessDeferredDestruction();
        ResetDebugDraw();
        // Hand the frame over: the render thread now works on what the
        // main thread finished, and the double buffered data flips.
        g_frames.renderBufferIndex = g_frames.bufferIndex;
        g_frames.renderFrame = g_frames.pendingFrame;
        g_frames.bufferIndex ^= 1;
        g_frames.pendingFrame = g_frames.frame;
    }
    if (phase == 3) {
        AglRenderTarget::UpdateAll();
        FlushDebugDraw();
        ResetDebugDraw();
    }
}

} // namespace agl
