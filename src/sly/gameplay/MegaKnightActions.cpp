// openTIT - Destructors and kinds of the mega knight behaviour actions.
#include "sly/gameplay/MegaKnightBehavior.h"

namespace sly {
namespace gameplay {

// @func 0x00768214 MegaKnightGroundSmashAction::~MegaKnightGroundSmashAction
// @func 0x00768718 MegaKnightGroundSmashAction::~MegaKnightGroundSmashAction (deleting)
MegaKnightGroundSmashAction::~MegaKnightGroundSmashAction() = default;
// @func 0x00768710 MegaKnightGroundSmashAction::Kind
int MegaKnightGroundSmashAction::Kind() const { return 0; }

// @func 0x00768224 MegaKnightLaserAction::~MegaKnightLaserAction
// @func 0x00768764 MegaKnightLaserAction::~MegaKnightLaserAction (deleting)
MegaKnightLaserAction::~MegaKnightLaserAction() = default;
// @func 0x0076875C MegaKnightLaserAction::Kind
int MegaKnightLaserAction::Kind() const { return 1; }

// @func 0x00768234 MegaKnightMoveAction::~MegaKnightMoveAction
// @func 0x007687B0 MegaKnightMoveAction::~MegaKnightMoveAction (deleting)
MegaKnightMoveAction::~MegaKnightMoveAction() = default;
// @func 0x007687A8 MegaKnightMoveAction::Kind
int MegaKnightMoveAction::Kind() const { return 2; }

// @func 0x00768244 MegaKnightStampMoveAction::~MegaKnightStampMoveAction
// @func 0x007687FC MegaKnightStampMoveAction::~MegaKnightStampMoveAction (deleting)
MegaKnightStampMoveAction::~MegaKnightStampMoveAction() = default;
// @func 0x007687F4 MegaKnightStampMoveAction::Kind
int MegaKnightStampMoveAction::Kind() const { return 3; }

// @func 0x00768254 MegaKnightCountAction::~MegaKnightCountAction
// @func 0x00768848 MegaKnightCountAction::~MegaKnightCountAction (deleting)
MegaKnightCountAction::~MegaKnightCountAction() = default;
// @func 0x00768840 MegaKnightCountAction::Kind
int MegaKnightCountAction::Kind() const { return 5; }

// @func 0x00768264 MegaKnightPauseAction::~MegaKnightPauseAction
// @func 0x00768894 MegaKnightPauseAction::~MegaKnightPauseAction (deleting)
MegaKnightPauseAction::~MegaKnightPauseAction() = default;
// @func 0x0076888C MegaKnightPauseAction::Kind
int MegaKnightPauseAction::Kind() const { return 6; }

// @func 0x00768274 MegaKnightCameraAction::~MegaKnightCameraAction
// @func 0x007688E0 MegaKnightCameraAction::~MegaKnightCameraAction (deleting)
MegaKnightCameraAction::~MegaKnightCameraAction() = default;
// @func 0x007688D8 MegaKnightCameraAction::Kind
int MegaKnightCameraAction::Kind() const { return 7; }

} // namespace gameplay
} // namespace sly
