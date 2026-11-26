// openTIT - Return-treasure microgame.
#include "sly/gameplay/ReturnTreasure.h"

#include "ape/anim/ApeAnimScript.h"
#include "sly/entity/Entity.h"
#include "sly/entity/EntityHooks.h"
#include "sly/game/GameScripts.h"
#include "sly/game/goals/JobManager.h"
#include "sly/game/hud/HudManager.h"
#include "sly/gameplay/GameplayWorld.h"

namespace sly {
namespace gameplay {

namespace {

constexpr uint32_t kScript_ReturnTreasure = 0x9F90E54Fu;
constexpr uint32_t kVar_Treasure = 0x811B12AFu;      // "Treasure"
constexpr uint32_t kVar_Carrier = 0xCBC95E9Eu;       // NOTE: name not recovered
constexpr uint32_t kState_End = 0x00010CBBu;
constexpr uint32_t kState_Returned = 0x4CF84295u;
constexpr uint32_t kState_Lost = 0xE26489ECu;
constexpr uint32_t kHud_TreasureStatus = 0x937B5F0Au; // HUD element (0x00F64CCC), name not recovered
constexpr uint32_t kHud_Message = 0x0001447Bu;

// TODO(0x0029D838): the targeting component's "carrying a treasure" flag
// (+0x922 bit 1) and its refresh.
void SetTargetingCarryFlag(entity::EntityComponent* targeting, bool on)
{
    (void)targeting;
    (void)on;
}
// TODO: targeting component +0x921 bit 4 (set when the treasure is lost).
void SetTargetingLostFlag(entity::EntityComponent* targeting)
{
    (void)targeting;
}
// TODO: the treasure entity's "returned" bit (+0x250 bit 30).
void MarkTreasureReturned(entity::Entity* treasure)
{
    (void)treasure;
}

bool HudShown(const hud::HudObject* element)
{
    return element && (element->stateFlags & hud::HudObject::kState_Shown) != 0;
}

} // namespace

// @func 0x00424D78 ReturnTreasureMicrogame::ReturnTreasureMicrogame
ReturnTreasureMicrogame::ReturnTreasureMicrogame()
{
    ape::ApeAnimScript* source = ape::ApeAnimScriptLibrary::FindScriptAnywhere(kScript_ReturnTreasure, 0, true);
    if (!source)
        return;
    script = new ape::ApeAnimScriptInstance(source);
    script->SetRemoveWhenFinished(false);
    ape::ApeAnimScriptInstance::Register(script);
    script->Update(1.0f / 60.0f);
}

// @func 0x00424F24 ReturnTreasureMicrogame::Shutdown
void ReturnTreasureMicrogame::Shutdown()
{
    if (!script)
        return;
    script->SetState(kState_End);
    script->SetRemoveWhenFinished(true);
    script = nullptr;
}

// @func 0x00425980 ReturnTreasureMicrogame::Reset
void ReturnTreasureMicrogame::Reset()
{
    field_00 = ape::Matrix34{};
    startRotation = {0.0f, 0.0f, 0.0f, 1.0f};
    targetRotation = {0.0f, 0.0f, 0.0f, 1.0f};
    startPosition = {0.0f, 0.0f, 0.0f, 0.0f};
    targetOffset = {0.0f, 0.0f, 0.0f, 0.0f};
    field_80 = 0;
    carrier = nullptr;
    treasure = nullptr;
    timeLimit = 0.0f;
    timeLeft = 0.0f;
    field_94 = 0.0f;
    timerHud = nullptr;
    field_A0 = 0;
    active = false;
    paused = false;
    if (script) {
        SetScriptObjectVar(script, kVar_Treasure, nullptr);
        SetScriptObjectVar(script, kVar_Carrier, nullptr);
    }
}

// TODO(0x00424FB8 ReturnTreasureMicrogame::Start): attaches the treasure to
// the carrier bone, binds the script variables, starts the countdown.
void ReturnTreasureMicrogame::Start(float limit, entity::Entity* newCarrier, entity::Entity* newTreasure,
                                    uint32_t carrierBone, const ape::Vec3& offset)
{
    (void)limit; (void)newCarrier; (void)newTreasure; (void)carrierBone; (void)offset;
}

// @func 0x00425BA8 ReturnTreasureMicrogame::Stop
void ReturnTreasureMicrogame::Stop(bool lost)
{
    if (!active)
        return;
    active = false;

    if (treasure) {
        if (lost) {
            if (script)
                script->SetStateAndUpdate(kState_Lost);
            WorldObject* object = reinterpret_cast<WorldObject*>(treasure);
            ActorDamage(object, ActorMaxHealth(object), 0);
        }
        if (carrier)
            carrier->DetachChild(treasure);
        if (treasure) {
            treasure->Release(false);
            treasure = nullptr;
        }
    }

    entity::EntityComponent* targeting = carrier ? entity::EntityTargeting(carrier) : nullptr;
    if (carrier) {
        if (targeting)
            SetTargetingCarryFlag(targeting, false);
        carrier->Release(false);
        carrier = nullptr;
    }

    if (timeLimit > 0.0f && HudShown(timerHud))
        timerHud->Show(true, true, false);

    if (hud::HudObject* status = hud::Find(kHud_TreasureStatus)) {
        if (lost)
            hud::ShowTextByName(3.0f, kHud_Message, "$STR_TREASURE_LOST", true);
        if (HudShown(status)) {
            status->SetFieldText("", 0);
            status->Show(true, true, false);
        }
        if (targeting)
            SetTargetingLostFlag(targeting);
    }
    Reset();
}

// @func 0x00425E54 ReturnTreasureMicrogame::Complete
void ReturnTreasureMicrogame::Complete()
{
    if (!active)
        return;
    if (script)
        script->SetStateAndUpdate(kState_Returned);
    if (treasure) {
        if (game::JobManager* jobs = game::GetJobManager())
            jobs->CollectTreasure(treasure, carrier);
        MarkTreasureReturned(treasure);
    }
    Stop(false);
}

// @func 0x0042681C ReturnTreasureMicrogame::OnEntityRemoved
void ReturnTreasureMicrogame::OnEntityRemoved(entity::Entity* entity)
{
    if (active && entity == carrier)
        Stop(true);
}

// TODO(0x0042685C ReturnTreasureMicrogame::UpdatePlacement) and
// TODO(0x00425F18): blend the treasure from its start transform onto the
// carrier.
void ReturnTreasureMicrogame::UpdatePlacement() {}

// TODO(0x00426380 ReturnTreasureMicrogame::Update): countdown and placement.
void ReturnTreasureMicrogame::Update(float dt)
{
    (void)dt;
}

// @func 0x004269E4 ReturnTreasureMicrogame::SetPaused
void ReturnTreasureMicrogame::SetPaused(bool on)
{
    paused = on;
    if (on && HudShown(timerHud))
        timerHud->Show(true, true, false);
    if (treasure)
        treasure->SetBusyFlag(on);
}

// @func 0x00426A90 ReturnTreasureMicrogame::RefreshHud
void ReturnTreasureMicrogame::RefreshHud()
{
    if (timerHud)
        timerHud->Refresh();
}

// @func 0x00426AD8 ReturnTreasureMicrogame::SetHudActive
void ReturnTreasureMicrogame::SetHudActive(bool on)
{
    if (timerHud)
        timerHud->SetActive(on);
}

} // namespace gameplay
} // namespace sly
