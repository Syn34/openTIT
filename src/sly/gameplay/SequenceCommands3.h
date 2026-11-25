// openTIT - Sequence commands whose construction and behaviour live in
// 0x00430A64-0x0043A000 and are not ported yet: only their destructors and
// trivial slots (0x0077F5F8-0x00781738) are. Each class names its console
// vtable and constructor; the level script services build them (see
// sly/levels/ScriptServices.h).
#pragma once

#include "sly/game/Tween.h"
#include "sly/gameplay/Sequencer.h"
#include "ape/memory/ApeTArray.h"

namespace sly {
namespace gameplay {

// Camera orbit (vtable 0x00B20F98, 0x80 bytes, constructor 0x00430A64).
class SeqCameraOrbitCommand : public SequenceCommand {
public:
    using SequenceCommand::SequenceCommand;
    ~SeqCameraOrbitCommand() override;
    // TODO(0x00430BE8 SeqCameraOrbitCommand::Execute, 0x00431030 Update)
};

// Arc QTE (vtable 0x00B21148, constructor 0x00434E0C).
class SeqArcQteCommand : public SequenceCommand {
public:
    using SequenceCommand::SequenceCommand;
    ~SeqArcQteCommand() override;
    // TODO(0x00434EFC SeqArcQteCommand::Execute, 0x00435168 Update)
};

// Actor QTE (vtable 0x00B21160, 100 bytes, constructor 0x0043554C).
class SeqActorQteCommand : public SequenceCommand {
public:
    using SequenceCommand::SequenceCommand;
    ~SeqActorQteCommand() override;
    // TODO(0x004358D0 SeqActorQteCommand::Execute, 0x00435B30 Update)

    // NOTE: the tween's kind is not known.
    FloatTween timer;                   // +0x2C
    ape::ApeTArray<void*> field_50;     // +0x50
};

// HUD type switch (vtable 0x00B211F0, constructor 0x00437F24).
class SeqHudTypeCommand : public SequenceCommand {
public:
    using SequenceCommand::SequenceCommand;
    ~SeqHudTypeCommand() override;
    // TODO(0x0043807C SeqHudTypeCommand::Execute, 0x004382D8 Update)
};

// Entity placement (vtable 0x00B21328, constructor 0x00439224): done as
// soon as it ran.
class SeqPlacementCommand : public SequenceCommand {
public:
    using SequenceCommand::SequenceCommand;
    ~SeqPlacementCommand() override;
    // TODO(0x00439348 SeqPlacementCommand::Execute)
    bool Update(float dt) override;
};

// Entity spawn (vtable 0x00B21388, constructor 0x00439E4C): done as soon
// as it ran.
class SeqSpawnEntityCommand : public SequenceCommand {
public:
    using SequenceCommand::SequenceCommand;
    ~SeqSpawnEntityCommand() override;
    // TODO(0x00439FD0 SeqSpawnEntityCommand::Execute)
    bool Update(float dt) override;
};

} // namespace gameplay
} // namespace sly
