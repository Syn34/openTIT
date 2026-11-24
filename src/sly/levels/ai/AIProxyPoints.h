// openTIT - AI proxy points: level point instances the AI uses as stand-ins
// for scripted positions (cover, look-at and approach points).
//
// Level data names point instances ("SAIProxyPt_..."); each named point is
// registered in a fixed-size table and tagged so the AI code can find its
// record from the world object.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>
#include <map>
#include <vector>

namespace sly {

class WorldObject;

namespace levels {

// User-data tag the proxy point records are attached under (world object
// +0x114). NOTE: hash name unknown.
constexpr uint32_t kUserTag_AIProxyPoint = 0x7B672814u;

// One registered point (0x14 bytes on the console).
struct AIProxyPoint {
    uint32_t userData = 0;          // +0x00 caller supplied
    WorldObject* object = nullptr;  // +0x04 the point instance
    // +0x08..+0x0F: the record's own index, stored four times (list links
    // initialised to "self").
    uint16_t links[4] = {};
};

class AIProxyPointTable {
public:
    explicit AIProxyPointTable(uint16_t capacity) : m_capacity(capacity) { m_points.reserve(capacity); }

    // Registers `object` (once; a second call returns the existing record).
    // Returns null when the table is full.
    AIProxyPoint* Register(WorldObject* object, uint32_t userData);
    // Registers the point instance named `name` and tags it with its record.
    void RegisterByName(uint32_t userData, const char* name);

    uint16_t Count() const { return static_cast<uint16_t>(m_points.size()); }
    AIProxyPoint* At(uint16_t index) { return index < m_points.size() ? &m_points[index] : nullptr; }

private:
    uint16_t m_capacity;
    std::vector<AIProxyPoint> m_points;      // +0x0C pool
    std::map<uint32_t, uint16_t> m_byKey;    // +0x10 object key -> record index
};

// The level's proxy point table (console pointer 0x00F557F4).
AIProxyPointTable* GetAIProxyPointTable();
void SetAIProxyPointTable(AIProxyPointTable* table);

// Cubic Bezier: start, first control, end, second control (argument order of
// the original).
void EvalBezier(float t, ape::Vec3& out, const ape::Vec3& start, const ape::Vec3& control1,
                const ape::Vec3& end, const ape::Vec3& control2);

} // namespace levels
} // namespace sly
