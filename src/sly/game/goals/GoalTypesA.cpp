// openTIT - Simple goal classes: wait, timer, input, costume, binocucom
// power/record, entity child.
#include "sly/game/goals/GoalTypes.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"
#include "sly/game/goals/GoalHooks.h"
#include "sly/tuning/BinocucomGoalTuning.h"

#include <cmath>
#include <cstdio>

namespace sly {
namespace goals {

namespace {

constexpr uint32_t kAttr_Name    = 0x0024728B;   // "Name"
constexpr uint32_t kAttr_Target  = 0x7A30D7E3;   // child element of the entity goals
constexpr uint32_t kAttr_Time    = 0x00274ACD;   // "Time"

// Every goal factory hands the description to the base constructor; the
// console's parse description also sizes the object.
template <class T>
T* NewGoal(const GoalDesc* desc)
{
    return new T(desc);
}

} // namespace

// ===========================================================================
// GoalWait

// @func 0x00341208 GoalWait::~GoalWait
// @func 0x003414C4 GoalWait::~GoalWait (deleting)
GoalWait::~GoalWait()
{
    Goal::SetStatus(kGoal_None, true);
}

// @func 0x00341510 GoalWait::Create
Goal* GoalWait::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    float time = 0.0f;
    xml->GetFloat(kAttr_Time, &time, 0.0f);
    GoalWait* goal = NewGoal<GoalWait>(desc);
    goal->remaining = 0.0f;
    goal->duration = time;
    return goal;
}

// @func 0x003415C8 GoalWait::Update
uint8_t GoalWait::Update(int player, const GameEvent& e)
{
    if (status == kGoal_Active && e.type == kEvent_06) {
        remaining -= e.arg0Float;
        if (remaining <= 0.0f) {
            remaining = 0.0f;
            SetStatus(kGoal_Complete, false);
        }
    }
    return Goal::Update(player, e);
}

// @func 0x003411A0 GoalWait::SetStatus
void GoalWait::SetStatus(uint8_t newStatus, bool force)
{
    if ((force || status != newStatus) && newStatus == kGoal_Active)
        remaining = duration;
    Goal::SetStatus(newStatus, force);
}

// ===========================================================================
// GoalTimer

// @func 0x00340134 GoalTimer::~GoalTimer
// @func 0x003403F0 GoalTimer::~GoalTimer (deleting)
GoalTimer::~GoalTimer()
{
    Goal::SetStatus(kGoal_None, true);
}

// @func 0x00340440 GoalTimer::Create
Goal* GoalTimer::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    float limit = 0.0f;
    bool startPaused = false;
    bool showHud = true;
    float warn = 10.0f;
    xml->GetFloat(0x0451539Bu, &limit, 0.0f);        // "limit"
    xml->GetBool(0x418A9FF0u, &startPaused, false);
    xml->GetBool(0xDAC2D715u, &showHud, true);
    xml->GetFloat(0xD4A5405Eu, &warn, 10.0f);

    GoalTimer* goal = NewGoal<GoalTimer>(desc);
    goal->remaining = 0.0f;
    goal->duration = limit;
    goal->startPaused = startPaused;
    goal->paused = startPaused;
    goal->showHud = showHud;
    goal->warnTime = warn;
    return goal;
}

// @func 0x00340038 GoalTimer::SetStatus
void GoalTimer::SetStatus(uint8_t newStatus, bool force)
{
    if (force || status != newStatus) {
        if (newStatus == kGoal_Complete || newStatus == kGoal_Failed) {
            if (showHud) {
                void* hud = HudFind(HudCountdownName());
                if (hud && HudIsShown(hud))
                    HudHide(hud, true, true, false);
            }
            JobManagerSetTimerGoal(nullptr);
        } else if (newStatus == kGoal_Active) {
            JobManagerSetTimerGoal(this);
            remaining = duration;
            paused = startPaused;
        }
    }
    Goal::SetStatus(newStatus, force);
}

// @func 0x003405A8 GoalTimer::Update
uint8_t GoalTimer::Update(int player, const GameEvent& e)
{
    if (status == kGoal_Active && e.type == kEvent_06) {
        const float secondsBefore = std::floor(remaining);
        if (!paused && !HudCountdownPaused())
            remaining -= e.arg0Float;
        if (remaining <= 0.0f) {
            remaining = 0.0f;
            SetStatus(kGoal_Complete, false);
        }

        void* hud = showHud ? HudFind(HudCountdownName()) : nullptr;
        if (hud) {
            if (!paused && !HudCountdownPaused()) {
                const unsigned minutes = unsigned(remaining / 60.0f);
                const int seconds = int(remaining - float(minutes) * 60.0f);
                char text[32];
                std::snprintf(text, sizeof(text), "%d:%02d", minutes, seconds);
                if (!HudIsShown(hud)) {
                    // NOTE: the console also passes a completion callback (0x00340580).
                    if (!HudTimerSlotBusy())
                        HudShowText(hud, text, true);
                } else if (secondsBefore <= warnTime &&
                           std::fabs(secondsBefore - std::floor(remaining)) >= 1e-5f) {
                    // Below the warning time every new second replays the intro.
                    HudShowText(hud, text, true);
                } else {
                    HudSetText(hud, text, false);
                }
            } else {
                HudHide(hud, true, true, false);
            }
        }
    }
    return Goal::Update(player, e);
}

