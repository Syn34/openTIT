// openTIT - The AI manager's frame: which controllers update, where the
// player's allies stand and whom they fight, and the shared timers.
#include "sly/levels/ai/AIController.h"

#include "agl/AglRenderTarget.h"
#include "sly/camera/CameraHooks.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/knowledge/Knowledge.h"
#include "sly/levels/ai/AIBehaviours.h"
#include "sly/levels/ai/AIHooks.h"
#include "sly/levels/nav/NavGraph.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace sly {
namespace levels {
namespace ai {

namespace {

constexpr int kSurroundSlots = 7;
constexpr int kMaxCandidates = 9;
constexpr float kNoDistance = 999999.0f;
constexpr float kFarAway = 415624608.0f;   // squared distance no partner is beyond

// Places around the player in the player's frame: behind, then pairs to
// either side.
const ape::Vec3 kSurroundOffsets[kSurroundSlots] = {
    {0.0f, 0.0f, 0.0f, 0.0f},  {2.0f, 0.0f, 0.0f, 0.0f},  {-2.0f, 0.0f, 0.0f, 0.0f}, {4.0f, 0.0f, 2.0f, 0.0f},
    {-4.0f, 0.0f, 2.0f, 0.0f}, {6.0f, 0.0f, 0.0f, 0.0f},  {-6.0f, 0.0f, 0.0f, 0.0f},
};

float DistSq(const ape::Vec3& a, const ape::Vec3& b)
{
    const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

ape::Vec3 TransformPoint(const ape::Matrix34& m, const ape::Vec3& p)
{
    return {m.xAxis.x * p.x + m.yAxis.x * p.y + m.zAxis.x * p.z + m.translation.x,
            m.xAxis.y * p.x + m.yAxis.y * p.y + m.zAxis.y * p.z + m.translation.y,
            m.xAxis.z * p.x + m.yAxis.z * p.y + m.zAxis.z * p.z + m.translation.z, 0.0f};
}

// The point is on the inner side of all eight plane slots of the view
// frustum, allowing `margin` (the plane groups are stored transposed, four
// planes per group).
bool InsideFrustum(const agl::AglFrustum& frustum, const ape::Vec3& p, float margin)
{
    for (int group = 0; group < 2; ++group) {
        const ape::Vec4* soa = &frustum.soa[group * 4];
        const float dist[4] = {
            soa[0].x * p.x + soa[3].x + soa[1].x * p.y + soa[2].x * p.z,
            soa[0].y * p.x + soa[3].y + soa[1].y * p.y + soa[2].y * p.z,
            soa[0].z * p.x + soa[3].z + soa[1].z * p.y + soa[2].z * p.z,
            soa[0].w * p.x + soa[3].w + soa[1].w * p.y + soa[2].w * p.z,
        };
        for (float d : dist) {
            if (!(margin + d >= 0.0f))
                return false;
        }
    }
    return true;
}

} // namespace

// @func 0x000182C0 AIManager::UpdateRecord50
// The controller snaps the record's spot onto the nav mesh. The point is
// good when it lies near the spot (within half the radius, at least three
// units, both vertically and across), its target can be seen from it (from
// the height of the controller's entity centre) and it is inside the main
// view; good points are remembered and add to the good time.
void AIManager::UpdateRecord50(Record50& record, float dt)
{
    AIController* controller = record.controller;
    if (!controller || !controller->target || !(controller->target->flags & entity::kEnt_Alive))
        return;

    const float radius = record.radius;
    const float tolerance = std::max(radius * 0.5f, 3.0f);
    const ape::Vec3 spot = {lastPlayerPosition.x + record.direction.x * radius,
                            lastPlayerPosition.y + record.direction.y * radius,
                            lastPlayerPosition.z + record.direction.z * radius, 0.0f};
    const ape::Vec3 point = controller->NavSnapPoint(spot);

    if (std::fabs(point.y - spot.y) < tolerance) {
        const float dx = point.x - spot.x;
        const float dz = point.z - spot.z;
        record.inRange = dx * dx + 0.0f + dz * dz < tolerance * tolerance;
        if (record.inRange) {
            bool visible = false;
            if (Entity* target = controller->target) {
                const ape::Vec3 center = controller->GetEntity()->GetCenter();
                const ape::Vec3& at = controller->Position();
                const ape::Vec3 eye = {point.x + (center.x - at.x), point.y + (center.y - at.y),
                                       point.z + (center.z - at.z), 0.0f};
                const ape::Vec3 targetCenter = target->GetCenter();
                visible = controller->TestLineOfSight(eye, targetCenter, target);
            }
            record.visible = visible;
        }
    } else {
        record.inRange = false;
    }

    float time = record.goodTime - dt;
    if (record.inRange) {
        if (record.visible) {
            // NOTE: the console reads the main render target without a null
            // check; without one the point counts as off screen.
            const agl::AglRenderTarget* view = camera::MainRenderTarget();
            record.onScreen = view && InsideFrustum(view->frustum, point, 1.0f);
        }
        if (record.visible && record.onScreen) {
            record.lastGoodPoint = point;
            time = record.goodTime + dt;
        }
    }
    record.goodTime = std::max(std::min(time, 2.0f), 0.0f);
}

// Part of 0x00019AF0. Two greedy matchings by smallest squared distance: the
// allies against the surround places (fewer places for fewer allies), then
// the allies against the engaged controllers that allow interaction (at most
// nine; an ally with a positive engage radius only takes targets within it
// of its start position).
void AIManager::AssignAllies(Entity* player)
{
    int slotCount = kSurroundSlots;
    if (allyCount < 3)
        slotCount = 3;
    else if (allyCount < 5)
        slotCount = 5;

    const ape::Matrix34 frame = player->transform;
    AIController* candidates[kMaxCandidates] = {};
    int candidateCount = 0;
    for (int i = 0; i < updateCount && candidateCount < kMaxCandidates; ++i) {
        AIController* controller = updating[i];
        if (controller->IsEngaged() && controller->AllowsInteraction())
            candidates[candidateCount++] = controller;
    }

    ape::Vec3 slotPoints[kSurroundSlots];
    for (int s = 0; s < kSurroundSlots; ++s)
        slotPoints[s] = TransformPoint(frame, kSurroundOffsets[s]);

    // Squared distances of every ally to every place and candidate.
    PlayerAI* buddies[kMaxAllies] = {};
    float toSlot[kMaxAllies][kMaxCandidates] = {};
    float toCandidate[kMaxAllies][kMaxCandidates] = {};
    for (int a = 0; a < allyCount; ++a) {
        buddies[a] = dynamic_cast<PlayerAI*>(allies[a]->listener);
        if (buddies[a])
            buddies[a]->target = nullptr;
        const ape::Vec3& position = allies[a]->Position();
        for (int s = 0; s < slotCount; ++s) {
            toSlot[a][s] = DistSq(slotPoints[s], position);
            if (s < candidateCount)
                toCandidate[a][s] = DistSq(candidates[s]->Position(), position);
        }
    }

    std::array<bool, kMaxCandidates> allyUsed{};
    std::array<bool, kSurroundSlots> slotUsed{};
    for (int pass = 0; pass < allyCount; ++pass) {
        float best = kNoDistance;
        int bestAlly = 0, bestSlot = 0;
        for (int a = 0; a < allyCount; ++a) {
            if (allyUsed[a])
                continue;
            for (int s = 0; s < slotCount; ++s) {
                if (!slotUsed[s] && toSlot[a][s] < best) {
                    best = toSlot[a][s];
                    bestAlly = a;
                    bestSlot = s;
                }
            }
        }
        if (buddies[bestAlly])
            buddies[bestAlly]->surroundOffset = kSurroundOffsets[bestSlot];
        allyUsed[bestAlly] = true;
        slotUsed[bestSlot] = true;
    }

    if (candidateCount <= 0 || allyCount <= 0)
        return;
    allyUsed.fill(false);
    std::array<bool, kMaxCandidates> candidateUsed{};
    for (int pass = 0; pass < allyCount; ++pass) {
        float best = kNoDistance;
        int bestAlly = 0, bestCandidate = 0;
        for (int a = 0; a < allyCount; ++a) {
            if (allyUsed[a] || candidateCount <= 0)
                continue;
            for (int c = 0; c < candidateCount; ++c) {
                if (candidateUsed[c] || !(toCandidate[a][c] < best))
                    continue;
                const PlayerAI* buddy = buddies[a];
                const float radius = buddy ? buddy->field_154 : 0.0f;
                if (radius > 0.0f &&
                    DistSq(candidates[c]->GetEntity()->Position(), buddy->startPosition) > radius * radius)
                    continue;
                best = toCandidate[a][c];
                bestAlly = a;
                bestCandidate = c;
            }
        }
        if (buddies[bestAlly])
            buddies[bestAlly]->target = candidates[bestCandidate]->GetEntity();
        allyUsed[bestAlly] = true;
        candidateUsed[bestCandidate] = true;
    }
}

// Part of 0x00019AF0: each updated controller remembers its nearest updated
// neighbour; the neighbour adopts it back when it has none or a farther one.
void AIManager::PairPartners()
{
    for (int i = 0; i < updateCount; ++i) {
        AIController* controller = updating[i];
        AIController* nearest = controller->partner;
        float nearestDist = nearest ? controller->partnerDistSq : kFarAway;
        for (int j = i + 1; j < updateCount; ++j) {
            const float d = DistSq(controller->Position(), updating[j]->Position());
            if (nearestDist > d) {
                nearestDist = d;
                nearest = updating[j];
            }
        }
        if (!nearest)
            continue;
        controller->partner = nearest;
        controller->partnerDistSq = nearestDist;
        if (!nearest->partner || nearest->partnerDistSq > nearestDist) {
            nearest->partner = controller;
            nearest->partnerDistSq = nearestDist;
        }
    }
}

// @func 0x00019AF0 AIManager::Tick
void AIManager::Tick(float dt)
{
    knowledge::KnowledgeUpdate();

    // A new render zoom stretches the controlled objects' render distances.
    if (RenderZoomActive()) {
        const float zoom = RenderZoom();
        if (field_40 != zoom) {
            field_40 = zoom;
            for (int i = 0; i < controllerCount; ++i)
                EntityApplyRenderZoom(controllers[i]->GetEntity(), zoom);
        }
    }

    Entity* player = entity::PlayerEntity(0);
    field_44 -= dt;
    field_48 -= dt;
    targetingPause -= dt;
    // No new targets for a second while the player character is in state 0/10.
    if (player && player->HasTypeBit(3) && entity::EntityGetState(player, 0) == 0 &&
        entity::EntityGetState(player, 1) == 10)
        targetingPause = 1.0f;

    // Controllers of living, awake entities update this frame.
    updateCount = 0;
    for (int i = 0; i < controllerCount; ++i) {
        AIController* controller = controllers[i];
        const Entity* e = controller->GetEntity();
        if (e->IsAlive() && !(e->flags & entity::kEnt_Suspended)) {
            controller->partner = nullptr;
            updating[updateCount++] = controller;
        }
    }

    for (Record50& record : records50)
        UpdateRecord50(record, dt);

    if (player)
        AssignAllies(player);
    if (updateCount > 0)
        PairPartners();

    // A few controllers each frame, in turn: up to five, a fifth of them
    // beyond twenty-five.
    int budget = updateCount;
    if (updateCount >= 5)
        budget = updateCount > 25 ? updateCount / 5 : 5;
    for (int n = 0; n < budget; ++n) {
        thinkCursor = (thinkCursor + 1) % updateCount;
        AIController* controller = updating[thinkCursor];
        if (controller->flags0 & AIController::kF0_NoApply)
            controller->ApplyTransform(dt);
    }

    // The alert flag.
    bool keep = false;
    for (int i = 0; i < controllerCount && !keep; ++i) {
        AIController* controller = controllers[i];
        if (!controller->IsListedActive() || controller->IsCharacterDead())
            continue;
        if (controller->IsEngaged()) {
            alert = true;
            keep = true;
        } else if (alert && controller->IsCalm()) {
            keep = true;
        }
    }
    if (!keep)
        alert = false;

    timeSinceSeen += dt;
    if (timeSinceSeen < 1.0f && player)
        lastPlayerPosition = player->GetCenter();

    UpdateAttacks();
    if (NavManager* nav = GetNavManager()) {
        Entity* tracked = entity::PlayerEntity(0);
        UpdatePlayerNavTracker(nav->playerTracker, tracked && tracked->IsAlive() ? &tracked->Position() : nullptr,
                               dt);
    }
}

} // namespace ai
} // namespace levels
} // namespace sly
