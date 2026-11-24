// openTIT - Game events: small fixed-size records describing something that
// happened to a world object. They are filled by the helpers below and
// posted to the game world, which queues them per player.
#pragma once

#include <cstdint>

namespace sly {

class WorldObject;
class Spawner;
class SpawnGroup;

// NOTE: the event type numbers are known, their meaning mostly is not. The
// helpers are named after the type number until the consumers are decoded.
enum GameEventType : uint32_t {
    kEvent_01 = 0x01,
    kEvent_03 = 0x03,
    kEvent_04 = 0x04,
    kEvent_05 = 0x05,
    kEvent_06 = 0x06,
    kEvent_07 = 0x07,
    kEvent_09 = 0x09,
    kEvent_0A = 0x0A,
    kEvent_0B = 0x0B,
    kEvent_0C = 0x0C,
    kEvent_0D = 0x0D,
    kEvent_0E = 0x0E,
    kEvent_0F = 0x0F,
    kEvent_10 = 0x10,
    kEvent_11 = 0x11,
    kEvent_12 = 0x12,
    kEvent_13 = 0x13,
    kEvent_14 = 0x14,
    kEvent_15 = 0x15,
    kEvent_16 = 0x16,
    kEvent_17 = 0x17,
    kEvent_18 = 0x18,
    kEvent_19 = 0x19,
    kEvent_1B = 0x1B,
    kEvent_1C = 0x1C,
    kEvent_1D = 0x1D,
    kEvent_1E = 0x1E,
    kEvent_1F = 0x1F,
    kEvent_20 = 0x20,
};

// 32-byte event record. The three ids default to values taken from the
// source object when the caller passes 0.
struct GameEvent {
    uint32_t type = 0;              // +0x00 GameEventType
    uint32_t time = 0;              // +0x04 SlyGame::EventTime() when filled
    uint32_t groupId = 0;           // +0x08 source->field_D8->field_34
    uint32_t objectId = 0;          // +0x0C source->field_A8
    uint32_t ownerId = 0;           // +0x10 source->field_150->field_A8
    WorldObject* source = nullptr;  // +0x14
    union {
        uintptr_t arg0 = 0;         // +0x18 event specific
        float arg0Float;            // +0x18 events 6 and 10
    };
    uintptr_t arg1 = 0;             // +0x1C event specific
};

// Ids of a world object used to default the event fields. Implemented by the
// world module; stubbed in GameEvent.cpp until it lands.
uint32_t WorldObjectGroupId(const WorldObject* object);
uint32_t WorldObjectId(const WorldObject* object);
uint32_t WorldObjectOwnerId(const WorldObject* object);

void InitEvent01(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId, uintptr_t arg0, uintptr_t arg1);
void InitEvent03(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId);
void InitEvent04(GameEvent& e, WorldObject* source, uint32_t objectId, uint32_t ownerId);
void InitEvent05(GameEvent& e, WorldObject* source, uint32_t objectId, uint32_t ownerId);
void InitEvent06(GameEvent& e, float value, WorldObject* source);
void InitEvent07(GameEvent& e, WorldObject* source, uintptr_t arg0);
void InitEvent09(GameEvent& e, WorldObject* source);
void InitEvent0A(GameEvent& e, float value, WorldObject* source);
void InitEvent0B(GameEvent& e, WorldObject* source, uint32_t objectId, uint32_t ownerId);
void InitEvent0C(GameEvent& e, WorldObject* source, uint32_t objectId, uint32_t ownerId);
void InitEvent0D(GameEvent& e, uintptr_t arg0);
void InitEvent0E(GameEvent& e, uintptr_t arg0);
void InitEvent0F(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId);
void InitEvent10(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId);
// Spawner exhausted: object id = key of the spawner's spawn point.
void InitEvent11(GameEvent& e, const Spawner& spawner);
// Spawn group exhausted: object id = key of the group's level object.
void InitEvent12(GameEvent& e, const SpawnGroup& group);
void InitEvent13(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId);
void InitEvent14(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId);
void InitEvent15(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId);
void InitEvent16(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId);
void InitEvent17(GameEvent& e, WorldObject* source, uintptr_t arg0, uint32_t groupId,
                 uint32_t objectId, uint32_t ownerId);
void InitEvent18(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId);
void InitEvent19(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId);
void InitEvent1B(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId);
void InitEvent1C(GameEvent& e, uintptr_t arg0, uint32_t groupId, uint32_t objectId);
void InitEvent1D(GameEvent& e, WorldObject* source);
void InitEvent1E(GameEvent& e, WorldObject* source);
void InitEvent1F(GameEvent& e, WorldObject* source, uint32_t objectId, uint32_t ownerId);
void InitEvent20(GameEvent& e, WorldObject* source, uint32_t groupId, uint32_t objectId,
                 uint32_t ownerId);

} // namespace sly
