// openTIT - Dragon boss attack sequences.
#include "sly/gameplay/DragonSequence.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/world/ApeObj.h"
#include "sly/entity/EntityDef.h"
#include "sly/game/GameMessages.h"
#include "sly/game/SpawnManager.h"
#include "sly/game/hud/HudManager.h"
#include "sly/game/hud/HudMeter.h"

#include <cstdio>
#include <cstring>

namespace sly {
namespace gameplay {

namespace {

constexpr uint32_t kAttr_Name     = 0x0024728B;   // "Name"
constexpr uint32_t kAttr_FireOnce = 0x64FDE756;
constexpr uint32_t kAttr_Time     = 0xB7EA3D94;
constexpr uint32_t kAttr_State    = 0x9178C4CB;
constexpr uint32_t kVar_Speed     = 0xE9BEF0B6;
constexpr uint32_t kState_Idle    = 0x002237D4;

// Thread body part names.
constexpr uint32_t kPart_RandomA = 0x0416F651;
constexpr uint32_t kPart_RandomB = 0x0416F652;
constexpr uint32_t kPart_Root    = 0x002678E2;
constexpr uint32_t kPart_Laser   = 0x044DC6AB;   // "Laser"
// Command names that pick their own body part.
constexpr uint32_t kCmd_RootState = 0x04B59DBC;
constexpr uint32_t kCmd_Wind      = 0x0028A808;   // "Wind"

// Element names of the commands.
constexpr uint32_t kElem_FireballA       = 0x952A45AC;
constexpr uint32_t kElem_FireballB       = 0x952A45AD;
constexpr uint32_t kElem_FireballC       = 0x952A45AE;
constexpr uint32_t kElem_FlameStart      = 0x9449490F;
constexpr uint32_t kElem_Rumble          = 0x90BF3291;
constexpr uint32_t kElem_FlameEnd        = 0x8F3458C8;
constexpr uint32_t kElem_Wind            = 0x0028A808;
constexpr uint32_t kElem_Wait            = 0x00288975;
constexpr uint32_t kElem_NextCollection  = 0xB6147A93;
constexpr uint32_t kElem_Prime           = 0x048DBA5F;
constexpr uint32_t kElem_Laser           = 0x044DC6AB;
constexpr uint32_t kElem_Flame           = 0x03FDF713;
constexpr uint32_t kElem_EnableThread    = 0x5D35AD4D;
constexpr uint32_t kElem_SpawnBabyDragons = 0x542784CF;
constexpr uint32_t kElem_UnPrime         = 0x1A196766;
constexpr uint32_t kElem_FireballHoming  = 0x0969AF51;
constexpr uint32_t kElem_Ignored         = 0x04B59DBC;
constexpr uint32_t kElem_Thread          = 0x9374020A;
constexpr uint32_t kElem_Collection      = 0x21E81828;

// Action states.
constexpr uint32_t kActionState[13] = {
    0, 0x048DBA5F, 0x1A196766, 0x90BF3291, 0x952A45AC, 0x952A45AD, 0x952A45AE,
    0, 0, 0, 0, 0, 0x04B59DBC,
};
constexpr uint32_t kHomingDefaultState = 0x0969AF51;
constexpr uint32_t kEvent_ProjectileMissed = 0x82485B95;

// The game's shared random generator (0x01164890).
uint32_t s_randomSeed = 0x12345678;

uint32_t Rotl(uint32_t v, int n) { return (v << n) | (v >> (32 - n)); }

void SetFloatVar(ape::ApeAnimScriptInstance* script, uint32_t name, float value)
{
    if (!script)
        return;
    if (ape::ApeAnimScriptVar* var = script->FindVar(name))
        if (var->GetType() == ape::AnimScriptVarType::Float)
            static_cast<ape::ApeAnimScriptFloatVar*>(var)->SetFloat(value);
}

void SetIntVar(ape::ApeAnimScriptInstance* script, uint32_t name, int32_t value)
{
    if (!script)
        return;
    if (ape::ApeAnimScriptVar* var = script->FindVar(name))
        if (var->GetType() == ape::AnimScriptVarType::Int)
            static_cast<ape::ApeAnimScriptIntVar*>(var)->SetInt(value);
}

void StepScript(ape::ApeAnimScriptInstance* script, float dt)
{
    if (script && !script->IsFinished(true))
        script->Update(dt);
}

} // namespace

// ===========================================================================
// Body parts

int DragonBoss::HeadIndex(const DragonBodyPart* part) const
{
    for (int i = 0; i < kHeadCount; ++i)
        if (heads[i] == part)
            return i;
    return -1;
}

// ===========================================================================
// Dragon entity (the parts of it that are ported)

// @func 0x002FE63C DragonBoss::~DragonBoss
// @func 0x002FE8E8 DragonBoss::~DragonBoss (deleting)
// @func 0x002FE9F0 DragonBoss::~DragonBoss (thunk, +0x17C)
// @func 0x002FE97C DragonBoss::~DragonBoss (deleting thunk, +0x17C)
// @func 0x002FE9C8 DragonBoss::~DragonBoss (thunk, +0x180)
// @func 0x002FE930 DragonBoss::~DragonBoss (deleting thunk, +0x180)
DragonBoss::~DragonBoss()
{
    for (int i = kHeadCount - 1; i >= 0; --i) {
        delete headParts[i].script;
        headParts[i].script = nullptr;
    }
    delete laser.script;
    laser.script = nullptr;
    delete root.script;
    root.script = nullptr;
}

// @func 0x003011B8 DragonBoss::PostUpdate
bool DragonBoss::PostUpdate(float dt)
{
    return ActorEntity::PostUpdate(dt);
}

namespace {
// Offset from a head's marker object to where its target entity is shown
// (0x0106AC80). NOTE: its writer is not identified; zero until then.
ape::Vec3 s_headTargetOffset{0.0f, 0.0f, 0.0f, 0.0f};
constexpr uint32_t kState_Reset = 0x0001314F;
constexpr uint32_t kState_HeadDown = 0x9F60B8DA;
constexpr uint32_t kProjectileDefAlt = 0x1AA5830E;
} // namespace

// @func 0x002F9AC0 DragonBoss::SetHeadTargetActive
void DragonBoss::SetHeadTargetActive(bool active, DragonBodyPart* part)
{
    for (int i = 0; i < kHeadCount; ++i) {
        if (heads[i] != part)
            continue;
        entity::Entity* target = headTargets[i];
        target->SetVisible(active, true, true, true);
        if (!active) {
            DragonHeadTargetSet(target, false, nullptr);
            continue;
        }
        ape::Vec3 position{0.0f, 0.0f, 0.0f, 0.0f};
        if (auto* marker = static_cast<ape::world::ApeObj*>(field_430[i])) {
            const ape::Vec3& at = marker->GetWorld().translation;
            position = {at.x + s_headTargetOffset.x, at.y + s_headTargetOffset.y, at.z + s_headTargetOffset.z, 0.0f};
        }
        ape::Matrix34 placement;
        ape::Matrix34FromTranslation(placement, position);
        DragonHeadTargetSet(target, true, &placement);
    }
}

// @func 0x002FEA20 DragonBoss::ResetEncounter
void DragonBoss::ResetEncounter()
{
    for (void* entry : spawners) {
        auto* spawner = static_cast<Spawner*>(entry);
        spawner->DespawnAll(false);
        spawner->Deactivate(false);
    }
    for (void* object : objectsB)
        static_cast<ape::world::ApeObj*>(object)->SetVisible(false, true);
    for (void* object : objectsA)
        static_cast<ape::world::ApeObj*>(object)->SetVisible(false, true);
    for (void* body : bodies)
        DragonBodyDisable(body);

    for (int i = 0; i < kHeadCount; ++i) {
        SetHeadTargetActive(false, heads[i]);
        heads[i]->SetStateAndStep(kState_HeadDown);
        heads[i]->SetStateAndStep(kState_Idle);
    }
    // The laser and the root are wound forward a whole second.
    for (DragonBodyPart* part : {&laser, &root}) {
        part->SetState(kState_Reset);
        StepScript(part->script, 1.0f);
    }
}

// @func 0x002FEFB4 DragonBoss::UpdatePhase
void DragonBoss::UpdatePhase(float dt)
{
    if (field_200)
        return;
    if (!currentPhase) {
        int aliveHeads = 0;
        for (DragonBodyPart* head : heads)
            if (head->alive)
                ++aliveHeads;
        auto* dragonDef = static_cast<DragonBossDef*>(def);
        for (DragonPhase* phase : dragonDef->phases) {
            if (phase->mode != mode || phase->headCount != aliveHeads)
                continue;
            currentPhase = phase;
            phase->dragon = this;
            phase->collectionIndex = 0;
            phase->current = nullptr;
            phase->advance = false;
            break;
        }
        if (currentPhase) {
            char name[64];
            std::snprintf(name, sizeof(name), "ProjectilePhase%i_NumHeads%i", mode == 1 ? 1 : 2, aliveHeads);
            entity::EntityDef* found = entity::FindEntityDef(ape::HashNameNoCase(name));
            if (found && found->IsClass(entity::kDefClass_Physical))
                projectileDef = found;
            found = entity::FindEntityDef(kProjectileDefAlt);
            if (found && found->IsClass(entity::kDefClass_Physical))
                projectileDefAlt = found;
        }
    }
    if (currentPhase) {
        currentPhase->Update(dt);
        field_1F0 = false;
    }
}

// @func 0x002FEEA0 DragonBoss::Release
bool DragonBoss::Release(bool kill)
{
    if (kill) {
        ResetEncounter();
        for (DragonPhase* phase : static_cast<DragonBossDef*>(def)->phases)
            phase->Reset(true);
        hud::SetContext(this, 0);
        hud::UpdateMeterA();
        UnregisterListener(this, true);
        delete ownedObject;
        ownedObject = nullptr;
    }
    return ActorEntity::Release(kill);
}

// @func 0x00302CCC DragonBoss::OnHitCategory0
void DragonBoss::OnHitCategory0(entity::EntityHitRecord& hit) { (void)hit; }
// @func 0x00302CD4 DragonBoss::OnHitCategory1
void DragonBoss::OnHitCategory1(entity::EntityHitRecord& hit) { (void)hit; }
// @func 0x00302CD8 DragonBoss::OnHitCategory2
void DragonBoss::OnHitCategory2(entity::EntityHitRecord& hit) { (void)hit; }
// @func 0x00302CDC DragonBoss::OnHitCategory3
void DragonBoss::OnHitCategory3(entity::EntityHitRecord& hit) { (void)hit; }
// @func 0x00302CD0 DragonBoss::OnHitCategory4
void DragonBoss::OnHitCategory4(entity::EntityHitRecord& hit) { (void)hit; }

// @func 0x00302CF0 DragonBoss::NotifyHeadEvent
bool DragonBoss::NotifyHeadEvent(entity::Entity* head)
{
    if (!head)
        return false;
    constexpr uint32_t kHeadNames[kHeadCount] = {0x37C9BF3Cu, 0x37C9BF3Du, 0x37C9BF3Eu};
    uint32_t index = 0x0FFFFFFFu;
    for (uint32_t i = 0; i < kHeadCount; ++i) {
        if (head->nameHash == kHeadNames[i]) {
            index = i;
            break;
        }
    }
    PostToListeners(head, 0xA07A77E3u, 0x8024237Bu, index);
    return true;
}

// @func 0x00302CC4 DragonBoss::Interface17CSlot5
// @func 0x00302CC8 DragonBoss::Interface17CSlot5 (thunk, +0x17C)
void DragonBoss::Interface17CSlot5(const void* event) { (void)event; }

// @func 0x002F9424 DragonBodyPart::SetState
void DragonBodyPart::SetState(uint32_t state)
{
    if (script)
        script->SetState(state);
    if (!dragon)
        return;
    const int head = dragon->HeadIndex(this);
    if (head < 0)
        return;
    if (dragon->mode == 1 && dragon->headModelA[head])
        DragonModelSetState(dragon->headModelA[head], state);
    if (dragon->headModelB[head])
        DragonModelSetState(dragon->headModelB[head], state);
}

// @func 0x002F9540 DragonBodyPart::SetStateAndStep
void DragonBodyPart::SetStateAndStep(uint32_t state)
{
    SetState(state);
    StepScript(script, 0.1f);
}

// ===========================================================================
// Actions

// @func 0x002F969C DragonAction::~DragonAction
// @func 0x002F96AC DragonAction::~DragonAction (deleting)
DragonAction::~DragonAction() = default;

// @func 0x002F96F0 DragonAction::Start
void DragonAction::Start()
{
    if (part)
        SetFloatVar(part->script, kVar_Speed, speed);
}

// @func 0x002F978C DragonAction::Stop
void DragonAction::Stop(bool reset)
{
    if (!part)
        return;
    part->busy = false;
    if (reset)
        part->SetState(kState_Idle);
}

// @func 0x002F98D0 DragonAction::Update
void DragonAction::Update(float dt)
{
    if (part)
        StepScript(part->script, dt * speed);
}

// @func 0x002F9940 DragonAction::IsDone
bool DragonAction::IsDone()
{
    if (!part || !part->script)
        return true;
    return part->script->IsFinished(false);
}

// @func 0x002FCA64 DragonStateAction::Start
void DragonStateAction::Start()
{
    DragonAction::Start();
    if (state != 0 && part)
        part->SetState(state);
}

// @func 0x002FCC14 DragonStateAction::Update
void DragonStateAction::Update(float dt)
{
    if (part)
        StepScript(part->script, dt * speed);
}

// @func 0x002F99A0 DragonFlameAction::~DragonFlameAction
// @func 0x002F9A20 DragonFlameAction::~DragonFlameAction (deleting)
DragonFlameAction::~DragonFlameAction()
{
    delete effect;
    effect = nullptr;
}

// TODO(0x002F9CA0 DragonFlameAction::Start)
void DragonFlameAction::Start()
{
    DragonAction::Start();
}

// TODO(0x002FA680 DragonFlameAction::Update)
void DragonFlameAction::Update(float dt)
{
    DragonAction::Update(dt);
}

// @func 0x002FB460 DragonFlameAction::IsDone
bool DragonFlameAction::IsDone()
{
    // Only the end of the flame finishes the action.
    if (kind != 8)
        return false;
    return DragonAction::IsDone();
}

// @func 0x002FA37C DragonFlameAction::Release
void DragonFlameAction::Release()
{
    Stop(false);
    delete effect;
    effect = nullptr;
}

// @func 0x002FA3F0 DragonFlameAction::StartEffect
void DragonFlameAction::StartEffect()
{
    delete effect;
    effect = nullptr;
    ape::ApeAnimScript* script = ape::ApeAnimScriptLibrary::FindScriptAnywhere(0x1C7F7772, 0, true);
    if (!script)
        return;
    auto* instance = new ape::ApeAnimScriptInstance(script);
    DragonSetScriptObject(instance, 0x0027469E, dragon ? dragon->worldObject : nullptr);
    SetIntVar(instance, 0x044DDD31, int32_t(part ? part->field_28 : 0));
    SetIntVar(instance, 0x00255BF3, int32_t(part ? part->field_24 : 0));
    SetFloatVar(instance, kVar_Speed, 1.0f);
    instance->SetStateAndUpdate(0x00011773);
    effect = instance;
}

DragonHomingAction::DragonHomingAction(DragonBoss* dragonIn, DragonBodyPart* partIn, uint32_t targetIn)
    : DragonAction(dragonIn, partIn), target(targetIn), projectiles(8)
{
}

// @func 0x002FB4E0 DragonHomingAction::~DragonHomingAction
// @func 0x002FB5B4 DragonHomingAction::~DragonHomingAction (deleting)
DragonHomingAction::~DragonHomingAction()
{
    // NOTE: the console releases each projectile through its slot 10.
    projectiles.Clear();
}

// @func 0x002FB6A8 DragonHomingAction::Start
void DragonHomingAction::Start()
{
    if (part)
        part->SetState(target);
    phase = kPhase_Firing;
}

// TODO(0x002FB7DC DragonHomingAction::Update)
void DragonHomingAction::Update(float dt)
{
    DragonAction::Update(dt);
}

// @func 0x002FCA20 DragonHomingAction::OnProjectileEvent
void DragonHomingAction::OnProjectileEvent(void* projectile, uint32_t event)
{
    if (projectile == part)
        OnProjectileResult(event == kEvent_ProjectileMissed ? 2 : 1);
}

// TODO(0x002FC600 DragonHomingAction::OnProjectileResult)
void DragonHomingAction::OnProjectileResult(int result)
{
    (void)result;
}

// @func 0x002FBB30 DragonHomingAction::IsDone
bool DragonHomingAction::IsDone()
{
    return phase == kPhase_Done;
}

// ===========================================================================
// Commands

DragonCommand::DragonCommand(DragonThread* owner, const char* commandName) : thread(owner)
{
    std::strncpy(name, commandName, sizeof(name) - 1);
}

// @func 0x002FD540 DragonCommand::Parse
void DragonCommand::Parse(ape::xml::XmlElement* xml)
{
    xml->GetBool(kAttr_FireOnce, &fireOnce, fireOnce);
}

// @func 0x002FD580 DragonCommand::Reset
void DragonCommand::Reset(bool rearm)
{
    if (rearm)
        fired = false;
}

// @func 0x002FD598 DragonCommand::Start
bool DragonCommand::Start(DragonBodyPart* part)
{
    (void)part;
    if (fireOnce && fired)
        return true;
    fired = true;
    return false;
}

// @func 0x002FD5C8 DragonWaitCommand::Parse
void DragonWaitCommand::Parse(ape::xml::XmlElement* xml)
{
    xml->GetBool(kAttr_FireOnce, &fireOnce, fireOnce);
    xml->GetFloat(kAttr_Time, &duration, duration);
}

// @func 0x002FD640 DragonWaitCommand::Reset
void DragonWaitCommand::Reset(bool rearm)
{
    DragonCommand::Reset(rearm);
    remaining = duration;
}

// @func 0x002FD660 DragonWaitCommand::Start
bool DragonWaitCommand::Start(DragonBodyPart* part)
{
    (void)part;
    if (fireOnce && fired)
        return true;
    fired = true;
    remaining = duration;
    return false;
}

// @func 0x002FD6A0 DragonWaitCommand::Update
void DragonWaitCommand::Update(float dt)
{
    remaining -= dt;
    if (remaining < 0.0f)
        remaining = 0.0f;
}

// @func 0x002FD6D8 DragonWaitCommand::IsDone
bool DragonWaitCommand::IsDone()
{
    return remaining <= 0.0f;
}

// @func 0x002FD708 DragonNextCollectionCommand::Parse
void DragonNextCollectionCommand::Parse(ape::xml::XmlElement* xml)
{
    xml->GetBool(kAttr_FireOnce, &fireOnce, fireOnce);
}

// @func 0x002FD748 DragonNextCollectionCommand::Reset
void DragonNextCollectionCommand::Reset(bool rearm)
{
    DragonCommand::Reset(rearm);
}

// @func 0x002FD768 DragonNextCollectionCommand::Start
bool DragonNextCollectionCommand::Start(DragonBodyPart* part)
{
    (void)part;
    if (!fireOnce || !fired) {
        fired = true;
        DragonBoss* dragon = thread->collection->phase->dragon;
        if (dragon && dragon->currentPhase)
            dragon->currentPhase->advance = true;
    }
    return true;
}

// @func 0x002FD7B4 DragonNextCollectionCommand::Update
void DragonNextCollectionCommand::Update(float dt)
{
    (void)dt;
}

// @func 0x002FD7B8 DragonNextCollectionCommand::IsDone
bool DragonNextCollectionCommand::IsDone()
{
    return true;
}

// @func 0x002FD7C0 DragonActionCommand::~DragonActionCommand
// @func 0x002FD838 DragonActionCommand::~DragonActionCommand (deleting)
DragonActionCommand::~DragonActionCommand()
{
    delete action;
    action = nullptr;
}

// @func 0x002FD8D0 DragonActionCommand::Parse
void DragonActionCommand::Parse(ape::xml::XmlElement* xml)
{
    xml->GetBool(kAttr_FireOnce, &fireOnce, fireOnce);
}

// @func 0x002FD910 DragonActionCommand::Reset
void DragonActionCommand::Reset(bool rearm)
{
    DragonCommand::Reset(rearm);
    delete action;
    action = nullptr;
}

// @func 0x002FD988 DragonActionCommand::Start
bool DragonActionCommand::Start(DragonBodyPart* part)
{
    if (fireOnce && fired)
        return true;
    fired = true;
    delete action;
    action = nullptr;

    DragonBoss* dragon = thread->collection->phase->dragon;
    switch (type) {
    case kAction_FireballHoming:
        if (homingState == 0)
            homingState = kHomingDefaultState;
        action = new DragonHomingAction(dragon, part, homingState);
        break;
    case kAction_Prime:
    case kAction_UnPrime:
    case kAction_Rumble:
    case kAction_FireballA:
    case kAction_FireballB:
    case kAction_FireballC:
        action = new DragonStateAction(dragon, part, kActionState[type]);
        break;
    case kAction_FlameStart:
    case kAction_FlameEnd:
        action = new DragonFlameAction(dragon, part, type, 0.0f, 0.0f);
        break;
    case kAction_Flame:
        if (flameA == 0.0f)
            flameA = 1.0f;
        if (flameB == 0.0f)
            flameB = 1.0f;
        action = new DragonFlameAction(dragon, part, type, flameA, flameB);
        break;
    default:
        // NOTE: type 12 (a plain state action) exists on the console but no
        // command creates it.
        break;
    }
    done = false;
    if (!action)
        return true;
    action->Start();
    return false;
}

// @func 0x002FE020 DragonActionCommand::Update
void DragonActionCommand::Update(float dt)
{
    if (done || !action)
        return;
    action->Update(dt);
    if (action->IsDone()) {
        delete action;
        action = nullptr;
        done = true;
    }
}

// @func 0x002FE13C DragonActionCommand::OnProjectileEvent
void DragonActionCommand::OnProjectileEvent(void* projectile, uint32_t event)
{
    if (action)
        action->OnProjectileEvent(projectile, event);
}

// @func 0x002FE0EC DragonActionCommand::IsDone
bool DragonActionCommand::IsDone()
{
    return !action || action->IsDone();
}

// @func 0x002FE198 DragonPartStateCommand::Parse
void DragonPartStateCommand::Parse(ape::xml::XmlElement* xml)
{
    xml->GetBool(kAttr_FireOnce, &fireOnce, fireOnce);
    xml->GetHash(kAttr_State, &state, state);
}

// @func 0x002FE210 DragonPartStateCommand::Reset
void DragonPartStateCommand::Reset(bool rearm)
{
    DragonCommand::Reset(rearm);
    target = nullptr;
}

// @func 0x002FE230 DragonPartStateCommand::Start
bool DragonPartStateCommand::Start(DragonBodyPart* part)
{
    if (fireOnce && fired)
        return true;
    fired = true;
    // Some commands act on a fixed part instead of the thread's.
    DragonBoss* dragon = thread->collection->phase->dragon;
    const uint32_t key = ape::HashNameNoCase(name);
    if (dragon) {
        if (key == kCmd_RootState || key == kCmd_Wind)
            part = &dragon->root;
        else if (key == kPart_Laser)
            part = &dragon->laser;
    }
    if (!part || state == 0)
        return true;
    target = part;
    part->SetState(state);
    return false;
}

// @func 0x002FE448 DragonPartStateCommand::Update
void DragonPartStateCommand::Update(float dt)
{
    if (target)
        StepScript(target->script, dt);
}

// @func 0x002FE4B4 DragonPartStateCommand::IsDone
bool DragonPartStateCommand::IsDone()
{
    if (!target || !target->script)
        return true;
    return target->script->IsFinished(false);
}

// @func 0x002FE510 DragonEnableThreadCommand::Parse
void DragonEnableThreadCommand::Parse(ape::xml::XmlElement* xml)
{
    xml->GetBool(kAttr_FireOnce, &fireOnce, fireOnce);
    xml->GetHash(kAttr_Name, &threadName, threadName);
}

// @func 0x002FE584 DragonEnableThreadCommand::Reset
void DragonEnableThreadCommand::Reset(bool rearm)
{
    DragonCommand::Reset(rearm);
}

// @func 0x002FE59C DragonEnableThreadCommand::Start
bool DragonEnableThreadCommand::Start(DragonBodyPart* part)
{
    (void)part;
    if (!fireOnce || !fired) {
        fired = true;
        for (DragonThread* other : thread->collection->threads) {
            if (other->nameHash == threadName) {
                other->enabled = true;
                break;
            }
        }
    }
    return true;
}

// @func 0x002FE630 DragonEnableThreadCommand::Update
void DragonEnableThreadCommand::Update(float dt)
{
    (void)dt;
}

// @func 0x002FE634 DragonEnableThreadCommand::IsDone
bool DragonEnableThreadCommand::IsDone()
{
    return true;
}

// ===========================================================================
// Threads

// @func 0x002F7D60 DragonThread::Parse
void DragonThread::Parse(ape::xml::XmlElement* xml)
{
    if (const char* text = xml->GetString(kAttr_Name))
        std::strncpy(name, text, sizeof(name) - 1);
    xml->GetHash(kAttr_Name, &nameHash, nameHash);
    xml->GetHash(0x9C939FA0u, &bodyPart, bodyPart);
    xml->GetBool(0x98D03360u, &randomPart, randomPart);
    xml->GetBool(0xCADB1721u, &startEnabled, startEnabled);

    for (ape::xml::XmlElement* child = xml->FirstChildElement(); child;
         child = child->NextSiblingElement()) {
        const uint32_t key = child->GetNameHash();
        DragonCommand* command = nullptr;
        using Cmd = DragonActionCommand;
        switch (key) {
        case kElem_FireballA:  command = new Cmd(this, "FireballA", Cmd::kAction_FireballA); break;
        case kElem_FireballB:  command = new Cmd(this, "FireballB", Cmd::kAction_FireballB); break;
        case kElem_FireballC:  command = new Cmd(this, "FireballC", Cmd::kAction_FireballC); break;
        case kElem_FlameStart: command = new Cmd(this, "FlameStart", Cmd::kAction_FlameStart); break;
        case kElem_FlameEnd:   command = new Cmd(this, "FlameEnd", Cmd::kAction_FlameEnd); break;
        case kElem_Rumble:     command = new Cmd(this, "Rumble", Cmd::kAction_Rumble); break;
        case kElem_Prime:      command = new Cmd(this, "Prime", Cmd::kAction_Prime); break;
        case kElem_UnPrime:    command = new Cmd(this, "UnPrime", Cmd::kAction_UnPrime); break;
        case kElem_Flame: {
            auto* flame = new Cmd(this, "Flame", Cmd::kAction_Flame);
            child->GetFloat(0x771571A7u, &flame->flameA, 0.0f);
            child->GetFloat(0x5AC9E44Eu, &flame->flameB, 0.0f);
            command = flame;
            break;
        }
        case kElem_FireballHoming: {
            auto* homing = new Cmd(this, "FireballHoming", Cmd::kAction_FireballHoming);
            child->GetHash(kAttr_State, &homing->homingState, 0);
            command = homing;
            break;
        }
        case kElem_Wind:             command = new DragonPartStateCommand(this, "Wind"); break;
        case kElem_Laser:            command = new DragonPartStateCommand(this, "Laser"); break;
        case kElem_SpawnBabyDragons: command = new DragonPartStateCommand(this, "SpawnBabyDragons"); break;
        case kElem_Wait:             command = new DragonWaitCommand(this, "Wait"); break;
        case kElem_NextCollection:   command = new DragonNextCollectionCommand(this, "StartNextCollection"); break;
        case kElem_EnableThread:     command = new DragonEnableThreadCommand(this, "EnableThread"); break;
        case kElem_Ignored:
            continue;
        default:
            std::fprintf(stderr, "Invalid sequence command type '%u'\n", key);
            continue;
        }
        command->Parse(child);
        commands.PushBack(command);
    }
}

// @func 0x002FCD4C DragonThread::Update
void DragonThread::Update(float dt)
{
    if (commands.Count() == 0 || finished || !enabled)
        return;

    if (!part && bodyPart != 0) {
        DragonBoss* dragon = collection->phase->dragon;
        DragonBodyPart* found = nullptr;
        if (bodyPart == kPart_RandomA || bodyPart == kPart_RandomB) {
            DragonBodyPart*& slot = bodyPart == kPart_RandomA ? dragon->randomPartA : dragon->randomPartB;
            if (!randomPart) {
                found = slot;
            } else {
                // Pick a random live, free head (the game's shared generator).
                const uint32_t seed = s_randomSeed;
                const uint32_t next = Rotl((Rotl(seed, 4) + Rotl(seed, 16)) ^ 0x87654321u, 16);
                uint32_t index = seed % 3;
                for (int tries = 0; tries < 3; ++tries) {
                    DragonBodyPart* head = dragon->heads[index];
                    if (head && head->alive && head->available) {
                        found = head;
                        break;
                    }
                    if (++index > 2)
                        index = 0;
                }
                s_randomSeed = next;
                slot = found;
            }
        } else if (bodyPart == kPart_Root) {
            found = &dragon->root;
        } else if (bodyPart == kPart_Laser) {
            found = &dragon->laser;
        } else {
            std::fprintf(stderr, "Can't find body part for thread '%s'!\n", name);
        }
        if (found && found->available)
            part = found;
    }
    if (!part && bodyPart != 0)
        return;

    if (!current) {
        // Commands that finish at once are skipped in the same update.
        while (commandIndex < commands.Count()) {
            current = commands[commandIndex];
            if (!current->Start(part))
                break;
            current = nullptr;
            ++commandIndex;
        }
        if (!current) {
            if (part) {
                part->available = true;
                part = nullptr;
            }
            finished = true;
            return;
        }
    }
    if (part)
        part->available = false;
    current->Update(dt);
    if (current->IsDone()) {
        current = nullptr;
        if (++commandIndex >= commands.Count()) {
            if (part) {
                part->available = true;
                part = nullptr;
            }
            finished = true;
        }
    }
}

// ===========================================================================
// Collections and phases

// @func 0x002F892C DragonCollection::Parse
void DragonCollection::Parse(ape::xml::XmlElement* xml)
{
    if (const char* text = xml->GetString(kAttr_Name))
        std::strncpy(name, text, sizeof(name) - 1);
    // NOTE: the attribute is passed by an address the decompiler did not
    // resolve (0x0027129B); read here as the "wait for scripts" flag.
    xml->GetBool(0x0027129Bu, &waitForScripts, waitForScripts);
    for (ape::xml::XmlElement* child = xml->FirstChildElement(kElem_Thread); child;
         child = child->NextSiblingElement(kElem_Thread)) {
        auto* thread = new DragonThread;
        thread->collection = this;
        thread->Parse(child);
        threads.PushBack(thread);
    }
}

// @func 0x002FCC8C DragonCollection::ScriptsFinished
bool DragonCollection::ScriptsFinished() const
{
    for (const DragonThread* thread : threads)
        if (thread->enabled && thread->part && thread->part->script &&
            !thread->part->script->IsFinished(false))
            return false;
    return true;
}

// @func 0x002FD0AC DragonCollection::Update
void DragonCollection::Update(float dt)
{
    if (done || !running)
        return;
    for (DragonThread* thread : threads)
        thread->Update(dt);
    for (DragonThread* thread : threads)
        if (thread->enabled && !thread->finished)
            return;
    done = true;
    running = false;
}

// @func 0x002FD1B0 DragonCollection::Begin
void DragonCollection::Begin()
{
    done = false;
    running = true;
    for (DragonThread* thread : threads) {
        thread->commandIndex = 0;
        thread->finished = false;
        thread->current = nullptr;
        thread->part = nullptr;
        thread->enabled = thread->startEnabled;
        for (DragonCommand* command : thread->commands)
            command->Reset(false);
    }
}

// @func 0x002F8AFC DragonPhase::Parse
void DragonPhase::Parse(ape::xml::XmlElement* xml)
{
    if (const char* text = xml->GetString(kAttr_Name))
        std::strncpy(name, text, sizeof(name) - 1);
    xml->GetInt(0x0489115Bu, &mode, 0);
    xml->GetInt(0xBE8976CDu, &headCount, 0);
    for (ape::xml::XmlElement* child = xml->FirstChildElement(kElem_Collection); child;
         child = child->NextSiblingElement(kElem_Collection)) {
        auto* collection = new DragonCollection;
        collection->phase = this;
        collection->Parse(child);
        collections.PushBack(collection);
    }
}

// @func 0x002FD2A8 DragonPhase::Update
void DragonPhase::Update(float dt)
{
    if (collections.Count() == 0)
        return;
    if (!current) {
        current = collections[collectionIndex];
        current->Begin();
    }
    for (DragonCollection* collection : collections)
        collection->Update(dt);
    if (!current || !(current->done || advance))
        return;
    if (current->waitForScripts)
        for (DragonCollection* collection : collections)
            if (!collection->ScriptsFinished())
                return;
    current = nullptr;
    advance = false;
    if (++collectionIndex >= collections.Count())
        collectionIndex = 0;
}

// @func 0x002FD414 DragonPhase::ForwardProjectileEvent
void DragonPhase::ForwardProjectileEvent(void* projectile, uint32_t event)
{
    for (DragonCollection* collection : collections) {
        if (!collection->running || collection->done)
            continue;
        for (DragonThread* thread : collection->threads)
            if (thread->enabled && thread->current)
                thread->current->OnProjectileEvent(projectile, event);
    }
}

// @func 0x002F6054 DragonPhase::Destroy
void DragonPhase::Destroy()
{
    for (DragonCollection* collection : collections) {
        if (!collection)
            continue;
        for (DragonThread* thread : collection->threads) {
            if (!thread)
                continue;
            for (DragonCommand* command : thread->commands)
                delete command;
            delete thread;
        }
        delete collection;
    }
    collections.Clear();
}

// @func 0x002F92C0 DragonPhase::Reset
void DragonPhase::Reset(bool rearm)
{
    collectionIndex = 0;
    current = nullptr;
    advance = false;
    for (DragonCollection* collection : collections) {
        collection->done = false;
        collection->running = false;
        for (DragonThread* thread : collection->threads) {
            thread->commandIndex = 0;
            thread->finished = false;
            thread->current = nullptr;
            thread->part = nullptr;
            for (DragonCommand* command : thread->commands)
                command->Reset(rearm);
        }
    }
}

// Hooks (the dragon entity and its models are not ported).
void DragonHeadTargetSet(entity::Entity* target, bool active, const ape::Matrix34* placement)
{
    (void)target;
    (void)active;
    (void)placement;
}

void DragonBodyDisable(void* body) { (void)body; }

void DragonModelSetState(void* model, uint32_t state)
{
    (void)model;
    (void)state;
}

void DragonSetScriptObject(ape::ApeAnimScriptInstance* script, uint32_t var, void* object)
{
    (void)script;
    (void)var;
    (void)object;
}

} // namespace gameplay
} // namespace sly