// ===========================================================================
// GoalInput

namespace {
// Stick directions (table 0x00ADB81C); the index picks the axis (3 - i/2)
// and the sign (odd = positive).
constexpr uint32_t kStickDirections[8] = {
    0x0040F679, 0xF3D58900, 0xF3D90465, 0x879DE95E,
    0x90F9E93F, 0x3A1CE246, 0x3A205DAB, 0x0A41B8D8,
};
} // namespace

// @func 0x003409B8 GoalInput::~GoalInput
// @func 0x00340C74 GoalInput::~GoalInput (deleting)
GoalInput::~GoalInput()
{
    Goal::SetStatus(kGoal_None, true);
}

// @func 0x00340CD8 GoalInput::Create
Goal* GoalInput::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    int32_t player = 0;
    xml->GetInt(0x368328E5u, &player, 0);
    if (player < 0 || player >= 9)
        player = 0;
    uint32_t button = 0;
    xml->GetHash(0x75751B32u, &button, 0);          // "Button"
    uint32_t directionName = 0;
    xml->GetHash(0x2A7C3553u, &directionName, 0);
    int32_t direction = -1;
    for (int i = 0; i < 8; ++i) {
        if (kStickDirections[i] == directionName) {
            direction = i;
            break;
        }
    }
    float threshold = 1.0f;
    xml->GetFloat(0x0B3E876Fu, &threshold, 1.0f);
    if (threshold > 1.0f)
        threshold = 1.0f;
    if (!(threshold >= 0.0f))
        threshold = 0.0f;

    GoalInput* goal = NewGoal<GoalInput>(desc);
    goal->player = player;
    goal->button = button;
    goal->direction = direction;
    goal->threshold = threshold;
    return goal;
}

// @func 0x00340990 GoalInput::SetStatus
void GoalInput::SetStatus(uint8_t newStatus, bool force)
{
    Goal::SetStatus(newStatus, force);
}

// @func 0x00340EA0 GoalInput::StickPushed
bool GoalInput::StickPushed(const void* pad) const
{
    if (direction < 0 || direction > 7)
        return false;
    const bool positive = (direction & 1) != 0;
    const float value = PadStickAxis(pad, 3 - direction / 2);
    return positive ? value > threshold : value < -threshold;
}

// @func 0x00340FC8 GoalInput::Update
uint8_t GoalInput::Update(int eventPlayer, const GameEvent& e)
{
    if (status == kGoal_Active && e.type == kEvent_06) {
        void* character = PlayerCharacter(player);
        if (character && EntityIsCharacter(character)) {
            const void* pad = PlayerPad(player);
            bool done = false;
            if (button != 0)
                done = (CharacterActionButtons(character, button) & PadButtonsDown(pad)) != 0;
            else if (direction != -1)
                done = StickPushed(pad);
            if (done)
                SetStatus(kGoal_Complete, false);
        }
    }
    return Goal::Update(eventPlayer, e);
}

// ===========================================================================
// GoalCostume

namespace {
constexpr uint32_t kCostume_Default = 0xFEE1E46A;   // the one costume the XML names
constexpr int kAbility_DefaultCostume = 0x3A;
} // namespace

// @func 0x0032EA68 GoalCostume::~GoalCostume
// @func 0x0032ED24 GoalCostume::~GoalCostume (deleting)
GoalCostume::~GoalCostume()
{
    Goal::SetStatus(kGoal_None, true);
}

// @func 0x0032ED6C GoalCostume::Create
Goal* GoalCostume::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    uint32_t costume = 0;
    xml->GetHash(0x63C971C0u, &costume, 0);   // "Costume"
    GoalCostume* goal = NewGoal<GoalCostume>(desc);
    goal->ability = costume == kCostume_Default ? kAbility_DefaultCostume : 0;
    return goal;
}

// @func 0x0032EA40 GoalCostume::SetStatus
void GoalCostume::SetStatus(uint8_t newStatus, bool force)
{
    Goal::SetStatus(newStatus, force);
}

