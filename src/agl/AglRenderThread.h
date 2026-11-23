// openTIT - Agl render thread control.
#pragma once

namespace agl {

using RenderCallback = void (*)();

// Starts the "AglRenderThread", which runs `callback` once per kicked frame.
void RenderThreadCreate(RenderCallback callback, void* userdata);
// Signals the render thread to start rendering the current frame.
void RenderThreadKick();
// Blocks until the frame kicked last has finished rendering.
void RenderThreadWait();
// Releases an object after the GPU is done with it (next frame).
void ScheduleDeferredRelease(void* object);
// End-of-frame bookkeeping for the graphics layer (phase 0..3).
void EndFrame(int phase);

} // namespace agl
