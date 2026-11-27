// openTIT - Sequence commands whose construction and behaviour live in
// 0x00430A64-0x0043A000 and are not ported yet: only their destructors and
// trivial slots (0x0077F5F8-0x00781738) are. Each class names its console
// vtable and constructor; the level script services build them (see
// sly/levels/ScriptServices.h).
#pragma once

#include "sly/game/Tween.h"
#include "sly/gameplay/Sequencer.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/memory/ApeTArray.h"

namespace ape::world { class ApePointInstance; }

namespace sly {
namespace gameplay {

// Scripted camera that turns from a start point towards a target
// ("LookAt", vtable 0x00B20F68, 0x70 bytes). The camera starts at a point
// instance or sequence entity and keeps its position; every frame it turns
// towards the target (a point instance or sequence entity) at `turnRate`
// full turns per second (snapping at once when the rate is 0) and pushes
// the view to the camera manager. Blocks for `duration` seconds when the
// target is a sequence entity.
class SeqLookAtCommand : public SequenceCommand {
public:
    SeqLookAtCommand(float duration, float turnRate, float fov, uint32_t entityHash,
                     uint32_t targetHash, uint32_t entityPointHash, uint32_t targetPointHash);
    ~SeqLookAtCommand() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    WorldObject* m_entity = nullptr;                    // +0x0C
    uint32_t m_entityHash = 0;                          // +0x10
    WorldObject* m_target = nullptr;                    // +0x14
    uint32_t m_targetHash = 0;                          // +0x18
    ape::world::ApePointInstance* m_entityPoint = nullptr;   // +0x1C
    uint32_t m_entityPointHash = 0;                     // +0x20
    ape::world::ApePointInstance* m_targetPoint = nullptr;   // +0x24
    uint32_t m_targetPointHash = 0;                     // +0x28
    float m_elapsed = 0.0f;                             // +0x2C
    float m_duration = 0.0f;                            // +0x30
    float m_turnRate = 0.0f;                            // +0x34
    ape::Quat m_rotation{0.0f, 0.0f, 0.0f, 1.0f};       // +0x40
    ape::Vec3 m_position{};                             // +0x50
    float m_fov = 0.0f;                                 // +0x60
    bool m_aligned = false;                             // +0x64 bit 31
};

// Scripted camera move ("MoveCamera", vtable 0x00B20F98, 0x80 bytes): from
// a start point (or where the camera is) the camera blends its field of
// view, moves to an end point after a delay and turns to face a look point,
// each over its own time.
class SeqMoveCameraCommand : public SequenceCommand {
public:
    SeqMoveCameraCommand(float fov, float fovTime, float moveDelay, float moveTime, float turnTime,
                         uint32_t startPoint, uint32_t endPoint, uint32_t lookPoint);
    ~SeqMoveCameraCommand() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    ape::world::ApePointInstance* m_startPoint = nullptr;   // +0x0C
    ape::Vec3 m_startPosition{};                        // +0x10
    ape::world::ApePointInstance* m_endPoint = nullptr;     // +0x20
    float m_startFov = 0.0f;                            // +0x24 camera's when started
    float m_targetFov = 0.0f;                           // +0x28 (0: keep the camera's)
    float m_fovElapsed = 0.0f;                          // +0x2C
    float m_fovTime = 0.0f;                             // +0x30
    float m_delayElapsed = 0.0f;                        // +0x34
    float m_moveDelay = 0.0f;                           // +0x38
    float m_moveElapsed = 0.0f;                         // +0x3C
    float m_moveTime = 0.0f;                            // +0x40
    ape::Quat m_startRotation{0.0f, 0.0f, 0.0f, 1.0f};  // +0x50
    ape::Quat m_lookRotation{0.0f, 0.0f, 0.0f, 1.0f};   // +0x60
    ape::world::ApePointInstance* m_lookPoint = nullptr;    // +0x70
    float m_turnElapsed = 0.0f;                         // +0x74
    float m_turnTime = 0.0f;                            // +0x78
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