// @func 0x0032EE20 GoalCostume::Update
uint8_t GoalCostume::Update(int player, const GameEvent& e)
{
    void* character = PlayerCharacter(player);
    if (character && EntityIsCharacter(character)) {
        // Worn: bit 7 set, bit 6 (locked) clear. No costume named = any.
        bool wearing = true;
        if (ability != 0) {
            const uint8_t bits = CharacterAbilityBits(character, ability);
            wearing = (bits & 0x80) && !(bits & 0x40);
        }
        if (status == kGoal_Active && wearing)
            SetStatus(kGoal_Complete, false);
        else if (status == kGoal_Complete && !wearing)
            SetStatus(kGoal_Active, false);
    }
    return Goal::Update(player, e);
}

// ===========================================================================
// GoalBinocucomPower

// @func 0x0032CA0C GoalBinocucomPower::~GoalBinocucomPower
// @func 0x0032CCC8 GoalBinocucomPower::~GoalBinocucomPower (deleting)
GoalBinocucomPower::~GoalBinocucomPower()
{
    Goal::SetStatus(kGoal_None, true);
}

// @func 0x0032CD18 GoalBinocucomPower::Create
Goal* GoalBinocucomPower::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    float minLevel = 0.0f;
    float maxLevel = 0.0f;
    bool fail = false;
    xml->GetFloat(0x00012A32u, &minLevel, 0.0f);   // "Min"
    xml->GetFloat(0x00012944u, &maxLevel, 0.0f);   // "max"
    xml->GetBool(0x6FE56119u, &fail, false);
    GoalBinocucomPower* goal = NewGoal<GoalBinocucomPower>(desc);
    goal->minLevel = minLevel;
    goal->maxLevel = maxLevel;
    goal->failInRange = fail;
    return goal;
}

// @func 0x0032CE1C GoalBinocucomPower::Update
uint8_t GoalBinocucomPower::Update(int player, const GameEvent& e)
{
    const float level = BinocucomPowerLevel();
    if (status == kGoal_Active && e.type == kEvent_06 && minLevel <= level && level <= maxLevel)
        SetStatus(failInRange ? kGoal_Failed : kGoal_Complete, false);
    return Goal::Update(player, e);
}

// ===========================================================================
// GoalBinocucomRecord

// @func 0x0032CF20 GoalBinocucomRecord::~GoalBinocucomRecord
// @func 0x0032D1EC GoalBinocucomRecord::~GoalBinocucomRecord (deleting)
GoalBinocucomRecord::~GoalBinocucomRecord() = default;

// @func 0x0032D240 GoalBinocucomRecord::Create
Goal* GoalBinocucomRecord::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    float failLevel = 0.0f;
    float duration = 0.0f;
    xml->GetFloat(0x56EF5AEBu, &failLevel, 0.0f);
    xml->GetFloat(0x19E32604u, &duration, 0.0f);
    GoalBinocucomRecord* goal = NewGoal<GoalBinocucomRecord>(desc);
    goal->failLevel = failLevel;
    goal->remaining = duration;
    goal->duration = duration;
    goal->player = 0;

    for (ape::xml::XmlElement* child = xml->FirstChildElement(kAttr_Target); child;
         child = child->NextSiblingElement(kAttr_Target)) {
        BinocucomRecordTarget target;
        if (child->GetHash(kAttr_Name, &target.name, 0) != 0)
            continue;   // the console warns about the missing name
        // NOTE: the defaults are globals next to the BinocucomRecord tuning
        // values; assumed to be those values.
        child->GetFloat(0x03B880B3u, &target.field_08, g_binocucomRecordDefaultAngle);
        child->GetFloat(0x8E727ACCu, &target.field_0C, g_binocucomRecordDefaultDistMin);
        child->GetFloat(0x8E7279DEu, &target.field_10, g_binocucomRecordDefaultDistMax);
        goal->targets.PushBack(target);
    }
    return goal;
}

// @func 0x0032D510 GoalBinocucomRecord::Update
uint8_t GoalBinocucomRecord::Update(int eventPlayer, const GameEvent& e)
{
    if (status == kGoal_Active) {
        if (e.type == kEvent_06) {
            remaining -= e.arg0Float;
            if (remaining <= 0.0f)
                SetStatus(kGoal_Complete, false);
        } else if (e.type == 0x08 && failLevel <= e.arg0Float) {
            SetStatus(kGoal_Failed, false);
        }
    }
    player = eventPlayer;
    return Goal::Update(eventPlayer, e);
}

// @func 0x0032D628 GoalBinocucomRecord::ResetHints
void GoalBinocucomRecord::ResetHints(bool stopHintScript)
{
    for (BinocucomRecordTarget& target : targets)
        target.entity = nullptr;
    Goal::ResetHints(stopHintScript);
}

