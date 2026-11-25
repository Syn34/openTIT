// openTIT - <MagNet> / <SpeedBoost> components (0x00255F18, 0x002560F0,
// 0x002B2A70-0x002B6728).
#include "sly/components/PulseField.h"

namespace sly {
namespace components {

// Inline part of both console constructors (the two copies are tagged on
// the derived constructors).
PulseFieldComponent::PulseFieldComponent(const PulseFieldComponentDesc* desc)
    : EntityComponent(desc)
    , radius(desc->radius)
    , strength(desc->strength)
    , damping(desc->damping)
    , onDuration(desc->onDuration)
    , offDuration(desc->offDuration)
    , timeOffset(desc->timeOffset)
{
    listener.outer = this;
    timer = timeOffset;
    period = onDuration + offDuration;
}

PulseFieldComponent::~PulseFieldComponent() = default;

// @func 0x002560F0 MagNetComponent::MagNetComponent
MagNetComponent::MagNetComponent(const MagNetComponentDesc* desc) : PulseFieldComponent(desc) {}

// @func 0x002B2A70 MagNetComponent::~MagNetComponent
// @func 0x002B2C30 MagNetComponent::~MagNetComponent (deleting)
// @func 0x002B2CC4 MagNetComponent::Listener::~Listener (thunk)
// @func 0x002B2C78 MagNetComponent::Listener::~Listener (deleting thunk)
MagNetComponent::~MagNetComponent() = default;

// @func 0x002B425C MagNetComponent::OnListenerMessage
// @func 0x002B4260 MagNetComponent::Listener::OnMessage (thunk)
void MagNetComponent::OnListenerMessage(uint32_t message)
{
    (void)message;
}

// @func 0x00255F18 SpeedBoostComponent::SpeedBoostComponent
SpeedBoostComponent::SpeedBoostComponent(const SpeedBoostComponentDesc* desc) : PulseFieldComponent(desc) {}

// @func 0x002B50D4 SpeedBoostComponent::~SpeedBoostComponent
// @func 0x002B5294 SpeedBoostComponent::~SpeedBoostComponent (deleting)
// @func 0x002B5328 SpeedBoostComponent::Listener::~Listener (thunk)
// @func 0x002B52DC SpeedBoostComponent::Listener::~Listener (deleting thunk)
SpeedBoostComponent::~SpeedBoostComponent() = default;

// @func 0x002B6724 SpeedBoostComponent::OnListenerMessage
// @func 0x002B6728 SpeedBoostComponent::Listener::OnMessage (thunk)
void SpeedBoostComponent::OnListenerMessage(uint32_t message)
{
    (void)message;
}

} // namespace components
} // namespace sly
