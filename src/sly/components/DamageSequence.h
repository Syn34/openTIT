// openTIT - <DamageSequence> component (type 0x2F, vtable 0x00B1D268, 0x60
// bytes): the owner must be damaged through its parts in a given order. Each
// "damaged" listener message from the expected part advances the sequence;
// a hit on another attached part fails it.
#pragma once

#include "ape/memory/ApeTArray.h"
#include "sly/components/ComponentHooks.h"
#include "sly/components/Components.h"

namespace ape { class ApeAnimScriptInstance; }

namespace sly {
namespace components {

class DamageSequenceComponent : public EntityComponent {
public:
    explicit DamageSequenceComponent(const DamageSequenceComponentDesc* desc);
    ~DamageSequenceComponent() override;
    // TODO(0x002D3850 DamageSequenceComponent::Update): runs the
    // init/enabled/disabled phases and the script.
    void SetOption(bool on) override;
    // Slot 16 (listener interface slot 2).
    virtual void OnListenerMessage(const ListenerMessage& msg);

    // One step of the sequence (0x100 bytes on the console). NOTE: only the
    // part entity (+0xD0) is decoded.
    struct Step {
        entity::Entity* part = nullptr;
    };

    struct Listener {
        DamageSequenceComponent* outer = nullptr;
    } listener;                                   // 0x18
    ape::ApeAnimScriptInstance* script = nullptr; // 0x20
    int32_t stepCount = -1;                       // 0x24
    float initTime = 1.0f;                        // 0x28
    float enabledTime = 1.0f;                     // 0x2C
    float disabledTime = 1.0f;                    // 0x30
    float timer = 0.0f;                           // 0x34
    int32_t phase = 3;                            // 0x38 NOTE: 2 ignores wrong hits
    int32_t field_3C = 2;                         // 0x3C
    int32_t nextStep = 0;                         // 0x40
    ape::ApeTArray<Step> steps;                   // 0x44
    uint32_t flag29 = 0;                          // 0x58
    bool completed = false;                       // 0x5C bit 31
    bool failed = false;                          // 0x5C bit 30
};

} // namespace components
} // namespace sly
