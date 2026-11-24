// openTIT - AI proxy points.
#include "sly/levels/ai/AIProxyPoints.h"

#include "ape/core/ApeString.h"
#include "sly/game/WorldInterface.h"
#include "sly/levels/GameServices.h"
#include "sly/levels/ai/AIHooks.h"

namespace sly {
namespace levels {

namespace {
AIProxyPointTable* g_proxyPoints = nullptr;
} // namespace

AIProxyPointTable* GetAIProxyPointTable()
{
    return g_proxyPoints;
}

void SetAIProxyPointTable(AIProxyPointTable* table)
{
    g_proxyPoints = table;
}

// @func 0x00064D48 AIProxyPointTable::Register
AIProxyPoint* AIProxyPointTable::Register(WorldObject* object, const void* properties)
{
    if (!object)
        return nullptr;
    const uint32_t key = WorldObjectKey(object);
    auto found = m_byKey.find(key);
    if (found != m_byKey.end())
        return &m_points[found->second];

    if (m_points.size() >= m_capacity) {
        ape::TempFormat("Run out of proxy point slot, proxy pt (%s) and any point after will not be "
                        "added to the system",
                        WorldObjectName(object));
        return nullptr;
    }
    const uint16_t index = static_cast<uint16_t>(m_points.size());
    AIProxyPoint point;
    point.properties = properties;
    point.object = object;
    point.self = point.prev = point.next = point.head = static_cast<int16_t>(index);
    m_points.push_back(point);
    m_byKey[key] = index;
    return &m_points.back();
}

// @func 0x00064F48 AIProxyPointTable::RegisterByName
void AIProxyPointTable::RegisterByName(const void* properties, const char* name)
{
    if (!name)
        return;
    WorldObject* point = FindPointInstance(ape::HashNameNoCase(name));
    if (!point) {
        ape::TempFormat("Fail to find the associate object (%s) from the scene, this AIProxy point will "
                        "be ignored!",
                        name);
        return;
    }
    AIProxyPoint* record = Register(point, properties);
    WorldObjectSetUserData(point, kUserTag_AIProxyPoint, record);
}

// @func 0x000190B4 AIProxyPointTable::BuildChains
void AIProxyPointTable::BuildChains()
{
    for (AIProxyPoint& head : m_points) {
        AIProxyPoint* last = nullptr;
        AIProxyPoint* previous = &head;
        for (WorldObject* child = ai::WorldObjectFirstChild(head.object); child;
             child = ai::WorldObjectNextSibling(child)) {
            AIProxyPoint* point = ai::ProxyPointOf(child);
            if (!point)
                continue;
            point->head = head.self;
            ++head.childCount;
            previous->next = point->self;
            point->prev = previous->self;
            previous = point;
            last = point;
        }
        if (last) {
            last->next = head.self;
            head.prev = last->self;
        }
    }
}

// @func 0x00064C88 EvalBezier
void EvalBezier(float t, ape::Vec3& out, const ape::Vec3& start, const ape::Vec3& control1,
                const ape::Vec3& end, const ape::Vec3& control2)
{
    const float u = 1.0f - t;
    const float wEnd = t * t * t;
    const float wStart = u * u * u;
    const float wControl1 = t * 3.0f * u * u;
    const float wControl2 = t * 3.0f * t * u;
    out.x = wEnd * end.x + wControl2 * control2.x + wStart * start.x + wControl1 * control1.x;
    out.y = wEnd * end.y + wControl2 * control2.y + wStart * start.y + wControl1 * control1.y;
    out.z = wEnd * end.z + wControl2 * control2.z + wStart * start.z + wControl1 * control1.z;
}

} // namespace levels
} // namespace sly
