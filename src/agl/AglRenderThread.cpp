// openTIT - Agl render thread control.
#include "agl/AglRenderThread.h"

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

// TODO(0x005124F4 EndFrame)
void EndFrame(int phase)
{
}

} // namespace agl
