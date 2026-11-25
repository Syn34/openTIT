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
//
// Points placed as children of another point form a patrol chain: the parent
// ("head") and its children are linked in a ring through prev/next, every
// child knows its head, and the head counts its children. A lone point links
// to itself.
struct AIProxyPoint {
    const void* properties = nullptr;  // +0x00 property block of the point (caller supplied)
    WorldObject* object = nullptr;     // +0x04 the point instance
    int16_t self = 0;                  // +0x08 own index
    int16_t prev = 0;                  // +0x0A
    int16_t next = 0;                  // +0x0C
    int16_t head = 0;                  // +0x0E
    int32_t childCount = 0;            // +0x10 (head only)
};

class AIProxyPointTable {
public:
    explicit AIProxyPointTable(uint16_t capacity) : m_capacity(capacity) { m_points.reserve(capacity); }

    // Registers `object` (once; a second call returns the existing record).
    // Returns null when the table is full.
    AIProxyPoint* Register(WorldObject* object, const void* properties);
    // Registers the point instance named `name` and tags it with its record.
    void RegisterByName(const void* properties, const char* name);

    uint16_t Count() const { return static_cast<uint16_t>(m_points.size()); }
    AIProxyPoint* At(uint16_t index) { return index < m_points.size() ? &m_points[index] : nullptr; }
    // Index form used by the chain links (negative or out of range: null).
    AIProxyPoint* AtIndex(int index) { return index >= 0 && index < (int)m_points.size() ? &m_points[index] : nullptr; }
    // Record registered for an object key (instance name hash), or null.
    const AIProxyPoint* Find(uint32_t key) const
    {
        auto found = m_byKey.find(key);
        return found != m_byKey.end() ? &m_points[found->second] : nullptr;
    }
    // Links every point's child points into its patrol chain (0x000190B4).
    void BuildChains();
    // Forgets every point (level shutdown).
    void Clear() { m_points.clear(); m_byKey.clear(); }

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
