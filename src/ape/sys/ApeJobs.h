// openTIT - Engine job queue front end (SPU jobs on the console).
//
// On the PS3 the engine submits work to SPURS job chains through a shared
// dispatcher guarded by one lock. The PC build has no SPUs: jobs whose work
// is known run synchronously on the calling thread.
#pragma once

#include <cstddef>
#include <cstdint>

namespace ape {

// Serialises access to the job dispatcher (0x0116B580 on the console).
void LockJobQueue();
void UnlockJobQueue();

enum JobMemcpyFlags : uint32_t {
    kJobMemcpy_Wait = 0x1,   // block until the copy has completed
};

// Copies `size` bytes from `src` to `dst`. Copies of up to 64 bytes are
// always done in place; larger ones were handed to an SPU DMA job.
void JobMemcpy(const void* src, void* dst, size_t size, uint32_t flags);

} // namespace ape
