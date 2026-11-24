// openTIT - Deferred entity requests queued by script commands.
#include "sly/levels/DeferredRequests.h"

namespace sly {
namespace levels {

namespace {
// @func 0x000ABB74 StaticInit_DeferredRequests
// @func 0x000ABB30 StaticDestroy_DeferredRequests
// Compiler-generated initialiser/finaliser of this file's globals: the request
// queue (0x00F9CDA8, an array growing in steps of 16). The initialiser also
// clears a file-local zero vector from a shared header.
std::vector<DeferredRequest> g_requests;
} // namespace

std::vector<DeferredRequest>& DeferredRequests()
{
    return g_requests;
}

// @func 0x000AB954 QueueEntityStateRequest
void QueueEntityStateRequest(uint32_t entity, uint32_t state, bool flag)
{
    DeferredRequest request;
    request.target = entity;
    request.value = state;
    request.kind = DeferredRequestKind::EntityState;
    request.flag = flag ? 1u : 0u;
    DeferredRequests().push_back(request);
}

// @func 0x000ABA14 QueueObjectVisibleRequest
void QueueObjectVisibleRequest(uint32_t object, bool visible, bool flag)
{
    DeferredRequest request;
    request.target = object;
    request.value = visible ? 1u : 0u;
    request.kind = DeferredRequestKind::ObjectVisible;
    request.flag = flag ? 1u : 0u;
    DeferredRequests().push_back(request);
}

} // namespace levels
} // namespace sly
