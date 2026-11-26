// openTIT - Deferred destruction of GPU resources.
#include "agl/AglDeferred.h"

#include "ape/memory/ApeTFreeList.h"

namespace agl {

namespace {

// Frames a released object is kept alive: the one being recorded and the
// one the GPU may still be drawing.
constexpr uint32_t kDestroyDelay = 2;

struct AglDeferredDestructionObj {
    void* object;
    uint32_t frame;          // frame at which the object may be destroyed
    DestroyFn destroy;
};

using DeferredAllocator = ape::ApeTListAllocator<AglDeferredDestructionObj>;

uint32_t g_frame = 0;        // 0x010DDC8C
bool g_enabled = false;      // 0x010DDC90

} // namespace

void StartDeferredDestruction()
{
    DeferredAllocator::GetAllocator();
    g_enabled = true;
    g_frame = 1;
}

void SetDeferredDestructionEnabled(bool enabled)
{
    g_enabled = enabled;
}

uint32_t GetDeferredDestructionFrame()
{
    return g_frame;
}

// @func 0x005082F4 AglDeferredDestruction::Schedule
void ScheduleDestroy(void* object, DestroyFn destroy)
{
    if (!g_enabled) {
        destroy(object);
        return;
    }
    auto* record = DeferredAllocator::GetAllocator().AllocT();
    if (!record)
        return;
    record->object = object;
    record->frame = g_frame + kDestroyDelay;
    record->destroy = destroy;
}

// @func 0x00512148 AglDeferredDestruction::Process
bool ProcessDeferredDestruction()
{
    auto& pool = DeferredAllocator::GetAllocator();
    pool.ForEach([&](AglDeferredDestructionObj* record) {
        if (record->frame <= g_frame) {
            record->destroy(record->object);
            pool.Free(record);
        }
    });
    ++g_frame;
    return pool.First() == nullptr;
}

} // namespace agl
