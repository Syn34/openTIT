// openTIT - Built-in anim script event types.
//
//   TimeEvent     fires once when the state clock crosses "Time" (or a random
//                 time between "TimeMin" and "TimeMax", re-rolled per clone)
//   AlwaysEvent   runs its commands on every update
//   event         runs when the instance receives the event "name"
//   RefcountEvent runs while nothing but the script holds the object
//                 referenced by variable "object"
#pragma once

#include "ape/anim/ApeAnimScript.h"

namespace ape {

class ApeAnimScriptTimeEvent : public ApeAnimScriptEvent {
public:
    ApeAnimScriptTimeEvent() = default;
    explicit ApeAnimScriptTimeEvent(xml::XmlElement* element);
    ~ApeAnimScriptTimeEvent() override;

    float GetTime() override;
    // @func 0x007941D8 ApeAnimScriptTimeEvent::IsTimed
    bool IsTimed() const override { return true; }
    // @func 0x007941E0 ApeAnimScriptTimeEvent::Check
    bool Check(ApeAnimScriptInstance*, ApeAnimScriptState*) override { return false; }
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptEvent* Create(xml::XmlElement* element) const override;

private:
    float m_minTime = 0.0f;
    float m_maxTime = 0.0f;
};

class ApeAnimScriptAlwaysEvent : public ApeAnimScriptEvent {
public:
    ApeAnimScriptAlwaysEvent() : ApeAnimScriptEvent(AnimHash("always")) {}
    ~ApeAnimScriptAlwaysEvent() override;

    // @func 0x007941E8 ApeAnimScriptAlwaysEvent::IsTimed
    bool IsTimed() const override { return false; }
    // @func 0x007941F0 ApeAnimScriptAlwaysEvent::Check
    bool Check(ApeAnimScriptInstance*, ApeAnimScriptState*) override { return true; }
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptEvent* Create(xml::XmlElement* element) const override;
};

class ApeAnimScriptGenericEvent : public ApeAnimScriptEvent {
public:
    ApeAnimScriptGenericEvent() : ApeAnimScriptEvent(AnimHash("event")) {}
    ~ApeAnimScriptGenericEvent() override;

    // @func 0x007941F8 ApeAnimScriptGenericEvent::IsTimed
    bool IsTimed() const override { return false; }
    // @func 0x00794200 ApeAnimScriptGenericEvent::Check
    bool Check(ApeAnimScriptInstance*, ApeAnimScriptState*) override { return false; }

    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptEvent* Create(xml::XmlElement* element) const override;
};

class ApeAnimScriptRefcountEvent : public ApeAnimScriptEvent {
public:
    ApeAnimScriptRefcountEvent() : ApeAnimScriptEvent(AnimHash("Refcount")) {}
    ~ApeAnimScriptRefcountEvent() override;

    // @func 0x00794208 ApeAnimScriptRefcountEvent::IsTimed
    bool IsTimed() const override { return false; }
    bool Check(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    xml::XmlElement* CreateDebugElement() const override;
    ApeAnimScriptEvent* Create(xml::XmlElement* element) const override;

private:
    uint32_t m_objectVar = 0;
};

} // namespace ape
