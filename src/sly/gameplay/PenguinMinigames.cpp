// openTIT - The penguin minigames' actor records (see PenguinMinigames.h).
#include "sly/gameplay/PenguinMinigames.h"

#include "sly/gameplay/GameplayWorld.h"

namespace sly {
namespace gameplay {

// Shared by every record (inlined into each destructor on the console).
MinigameActorRecord::~MinigameActorRecord()
{
    if (entity != nullptr && EntityIsInWorld(entity))
        EntityMarkForRemoval(entity);
    entity = nullptr;
}

// @func 0x003F6848 PenguinBalanceRecord::~PenguinBalanceRecord
// @func 0x003F6888 PenguinBalanceRecord::~PenguinBalanceRecord (deleting)
PenguinBalanceRecord::~PenguinBalanceRecord() = default;

// @func 0x003FF858 PenguinDodgeRecord::~PenguinDodgeRecord
// @func 0x003FF898 PenguinDodgeRecord::~PenguinDodgeRecord (deleting)
PenguinDodgeRecord::~PenguinDodgeRecord() = default;

// @func 0x0040549C PenguinPunchRecord::~PenguinPunchRecord
// @func 0x004054DC PenguinPunchRecord::~PenguinPunchRecord (deleting)
PenguinPunchRecord::~PenguinPunchRecord() = default;

// @func 0x0040ACD4 PenguinSlingshotRecord::~PenguinSlingshotRecord
// @func 0x0040AD14 PenguinSlingshotRecord::~PenguinSlingshotRecord (deleting)
PenguinSlingshotRecord::~PenguinSlingshotRecord() = default;

// @func 0x0041310C PenguinSumoRecord::~PenguinSumoRecord
// @func 0x004131BC PenguinSumoRecord::~PenguinSumoRecord (deleting)
PenguinSumoRecord::~PenguinSumoRecord()
{
    // NOTE: the arrays' storage is released by their own ApeTArray members on
    // the console; their element type is unknown, so they stay opaque here.
    listB = nullptr;
    listA = nullptr;
}

// @func 0x0041A35C WhackPenguinRecord::~WhackPenguinRecord
// @func 0x0041A39C WhackPenguinRecord::~WhackPenguinRecord (deleting)
WhackPenguinRecord::~WhackPenguinRecord() = default;

} // namespace gameplay
} // namespace sly
