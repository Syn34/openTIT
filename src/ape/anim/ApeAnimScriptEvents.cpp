// openTIT - Built-in anim script event types.
#include "ape/anim/ApeAnimScriptEvents.h"

#include "ape/core/ApeString.h"
#include "ape/math/ApeMath.h"
#include "ape/xml/ApeXml.h"

namespace ape {

namespace {
constexpr uint32_t kAttrName = AnimHash("name");
constexpr uint32_t kAttrTime = AnimHash("Time");
constexpr uint32_t kAttrTimeMin = AnimHash("TimeMin");
constexpr uint32_t kAttrTimeMax = AnimHash("TimeMax");
constexpr uint32_t kAttrObjectHash = AnimHash("ObjectHash");

float RandomInRange(float minValue, float maxValue)
{
    return (maxValue - minValue) * RandomUnit(g_gameRandom) + minValue;
}
} // namespace

// ---------------------------------------------------------------------------
// TimeEvent

// @func 0x004C0E00 ApeAnimScriptTimeEvent::ApeAnimScriptTimeEvent
ApeAnimScriptTimeEvent::ApeAnimScriptTimeEvent(xml::XmlElement* element)
{
    if (element->GetFloat(kAttrTime, &m_time, 0.0f) != xml::kQuery_Ok &&
        element->GetFloat(kAttrTimeMin, &m_minTime, 0.0f) == xml::kQuery_Ok &&
        element->GetFloat(kAttrTimeMax, &m_maxTime, 0.0f) == xml::kQuery_Ok) {
        m_time = RandomInRange(m_minTime, m_maxTime);
    }
}

// @func 0x004C0FB4 ApeAnimScriptTimeEvent::~ApeAnimScriptTimeEvent
// @func 0x004C1094 ApeAnimScriptTimeEvent::~ApeAnimScriptTimeEvent (deleting)
ApeAnimScriptTimeEvent::~ApeAnimScriptTimeEvent() = default;

// @func 0x004C11A0 ApeAnimScriptTimeEvent::GetTime
float ApeAnimScriptTimeEvent::GetTime()
{
    // A random window is re-rolled every time the time is asked for, so
    // every state clone fires at its own moment.
    if (m_minTime < m_maxTime)
        m_time = RandomInRange(m_minTime, m_maxTime);
    return m_time;
}

// @func 0x004C1190 ApeAnimScriptTimeEvent::CreateDebugElement
xml::XmlElement* ApeAnimScriptTimeEvent::CreateDebugElement() const
{
    return nullptr;
}

// @func 0x004C1230 ApeAnimScriptTimeEvent::Create
ApeAnimScriptEvent* ApeAnimScriptTimeEvent::Create(xml::XmlElement* element) const
{
    return new ApeAnimScriptTimeEvent(element);
}

// ---------------------------------------------------------------------------
// AlwaysEvent

// @func 0x004C12A0 ApeAnimScriptAlwaysEvent::~ApeAnimScriptAlwaysEvent
// @func 0x004C1380 ApeAnimScriptAlwaysEvent::~ApeAnimScriptAlwaysEvent (deleting)
ApeAnimScriptAlwaysEvent::~ApeAnimScriptAlwaysEvent() = default;

// @func 0x004C147C ApeAnimScriptAlwaysEvent::CreateDebugElement
xml::XmlElement* ApeAnimScriptAlwaysEvent::CreateDebugElement() const
{
    // NOTE: the label "AlwaysEvent" is passed to the element constructor, which
    // ignores it in release builds.
    return new xml::XmlElement();
}

// @func 0x004C14E0 ApeAnimScriptAlwaysEvent::Create
ApeAnimScriptEvent* ApeAnimScriptAlwaysEvent::Create(xml::XmlElement* element) const
{
    return new ApeAnimScriptAlwaysEvent();
}

// ---------------------------------------------------------------------------
// GenericEvent

// @func 0x004C15A8 ApeAnimScriptGenericEvent::~ApeAnimScriptGenericEvent
// @func 0x004C1688 ApeAnimScriptGenericEvent::~ApeAnimScriptGenericEvent (deleting)
ApeAnimScriptGenericEvent::~ApeAnimScriptGenericEvent() = default;

// @func 0x004C1784 ApeAnimScriptGenericEvent::CreateDebugElement
xml::XmlElement* ApeAnimScriptGenericEvent::CreateDebugElement() const
{
    // NOTE: the label "GenericEvent" is passed to the element constructor, which
    // ignores it in release builds.
    return new xml::XmlElement();
}

// @func 0x004C17E8 ApeAnimScriptGenericEvent::Create
ApeAnimScriptEvent* ApeAnimScriptGenericEvent::Create(xml::XmlElement* element) const
{
    auto* event = new ApeAnimScriptGenericEvent();
    element->GetHash(kAttrName, &event->m_nameHash, event->m_nameHash);
    return event;
}

// ---------------------------------------------------------------------------
// RefcountEvent

// @func 0x004C18E0 ApeAnimScriptRefcountEvent::~ApeAnimScriptRefcountEvent
// @func 0x004C19C0 ApeAnimScriptRefcountEvent::~ApeAnimScriptRefcountEvent (deleting)
ApeAnimScriptRefcountEvent::~ApeAnimScriptRefcountEvent() = default;

// @func 0x004C1ABC ApeAnimScriptRefcountEvent::Check
bool ApeAnimScriptRefcountEvent::Check(ApeAnimScriptInstance* instance, ApeAnimScriptState* state)
{
    // True once the script's variable holds the last reference (or none).
    ApeRefCounted* object = instance->ResolveObject(m_objectVar, false);
    return !object || object->GetRefCount() < 2;
}

// @func 0x004C1B18 ApeAnimScriptRefcountEvent::CreateDebugElement
xml::XmlElement* ApeAnimScriptRefcountEvent::CreateDebugElement() const
{
    // NOTE: the label "Refcount" is passed to the element constructor, which
    // ignores it in release builds.
    return new xml::XmlElement();
}

// @func 0x004C1B7C ApeAnimScriptRefcountEvent::Create
ApeAnimScriptEvent* ApeAnimScriptRefcountEvent::Create(xml::XmlElement* element) const
{
    auto* event = new ApeAnimScriptRefcountEvent();
    if (const char* name = element->GetString("object")) {
        event->m_objectVar = HashNameNoCase(name);
    } else {
        int32_t value = 0;
        element->GetInt(kAttrObjectHash, &value, 0);
        event->m_objectVar = (uint32_t)value;
    }
    return event;
}

} // namespace ape
