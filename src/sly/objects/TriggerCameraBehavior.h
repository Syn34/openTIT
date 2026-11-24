// openTIT - Camera behaviour trigger volumes: switch the game camera's
// behaviour (follow, orbit, ...) while a player character is inside.
//
// Definition vtable 0x00B19FB0, entity 0x00B19FE0.
#pragma once

#include "sly/objects/Trigger.h"

#include <cstdint>

namespace sly {
namespace objects {

constexpr uint32_t kDefClass_CameraBehaviorTrigger = 0x01000000;

// TODO(0x002183FC CreateCameraBehaviorTriggerDef, 0x00217470 parser,
// 0x00216AB0 definition constructor): the behaviour parameters are camera
// module blocks (0x000FD7AC) that are not ported.
class CameraBehaviorTriggerDef : public TriggerDef {
public:
    using TriggerDef::TriggerDef;
    ~CameraBehaviorTriggerDef() override;
    ObjEntity* Create(EntityPlacement* placement) override;

private:
    // Camera module parameter blocks the parser allocates (0xB4..0xC0).
    // NOTE: which camera behaviour each slot holds is not known.
    void* m_behaviorBlocks[4] = {};
};

class CameraBehaviorTrigger : public TriggerBase {
public:
    explicit CameraBehaviorTrigger(const TriggerInstanceDesc& desc) : TriggerBase(desc, 0x20, true) {}
    ~CameraBehaviorTrigger() override;

    void Update(float dt) override;
    void OnEnter(ObjEntity* entity) override;
    bool IsDebugDisplayed() const override;
    // TODO(0x002194DC CameraBehaviorTrigger::OnExit): hands the camera back to
    // the next behaviour trigger the character is in, or resets the camera
    // target of the behaviour kind.

private:
    // TODO(0x00218DD8): applies the definition's behaviour to the camera.
    void ApplyBehavior(ObjEntity* entity) { (void)entity; }
};

} // namespace objects
} // namespace sly
