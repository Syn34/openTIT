// openTIT - Paradox boss behaviour (vtable 0x00B0EC30), partial.
#include "sly/levels/ai/AIBehaviours.h"
#include "sly/camera/CameraModes.h"
#include "sly/entity/GameCamera.h"
#include "sly/game/GameMessages.h"
#include "sly/game/SlyGame.h"
#include "sly/game/hud/HudManager.h"
#include "ape/world/ApeObj.h"
#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
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

// @func 0x0005E470 ParadoxAI::StartPhaseOne
void ParadoxAI::StartPhaseOne()
{
    phase = 1;
    phaseObjectCount = 0;
    if (arena) {
        static constexpr uint32_t kObjectVars[5] = {0x11A825BAu, 0x7D63B6B9u, 0xE91F47B8u, 0x54DAD8B7u,
                                                    0xC09669B6u};
        auto* script = static_cast<ape::ApeAnimScriptInstance*>(arena->GetAnimScript());
        for (uint32_t name : kObjectVars) {
            auto* var = static_cast<ape::ApeAnimScriptObjectVar*>(script->FindVar(name));
            phaseObjects[phaseObjectCount++] = var ? var->GetObject() : nullptr;
        }
    }
    SetState(0);
}

// @func 0x0005BB20 ParadoxAI::Attach
// NOTE: the console reads the player and the game camera without checks.
void ParadoxAI::Attach(Entity* attached)
{
    AIController::Attach(attached);
    Entity* player = PlayerCharacter();
    fightCamera = g_game->camera;
    fightCamera->RestartBlend(-1.0f);
    fightCamera->resetTimer = 0.1f;
    player->object->SetBoundingRadius(20.0f);
    entity->object->SetBoundingRadius(20.0f);
    hud::SetContext(character, 0xFB7FA14Du);
    RegisterListener(entity, 0xF11D508Eu, true);
    pendingCommand = 0;
    field_1D0 = 0;
    field_1DC = 0;
    field_138 = 0;
    phaseObjectCount = 0;
    field_13C = 0;
    field_1C0 = 0;
    strafeMode = nullptr;
    cameraMode = nullptr;
    field_1E0 = -1;
    phase = 0;
    field_200 = field_1FD = field_1FE = false;
    field_1E4 = 0;
    field_1FF = false;
    field_1C8 = 1.0f;
    field_1CC = 5.0f;
    field_1D4 = 1;
    field_1BC = 1.0f;
    settled = 1;
    field_202 = field_203 = false;
    arena = entity::FindEntity(0x4E16D4BEu);
    static constexpr uint32_t kBars[4] = {0x392E4B66u, 0xF92BBACFu, 0xDD6C1E82u, 0xE573C33Au};
    for (int i = 0; i < 4; ++i)
        bars[i] = hud::CloneElement(hud::Find(kBars[i]), 0x821DC31Fu);
    state = 0;
    EnterState();
}

} // namespace ai
} // namespace levels
} // namespace sly
