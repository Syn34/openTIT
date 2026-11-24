// openTIT - Racer behaviour (vtable 0x00B0E428) and the spawner race tracker.
#include "sly/levels/ai/AIBehaviours.h"

#include "ape/world/ApeInstances.h"
#include "sly/levels/ai/AIHooks.h"

namespace sly {
namespace levels {
namespace ai {

namespace {

constexpr uint32_t kMsg_Kill = 0x03DE9A54u;
constexpr uint32_t kBone_Effect = 0x001F0BAFu;   // NOTE: bone name unknown
constexpr uint32_t kRacerTypeBit = 0x200u;       // entity type bit 9 (+0xC0): racer

RacerAI* RacerOf(Entity* entity)
{
    if (!entity || !(entity->typeBits[0] & kRacerTypeBit))
        return nullptr;
    return dynamic_cast<RacerAI*>(entity->listener);
}

bool IsRacingEntity(const Entity* entity)
{
    return entity && (entity->typeBits[0] & kRacerTypeBit) && CharacterHealth(entity) > 0.0f;
}

} // namespace

// @func 0x0002B618 RaceTracker::UpdateRanking
void RaceTracker::UpdateRanking(SpawnerRef* spawner)
{
    const int count = SpawnerLiveCount(spawner);
    // One bubble pass per call: racers drift into order over a few frames.
    for (int i = 0; i < count; ++i) {
        Entity* current = SpawnerLive(spawner, i);
        Entity* previous = i > 0 ? SpawnerLive(spawner, i - 1) : nullptr;
        if (!IsRacingEntity(current))
            continue;
        RacerAI* racer = dynamic_cast<RacerAI*>(current->listener);
        if (!racer)
            continue;
        if (track)
            racer->progress = track->FindClosestParam(current->Position(), nullptr, nullptr);
        RacerAI* ahead = (previous && (previous->typeBits[0] & kRacerTypeBit))
                             ? dynamic_cast<RacerAI*>(previous->listener)
                             : nullptr;
        if (ahead && racer->progress < ahead->progress)
            SpawnerSwapLive(spawner, i - 1, i);
    }
    int place = 0;
    for (int i = 0; i < count; ++i) {
        Entity* racerEntity = SpawnerLive(spawner, i);
        if (!IsRacingEntity(racerEntity))
            continue;
        if (RacerAI* racer = RacerOf(racerEntity))
            racer->SetRacePosition(static_cast<float>(place));
        ++place;
    }
}

// @func 0x0002BA28 RacerAI::~RacerAI
// @func 0x0002BAA4 RacerAI::~RacerAI (deleting)
RacerAI::~RacerAI()
{
    // Leave the ranking with a final update.
    SpawnerRef* spawner = entity ? EntitySpawner(entity) : nullptr;
    if (RaceTracker* race = spawner ? SpawnerRaceTracker(spawner) : nullptr)
        race->UpdateRanking(spawner);
}

// @func 0x0002BCB4 RacerAI::Update
void RacerAI::Update(float dt)
{
    // NOTE: the console also skips the update while the HUD element
    // 0x2508CAD1 it looked up on attach (+0x160) has its object's bit 15 of
    // +0xA4 set; the HUD element is not tracked here.
    if (entity->flags & (0x40u | 0x20u))
        return;
    AIController::Update(dt);
    PreUpdate(dt);
}

// @func 0x0002BD64 RacerAI::PreUpdate
void RacerAI::PreUpdate(float dt)
{
    (void)dt;
    // NOTE: the console looks the bone up but does nothing with it.
    const int bone = entity->FindBone(kBone_Effect);
    if (bone >= 0)
        (void)entity->GetBone(bone);
}

// @func 0x0002BDB8 RacerAI::OnMessage
void RacerAI::OnMessage(const entity::EntityMessage& msg)
{
    if (msg.id == kMsg_Kill) {
        SpawnerRef* spawner = EntitySpawner(entity);
        if (RaceTracker* race = spawner ? SpawnerRaceTracker(spawner) : nullptr)
            race->UpdateRanking(spawner);
    }
    AIController::OnMessage(msg);
}

// @func 0x0002BE48 RacerAI::OnDamaged
void RacerAI::OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info)
{
    (void)kind;
    if (amount > 0.0f && entity && (entity->typeBits[0] & 0x2u))
        EntityDamage(entity, amount, info->source, 0);
}

// @func 0x0002BED0 RacerAI::IsCharacterDead
bool RacerAI::IsCharacterDead()
{
    return false;
}

// @func 0x0002BF14 RacerAI::ChooseState
void RacerAI::ChooseState() {}

// @func 0x0002BED8 RacerAI::OnScriptCommand
void RacerAI::OnScriptCommand(uint32_t command, uint32_t arg1, uint32_t arg2, float value)
{
    (void)arg1; (void)arg2; (void)value;
    // Script commands are racer states.
    SetState(static_cast<int>(command));
}

// @func 0x0002BF1C RacerAI::Think
bool RacerAI::Think(float dt)
{
    (void)dt;
    return false;
}

// @func 0x0002E4F8 RacerAI::ExitState
void RacerAI::ExitState() {}

} // namespace ai
} // namespace levels
} // namespace sly
