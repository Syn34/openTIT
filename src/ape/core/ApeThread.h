// openTIT - Ape engine thread identity helpers.
#pragma once

namespace ape {

// The engine remembers which OS thread runs the game loop ("main") and which
// one runs the renderer. Several systems pick per-thread scratch resources
// based on these checks.
void RegisterMainThread();
void RegisterRenderThread();
void UnregisterRenderThread();

// True when called on the registered main thread, or if none is registered yet.
bool IsMainThread();
// True when called on the registered render thread, or if none is registered yet.
bool IsRenderThread();

} // namespace ape
