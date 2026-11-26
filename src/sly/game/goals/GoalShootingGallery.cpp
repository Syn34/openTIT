// openTIT - "ShootingGallery" goal (0x0033BEA4-0x0033E6EC).
#include "sly/game/goals/GoalShootingGallery.h"

#include "ape/math/ApeMath.h"
#include "platform/Platform.h"
#include "sly/game/GameEvent.h"
#include "sly/game/SpawnManager.h"
#include "sly/game/goals/GoalHooks.h"

#include <cstring>

namespace sly {
namespace goals {

namespace {
constexpr uint32_t kAttr_Name = 0x0024728Bu;            // "Name"
constexpr uint32_t kElem_Group = 0x040EFE5Fu;           // "Group"
constexpr uint32_t kElem_Commands = 0x0ABBFA88u;        // "Commands"
constexpr uint32_t kCmd_Pause = 0x04862DD6u;            // "Pause"
constexpr uint32_t kCmd_EnableGroup = 0x2B995EBCu;      // NOTE: hash matches "EnableGroup"
constexpr uint32_t kElem_ScoreSources = 0xBDF2F786u;    // NOTE: name unknown
constexpr uint32_t kElem_Type = 0x0027873Au;            // "Type"
// Listener group of the score reports and the message a target sends.
constexpr uint32_t kListenScores = 0xA8B960FFu;
constexpr uint32_t kMsg_Score = 0x36FB367Fu;

// The callbacks installed on the spawn groups (+0x38 and +0x30): a target
// went away. They pass different flags that the handler ignores.
// @func 0x0033CDC4 OnSpawnTargetGoneA
void OnSpawnTargetGoneA(Spawner* spawner, int32_t, void* data)
{
    static_cast<GoalShootingGallery::Group*>(data)->OnTargetGone(spawner, true);
}

// @func 0x0033CDF0 OnSpawnTargetGoneB
void OnSpawnTargetGoneB(Spawner* spawner, int32_t, void* data)
{
    static_cast<GoalShootingGallery::Group*>(data)->OnTargetGone(spawner, false);
}

void CopyName(char* dst, size_t size, const char* src)
{
    std::strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
}
} // namespace

// ===========================================================================
// Groups

GoalShootingGallery::Group::Group()
    : slots(16), freeSlots(16), busy(16), available(16)
{
}

GoalShootingGallery::Group::~Group()
{
    Clear();
}

// @func 0x0033C03C GoalShootingGallery::Group::Clear
void GoalShootingGallery::Group::Clear()
{
    if (spawnGroup) {
        spawnGroup->onSpawn = nullptr;
        spawnGroup->onSpawnData = nullptr;
        spawnGroup->onRelease = nullptr;
        spawnGroup->onReleaseData = nullptr;
    }
    spawned = 0;
    spawnGroup = nullptr;
    for (uint32_t i = 0; i < slots.Count(); ++i)
        delete slots[i];
    slots.Clear();
    for (uint32_t i = 0; i < freeSlots.Count(); ++i)
        delete freeSlots[i];
    freeSlots.Clear();
    available.Clear();
    busy.Clear();
}

int GoalShootingGallery::Group::ActiveCount() const
{
    int count = 0;
    for (uint32_t i = 0; i < slots.Count(); ++i)
        if (slots[i]->spawner)
            ++count;
    return count;
}

GoalShootingGallery::Slot* GoalShootingGallery::Group::FirstReadySlot() const
{
    for (uint32_t i = 0; i < slots.Count(); ++i)
        if (!slots[i]->spawner && !slots[i]->justFreed)
            return slots[i];
    return nullptr;
}

// @func 0x0033C130 GoalShootingGallery::Group::SpawnInto
void GoalShootingGallery::Group::SpawnInto(Slot* slot)
{
    if (!slot || slot->spawner)
        return;
    if (maxSpawn != 0 && spawned >= maxSpawn)
        return;
    // NOTE: like the console this assumes a spawner is available.
    const int pick = ape::RandomInt(ape::g_gameRandom, 0, int(available.Count()) - 1);
    Spawner* spawner = available[pick];
    available.RemoveAtSwap(pick);
    busy.PushBack(spawner);
    slot->spawner = spawner;
    spawner->Activate();
    slot->delay = 0.0f;
    ++spawned;
}

// @func 0x0033C2C0 GoalShootingGallery::Group::SetState
void GoalShootingGallery::Group::SetState(int newState)
{
    state = newState;
    stateTime = 0.0f;
    if (newState != kState_Running)
        return;

    spawned = 0;
    // Take free slots up to the active maximum.
    while (int(slots.Count()) < maxActive && freeSlots.Count() != 0) {
        Slot* slot = freeSlots[0];
        freeSlots.RemoveAtSwap(0);
        slots.PushBack(slot);
    }
    // Bring the group up to its active minimum at once. NOTE: unlike
    // SpawnInto this test does not treat a zero maxSpawn as unlimited.
    for (int active = ActiveCount(); active < minActive; ++active) {
        if (spawned < maxSpawn)
            SpawnInto(FirstReadySlot());
    }
    for (uint32_t i = 0; i < slots.Count(); ++i) {
        if (!slots[i]->spawner) {
            slots[i]->spawner = nullptr;
            slots[i]->delay = ape::RandomRange(delayBetweenMin, delayBetweenMax, ape::g_gameRandom);
        }
    }
}

// @func 0x0033C5B4 GoalShootingGallery::Group::Start
void GoalShootingGallery::Group::Start()
{
    Clear();
    spawnGroup = g_spawnManager ? g_spawnManager->FindGroup(nameHash) : nullptr;
    if (!spawnGroup)
        return;
    spawnGroup->Deactivate(false);
    spawnGroup->onSpawn = OnSpawnTargetGoneA;
    spawnGroup->onSpawnData = this;
    spawnGroup->onRelease = OnSpawnTargetGoneB;
    spawnGroup->onReleaseData = this;
    for (Spawner* spawner = spawnGroup->firstSpawner; spawner; spawner = spawner->nextInGroup)
        available.PushBack(spawner);
    // As many slots as the free list holds without growing.
    for (uint32_t i = 0; i < freeSlots.Capacity(); ++i)
        freeSlots.PushBack(new Slot());
    SetState(kState_Stopping);
}

// @func 0x0033C7A0 GoalShootingGallery::Group::Update
void GoalShootingGallery::Group::Update(float dt)
{
    stateTime += dt;
    if (state == kState_Running) {
        for (uint32_t i = 0; i < slots.Count(); ++i) {
            Slot* slot = slots[i];
            if (slot->spawner)
                continue;
            slot->delay -= dt;
            if (slot->delay < 0.0f)
                slot->delay = 0.0f;
            if (slot->delay == 0.0f && ActiveCount() < maxActive)
                SpawnInto(slot);
        }
        for (int active = ActiveCount(); active < minActive; ++active)
            SpawnInto(FirstReadySlot());
        for (uint32_t i = 0; i < slots.Count(); ++i)
            slots[i]->justFreed = false;
    } else if (state == kState_Stopping) {
        // Empty slots go back to the free list; occupied ones stay until
        // their target goes.
        for (uint32_t i = 0; i < slots.Count(); ++i) {
            Slot* slot = slots[i];
            if (slot->spawner)
                continue;
            slot->spawner = nullptr;
            slot->delay = ape::RandomRange(delayBetweenMin, delayBetweenMax, ape::g_gameRandom);
            freeSlots.PushBack(slot);
            slots.RemoveAtSwap(i);
            --i;
        }
    }
}

// @func 0x0033CB98 GoalShootingGallery::Group::OnTargetGone
void GoalShootingGallery::Group::OnTargetGone(Spawner* spawner, bool flag)
{
    (void)flag;   // NOTE: passed by the two callbacks, unused
    for (uint32_t i = 0; i < busy.Count(); ++i) {
        if (busy[i] == spawner) {
            busy.RemoveAtSwap(i);
            available.PushBack(spawner);
            break;
        }
    }
    for (uint32_t i = 0; i < slots.Count(); ++i) {
        Slot* slot = slots[i];
        if (slot->spawner != spawner)
            continue;
        const float delay = ape::RandomRange(delayBetweenMin, delayBetweenMax, ape::g_gameRandom);
        if (slot->spawner)
            slot->spawner->Deactivate(false);
        slot->spawner = nullptr;
        slot->delay = delay;
        slot->justFreed = true;
        return;
    }
}

// ===========================================================================
// Commands

// @func 0x0033BEA4 GoalShootingGallery::Command::ParseGroupParams
void GoalShootingGallery::Command::ParseGroupParams(ape::xml::XmlElement* xml)
{
    xml->GetHash(kAttr_Name, &group, group);
    xml->GetInt(0xC8B3AFB8u, &minActive, minActive);           // "minActive"
    xml->GetInt(0x9AA8994Au, &maxActive, maxActive);           // "maxActive"
    xml->GetInt(0x4810FC97u, &maxSpawn, maxSpawn);
    xml->GetFloat(0xC0C3EAEDu, &delayBetweenMin, delayBetweenMin);   // "delayBetweenMin"
    xml->GetFloat(0xC0C3E9FFu, &delayBetweenMax, delayBetweenMax);   // "delayBetweenMax"
    const char* name = xml->GetString(kAttr_Name);
    if (delayBetweenMax < delayBetweenMin)
        plat::Log(plat::LogLevel::Warning,
                  "delayBetweenMax must be greater than or equal to delayBetweenMin for shooting gallery spawner group '%s'\n",
                  name ? name : "");
    if (maxActive < minActive)
        plat::Log(plat::LogLevel::Warning,
                  "maxActive must be greater than or equal to minActive for shooting gallery spawner group '%s'\n",
                  name ? name : "");
}

// ===========================================================================
// The goal

GoalShootingGallery::GoalShootingGallery(const GoalDesc* desc)
    : Goal(desc), groups(16), commands(32), popups(16), scoreSources(32)
{
    const ape::xml::Color8 white = {0xFF, 0xFF, 0xFF, 0xFF};   // console global 0x01164CA0
    positiveColor = white;
    negativeColor = white;
}

// @func 0x0033DC0C GoalShootingGallery::Create
Goal* GoalShootingGallery::Create(ape::xml::XmlElement* xml, const GoalDesc* desc)
{
    GoalShootingGallery* goal = new GoalShootingGallery(desc);
    goal->Parse(xml);
    return goal;
}

// @func 0x0033D4E4 GoalShootingGallery::Parse
void GoalShootingGallery::Parse(ape::xml::XmlElement* xml)
{
    for (ape::xml::XmlElement* child = xml->FirstChildElement(kElem_Group); child;
         child = child->NextSiblingElement(kElem_Group)) {
        Group* group = new Group();
        if (const char* name = child->GetString(kAttr_Name))
            CopyName(group->name, sizeof(group->name), name);
        child->GetHash(kAttr_Name, &group->nameHash, group->nameHash);
        groups.PushBack(group);
    }

    if (ape::xml::XmlElement* list = xml->FirstChildElement(kElem_Commands)) {
        for (ape::xml::XmlElement* child = list->FirstChildElement(); child; child = child->NextSiblingElement()) {
            const uint32_t element = child->IsValid() ? child->GetNameHash() : 0;
            if (element != kCmd_Pause && element != kCmd_EnableGroup) {
                plat::Log(plat::LogLevel::Warning, "unknown cmd '%u'\n", element);
                continue;
            }
            Command* command = new Command();
            if (element == kCmd_Pause) {
                child->GetFloat(0x00274ACDu, &command->time, command->time);   // "time"
                command->type = Command::kPause;
            } else {
                command->ParseGroupParams(child);
                child->GetBool(0x7A281983u, &command->enable, command->enable);   // "Enable"
                command->type = Command::kEnableGroup;
            }
            commands.PushBack(command);
        }
    }

    xml->GetFloat(0x1E3F7010u, &popup_A3C, popup_A3C);
    xml->GetFloat(0xB6BFACEEu, &popup_A40, popup_A40);
    xml->GetFloat(0xF00CE6B3u, &popup_A44, popup_A44);
    xml->GetFloat(0x2AF63B94u, &popup_A48, popup_A48);
    if (const char* text = xml->GetString(0xB8B3BE3Au))
        CopyName(positiveText, sizeof(positiveText), text);
    if (const char* text = xml->GetString(0xB8B3B576u))
        CopyName(negativeText, sizeof(negativeText), text);
    xml->GetBool(0xBA4D5487u, &popupsEnabled, popupsEnabled);
    xml->GetColor(0x0800836Bu, &positiveColor, &positiveColor);
    xml->GetColor(0x08007AA7u, &negativeColor, &negativeColor);

    if (ape::xml::XmlElement* list = xml->FirstChildElement(kElem_ScoreSources)) {
        for (ape::xml::XmlElement* child = list->FirstChildElement(kElem_Type); child;
             child = child->NextSiblingElement(kElem_Type)) {
            ScoreSource source;
            child->GetHash(kAttr_Name, &source.group, 0);
            float offset[3] = {0.0f, 0.0f, 0.0f};   // console global 0x01164B40
            child->GetVec3(0xD005381Fu, offset, offset);
            source.popupOffset = {offset[0], offset[1], offset[2], 0.0f};
            scoreSources.PushBack(source);
        }
    }
}

// @func 0x0033D008 GoalShootingGallery::~GoalShootingGallery
// @func 0x0033D49C GoalShootingGallery::~GoalShootingGallery (deleting)
GoalShootingGallery::~GoalShootingGallery()
{
    SetStatus(kGoal_None, true);
    for (uint32_t i = 0; i < groups.Count(); ++i)
        delete groups[i];
    for (uint32_t i = 0; i < commands.Count(); ++i)
        delete commands[i];
    ClearPopups();
    GoalStopListening(this);
}

// @func 0x0076BAC0 GoalShootingGallery::TypeId
int GoalShootingGallery::TypeId() const
{
    return 0x23;
}

void GoalShootingGallery::ClearPopups()
{
    for (uint32_t i = 0; i < popups.Count(); ++i)
        delete popups[i];
    popups.Clear();
}

// @func 0x0033CE20 GoalShootingGallery::SetStatus
void GoalShootingGallery::SetStatus(uint8_t newStatus, bool force)
{
    Goal::SetStatus(newStatus, force);
    if (newStatus == kGoal_Active) {
        commandIndex = 0;
        pauseTimer = 0.0f;
        for (uint32_t i = 0; i < groups.Count(); ++i)
            groups[i]->Start();
        GoalStopListening(this);
        GoalListenTo(this, kListenScores);
        ClearPopups();
    } else if (newStatus == kGoal_Complete) {
        for (uint32_t i = 0; i < groups.Count(); ++i) {
            SpawnGroup* spawnGroup = groups[i]->spawnGroup;
            if (!spawnGroup)
                continue;
            spawnGroup->onSpawn = nullptr;
            spawnGroup->onSpawnData = nullptr;
            spawnGroup->onRelease = nullptr;
            spawnGroup->onReleaseData = nullptr;
            spawnGroup->DespawnAll(true);
            spawnGroup->Deactivate(false);
        }
        GoalStopListening(this);
        ClearPopups();
    }
}

// @func 0x0033DE30 GoalShootingGallery::UpdatePopups
void GoalShootingGallery::UpdatePopups(float dt)
{
    if (!popupsEnabled)
        return;
    for (uint32_t i = 0; i < popups.Count(); ++i) {
        ScorePopup* popup = popups[i];
        popup->speed += popup->acceleration;
        popup->height += popup->speed * dt;
        float fade = popup->remaining / popup->lifetime;
        if (255.0f - fade < 0.0f)
            fade = 255.0f;
        if (fade < 0.0f)
            fade = 0.0f;
        popup->remaining -= dt;
        popup->alpha = uint8_t(int(fade * 255.0f));
        if (popup->remaining <= 0.0f)
            popup->remaining = 0.0f;
        if (popup->remaining <= 0.0f) {
            delete popup;
            popups.RemoveAtSwap(i);
            --i;
        }
    }
}

// @func 0x0033DFC8 GoalShootingGallery::Update
uint8_t GoalShootingGallery::Update(int player, const GameEvent& e)
{
    const float dt = e.arg0Float;
    if (status == kGoal_Active && e.type == kEvent_06) {
        if (pauseTimer != 0.0f) {
            pauseTimer -= dt;
            if (pauseTimer < 0.0f)
                pauseTimer = 0.0f;
        }
        UpdatePopups(dt);
        // Run commands until one pauses.
        while (pauseTimer == 0.0f && commandIndex < commands.Count()) {
            const Command& command = *commands[commandIndex];
            if (command.type == Command::kEnableGroup) {
                Group* group = nullptr;
                for (uint32_t i = 0; i < groups.Count(); ++i) {
                    if (groups[i]->nameHash == command.group) {
                        group = groups[i];
                        break;
                    }
                }
                if (!group) {
                    plat::Log(plat::LogLevel::Warning, "can't find group %u\n", command.group);
                } else if (!command.enable) {
                    group->SetState(Group::kState_Stopping);
                } else {
                    group->commandName = command.group;
                    group->minActive = command.minActive;
                    group->maxActive = command.maxActive;
                    group->maxSpawn = command.maxSpawn;
                    group->delayBetweenMin = command.delayBetweenMin;
                    group->delayBetweenMax = command.delayBetweenMax;
                    group->SetState(Group::kState_Running);
                }
            } else if (command.type == Command::kPause) {
                pauseTimer = command.time;
            }
            ++commandIndex;
        }
        for (uint32_t i = 0; i < groups.Count(); ++i)
            groups[i]->Update(dt);
    }
    return Goal::Update(player, e);
}

// @func 0x0033E6EC GoalShootingGallery::HandleMessage
void GoalShootingGallery::HandleMessage(const GoalMessage& msg)
{
    void* sender = msg.sender;
    if (!sender || msg.id != kMsg_Score)
        return;
    for (uint32_t i = 0; i < scoreSources.Count(); ++i) {
        if (scoreSources[i].group == EntityGroupName(sender))
            AddScorePopup(sender, int32_t(msg.arg), scoreSources[i].popupOffset);
    }
}

// TODO(0x0033E2C0 GoalShootingGallery::AddScorePopup): builds a popup from the
// positive/negative GUI object and colour above the entity (vector code the
// disassembler cannot follow).
void GoalShootingGallery::AddScorePopup(void* entity, int score, const ape::Vec3& offset)
{
    (void)entity;
    (void)score;
    (void)offset;
}

// TODO(0x0033B9F8 GoalShootingGallery::DrawPopup): draws one popup (vector
// code the disassembler cannot follow).
void GoalShootingGallery::DrawPopup(const ScorePopup& popup)
{
    (void)popup;
}

// @func 0x0033E218 GoalShootingGallery::OnLevelLoaded
void GoalShootingGallery::OnLevelLoaded()
{
    // NOTE: slot 5 is called every frame for the popups here.
    if (!popupsEnabled)
        return;
    for (uint32_t i = 0; i < popups.Count(); ++i)
        DrawPopup(*popups[i]);
}

} // namespace goals
} // namespace sly
