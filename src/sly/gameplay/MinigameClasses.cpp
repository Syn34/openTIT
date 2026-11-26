// openTIT - Minigame destructors and small overrides (see MinigameClasses.h).
#include "sly/gameplay/MinigameClasses.h"

#include "sly/gameplay/GameplayWorld.h"

namespace sly {
namespace gameplay {

namespace {
// Game value 0x010854AC that MinigameB20630's slot 16 tests. NOTE: what
// writes it is not ported; it stays zero here.
float g_value10854AC = 0.0f;

// A spawned entity the minigame owns leaves the world with it.
void ReleaseSpawned(WorldObject*& entity)
{
    if (entity != nullptr && EntityIsInWorld(entity))
        EntityMarkForRemoval(entity);
    entity = nullptr;
}
} // namespace

// @func 0x0077D934 BartenderMinigame::~BartenderMinigame
// @func 0x0077DB10 BartenderMinigame::~BartenderMinigame (deleting)
// @func 0x0077E6BC BartenderMinigame::~BartenderMinigame (thunk, +0x38)
// @func 0x0077E100 BartenderMinigame::~BartenderMinigame (deleting thunk, +0x38)
// @func 0x0077E4DC BartenderMinigame::~BartenderMinigame (thunk, +0x3C)
// @func 0x0077DF04 BartenderMinigame::~BartenderMinigame (deleting thunk, +0x3C)
// @func 0x0077E2FC BartenderMinigame::~BartenderMinigame (thunk, +0x40)
// @func 0x0077DD08 BartenderMinigame::~BartenderMinigame (deleting thunk, +0x40)
BartenderMinigame::~BartenderMinigame() = default;

// @func 0x0077AF08 FishingMinigame::~FishingMinigame
// @func 0x0077E8AC FishingMinigame::~FishingMinigame (deleting)
FishingMinigame::~FishingMinigame() = default;

// @func 0x0077B194 MinigameB20488::~MinigameB20488
// @func 0x0077E92C MinigameB20488::~MinigameB20488 (deleting)
MinigameB20488::~MinigameB20488() = default;

// @func 0x0077E9AC MinigameB204E8::Slot22
bool MinigameB204E8::Slot22() { return false; }
// @func 0x0077E9B4 MinigameB204E8::Slot23
bool MinigameB204E8::Slot23() { return false; }
// @func 0x0077E9BC MinigameB204E8::Slot24
bool MinigameB204E8::Slot24() { return false; }

// @func 0x0077B384 QteMinigame::~QteMinigame
// @func 0x0077E9C4 QteMinigame::~QteMinigame (deleting)
QteMinigame::~QteMinigame() = default;

MinigameListener::~MinigameListener()
{
    if (joined)
        ListenerLeaveAllGroups(this);
}

// @func 0x0077B514 FencingMinigame::~FencingMinigame
// @func 0x0077EBA4 FencingMinigame::~FencingMinigame (deleting)
// @func 0x0077EC98 FencingMinigame::~FencingMinigame (thunk, +0x38)
// @func 0x0077EC1C FencingMinigame::~FencingMinigame (deleting thunk, +0x38)
FencingMinigame::~FencingMinigame() = default;

// @func 0x0077EB9C FencingMinigame::Slot14
bool FencingMinigame::Slot14() { return false; }

// @func 0x0077ED00 MinigameB20630::Slot16
bool MinigameB20630::Slot16() { return g_value10854AC <= 0.0f; }

// @func 0x0077CF7C MinigameB206A8::~MinigameB206A8
// @func 0x0077ED4C MinigameB206A8::~MinigameB206A8 (deleting)
MinigameB206A8::~MinigameB206A8() = default;

// @func 0x0077ED30 MinigameB206A8::AllowsPause
bool MinigameB206A8::AllowsPause(bool) { return field_917 == 0; }

// @func 0x0077D178 MinigameB20748::~MinigameB20748
// @func 0x0077EDD8 MinigameB20748::~MinigameB20748 (deleting)
MinigameB20748::~MinigameB20748() = default;

// @func 0x0041F1FC PenguinMontageMinigame::PenguinMontageMinigame
// NOTE: the meaning of flags1C bit 15 is unknown.
PenguinMontageMinigame::PenguinMontageMinigame(SlyGame* gameIn, const Desc* descIn, uint32_t scriptLibrary, void* ownerIn)
    : Minigame(gameIn, scriptLibrary, descIn->scriptName, ownerIn), desc(descIn)
{
    flags1C |= 0x8000u;
}

// @func 0x0077D75C PenguinMontageMinigame::~PenguinMontageMinigame
// @func 0x0077F4E8 PenguinMontageMinigame::~PenguinMontageMinigame (deleting)
PenguinMontageMinigame::~PenguinMontageMinigame() = default;
// @func 0x00421468 PenguinMontageMinigame::Slot12
void PenguinMontageMinigame::Slot12() {}
// @func 0x00421440 PenguinMontageMinigame::SetHudHidden
void PenguinMontageMinigame::SetHudHidden(bool hidden) { Minigame::SetHudHidden(hidden); }

// -- penguin minigames ---------------------------------------------------------------

// @func 0x0077D220 PenguinMinigame::~PenguinMinigame
// @func 0x0077EEE8 PenguinMinigame::~PenguinMinigame (deleting)
PenguinMinigame::~PenguinMinigame() = default;

// @func 0x0077EE98 PenguinMinigame::Slot22
bool PenguinMinigame::Slot22() { return false; }
// @func 0x0077EEA0 PenguinMinigame::Slot23
void PenguinMinigame::Slot23() {}
// @func 0x0077EEA4 PenguinMinigame::Slot24
void PenguinMinigame::Slot24() {}
// @func 0x0077EEA8 PenguinMinigame::Slot25
bool PenguinMinigame::Slot25() { return false; }
// @func 0x0077EEB0 PenguinMinigame::Slot26
bool PenguinMinigame::Slot26() { return false; }
// @func 0x0077EEB8 PenguinMinigame::Slot27
bool PenguinMinigame::Slot27() { return false; }
// @func 0x0077EEC0 PenguinMinigame::Slot28
void PenguinMinigame::Slot28() {}
// @func 0x0077EEC4 PenguinMinigame::Slot29
bool PenguinMinigame::Slot29() { return field_36 != 0; }
// @func 0x0077EECC PenguinMinigame::Slot30
void PenguinMinigame::Slot30(bool value) { field_36 = value ? 1 : 0; }
// @func 0x0077EED4 PenguinMinigame::Slot31
void PenguinMinigame::Slot31() {}
// @func 0x0077EED8 PenguinMinigame::Slot32
void PenguinMinigame::Slot32() {}
// @func 0x0077EEDC PenguinMinigame::Slot33
void PenguinMinigame::Slot33() {}
// @func 0x0077EEE0 PenguinMinigame::Slot34
bool PenguinMinigame::Slot34() { return true; }

// @func 0x0077D250 PenguinBalanceMinigame::~PenguinBalanceMinigame
// @func 0x0077F044 PenguinBalanceMinigame::~PenguinBalanceMinigame (deleting)
PenguinBalanceMinigame::~PenguinBalanceMinigame() = default;
// @func 0x0077EF40 PenguinBalanceMinigame::Slot25
bool PenguinBalanceMinigame::Slot25() { return flags[0] != 0; }
// @func 0x0077EF48 PenguinBalanceMinigame::Slot26
bool PenguinBalanceMinigame::Slot26() { return flags[1] != 0; }
// @func 0x0077EF50 PenguinBalanceMinigame::Slot27
bool PenguinBalanceMinigame::Slot27() { return flags[2] != 0; }
// @func 0x0077EF58 PenguinBalanceMinigame::Slot22
bool PenguinBalanceMinigame::Slot22() { return flags[3] != 0; }
// Same as the base; the class has its own copy.
// @func 0x0077EF60 PenguinBalanceMinigame::Slot29
bool PenguinBalanceMinigame::Slot29() { return field_36 != 0; }

// @func 0x0077D2C4 PenguinDodgeMinigame::~PenguinDodgeMinigame
// @func 0x0077F0F0 PenguinDodgeMinigame::~PenguinDodgeMinigame (deleting)
PenguinDodgeMinigame::~PenguinDodgeMinigame() = default;
// @func 0x00404438 PenguinDodgeMinigame::Slot12
void PenguinDodgeMinigame::Slot12() {}
// @func 0x00404410 PenguinDodgeMinigame::SetHudHidden
void PenguinDodgeMinigame::SetHudHidden(bool hidden) { Minigame::SetHudHidden(hidden); }
// @func 0x0077F0D0 PenguinDodgeMinigame::Slot25
bool PenguinDodgeMinigame::Slot25() { return flags[0] != 0; }
// @func 0x0077F0D8 PenguinDodgeMinigame::Slot26
bool PenguinDodgeMinigame::Slot26() { return flags[1] != 0; }
// @func 0x0077F0E0 PenguinDodgeMinigame::Slot27
bool PenguinDodgeMinigame::Slot27() { return flags[2] != 0; }
// @func 0x0077F0E8 PenguinDodgeMinigame::Slot22
bool PenguinDodgeMinigame::Slot22() { return flags[3] != 0; }

// @func 0x0077D338 PenguinPunchMinigame::~PenguinPunchMinigame
// @func 0x0077F19C PenguinPunchMinigame::~PenguinPunchMinigame (deleting)
PenguinPunchMinigame::~PenguinPunchMinigame() = default;
// @func 0x00409ED4 PenguinPunchMinigame::Slot12
void PenguinPunchMinigame::Slot12() {}
// @func 0x00409EAC PenguinPunchMinigame::SetHudHidden
void PenguinPunchMinigame::SetHudHidden(bool hidden) { Minigame::SetHudHidden(hidden); }
// @func 0x0077F17C PenguinPunchMinigame::Slot25
bool PenguinPunchMinigame::Slot25() { return flags[0] != 0; }
// @func 0x0077F184 PenguinPunchMinigame::Slot26
bool PenguinPunchMinigame::Slot26() { return flags[1] != 0; }
// @func 0x0077F18C PenguinPunchMinigame::Slot27
bool PenguinPunchMinigame::Slot27() { return flags[2] != 0; }
// @func 0x0077F194 PenguinPunchMinigame::Slot22
bool PenguinPunchMinigame::Slot22() { return flags[3] != 0; }

// @func 0x0077D3AC PenguinSlingshotMinigame::~PenguinSlingshotMinigame
// @func 0x0077F248 PenguinSlingshotMinigame::~PenguinSlingshotMinigame (deleting)
PenguinSlingshotMinigame::~PenguinSlingshotMinigame()
{
    ReleaseSpawned(spawned[0]);
    ReleaseSpawned(spawned[1]);
}
// @func 0x0077F228 PenguinSlingshotMinigame::Slot25
bool PenguinSlingshotMinigame::Slot25() { return flags[0] != 0; }
// @func 0x0077F230 PenguinSlingshotMinigame::Slot26
bool PenguinSlingshotMinigame::Slot26() { return flags[1] != 0; }
// @func 0x0077F238 PenguinSlingshotMinigame::Slot27
bool PenguinSlingshotMinigame::Slot27() { return flags[2] != 0; }
// @func 0x0077F240 PenguinSlingshotMinigame::Slot22
bool PenguinSlingshotMinigame::Slot22() { return flags[3] != 0; }
// @func 0x004125F0 PenguinSlingshotMinigame::SetHudHidden
void PenguinSlingshotMinigame::SetHudHidden(bool hidden) { Minigame::SetHudHidden(hidden); }

// @func 0x0077D484 PenguinSumoMinigame::~PenguinSumoMinigame
// @func 0x0077F350 PenguinSumoMinigame::~PenguinSumoMinigame (deleting)
PenguinSumoMinigame::~PenguinSumoMinigame() = default;
// @func 0x00418E90 PenguinSumoMinigame::Slot12
void PenguinSumoMinigame::Slot12() {}
// @func 0x00418E68 PenguinSumoMinigame::SetHudHidden
void PenguinSumoMinigame::SetHudHidden(bool hidden) { Minigame::SetHudHidden(hidden); }
// @func 0x00419C78 PenguinSumoMinigame::Slot22
bool PenguinSumoMinigame::Slot22() { return roundTime > 2.0f && roundFlag != 0; }
// @func 0x0041A09C PenguinSumoMinigame::Slot31
void PenguinSumoMinigame::Slot31() {}
// @func 0x0041A0A0 PenguinSumoMinigame::Slot32
void PenguinSumoMinigame::Slot32() {}
// @func 0x0077F338 PenguinSumoMinigame::Slot25
bool PenguinSumoMinigame::Slot25() { return flags[0] != 0; }
// @func 0x0077F340 PenguinSumoMinigame::Slot26
bool PenguinSumoMinigame::Slot26() { return flags[1] != 0; }
// @func 0x0077F348 PenguinSumoMinigame::Slot27
bool PenguinSumoMinigame::Slot27() { return flags[2] != 0; }

// @func 0x0077D558 WhackPenguinMinigame::~WhackPenguinMinigame
// @func 0x0077F45C WhackPenguinMinigame::~WhackPenguinMinigame (deleting)
WhackPenguinMinigame::~WhackPenguinMinigame() = default;
// @func 0x0041E690 WhackPenguinMinigame::Slot12
void WhackPenguinMinigame::Slot12() {}
// @func 0x0041E668 WhackPenguinMinigame::SetHudHidden
void WhackPenguinMinigame::SetHudHidden(bool hidden) { Minigame::SetHudHidden(hidden); }
// @func 0x0077F43C WhackPenguinMinigame::Slot25
bool WhackPenguinMinigame::Slot25() { return flags[0] != 0; }
// @func 0x0077F444 WhackPenguinMinigame::Slot26
bool WhackPenguinMinigame::Slot26() { return flags[1] != 0; }
// @func 0x0077F44C WhackPenguinMinigame::Slot27
bool WhackPenguinMinigame::Slot27() { return flags[2] != 0; }
// @func 0x0077F454 WhackPenguinMinigame::Slot22
bool WhackPenguinMinigame::Slot22() { return flags[3] != 0; }

} // namespace gameplay
} // namespace sly
