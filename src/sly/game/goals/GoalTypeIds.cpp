// openTIT - Goal class type ids (vtable slot 3), out-of-line destructors
// and the penguin goal base.
//
// Every goal class answers a small constant identifying its type (the job
// manager and the HUD switch on it). The two unnamed goal classes with ids
// 0x22 (vtable 0x00B1EDE8) and 0x23 (vtable 0x00B1EFD0) are not ported.
#include "sly/game/goals/GoalTypes.h"

namespace sly {
namespace goals {

// @func 0x00769B3C GoalGroup::TypeId
int GoalGroup::TypeId() const { return 1; }
// @func 0x0076B364 GoalBartenderGame::TypeId
int GoalBartenderGame::TypeId() const { return 2; }
// @func 0x0076B36C GoalBentleyVsPenelope::TypeId
int GoalBentleyVsPenelope::TypeId() const { return 3; }
// @func 0x0076B408 GoalBinocucomPhoto::TypeId
int GoalBinocucomPhoto::TypeId() const { return 4; }
// @func 0x0076B410 GoalBinocucomPower::TypeId
int GoalBinocucomPower::TypeId() const { return 5; }
// @func 0x0076B418 GoalBinocucomRecord::TypeId
int GoalBinocucomRecord::TypeId() const { return 6; }
// @func 0x0076B428 GoalCostume::TypeId
int GoalCostume::TypeId() const { return 7; }
// @func 0x0076B430 GoalEntityChild::TypeId
int GoalEntityChild::TypeId() const { return 9; }
// @func 0x0076B438 GoalInteraction::TypeId
int GoalInteraction::TypeId() const { return 10; }
// @func 0x0076B440 GoalFencing::TypeId
int GoalFencing::TypeId() const { return 0x0B; }
// @func 0x0076B448 GoalFishingGame::TypeId
int GoalFishingGame::TypeId() const { return 0x0C; }
// @func 0x0076B450 GoalFollow::TypeId
int GoalFollow::TypeId() const { return 0x0D; }
// @func 0x0076BA08 GoalHealthMonitor::TypeId
int GoalHealthMonitor::TypeId() const { return 0x0E; }
// @func 0x0076BDFC GoalInput::TypeId
int GoalInput::TypeId() const { return 0x10; }
// @func 0x0076BA10 GoalMarkEntity::TypeId
int GoalMarkEntity::TypeId() const { return 0x11; }
// @func 0x0076B420 GoalCameraEntity::TypeId
int GoalCameraEntity::TypeId() const { return 0x12; }
// Shared by the penguin goals' base (vtable 0x00B1E4D0).
// @func 0x0076B35C MinigameGoal::TypeId
int MinigameGoal::TypeId() const { return 0x13; }
// @func 0x0076BAB8 GoalQteGame::TypeId
int GoalQteGame::TypeId() const { return 0x14; }
// @func 0x0076BAC8 GoalScore::TypeId
int GoalScore::TypeId() const { return 0x15; }
// @func 0x0076BDF4 GoalTimer::TypeId
int GoalTimer::TypeId() const { return 0x18; }
// @func 0x0076BE04 GoalWait::TypeId
int GoalWait::TypeId() const { return 0x19; }
// @func 0x0076B400 GoalPenguinBalanceGame::TypeId
int GoalPenguinBalanceGame::TypeId() const { return 0x1A; }
// @func 0x0076BA20 GoalPenguinDodgeGame::TypeId
int GoalPenguinDodgeGame::TypeId() const { return 0x1B; }
// @func 0x0076BA28 GoalPenguinPunchGame::TypeId
int GoalPenguinPunchGame::TypeId() const { return 0x1C; }
// @func 0x0076BA30 GoalPenguinSlingshotGame::TypeId
int GoalPenguinSlingshotGame::TypeId() const { return 0x1D; }
// @func 0x0076BA38 GoalPenguinSumoGame::TypeId
int GoalPenguinSumoGame::TypeId() const { return 0x1E; }
// @func 0x0076BA48 GoalPenguinWhackGame::TypeId
int GoalPenguinWhackGame::TypeId() const { return 0x1F; }
// @func 0x0076BA50 GoalPenguinMontageGame::TypeId
int GoalPenguinMontageGame::TypeId() const { return 0x20; }
// @func 0x0076BA18 GoalOperationMousetrap::TypeId
int GoalOperationMousetrap::TypeId() const { return 0x21; }

// A group holds back the next goal's start scripts while it has children.
// @func 0x00769B44 GoalGroup::BlocksNextScripts
bool GoalGroup::BlocksNextScripts() const { return firstChild != nullptr; }

// @func 0x00769B58 GoalSet::~GoalSet
// @func 0x00769B84 GoalSet::~GoalSet (deleting)
GoalSet::~GoalSet() = default;

// @func 0x0076B458 GoalHealthMonitor::~GoalHealthMonitor
// @func 0x0076B724 GoalHealthMonitor::~GoalHealthMonitor (deleting)
GoalHealthMonitor::~GoalHealthMonitor() = default;

// @func 0x0076B050 GoalScore::~GoalScore
// @func 0x0076BAD0 GoalScore::~GoalScore (deleting)
GoalScore::~GoalScore() = default;

// ---------------------------------------------------------------------------
// PenguinGameGoal

// @func 0x0076B374 PenguinGameGoal::~PenguinGameGoal
// @func 0x0076B3A0 PenguinGameGoal::~PenguinGameGoal (deleting)
PenguinGameGoal::~PenguinGameGoal() = default;

// @func 0x0076B3F4 PenguinGameGoal::BuildMinigame
gameplay::Minigame* PenguinGameGoal::BuildMinigame() { return nullptr; }

// @func 0x0076B3FC PenguinGameGoal::PenguinHook
void PenguinGameGoal::PenguinHook() {}

} // namespace goals
} // namespace sly
