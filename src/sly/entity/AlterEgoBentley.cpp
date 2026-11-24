// openTIT - States of the Bentley alter ego entity (see AlterEgoBentley.h).
#include "sly/entity/AlterEgoBentley.h"

#include "sly/entity/GameEntityHooks.h"

namespace sly {
namespace entity {

// @func 0x0016CD1C AlterEgoBentleyState::~AlterEgoBentleyState
// @func 0x0016CD2C AlterEgoBentleyState::~AlterEgoBentleyState (deleting)
AlterEgoBentleyState::~AlterEgoBentleyState() = default;

// @func 0x0016CD70 AlterEgoBentleyState::Enter
void AlterEgoBentleyState::Enter(void* /*owner*/) {}

// @func 0x0016CD74 AlterEgoBentleyState::Update
void AlterEgoBentleyState::Update(void* /*owner*/, float /*dt*/) {}

// @func 0x0016CD84 AlterEgoBentleyStateA::~AlterEgoBentleyStateA
// @func 0x0016CD94 AlterEgoBentleyStateA::~AlterEgoBentleyStateA (deleting)
AlterEgoBentleyStateA::~AlterEgoBentleyStateA() = default;

// @func 0x0016D0F8 AlterEgoBentleyStateB::~AlterEgoBentleyStateB
// @func 0x0016D108 AlterEgoBentleyStateB::~AlterEgoBentleyStateB (deleting)
AlterEgoBentleyStateB::~AlterEgoBentleyStateB() = default;

// @func 0x0016D3FC AlterEgoBentleyStateC::~AlterEgoBentleyStateC
// @func 0x0016D40C AlterEgoBentleyStateC::~AlterEgoBentleyStateC (deleting)
AlterEgoBentleyStateC::~AlterEgoBentleyStateC() = default;

// @func 0x0016D6B0 AlterEgoBentleyStateC::Slot4
void AlterEgoBentleyStateC::Slot4() {}

// @func 0x0016D6BC AlterEgoBentleyStateD::~AlterEgoBentleyStateD
// @func 0x0016D6CC AlterEgoBentleyStateD::~AlterEgoBentleyStateD (deleting)
AlterEgoBentleyStateD::~AlterEgoBentleyStateD() = default;

void AlterEgoBentleyScriptState::ReleaseScript()
{
    if (!script)
        return;
    OrphanAndReleaseAnimScript(script);
    script = nullptr;
}

// @func 0x0016D8A0 AlterEgoBentleyScriptStateA::~AlterEgoBentleyScriptStateA
// @func 0x0016D928 AlterEgoBentleyScriptStateA::~AlterEgoBentleyScriptStateA (deleting)
AlterEgoBentleyScriptStateA::~AlterEgoBentleyScriptStateA()
{
    ReleaseScript();
}

// @func 0x0016E7C8 AlterEgoBentleyScriptStateB::~AlterEgoBentleyScriptStateB
// @func 0x0016E850 AlterEgoBentleyScriptStateB::~AlterEgoBentleyScriptStateB (deleting)
AlterEgoBentleyScriptStateB::~AlterEgoBentleyScriptStateB()
{
    ReleaseScript();
}

// @func 0x0016EC48 AlterEgoBentleyScriptStateB::Slot4
void AlterEgoBentleyScriptStateB::Slot4() {}

} // namespace entity
} // namespace sly
