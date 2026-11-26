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
// Start of a render frame: advances the renderer clocks, resets the GPU
// state and binds the back buffer, starts the draw list's frame.
void BeginRenderFrame();
// GPU side of the frame start: gamma, render state reset, stereo eye and
// (with 'clearBackBuffer') a full clear of the back buffer.
void BeginGpuFrame(bool clearBackBuffer);
// BeginGpuFrame without the clear, then the back buffer bound (and
// cleared with its own clear settings).
void BeginBackBufferFrame();
// Main thread update of the graphics layer (counts updates, then the
// platform part, which has nothing to do). Always true.
bool AglUpdate();
bool AglPlatformUpdate();
uint32_t GetAglUpdateCount();
// Seconds the renderer has run (0x010DD9E0), the sum of the frame times.
float GetRenderTime();
// Waits until the render thread and the GPU are idle and every deferred
// destruction has run (used before a scene is torn down).
void FlushRendering();
// Releases an occlusion query at the next end of frame (main thread part).
void QueueEndOfFrameRelease(class AglOcclusionQuery* query);
void ResetEndOfFrameReleases();
// Frame the render thread is drawing (materials update animations once per frame).
uint32_t GetRenderFrame();
void AdvanceFrame();

} // namespace agl
