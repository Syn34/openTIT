// openTIT - Paradox boss behaviour (vtable 0x00B0EC30), partial.
#include "sly/levels/ai/AIBehaviours.h"
#include "sly/levels/ai/AITuning.h"

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

// @func 0x0005FA08 ParadoxAI::OnScriptCommand
void ParadoxAI::OnScriptCommand(uint32_t command, uint32_t arg1, uint32_t arg2, float value)
{
    (void)arg1; (void)arg2;
    if (command == 0xCC65890Au) {
        static constexpr uint32_t kCameras[3] = {0x6021F400u, 0x6021F401u, 0x6021F402u};
        const int index = phase == 1 ? 0 : (phase == 2 ? 1 : 2);
        PlayCamera(kCameras[index], g_paradoxCameraInterp);
    }
    settled = 0;
    pendingCommand = command;
    pendingValue = value;
    hasPendingCommand = true;
}

} // namespace ai
} // namespace levels
} // namespace sly