// @func 0x0032D688 GoalBinocucomRecord::ReapplyHints
void GoalBinocucomRecord::ReapplyHints(Goal* owner)
{
    Goal::ReapplyHints(owner);
    if (status != kGoal_Active)
        return;
    ResetHints(false);
    for (BinocucomRecordTarget& target : targets)
        if (!target.entity && target.name != 0)
            target.entity = GoalFindEntity(target.name);
}

// @func 0x0032D758 GoalBinocucomRecord::CollectMarkers
void GoalBinocucomRecord::CollectMarkers(GoalMarkerList* list)
{
    if (status != kGoal_Active)
        return;
    for (BinocucomRecordTarget& target : targets)
        if (target.entity && target.visible)
            GoalAddEntityMarker(list, target.entity);
}

// @func 0x0032D8E4 GoalBinocucomRecord::SetStatus
void GoalBinocucomRecord::SetStatus(uint8_t newStatus, bool force)
{
    if (force || newStatus != status) {
        // The recording pose is animation layer 1, state 7.
        void* character = PlayerCharacter(player);
        if (newStatus == kGoal_Active) {
            if (character)
                CharacterSetState(character, 1, 7);
            remaining = duration;
        } else if (newStatus == kGoal_Complete) {
            if (character)
                CharacterSetState(character, 1, 0);
        }
    }
    Goal::SetStatus(newStatus, force);
}

// ===========================================================================
// GoalEntityChild

// @func 0x0032EF90 GoalEntityChild::~GoalEntityChild
// @func 0x0032F25C GoalEntityChild::~GoalEntityChild (deleting)
GoalEntityChild::~GoalEntityChild() = default;

// @func 0x0032F2B0 GoalEntityChild::Create
Goal* GoalEntityChild::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    GoalEntityChild* goal = NewGoal<GoalEntityChild>(desc);
    static const float kZero[3] = {0.0f, 0.0f, 0.0f};
    static const uint32_t kOptionAttrs[6] = {
        0x5D28E3FC, 0x9446A097, 0xD9159FE7, 0x09DA53D5, 0x04656B91, 0x310F51B2,
    };
    for (ape::xml::XmlElement* child = xml->FirstChildElement(kAttr_Target); child;
         child = child->NextSiblingElement(kAttr_Target)) {
        EntityChildLink link;
        if (child->GetHash(kAttr_Name, &link.parentName, 0) != 0)
            continue;   // the console warns about the missing name
        float offset[3];
        child->GetVec3(0x4A4D4D4Du, offset, kZero);
        link.offset.x = offset[0];
        link.offset.y = offset[1];
        link.offset.z = offset[2];
        child->GetHash(0x373DE2C2u, &link.childName, 0);
        bool unused = false;
        child->GetBool(0x3B66FD69u, &unused, false);   // NOTE: read but not kept by the console
        for (int i = 0; i < 6; ++i) {
            bool value = false;
            child->GetBool(kOptionAttrs[i], &value, false);
            if (value)
                link.options |= 1u << (30 - i);
        }
        goal->links.PushBack(link);
    }
    return goal;
}

// @func 0x0032F5FC GoalEntityChild::ResetHints
void GoalEntityChild::ResetHints(bool stopHintScript)
{
    for (EntityChildLink& link : links)
        link.parent = nullptr;
    Goal::ResetHints(stopHintScript);
}

// @func 0x0032F65C GoalEntityChild::ReapplyHints
void GoalEntityChild::ReapplyHints(Goal* owner)
{
    Goal::ReapplyHints(owner);
    if (status != kGoal_Active)
        return;
    ResetHints(false);
    for (EntityChildLink& link : links)
        link.parent = GoalFindEntity(link.parentName);
}

// @func 0x0032F714 GoalEntityChild::CollectMarkers
void GoalEntityChild::CollectMarkers(GoalMarkerList* list)
{
    if (status != kGoal_Active)
        return;
    for (EntityChildLink& link : links)
        if (link.parent)
            GoalAddEntityMarker(list, link.parent);
}

// @func 0x0032F8CC GoalEntityChild::Update
uint8_t GoalEntityChild::Update(int player, const GameEvent& e)
{
    if (status == kGoal_Active) {
        uint32_t attached = 0;
        for (EntityChildLink& link : links)
            if (link.parent && EntityFindDescendant(link.parent, link.childName))
                ++attached;
        if (attached != 0 && attached == links.Count())
            SetStatus(kGoal_Complete, false);
    }
    return Goal::Update(player, e);
}

// @func 0x0032F9D8 GoalEntityChild::SetStatus
void GoalEntityChild::SetStatus(uint8_t newStatus, bool force)
{
    if ((force || newStatus != status) && newStatus != kGoal_Active)
        ResetHints(false);
    Goal::SetStatus(newStatus, force);
}

} // namespace goals
} // namespace sly
