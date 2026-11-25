// openTIT - Destructors and trivial slots of the unported sequence commands
// (see SequenceCommands3.h).
#include "sly/gameplay/SequenceCommands3.h"

namespace sly {
namespace gameplay {

// @func 0x00780540 SeqCameraOrbitCommand::~SeqCameraOrbitCommand
// @func 0x00780570 SeqCameraOrbitCommand::~SeqCameraOrbitCommand (deleting)
SeqCameraOrbitCommand::~SeqCameraOrbitCommand() = default;

// @func 0x0077F5F8 SeqArcQteCommand::~SeqArcQteCommand
// @func 0x00780B78 SeqArcQteCommand::~SeqArcQteCommand (deleting)
SeqArcQteCommand::~SeqArcQteCommand() = default;

// @func 0x0077F628 SeqActorQteCommand::~SeqActorQteCommand
// @func 0x00780BD0 SeqActorQteCommand::~SeqActorQteCommand (deleting)
SeqActorQteCommand::~SeqActorQteCommand() = default;

// @func 0x00780DF8 SeqHudTypeCommand::~SeqHudTypeCommand
// @func 0x00780E28 SeqHudTypeCommand::~SeqHudTypeCommand (deleting)
SeqHudTypeCommand::~SeqHudTypeCommand() = default;

// @func 0x00781480 SeqPlacementCommand::~SeqPlacementCommand
// @func 0x007814B0 SeqPlacementCommand::~SeqPlacementCommand (deleting)
SeqPlacementCommand::~SeqPlacementCommand() = default;

// @func 0x00781508 SeqPlacementCommand::Update
bool SeqPlacementCommand::Update(float dt)
{
    (void)dt;
    return true;
}

// @func 0x007816B0 SeqSpawnEntityCommand::~SeqSpawnEntityCommand
// @func 0x007816E0 SeqSpawnEntityCommand::~SeqSpawnEntityCommand (deleting)
SeqSpawnEntityCommand::~SeqSpawnEntityCommand() = default;

// @func 0x00781738 SeqSpawnEntityCommand::Update
bool SeqSpawnEntityCommand::Update(float dt)
{
    (void)dt;
    return true;
}

} // namespace gameplay
} // namespace sly
