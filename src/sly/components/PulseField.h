// openTIT - <MagNet> (type 0x2A, vtable 0x00B1CCD8) and <SpeedBoost> (type
// 0x2B, vtable 0x00B1CDB0) components, 0x84 bytes each: radial fields that
// switch on and off periodically and push or pull what is inside. Both embed
// a listener interface (vtables 0x00B1CD24 / 0x00B1CDFC) that ignores its
// messages.
#pragma once

#include "ape/math/ApeMath.h"
#include "ape/memory/ApeTArray.h"
#include "sly/components/ComponentHooks.h"
#include "sly/components/Components.h"

namespace sly {
namespace components {

class PulseFieldComponent : public EntityComponent {
public:
    explicit PulseFieldComponent(const PulseFieldComponentDesc* desc);
    ~PulseFieldComponent() override;
    // Slot 16 (listener interface slot 2): messages are ignored.
    virtual void OnListenerMessage(const ListenerMessage& msg) { (void)msg; }

    struct Listener {
        PulseFieldComponent* outer = nullptr;
    } listener;                               // 0x18
    float radius = 0.0f;                      // 0x20
    float strength = 0.0f;                    // 0x24
    float damping = 0.0f;                     // 0x28
    float onDuration = 0.0f;                  // 0x2C
    float offDuration = 0.0f;                 // 0x30
    float timeOffset = 0.0f;                  // 0x34
    float timer = 0.0f;                       // 0x38 starts at timeOffset
    float period = 0.0f;                      // 0x3C on + off
    // Working sets of the field update. NOTE: element meaning not decoded.
    ape::ApeTArray<entity::Entity*> affected;   // 0x40
    ape::ApeTArray<ape::Vec3> impulses;         // 0x54
    ape::ApeTArray<void*> field_68;             // 0x68
    ape::ApeTArray<void*> field_7C;             // 0x7C
};

class MagNetComponent : public PulseFieldComponent {
public:
    explicit MagNetComponent(const MagNetComponentDesc* desc);
    ~MagNetComponent() override;
    void OnListenerMessage(const ListenerMessage& msg) override;
    // TODO(0x002B31F8 MagNetComponent::Update): pulls bodies in range.
};

class SpeedBoostComponent : public PulseFieldComponent {
public:
    explicit SpeedBoostComponent(const SpeedBoostComponentDesc* desc);
    ~SpeedBoostComponent() override;
    void OnListenerMessage(const ListenerMessage& msg) override;
    // TODO(0x002B5848 SpeedBoostComponent::Update): boosts characters in range.
};

} // namespace components
} // namespace sly
