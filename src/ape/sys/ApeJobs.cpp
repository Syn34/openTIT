// openTIT - Engine job queue front end.
#include "ape/sys/ApeJobs.h"

#include "platform/Platform.h"

#include <cstring>

namespace ape {

namespace {
plat::Mutex& JobQueueLock()
{
    static plat::Mutex s_lock;
    return s_lock;
}
}

// @func 0x0056DFD4 LockJobQueue
void LockJobQueue()
{
    JobQueueLock().lock();
}

// @func 0x0056E008 UnlockJobQueue
void UnlockJobQueue()
{
    JobQueueLock().unlock();
}

// NOTE: the console builds a DMA job (SPU program at 0x00D67A80) for copies
// over 64 bytes and, with kJobMemcpy_Wait, waits on an event flag for it.
// Here every copy completes before returning, which satisfies both modes.
// @func 0x0056E2EC JobMemcpy
void JobMemcpy(const void* src, void* dst, size_t size, uint32_t flags)
{
    (void)flags;
    std::memcpy(dst, src, size);
}

} // namespace ape
