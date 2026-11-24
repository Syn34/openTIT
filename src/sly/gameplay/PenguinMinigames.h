// openTIT - The penguin minigames ("Getting Stronger" HUD): the per-game
// actor records.
//
// Each minigame keeps a record for its penguin (or other opponent) that owns
// the spawned world entity at +0x04; the only virtual member is the
// destructor, which hands the entity back to the world for removal. The
// rest of each record is plain data filled by the game's set-up code (not
// reconstructed yet), so only the owned entity is modelled.
#pragma once

#include <cstdint>

namespace sly {

class WorldObject;

namespace gameplay {

class MinigameActorRecord {
public:
    MinigameActorRecord() = default;
    virtual ~MinigameActorRecord();
    MinigameActorRecord(const MinigameActorRecord&) = delete;
    MinigameActorRecord& operator=(const MinigameActorRecord&) = delete;

    WorldObject* entity = nullptr;   // +0x04
};

// Penguin of the balance game (vtable 0x00B20840, 0xB0 bytes).
class PenguinBalanceRecord : public MinigameActorRecord {
public:
    ~PenguinBalanceRecord() override;
};

// Penguin of the dodge game (vtable 0x00B208E8, 0xC0 bytes).
class PenguinDodgeRecord : public MinigameActorRecord {
public:
    ~PenguinDodgeRecord() override;
};

// Penguin of the punch game (vtable 0x00B20990, 0xE0 bytes).
class PenguinPunchRecord : public MinigameActorRecord {
public:
    ~PenguinPunchRecord() override;
};

// Penguin of the slingshot game (vtable 0x00B20A38, 0x180 bytes).
class PenguinSlingshotRecord : public MinigameActorRecord {
public:
    ~PenguinSlingshotRecord() override;
};

// Penguin of the sumo game (vtable 0x00B20AE0, 0xB4 bytes); it also owns two
// arrays (+0x8C, +0xA0). NOTE: element types unknown.
class PenguinSumoRecord : public MinigameActorRecord {
public:
    ~PenguinSumoRecord() override;

    void* listA = nullptr;   // +0x8C ApeTArray storage
    void* listB = nullptr;   // +0xA0 ApeTArray storage
};

// Target of the whack-a-penguin game (vtable 0x00B20B88, 0xB0 bytes).
class WhackPenguinRecord : public MinigameActorRecord {
public:
    ~WhackPenguinRecord() override;
};

} // namespace gameplay
} // namespace sly
