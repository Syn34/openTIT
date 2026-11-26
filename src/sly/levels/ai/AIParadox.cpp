// openTIT - Paradox boss behaviour (vtable 0x00B0EC30), partial.
#include "sly/levels/ai/AIBehaviours.h"
#include "sly/camera/CameraModes.h"
#include "sly/entity/GameCamera.h"
#include "sly/game/GameMessages.h"
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

// @func 0x0005BDC0 ParadoxAI::Detach
void ParadoxAI::Detach()
{
    UnregisterListener(entity, true);
    GameWorldSetBossFlag(false);
    Entity* player = PlayerCharacter();
    if (player && fightCamera) {
        player->DetachChild(fightCamera);
        fightCamera->SetActiveVisual(false);
        fightCamera->flags &= ~0x8000u;
    }
    if (strafeMode) {
        fightCamera->RemoveMode(1.0f, strafeMode);
        delete strafeMode;
        strafeMode = nullptr;
    }
    if (cameraMode) {
        fightCamera->RemoveMode(1.0f, cameraMode);
        delete cameraMode;
        cameraMode = nullptr;
    }
    if (player) {
        player->SetScriptState(0xBC1BA9E7u);
        player->SetActive(false);
    }
    AIController::Detach();
}

// @func 0x0005C848 ParadoxAI::UpdateStrafeCamera
// NOTE: the console reads the player without checking that there is one.
void ParadoxAI::UpdateStrafeCamera()
{
    Entity* player = PlayerCharacter();
    const bool wanted = state == 0 || state == 1;
    if (!strafeMode) {
        if (!wanted)
            return;
        strafeMode = new camera::StrafeCameraMode(player->transform.translation.y, fightCamera, player, entity);
        fightCamera->PushMode(0.0f, strafeMode, false);
        return;
    }
    strafeMode->SetSubject(player);
    strafeMode->current.height = strafeMode->target.height = player->transform.translation.y;
    if (!wanted) {
        fightCamera->RemoveMode(0.0f, strafeMode);
        delete strafeMode;
        strafeMode = nullptr;
    }
}

} // namespace ai
} // namespace levels
} // namespace sly
