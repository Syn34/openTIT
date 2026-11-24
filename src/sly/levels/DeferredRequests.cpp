// openTIT - Deferred entity requests queued by script commands.
#include "sly/levels/DeferredRequests.h"

namespace sly {
namespace levels {

std::vector<DeferredRequest>& DeferredRequests()
{
    static std::vector<DeferredRequest> s_requests;
    return s_requests;
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
