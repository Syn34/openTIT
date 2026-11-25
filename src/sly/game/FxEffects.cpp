// openTIT - "Attached to" overrides of the FX manager's effect kinds (see FxEffects.h).
#include "sly/game/FxEffects.h"

namespace sly {
namespace game {

// @func 0x00767AB0 FxEffectB1D528::KillIfAttachedTo
void FxEffectB1D528::KillIfAttachedTo(ape::world::ApeObj* object)
{
    if (attachedTo == object)
        Stop();
}
// @func 0x00767AF8 FxEffectB1D528::SetValueIfAttachedTo
void FxEffectB1D528::SetValueIfAttachedTo(ape::world::ApeObj* object, float newValue)
{
    if (attachedTo == object)
        value = newValue;
}

// @func 0x00767B0C FxEffectB1D550::KillIfAttachedTo
void FxEffectB1D550::KillIfAttachedTo(ape::world::ApeObj* object)
{
    if (attachedTo == object)
        Stop();
}
// @func 0x00767B54 FxEffectB1D550::SetValueIfAttachedTo
void FxEffectB1D550::SetValueIfAttachedTo(ape::world::ApeObj* object, float newValue)
{
    if (attachedTo == object)
        value = newValue;
}

// @func 0x00767B68 FxEffectB1D578::KillIfAttachedTo
void FxEffectB1D578::KillIfAttachedTo(ape::world::ApeObj* object)
{
    if (attachedTo == object)
        Stop();
}
// @func 0x00767BB0 FxEffectB1D578::SetValueIfAttachedTo
void FxEffectB1D578::SetValueIfAttachedTo(ape::world::ApeObj* object, float newValue)
{
    if (attachedTo == object)
        value = newValue;
}

// @func 0x00767BC4 FxEffectB1D5A0::KillIfAttachedTo
void FxEffectB1D5A0::KillIfAttachedTo(ape::world::ApeObj* object)
{
    if (attachedTo == object)
        Stop();
}
// @func 0x00767C0C FxEffectB1D5A0::SetValueIfAttachedTo
void FxEffectB1D5A0::SetValueIfAttachedTo(ape::world::ApeObj* object, float newValue)
{
    if (attachedTo == object)
        value = newValue;
}

} // namespace game
} // namespace sly
