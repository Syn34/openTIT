// openTIT - Upgrade shop tuning and the table of upgrade records.
#include "sly/game/Upgrades.h"

#include <cstring>

namespace sly {

// @func 0x0049E78C StaticInit_UpgradeTuning
// @func 0x0049E738 StaticDestroy_UpgradeTuning
// Compiler-generated initialiser/destructor pair for the globals of this file
// (the two tuning values and the zero-initialised upgrade table).
ape::TuningInt g_upgradePurchaseFirstAwardNum("Game.Upgrades.PurchaseFirstAwardNum", 0x11);
ape::TuningInt g_upgradePurchaseAllAwardNum("Game.Upgrades.PurchaseAllAwardNum", 0x2A);

static UpgradeEntry s_upgrades[kUpgradeCount];   // 0x010BE8BC

// @func 0x0049E704 GetUpgradeEntry
UpgradeEntry* GetUpgradeEntry(uint32_t index)
{
    return index < kUpgradeCount ? &s_upgrades[index] : nullptr;
}

// @func 0x0049E860 TextField::Clear
void TextField::Clear()
{
    Reset(0);
}

// @func 0x0049E8A8 TextField::Reset
void TextField::Reset(uint32_t newId)
{
    id = newId;
    flagA = false;
    flagB = false;
    std::memset(text, 0, sizeof(text));
}

} // namespace sly
