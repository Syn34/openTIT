// openTIT - The entity module's per-frame jobs.
//
// Each frame the game sorts the global entity list (Entity.cpp, list 0) with
// four selection jobs. On the console they are SPU programs started through
// 0x00456918 / 0x00456A60 / 0x00456BA8 / 0x00456CF0: the job reads the entity
// list, writes the indices or pointers of the entities it selects to an output
// list and clears a busy word when done; the game thread spins on the busy
// word before it walks the output. The port runs the selection on the calling
// thread (EntityJobSelect* in GameEntityHooks), so the waits disappear.
//
//   * suspension: entities to suspend / resume by distance (0x000CFBD0,
//     results applied by 0x000CFF4C);
//   * deletion: dead entities to destroy (0x000CF97C, 0x000CFA24);
//   * update: entities to run the update step on (0x000CE5D4, 0x000CF398);
//   * post-update: entities to run the post-update step on (0x000CF480,
//     0x000CF898).
#include "sly/entity/Entity.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/EntityTuning.h"
#include "sly/entity/GameEntityHooks.h"

#include <vector>

namespace sly {
namespace entity {

void FlushDeferredHits();
void ResetEntityLoadScopes();
void EntityUpdateStep(Entity* entity, float dt);
void EntityPostUpdateStep(Entity* entity, float dt);

namespace {

// Distance beyond which entities are suspended (0x00F64734; negative: the
// default, Entity.CullDistDefault).
// NOTE: the console default comes from a tuning value at 0x01021C84,
// assumed to be Entity.CullDistDefault.
float g_suspendDistance = -1.0f;

// Output lists of the jobs (0x01015BC0/0x01017BD0 index lists for the
// suspension job, 0x01013BB0 for deletion, 0x01019BE0 / 0x0101DBF0 entity
// lists for the update passes).
std::vector<uint16_t> g_toSuspend;
std::vector<uint16_t> g_toResume;
std::vector<uint16_t> g_toDelete;
std::vector<Entity*> g_toUpdate;
std::vector<Entity*> g_toPostUpdate;

} // namespace

void SetEntitySuspendDistance(float distance) { g_suspendDistance = distance; }

// @func 0x000CE0A8 EntityModuleLevelStart
// Level start: empty entity list, fresh load scopes, the default suspension
// distance.
bool EntityModuleLevelStart()
{
    while (EntityCount(kList_All) != 0)
        RemoveEntityAt(kList_All, EntityCount(kList_All) - 1);
    ResetEntityLoadScopes();
    g_suspendDistance = -1.0f;
    EntityModuleLevelStartHooks();
    return true;
}

// @func 0x000CFBD0 StartEntitySuspendJob
// The job compares every entity with the camera view and the local player
// (the camera when there is no player) against the suspension distance.
void StartEntitySuspendJob()
{
    if (EntityCount(kList_All) == 0)
        return;
    EntitySuspendJobParams params;
    params.camera = GameCameraTransform();
    Entity* player = PlayerEntity(0);
    params.player = player ? player->transform : params.camera;
    const float distance = g_suspendDistance < 0.0f ? g_entityCullDistDefault.Get() : g_suspendDistance;
    params.distanceSq = distance * distance;
    EntityJobSelectSuspended(params, g_toSuspend, g_toResume);
}

// @func 0x000CFF4C ApplyEntitySuspendJob
void ApplyEntitySuspendJob()
{
    for (uint16_t index : g_toSuspend) {
        Entity* entity = EntityAt(kList_All, index);
        if (entity && !(entity->flags & kEnt_Suspended)) {
            entity->flags |= kEnt_Suspended;
            entity->OnSuspend();
        }
    }
    g_toSuspend.clear();
    for (uint16_t index : g_toResume) {
        Entity* entity = EntityAt(kList_All, index);
        if (entity && (entity->flags & kEnt_Suspended)) {
            entity->flags &= ~kEnt_Suspended;
            entity->OnResume();
        }
    }
    g_toResume.clear();
}

// @func 0x000CF97C StartEntityDeleteJob
void StartEntityDeleteJob()
{
    if (EntityCount(kList_All) != 0)
        EntityJobSelectDead(g_toDelete);
}

// @func 0x000CFA24 FinishEntityDeleteJob
// Destroys the selected entities, last index first, swap-removing each
// from the list. An entity whose reference count went negative gets its
// slot 19 call first. NOTE: the role of that call is not known.
void FinishEntityDeleteJob()
{
    for (size_t i = g_toDelete.size(); i-- > 0 && EntityCount(kList_All) != 0;) {
        const uint16_t index = g_toDelete[i];
        Entity* entity = EntityAt(kList_All, index);
        if (entity->interactionCount < 0)
            entity->GetDebugName();
        entity->interactionCount = 0;
        delete entity;
        RemoveEntityAt(kList_All, index);
    }
    g_toDelete.clear();
}

// @func 0x000CE5D4 StartEntityUpdateJob
void StartEntityUpdateJob()
{
    g_toUpdate.clear();
    if (EntityCount(kList_All) != 0)
        EntityJobSelectForUpdate(g_toUpdate);
}

// @func 0x000CF398 RunEntityUpdates
// Deferred hits and trigger reports first, then the update step of every
// selected entity.
void RunEntityUpdates(float dt)
{
    FlushDeferredHits();
    FlushTriggerReports();
    for (Entity* entity : g_toUpdate)
        EntityUpdateStep(entity, dt);
    g_toUpdate.clear();
}

// @func 0x000CF480 StartEntityPostUpdateJob
void StartEntityPostUpdateJob()
{
    g_toPostUpdate.clear();
    if (EntityCount(kList_All) != 0)
        EntityJobSelectForPostUpdate(g_toPostUpdate);
}

// @func 0x000CF898 RunEntityPostUpdates
void RunEntityPostUpdates(float dt)
{
    for (Entity* entity : g_toPostUpdate)
        EntityPostUpdateStep(entity, dt);
    g_toPostUpdate.clear();
    EntityPostUpdateDone();
}

} // namespace entity
} // namespace sly
