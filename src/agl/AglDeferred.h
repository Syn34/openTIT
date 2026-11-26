// openTIT - Deferred destruction of GPU resources.
//
// The GPU renders a frame after the CPU recorded it, so a resource released
// by game code may still be referenced by command lists in flight. Released
// resources are queued and destroyed two frames later.
#pragma once

#include <cstdint>

namespace agl {

using DestroyFn = void (*)(void* object);

// Destroys 'object' with 'destroy' once the GPU can no longer use it (or
// immediately when deferral is disabled).
void ScheduleDestroy(void* object, DestroyFn destroy);
// Runs the destructions that are due and advances the frame counter.
// Returns true when nothing is left queued.
bool ProcessDeferredDestruction();
void SetDeferredDestructionEnabled(bool enabled);
// Agl start-up: creates the record pool, enables deferral and starts at frame 1.
void StartDeferredDestruction();
uint32_t GetDeferredDestructionFrame();

// Common base of reference counted Agl resources (count stored as a
// signed 16-bit value as on the console).
struct AglRefCounted {
    int16_t refCount = 1;
    // 1-based slot of the scene that loaded the resource (0 for none); the
    // console keeps it in bits 6..9 of the word holding the count.
    uint8_t sceneSlot = 0;

    void AddRef() { ++refCount; }
    // Drops a reference; true when this was the last one.
    bool DropRef() { return --refCount < 1; }
};

} // namespace agl
