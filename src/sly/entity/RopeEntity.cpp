// openTIT - The rope entity (see RopeEntity.h).
#include "sly/entity/RopeEntity.h"

#include "ape/memory/ApeMem.h"
#include "ape/world/ApeObj.h"
#include "sly/game/SlyGame.h"
#include "sly/objects/ChainTuning.h"

#include <algorithm>

namespace sly {
namespace entity {

namespace {
constexpr int kCallbackList_Transform = 0;   // game world callback list of the queued transforms
}

// @func 0x001BB9B0 RopeEntity::~RopeEntity
// @func 0x001BBBC8 RopeEntity::~RopeEntity (deleting)
RopeEntity::~RopeEntity()
{
    if (solverObject) {
        ropeObject->Release();
        ropeObject = nullptr;
        solverObject->Release();
        solverObject = nullptr;
        for (uint16_t i = 0; i < segmentCount; ++i) {
            segments[i].object->Release();
            segments[i].object = nullptr;
        }
        ape::MemFree(segments, 0);
        if (renderObject) {
            if (!(ropeFlags & kRope_SharedObject)) {
                if (ape::world::ApeObj* parent = renderObject->GetParent())
                    parent->DetachChild(renderObject);
                // Forget the rope as the object's owner.
                const bool tagged = reinterpret_cast<uintptr_t>(renderObject->field_114) == kObjectTag;
                if (tagged && renderObject->field_118 == this) {
                    renderObject->field_118 = nullptr;
                    renderObject->field_114 = reinterpret_cast<void*>(uintptr_t(kObjectTag));
                }
            }
            renderObject->Release();
        }
    }
    if (g_game)
        g_game->RemoveCallback(kCallbackList_Transform, this, this);
}

// @func 0x001BD10C RopeEntity::PostUpdate
bool RopeEntity::PostUpdate(float dt)
{
    (void)dt;
    if (objects::g_ropeUpdateMode.Get() & 3)
        Simulate();
    ropeFlags &= ~kRope_Pending;
    return true;
}

// @func 0x001BBC10 RopeEntity::Release
bool RopeEntity::Release(bool kill)
{
    return Entity::Release(kill);
}

// @func 0x001BD168 RopeEntity::UpdateTransform
// NOTE: the console first waits for the solver job (busy word 0x01042500);
// the port's solver runs synchronously.
void RopeEntity::UpdateTransform(float dt)
{
    (void)dt;
    if (objects::g_ropeUpdateMode.Get() == 3)
        ApplySolver();
}

// @func 0x001BD1E0 RopeEntity::SetVisible
void RopeEntity::SetVisible(bool visible, bool immediate, bool children, bool components)
{
    Entity::SetVisible(visible, immediate, children, components);
    for (uint16_t i = 0; i < segmentCount; ++i)
        segments[i].object->SetVisible(visible, immediate);
}

// @func 0x001BD2B0 RopeEntity::SetSegmentsSlot13
void RopeEntity::SetSegmentsSlot13(uint8_t value, bool recurse)
{
    for (uint16_t i = 0; i < segmentCount; ++i)
        segments[i].object->VirtualSlot13(value, recurse);
}

// @func 0x001BD370 RopeEntity::PushSegment
void RopeEntity::PushSegment(const ape::Vec3& impulse, float along)
{
    const int index = std::min(int(along * float(segmentCount) + 0.5f), int(segmentCount) - 1);
    Segment& segment = segments[index];
    const ape::Vec3& axis = segment.axis;
    const float alongAxis = axis.x * impulse.x + axis.y * impulse.y + axis.z * impulse.z;
    segment.push.x += impulse.x - axis.x * alongAxis;
    segment.push.y += impulse.y - axis.y * alongAxis;
    segment.push.z += impulse.z - axis.z * alongAxis;
}

// @func 0x001BD450 RopeEntity::SetRopeObjectVisible
void RopeEntity::SetRopeObjectVisible(bool visible)
{
    if (ropeObject)
        ropeObject->SetVisible(visible, false);
}

// @func 0x001BD4A0 RopeEntity::IsRopeObjectVisible
bool RopeEntity::IsRopeObjectVisible() const
{
    return ropeObject && ropeObject->IsVisible();
}

// @func 0x001BD4CC RopeEntity::SetRenderObjectVisible
void RopeEntity::SetRenderObjectVisible(bool visible)
{
    if (renderObject)
        renderObject->SetVisible(visible, true);
}

// @func 0x001BD51C RopeEntity::SegmentAt
RopeEntity::Segment& RopeEntity::SegmentAt(int index)
{
    return segments[index];
}

// TODO(0x001BC5C8 RopeEntity::Simulate)
void RopeEntity::Simulate() {}
// TODO(0x001BBC40 RopeEntity::ApplySolver)
void RopeEntity::ApplySolver() {}

// @func 0x001BD614 RopeQueuedUpdateTransform
void RopeQueuedUpdateTransform(void* owner, Entity* rope)
{
    (void)owner;
    rope->UpdateTransform();
}

} // namespace entity
} // namespace sly
