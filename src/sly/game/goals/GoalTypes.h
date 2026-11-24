// openTIT - Goal classes (one per <Goal...> element of the job XML).
//
// The element names are recovered from their hashes (the XML is compiled);
// the classes are named after them. Console vtables are given per class.
#pragma once

#include "sly/game/goals/Goal.h"

#include "ape/math/ApeMath.h"

namespace sly {
namespace goals {

// "GoalWait" (vtable 0x00B1F1B0): completes after "Time" seconds.
class GoalWait : public Goal {
public:
    using Goal::Goal;
    ~GoalWait() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    float remaining = 0.0f;   // +0xA0C
    float duration = 0.0f;    // +0xA10 "Time"
};

// "GoalTimer" (vtable 0x00B1F0C0): a countdown shown on the HUD; completes
// when it runs out.
class GoalTimer : public Goal {
public:
    using Goal::Goal;
    ~GoalTimer() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    float remaining = 0.0f;     // +0xA0C
    float duration = 0.0f;      // +0xA10 "limit"
    bool startPaused = false;   // +0xA14 (XML 0x418A9FF0)
    bool paused = false;        // +0xA15
    bool showHud = true;        // +0xA16 (XML 0xDAC2D715)
    float warnTime = 10.0f;     // +0xA18 (XML 0xD4A5405E) the HUD flashes every second below it
};

// "GoalInput" (vtable 0x00B1F138): completes when a player presses a button
// or pushes a stick in a direction.
class GoalInput : public Goal {
public:
    using Goal::Goal;
    ~GoalInput() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void SetStatus(uint8_t newStatus, bool force) override;
    // 0x00340EA0: the stick of `pad` is pushed past the threshold.
    bool StickPushed(const void* pad) const;

    int32_t player = 0;        // +0xA0C (XML 0x368328E5, 0-8)
    uint32_t button = 0;       // +0xA10 "Button" action name
    int32_t direction = -1;    // +0xA14 index in the direction table, -1 for none
    float threshold = 1.0f;    // +0xA18 (XML 0x0B3E876F, clamped to 0..1)
};

// "GoalCostume" (vtable 0x00B1E7B0): the player wears a costume.
class GoalCostume : public Goal {
public:
    using Goal::Goal;
    ~GoalCostume() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    int32_t ability = 0;       // +0xA0C ability index of the costume, 0 = any
};

// "GoalBinocucomPower" (vtable 0x00B1E648): the binocucom power level is
// inside [Min, max] when event 6 arrives.
class GoalBinocucomPower : public Goal {
public:
    using Goal::Goal;
    ~GoalBinocucomPower() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;

    float minLevel = 0.0f;     // +0xA0C "Min"
    float maxLevel = 0.0f;     // +0xA10 "max"
    bool failInRange = false;  // +0xA14 (XML 0x6FE56119) fail instead of completing
};

// One entity the binocucom must keep in view (0x18 bytes on the console).
struct BinocucomRecordTarget {
    uint32_t name = 0;          // +0x00
    void* entity = nullptr;     // +0x04 resolved
    float field_08 = 0.0f;      // +0x08 (XML 0x03B880B3)
    float field_0C = 0.0f;      // +0x0C (XML 0x8E727ACC)
    float field_10 = 0.0f;      // +0x10 (XML 0x8E7279DE)
    bool visible = false;       // +0x14 NOTE: set by the binocucom when the target is in view
};

// "GoalBinocucomRecord" (vtable 0x00B1E6C0): record with the binocucom for
// a while; fails when the recording quality (event 8) drops too low.
class GoalBinocucomRecord : public Goal {
public:
    using Goal::Goal;
    ~GoalBinocucomRecord() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void ResetHints(bool stopHintScript) override;
    void ReapplyHints(Goal* owner) override;
    void Draw(void* context) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    float failLevel = 0.0f;     // +0xA0C (XML 0x56EF5AEB)
    float remaining = 0.0f;     // +0xA10
    float duration = 0.0f;      // +0xA14 (XML 0x19E32604)
    int32_t player = 0;         // +0xA18 player of the last update
    ape::ApeTArray<BinocucomRecordTarget> targets;   // +0xA1C
};

// One attachment that must exist (0x20 bytes on the console).
struct EntityChildLink {
    ape::Vec3 offset;            // +0x00 (XML 0x4A4D4D4D)
    uint32_t parentName = 0;     // +0x10 "Name"
    void* parent = nullptr;      // +0x14 resolved
    uint32_t childName = 0;      // +0x18 (XML 0x373DE2C2)
    // NOTE: option bits from XML 0x5D28E3FC, 0x9446A097, 0xD9159FE7,
    // 0x09DA53D5, 0x04656B91, 0x310F51B2 (bits 30..25); unused by the goal.
    uint32_t options = 0;        // +0x1C
};

// "GoalEntityChild" (vtable 0x00B1E828): every listed entity has the given
// child attached.
class GoalEntityChild : public Goal {
public:
    using Goal::Goal;
    ~GoalEntityChild() override;
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    uint8_t Update(int player, const GameEvent& e) override;
    void ResetHints(bool stopHintScript) override;
    void ReapplyHints(Goal* owner) override;
    void Draw(void* context) override;
    void SetStatus(uint8_t newStatus, bool force) override;

    ape::ApeTArray<EntityChildLink> links;   // +0xA0C
};

} // namespace goals
} // namespace sly
