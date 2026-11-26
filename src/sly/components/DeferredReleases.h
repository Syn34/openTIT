// openTIT - Entities whose release waits a frame (table 0x010573A8, 32
// records of 12 bytes, count 0x01057528), kept by the targeting module: a
// record's entity is released (slot 10) on the second update after it was
// queued, so that targeting queries of the current frame can still see it.
// NOTE: the code that queues entities is not ported yet.
#pragma once

#include <cstdint>

namespace sly { namespace entity { class Entity; } }

namespace sly {
namespace components {

struct DeferredRelease {
    entity::Entity* entity = nullptr;   // +0x00
    uint32_t field_04 = 0;              // +0x04 NOTE: meaning unknown
    bool due = false;                   // +0x08 one update has passed
};

constexpr int kMaxDeferredReleases = 32;

// The table itself (for the queueing code).
DeferredRelease* DeferredReleases();
uint8_t& DeferredReleaseCount();

// Empties the table (entity module level start).
void ResetDeferredReleases();
// After the entity post-update pass: releases the due records and marks the
// others due.
void FlushDeferredReleases();

} // namespace components
} // namespace sly
