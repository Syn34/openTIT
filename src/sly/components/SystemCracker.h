// openTIT - <SystemCracker> component (type 0x29, vtable 0x00B1CC10, 0x2C
// bytes): a hackable console. It listens to the "SystemCracker" listener
// group and plays the state named by each message, on its own script (made
// from the description's script, with the owner's transform) or else on the
// owner's anim script.
#pragma once

#include "sly/components/Components.h"

namespace ape { class ApeAnimScriptInstance; }

namespace sly {
namespace components {

class SystemCrackerComponent : public EntityComponent {
public:
    explicit SystemCrackerComponent(const SystemCrackerComponentDesc* desc);
    ~SystemCrackerComponent() override;
    // Nothing to do per frame (slot 6, 0x002B1A10).
    void Update(float dt) override;
    // Slot 16 (listener interface slot 2): a message of the listener group.
    virtual void OnListenerMessage(uint32_t message);

    // Listener interface at +0x18 (vtable 0x00B1CC5C).
    struct Listener {
        SystemCrackerComponent* outer = nullptr;
    } listener;                                   // 0x18
    uint32_t scriptName = 0;                      // 0x20
    bool renderOnTop = false;                     // 0x24 render layer 9 for the owner
    ape::ApeAnimScriptInstance* script = nullptr; // 0x28
};

} // namespace components
} // namespace sly
