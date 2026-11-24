// openTIT - Game event record helpers.
#include "sly/game/GameEvent.h"

#include "sly/game/SlyGame.h"
#include "sly/game/SpawnManager.h"
#include "sly/game/WorldInterface.h"

namespace sly {

// TODO: the world object layout belongs to the world module. Until it is
// available these accessors return "no id", which is what the original
// returns for a null object.
uint32_t WorldObjectGroupId(const WorldObject* object) { (void)object; return 0; }
uint32_t WorldObjectId(const WorldObject* object) { (void)object; return 0; }
uint32_t WorldObjectOwnerId(const WorldObject* object) { (void)object; return 0; }

namespace {

uint32_t CurrentEventTime()
{
    return g_game ? g_game->EventTime() : 0;
}

// Header shared by every event: type, time and the source object.
void BeginEvent(GameEvent& e, uint32_t type, WorldObject* source)
{
    e.type = type;
    e.time = CurrentEventTime();
    e.source = source;
}

// The usual source block: each id defaults to the matching id of 'source'.
void FillSourceIds(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                   uint32_t ownerId)
{
    e.groupId = (groupId == 0 && source) ? WorldObjectGroupId(source) : groupId;
    e.objectId = (objectId == 0 && source) ? WorldObjectId(source) : objectId;
    e.ownerId = (ownerId == 0 && source) ? WorldObjectOwnerId(source) : ownerId;
}

// Variant without a group id.
void FillObjectIds(GameEvent& e, WorldObject* source, uint32_t objectId, uint32_t ownerId)
{
    e.groupId = 0;
    e.objectId = (objectId == 0 && source) ? WorldObjectId(source) : objectId;
    e.ownerId = (ownerId == 0 && source) ? WorldObjectOwnerId(source) : ownerId;
}

void StandardEvent(GameEvent& e, uint32_t type, WorldObject* source, uint32_t groupId,
                   uint32_t objectId, uint32_t ownerId)
{
    BeginEvent(e, type, source);
    FillSourceIds(e, source, groupId, objectId, ownerId);
}

void ClearIds(GameEvent& e)
{
    e.groupId = 0;
    e.objectId = 0;
    e.ownerId = 0;
}

} // namespace

// @func 0x00464ED4 InitEvent01
void InitEvent01(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId, uintptr_t arg0, uintptr_t arg1)
{
    StandardEvent(e, kEvent_01, source, groupId, objectId, ownerId);
    e.arg0 = arg0;
    e.arg1 = arg1;
}

// @func 0x00464F88 InitEvent03
// The source object is also stored as the first argument.
void InitEvent03(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId)
{
    StandardEvent(e, kEvent_03, source, groupId, objectId, ownerId);
    e.arg0 = reinterpret_cast<uintptr_t>(source);
}

// @func 0x0046502C InitEvent04
void InitEvent04(GameEvent& e, WorldObject* source, uint32_t objectId, uint32_t ownerId)
{
    BeginEvent(e, kEvent_04, source);
    FillObjectIds(e, source, objectId, ownerId);
}

// @func 0x004650A8 InitEvent05
void InitEvent05(GameEvent& e, WorldObject* source, uint32_t objectId, uint32_t ownerId)
{
    BeginEvent(e, kEvent_05, source);
    FillObjectIds(e, source, objectId, ownerId);
}

// @func 0x004651A0 InitEvent06
void InitEvent06(GameEvent& e, float value, WorldObject* source)
{
    BeginEvent(e, kEvent_06, source);
    ClearIds(e);
    e.arg0Float = value;
}

// @func 0x004651D8 InitEvent07
void InitEvent07(GameEvent& e, WorldObject* source, uintptr_t arg0)
{
    BeginEvent(e, kEvent_07, source);
    ClearIds(e);
    e.arg0 = arg0;
}

// @func 0x00465218 InitEvent09
void InitEvent09(GameEvent& e, WorldObject* source)
{
    BeginEvent(e, kEvent_09, source);
    ClearIds(e);
}

// @func 0x0046524C InitEvent0A
// Built like event 6 and then retyped.
void InitEvent0A(GameEvent& e, float value, WorldObject* source)
{
    InitEvent06(e, value, source);
    e.type = kEvent_0A;
}

// @func 0x0046528C InitEvent0B
void InitEvent0B(GameEvent& e, WorldObject* source, uint32_t objectId, uint32_t ownerId)
{
    BeginEvent(e, kEvent_0B, source);
    FillObjectIds(e, source, objectId, ownerId);
}

// @func 0x00465308 InitEvent0C
void InitEvent0C(GameEvent& e, WorldObject* source, uint32_t objectId, uint32_t ownerId)
{
    BeginEvent(e, kEvent_0C, source);
    FillObjectIds(e, source, objectId, ownerId);
}

// @func 0x00465384 InitEvent0D
void InitEvent0D(GameEvent& e, uintptr_t arg0)
{
    BeginEvent(e, kEvent_0D, nullptr);
    ClearIds(e);
    e.arg0 = arg0;
}

// @func 0x004653BC InitEvent0E
void InitEvent0E(GameEvent& e, uintptr_t arg0)
{
    BeginEvent(e, kEvent_0E, nullptr);
    ClearIds(e);
    e.arg0 = arg0;
}

// @func 0x004653F4 InitEvent0F
void InitEvent0F(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId)
{
    StandardEvent(e, kEvent_0F, source, groupId, objectId, ownerId);
}

// @func 0x00465494 InitEvent10
void InitEvent10(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId)
{
    StandardEvent(e, kEvent_10, source, groupId, objectId, ownerId);
}

// @func 0x004655D4 InitEvent11
void InitEvent11(GameEvent& e, const Spawner& spawner)
{
    BeginEvent(e, kEvent_11, nullptr);
    ClearIds(e);
    e.objectId = WorldObjectKey(spawner.spawnPoint);
}

// @func 0x00465610 InitEvent12
void InitEvent12(GameEvent& e, const SpawnGroup& group)
{
    BeginEvent(e, kEvent_12, nullptr);
    ClearIds(e);
    e.objectId = WorldObjectKey(group.levelObject);
}

// @func 0x0046564C InitEvent13
void InitEvent13(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId)
{
    StandardEvent(e, kEvent_13, source, groupId, objectId, ownerId);
}

// @func 0x004656EC InitEvent14
void InitEvent14(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId)
{
    StandardEvent(e, kEvent_14, source, groupId, objectId, ownerId);
}

// @func 0x0046578C InitEvent15
void InitEvent15(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId)
{
    StandardEvent(e, kEvent_15, source, groupId, objectId, ownerId);
}

// @func 0x0046582C InitEvent16
void InitEvent16(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId)
{
    StandardEvent(e, kEvent_16, source, groupId, objectId, ownerId);
}

// @func 0x004658CC InitEvent17
void InitEvent17(GameEvent& e, WorldObject* source, uintptr_t arg0, uint32_t groupId,
                 uint32_t objectId, uint32_t ownerId)
{
    StandardEvent(e, kEvent_17, source, groupId, objectId, ownerId);
    e.arg0 = arg0;
}

// @func 0x00465970 InitEvent18
void InitEvent18(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId)
{
    StandardEvent(e, kEvent_18, source, groupId, objectId, ownerId);
}

// @func 0x00465A10 InitEvent19
void InitEvent19(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId)
{
    StandardEvent(e, kEvent_19, source, groupId, objectId, ownerId);
}

// @func 0x00465534 InitEvent1B
void InitEvent1B(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId)
{
    StandardEvent(e, kEvent_1B, source, groupId, objectId, ownerId);
}

// @func 0x00464EA0 InitEvent1C
// No source object; the owner id is left untouched by the original.
void InitEvent1C(GameEvent& e, uintptr_t arg0, uint32_t groupId, uint32_t objectId)
{
    BeginEvent(e, kEvent_1C, nullptr);
    e.groupId = groupId;
    e.objectId = objectId;
    e.arg0 = arg0;
}

// @func 0x00465AB0 InitEvent1D
void InitEvent1D(GameEvent& e, WorldObject* source)
{
    StandardEvent(e, kEvent_1D, source, 0, 0, 0);
}

// @func 0x00465B30 InitEvent1E
void InitEvent1E(GameEvent& e, WorldObject* source)
{
    StandardEvent(e, kEvent_1E, source, 0, 0, 0);
}

// @func 0x00465124 InitEvent1F
void InitEvent1F(GameEvent& e, WorldObject* source, uint32_t objectId, uint32_t ownerId)
{
    BeginEvent(e, kEvent_1F, source);
    FillObjectIds(e, source, objectId, ownerId);
}

// @func 0x00465BB0 InitEvent20
void InitEvent20(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId)
{
    StandardEvent(e, kEvent_20, source, groupId, objectId, ownerId);
}

} // namespace sly
