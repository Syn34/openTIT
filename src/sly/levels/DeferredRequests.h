// openTIT - Deferred entity requests queued by script commands.
//
// Two script commands (0x83F48558, 0x2C1E0CC1) do not act at once: they
// append a 16-byte request to a global queue (console ApeArray at 0x00F9CDA8)
// that the level code works through later. Each record carries two operands,
// a request kind and a flag.
#pragma once

#include <cstdint>
#include <vector>

namespace sly {
namespace levels {

enum class DeferredRequestKind : uint32_t {
    EntityState = 1,    // entity name, state name, flag
    ObjectVisible = 2,  // object name, visible, flag
};

struct DeferredRequest {
    uint32_t target = 0;   // +0x00 entity / object name hash
    uint32_t value = 0;    // +0x04 state hash or visible flag
    DeferredRequestKind kind = DeferredRequestKind::EntityState;  // +0x08
    uint32_t flag = 0;     // +0x0C
};

// The global queue (consumer not ported).
std::vector<DeferredRequest>& DeferredRequests();

// Queues an entity state change (0x000AB954).
void QueueEntityStateRequest(uint32_t entity, uint32_t state, bool flag);
// Queues an object visibility change (0x000ABA14).
void QueueObjectVisibleRequest(uint32_t object, bool visible, bool flag);

} // namespace levels
} // namespace sly
