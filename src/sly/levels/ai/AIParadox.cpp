// openTIT - Paradox boss behaviour (vtable 0x00B0EC30), partial.
#include "sly/levels/ai/AIBehaviours.h"

namespace sly {
namespace levels {
namespace ai {

// @func 0x0005F9DC ParadoxAI::OnDamaged
void ParadoxAI::OnDamaged(float amount, uint32_t kind, const AIDamageInfo* info)
{
    (void)amount; (void)kind; (void)info;
}

// @func 0x0005F9E8 ParadoxAI::GravitySpeed
float ParadoxAI::GravitySpeed()
{
    return 0.0f;
}

// @func 0x0005F9F8 ParadoxAI::OnTauntChance
void ParadoxAI::OnTauntChance() {}

// @func 0x0005FADC ParadoxAI::IsSettled
bool ParadoxAI::IsSettled()
{
    return settled != 0;
}

// @func 0x0005FE2C ParadoxAI::Think
bool ParadoxAI::Think(float dt)
{
    (void)dt;
    return false;
}

} // namespace ai
} // namespace levels
} // namespace sly
