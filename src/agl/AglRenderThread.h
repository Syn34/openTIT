// openTIT - Agl render thread control.
//
// The game simulates frame N+1 on the main thread while a dedicated
// "AglRenderThread" draws frame N. Two events hand the frame over:
// WaitRenderStart (main -> render) and WaitRenderComplete (render -> main).
#pragma once

#include <cstdint>

namespace agl {

using RenderCallback = void (*)(void* userdata);

// Starts the render thread, which runs `callback(userdata)` once per kicked frame.
void RenderThreadCreate(RenderCallback callback, void* userdata);
// Signals the render thread to start rendering the current frame.
void RenderThreadKick();
// Blocks until the frame kicked last has finished rendering.
void RenderThreadWait();
// Hands an object to the renderer to be released once the GPU is done with it.
void ScheduleDeferredRelease(void* object);
// Object queued by ScheduleDeferredRelease (consumed by the renderer).
void* TakeDeferredRelease();
// End-of-frame bookkeeping for the graphics layer (phase 0..3).
void EndFrame(int phase);
// Releases an occlusion query at the next end of frame (main thread part).
void QueueEndOfFrameRelease(class AglOcclusionQuery* query);
void ResetEndOfFrameReleases();
// Frame the render thread is drawing (materials update animations once per frame).
uint32_t GetRenderFrame();
void AdvanceFrame();

} // namespace agl
