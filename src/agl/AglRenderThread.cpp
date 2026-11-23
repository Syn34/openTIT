// openTIT - Agl render thread control.
#include "agl/AglRenderThread.h"

namespace agl {

namespace {
RenderCallback g_renderCallback = nullptr;
bool g_frameKicked = false;
}

// TODO(0x005129BC RenderThreadCreate): the original spawns a dedicated thread
// waiting on the WaitRenderStart / WaitRenderComplete semaphores. Until the
// renderer exists the callback runs synchronously in RenderThreadWait().
void RenderThreadCreate(RenderCallback callback, void* userdata)
{
    g_renderCallback = callback;
}

// TODO(0x00512BE0 RenderThreadKick)
void RenderThreadKick()
{
    g_frameKicked = true;
}

// TODO(0x0051265C RenderThreadWait)
void RenderThreadWait()
{
    if (!g_frameKicked)
        return;
    if (g_renderCallback)
        g_renderCallback();
    g_frameKicked = false;
}

// TODO(0x0051299C ScheduleDeferredRelease)
void ScheduleDeferredRelease(void* object)
{
}

// TODO(0x005124F4 EndFrame)
void EndFrame(int phase)
{
}

} // namespace agl
