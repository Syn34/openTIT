// openTIT - The combination safe (Safe / SafeDef, see ActorEntities.h).
//
// Each of the three dials is scrambled away from 0; cracking the safe means
// turning every dial back to 0 (within SafeDef::tolerance degrees) with the
// control drawn for it, before SafeDef::timeLimit runs out.
#include "sly/entity/ActorEntities.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/math/ApeMath.h"
#include "ape/sys/ApeGeometry.h"
#include "ape/world/ApeObj.h"
#include "sly/entity/Character.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/ActorHooks.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/game/GameScripts.h"
#include "sly/game/SlyGame.h"
#include "sly/game/hud/HudManager.h"
#include "sly/camera/CameraModes.h"
#include "sly/entity/GameCamera.h"
#include "sly/entity/PropEntities.h"
#include "sly/game/GameShellHooks.h"
#include "sly/game/AppRuntime.h"
#include "sly/objects/EntityPlacements.h"
#include "sly/tuning/TargetRangeTuning.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace sly {
namespace entity {

namespace {

constexpr float kDegToRad = 0.017453292f;
// Axis the dials turn about. NOTE: console global 0x01164B70, assumed +Z.
const ape::Vec3 kDialAxis = {0.0f, 0.0f, 1.0f, 0.0f};
// HUD element shown while the tilt control is active.
constexpr uint32_t kHud_SafeTilt = 0x569F957Fu;

// Three fresh samples of the game's random generator. NOTE: the console
// inlines the generator here and uses the value of each new step (one extra
// step follows), which is what the second to fourth RandomUnit calls return.
void DrawDialSamples(float samples[3])
{
    ape::RandomUnit(ape::g_gameRandom);
    for (int i = 0; i < 3; ++i)
        samples[i] = ape::RandomUnit(ape::g_gameRandom);
}

// Whether the generator's current seed is odd (read before drawing).
bool SeedIsOdd()
{
    return (ape::g_gameRandom.seed & 1u) != 0;
}

void ShowObject(ape::world::ApeObj* object, bool visible)
{
    if (object)
        object->SetVisible(visible, false);
}

// Hides a HUD element (its outro).
void HideHud(hud::HudObject* element)
{
    if (element)
        element->Show(true, false, false);
}

} // namespace

// @func 0x001E60E0 SafeDef::Spawn
Entity* SafeDef::Spawn(EntityPlacement* placement)
{
    objects::SafeInstanceDesc desc(InstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    auto* safe = new Safe(desc);
    safe->OnSpawn();
    return safe;
}

// @func 0x001E5CD8 Safe::Safe
// The first dial starts in 45..60 degrees with an odd seed, else in
// 290..330; the other two in 70..290.
Safe::Safe(const objects::SafeInstanceDesc& desc)
    : ActorEntity(desc), rewardDefName(desc.rewardDefName)
{
    for (int i = 0; i < 3; ++i)
        rewardOffset[i] = desc.rewardOffset[i];

    const SafeDef* safeDef = Def();
    const bool odd = SeedIsOdd();
    float samples[3];
    DrawDialSamples(samples);
    const float first = odd ? samples[0] * 15.0f + 45.0f : samples[0] * 40.0f + 290.0f;
    const float positions[3] = {first, samples[1] * 220.0f + 70.0f, samples[2] * 220.0f + 70.0f};
    for (int i = 0; i < 3; ++i)
        dialAngle[i] = dialTarget[i] = dialStart[i] = positions[i];

    timerHud = hud::Find(safeDef->timerHudName);
    promptHud = hud::Find(safeDef->promptHudName);
    tiltHud = hud::Find(kHud_SafeTilt);
    FindParts(object);

    const uint32_t bank = safeDef->soundBank;
    failSound = audio::CreateSoundName(safeDef->failSound, bank);
    openSound = audio::CreateSoundName(safeDef->openSound, bank);
    for (int i = 0; i < 3; ++i)
        tickSounds[i] = audio::CreateSoundName(safeDef->tickSounds[i], bank);
    for (int i = 0; i < 3; ++i)
        solvedSounds[i] = audio::CreateSoundName(safeDef->solvedSounds[i], bank);

    Reset(false);
    script = StartObjectScript(desc.openScript, nullptr, 0, false);
}

// @func 0x001E57E8 Safe::FindParts
// Depth first over the tree; the dials take their current angle at once.
void Safe::FindParts(ape::world::ApeObj* node)
{
    if (!node)
        return;
    const SafeDef* safeDef = Def();
    const uint32_t name = node->GetNameHash();
    bool found = false;
    for (int k = 0; k < 3 && !found; ++k) {
        if (name == safeDef->dialNames[k]) {
            const int dial = 2 - k;
            dials[dial] = node;
            PoseDial(dial, dialAngle[dial]);
            found = true;
        }
    }
    // NOTE: the console tests the solved and highlight names interleaved;
    // a name matching several entries takes the first in its order.
    static constexpr int kOrder[6][2] = {{1, 2}, {0, 2}, {1, 0}, {0, 0}, {1, 1}, {0, 1}};
    for (int i = 0; i < 6 && !found; ++i) {
        const bool solved = kOrder[i][0] != 0;
        const int k = kOrder[i][1];
        if (name == (solved ? safeDef->solvedNames[k] : safeDef->highlightNames[k])) {
            (solved ? solvedMarks : highlights)[2 - k] = node;
            node->SetVisible(false, false);
            found = true;
        }
    }
    if (!found && name == safeDef->openObjectName) {
        openObject = node;
        node->SetVisible(false, false);
    }
    for (ape::world::ApeObj* child = node->GetFirstChild(); child; child = child->GetNextSibling())
        FindParts(child);
}

void Safe::PoseDial(int index, float angle)
{
    ape::world::ApeObj* dial = dials[index];
    if (!dial)
        return;
    ape::Matrix34 local;
    ape::MatrixAxisAngle(angle * kDegToRad, local, kDialAxis);
    local.translation = dial->GetLocal().translation;
    dial->SetLocalTransform(local);
}

// @func 0x001E5398 Safe::Reset
// NOTE: the first dial's base is meant to depend on the seed like in the
// constructor, but the console compares (seed & 1) + 360 with 1, which never
// holds: it is always 300..330.
void Safe::Reset(bool failed)
{
    EntitySetState(this, 0, 0);
    const bool firstLow = float(ape::g_gameRandom.seed & 1u) + 360.0f == 1.0f;
    float samples[3];
    DrawDialSamples(samples);
    const float first = samples[0] * 30.0f + (firstLow ? 30.0f : 300.0f);
    const float positions[3] = {first, samples[1] * 220.0f + 70.0f - 360.0f, samples[2] * 220.0f + 70.0f + 360.0f};
    for (int i = 0; i < 3; ++i)
        dialTarget[i] = dialStart[i] = positions[i];
    timeLeft = 0.0f;

    for (int i = 0; i < 3; ++i) {
        ShowObject(highlights[i], false);
        ShowObject(solvedMarks[i], false);
    }
    ShowObject(openObject, false);

    if (timerHud) {
        // A running return-treasure microgame owns the timer element.
        if (failed && g_game->ReturnTreasureActive())
            g_game->RefreshReturnTreasureHud();
        else
            HideHud(timerHud);
    }
    HideHud(promptHud);
    HideHud(tiltHud);

    if (failed) {
        PlayEntitySound(failSound, nullptr, nullptr);
        if (EntityComponent* interaction = EntityInteraction(this)) {
            Entity* user = InteractionUser(interaction);
            if (user && user->HasTypeBit(3))
                static_cast<Character*>(user)->LeaveInteractState();
            InteractionStop(interaction);
        }
    }
    PlayClipByName(1.0f, 0.25f, 0.0f, "SafeA_Idle_Closed", 0, false);
}

// @func 0x001E70D0 Safe::Scramble
// Like Reset's draw, except that here the seed test works (the first dial
// lands in 30..60 with an odd seed, else in 300..330) and no full turns are
// added; the dials jump there and the marks hide.
void Safe::Scramble()
{
    const bool odd = SeedIsOdd();
    float samples[3];
    DrawDialSamples(samples);
    const float positions[3] = {samples[0] * 30.0f + (odd ? 30.0f : 300.0f), samples[1] * 220.0f + 70.0f,
                                samples[2] * 220.0f + 70.0f};
    for (int i = 0; i < 3; ++i) {
        dialAngle[i] = dialTarget[i] = positions[i];
        PoseDial(i, dialAngle[i]);
    }
    for (int i = 0; i < 3; ++i) {
        ShowObject(highlights[i], false);
        ShowObject(solvedMarks[i], false);
    }
    ShowObject(openObject, false);
}

// @func 0x001E77C8 Safe::UpdatePrompt
// The sticks use the prompt element (its text names the stick, set once it
// shows); the tilt control has an element of its own.
void Safe::UpdatePrompt()
{
    if (control == kControl_Tilt) {
        HideHud(promptHud);
        if (tiltHud && !(tiltHud->stateFlags & hud::HudObject::kState_Shown))
            tiltHud->ShowText(-1.0f, nullptr, true);
        return;
    }
    if (control < kControl_LeftStick || control > kControl_Tilt)
        return;
    HideHud(tiltHud);
    if (!promptHud)
        return;
    if (!(promptHud->stateFlags & hud::HudObject::kState_Shown)) {
        promptShown = true;
        promptHud->ShowText(-1.0f, nullptr, true);
    }
    if (promptPending && promptShown) {
        promptPending = false;
        // NOTE: the console also has the tilt text ("$BUTTON_TILT $STR_ROTATE")
        // here, unreachable for the stick controls.
        promptHud->SetFieldText(control == kControl_LeftStick ? "$LSTICK_LR_ANIM $STR_ROTATE"
                                                              : "$RSTICK_LR_ANIM $STR_ROTATE",
                                0);
    }
}

int32_t Safe::DrawControl()
{
    if (controlBag.Count() == 0)
        return -1;
    const int index = ape::RandomInt(ape::g_gameRandom, 0, int(controlBag.Count()) - 1);
    const int32_t picked = controlBag[size_t(index)];
    controlBag.RemoveAtSwap(size_t(index));
    return picked;
}

// @func 0x001E7588 Safe::ReadInput
// The sticks turn by their x axis; the tilt control by the x component of
// the average motion sample of the last quarter second (normalised),
// turning the other way.
void Safe::ReadInput(float& target)
{
    Entity* user = InteractionUser(EntityInteraction(this));
    if (!user)
        return;
    const float speed = Def()->turnSpeed;
    AppPlayer& pad = GetAppRuntime().players[g_game->players[user->PlayerSlot()].padIndex];
    switch (control) {
    case kControl_LeftStick:
        target += speed * pad.GetAnalog(0);
        break;
    case kControl_RightStick:
        target += speed * pad.GetAnalog(2);
        break;
    case kControl_Tilt: {
        static constexpr int kMaxSamples = 360;
        static ape::Vec3 samples[kMaxSamples];
        int count = 0;
        if (!AppPlayerMotionSamples(pad, 0.25f, samples, kMaxSamples, count) || count <= 0)
            return;
        ape::Vec3 sum = {0.0f, 0.0f, 0.0f, 0.0f};
        for (int i = 0; i < count; ++i) {
            sum.x += samples[i].x;
            sum.y += samples[i].y;
            sum.z += samples[i].z;
        }
        const float n = float(count);
        const float x = sum.x / n, y = sum.y / n, z = sum.z / n;
        const float length = std::sqrt(x * x + y * y + z * z);
        const float tilt = length > 1e-05f ? x / length : x;
        target -= speed * tilt;
        break;
    }
    default:
        break;
    }
}

// The reward stands at the safe object's local frame moved by the reward
// offset. TargetableC rewards (definition class bit 0x40) get a linked
// TargetableC placement; anything else a plain one.
void Safe::SpawnReward()
{
    EntityDef* rewardDef = rewardDefName ? FindEntityDef(rewardDefName) : nullptr;
    if (!rewardDef)
        return;
    ape::Matrix34 frame = object->GetLocal();
    const ape::Vec3 offset = {rewardOffset[0], rewardOffset[1], rewardOffset[2], 0.0f};
    frame.translation.x += frame.xAxis.x * offset.x + frame.yAxis.x * offset.y + frame.zAxis.x * offset.z;
    frame.translation.y += frame.xAxis.y * offset.x + frame.yAxis.y * offset.y + frame.zAxis.y * offset.z;
    frame.translation.z += frame.xAxis.z * offset.x + frame.yAxis.z * offset.y + frame.zAxis.z * offset.z;
    if (rewardDef->classBits & 0x40u) {
        objects::TargetableCLinkedPlacement placement(100.0f, "", rewardDef, nullptr, frame,
                                                      ape::world::kWorldUnitScale, true, false);
        placement.flags |= 0x60u;
        const auto* targetDef = static_cast<const TargetableDefC*>(rewardDef);
        placement.bits84 = (placement.bits84 & ~0x40000000u) | ((targetDef->hitFlags & 0x80000000u) >> 1);
        placement.field_11D = true;
        rewardDef->Spawn(&placement);
    } else {
        EntityPlacement placement("", rewardDef, nullptr, frame, ape::world::kWorldUnitScale, true, false, 0);
        rewardDef->Spawn(&placement);
    }
}

// @func 0x001E79A8 Safe::Update
bool Safe::Update(float dt)
{
    if (!ActorEntity::Update(dt))
        return false;
    const SafeDef* safeDef = Def();
    EntityComponent* interaction = EntityInteraction(this);
    if (interaction && InteractionUsers(interaction) == 2) {
        GameWorldQueueSafeCallback(this);
        switch (EntityGetState(this, 0)) {
        case 0:
            // Idle: once every dial has caught up, they get another full turn.
            if (std::fabs(dialTarget[0] - dialAngle[0]) < 1.0f && std::fabs(dialTarget[1] - dialAngle[1]) < 1.0f &&
                std::fabs(dialTarget[2] - dialAngle[2]) < 1.0f) {
                const float turns[3] = {dialStart[0] - 360.0f, dialStart[1] + 360.0f, dialStart[2] - 360.0f};
                for (int i = 0; i < 3; ++i)
                    dialAngle[i] = dialTarget[i] = dialStart[i] = turns[i];
            }
            break;
        case 1:
            if (!UpdateCracking(dt))
                return true;   // the time ran out and the safe was reset
            break;
        case 2:
            if (EntityStateTime(this, 0) > safeDef->openDelay) {
                PlayEntitySound(openSound, nullptr, nullptr);
                Scramble();
                Reset(false);
                SpawnReward();
                Entity* user = InteractionUser(interaction);
                if (user && user->HasTypeBit(3))
                    static_cast<Character*>(user)->LeaveInteractState();
                InteractionWithdraw(interaction);
            }
            break;
        default:
            break;
        }
    }
    for (int i = 0; i < 3; ++i) {
        dialAngle[i] += (dialTarget[i] - dialAngle[i]) * safeDef->dialSmoothing[i];
        PoseDial(i, dialAngle[i]);
    }
    return true;
}

// The cracking state of Update (inlined there on the console): the timer
// counts down (the timer element is shown again each whole second below
// Entity.Safe.CountDownBeepStartTime, which beeps), the active dial follows
// the input, ticking every 5 degrees, and a dial within the tolerance of 0
// is solved; the third opens the safe. False once the time ran out.
bool Safe::UpdateCracking(float dt)
{
    if (activeDial == -1 || !dials[activeDial])
        return true;
    const SafeDef* safeDef = Def();
    const float previousWhole = std::floor(timeLeft);
    timeLeft = std::max(timeLeft - dt, 0.0f);
    if (timerHud) {
        char text[32];
        std::snprintf(text, sizeof text, "%2.2f", double(timeLeft));
        if (!(timerHud->stateFlags & hud::HudObject::kState_Shown))
            timerHud->ShowText(-1.0f, text, true);
        else if (previousWhole <= g_safeCountDownBeepStartTime &&
                 std::fabs(previousWhole - std::floor(timeLeft)) >= 1e-05f)
            timerHud->ShowText(-1.0f, text, true);
        else
            hud::SetFieldText(timerHud, text, 0);
    }
    UpdatePrompt();
    if (timeLeft <= 0.0f) {
        PlayEntitySound(failSound, nullptr, nullptr);
        Reset(true);
        return false;
    }

    const int dial = activeDial;
    ShowObject(highlights[dial], true);
    ReadInput(dialTarget[dial]);
    const int target = int(dialTarget[dial]);
    if (target != int(dialAngle[dial]) && target % 5 == 0)
        PlayEntitySound(dial < 3 ? tickSounds[dial] : nullptr, nullptr, nullptr);
    if (dialTarget[dial] > 360.0f)
        dialTarget[dial] -= 360.0f;
    else if (dialTarget[dial] < -360.0f)
        dialTarget[dial] += 360.0f;
    dialAngle[dial] = dialTarget[dial];

    const float angle = dialAngle[dial];
    const float tolerance = safeDef->tolerance;
    const bool solved = (angle >= -tolerance && angle <= tolerance) ||
                        (angle >= 360.0f - tolerance && angle <= tolerance + 360.0f);
    if (!solved)
        return true;
    PlayEntitySound(solvedSounds[dial], nullptr, nullptr);
    ShowObject(solvedMarks[dial], true);
    ShowObject(highlights[dial], false);
    dialAngle[dial] = dialTarget[dial] = 0.0f;
    if (dial == 2) {
        EntitySetState(this, 0, 2);
        PlayClipByName(1.0f, 0.25f, 0.0f, "SafeA_Open_A", 0, false);
        if (script)
            script->SetStateAndUpdate(0x00251E4Au);
    } else {
        activeDial = dial + 1;
        control = DrawControl();
        promptPending = true;
    }
    return true;
}

// @func 0x001E8AB0 Safe::InteractionBegin
// The camera looks along the safe's -z with the world's up, standing at the
// camera offset taken along -x, up and -z of the safe. The return-treasure
// microgame pauses (its HUD off) and the AI entities hide while cracking.
bool Safe::InteractionBegin(Entity* /*user*/)
{
    const SafeDef* safeDef = Def();
    GameCamera* camera = MainGameCamera();
    Entity* user = InteractionUser(EntityInteraction(this));
    if (camera) {
        const ape::Vec3 x = {-transform.xAxis.x, -transform.xAxis.y, -transform.xAxis.z, 0.0f};
        const ape::Vec3 up = {0.0f, 1.0f, 0.0f, 0.0f};
        const ape::Vec3 z = {-transform.zAxis.x, -transform.zAxis.y, -transform.zAxis.z, 0.0f};
        const ape::Vec3& offset = safeDef->cameraOffset;
        auto* mode = new camera::FixedCameraMode(camera);
        mode->transform.xAxis = x;
        mode->transform.yAxis = up;
        mode->transform.zAxis = z;
        mode->transform.translation = {transform.translation.x + x.x * offset.x + up.x * offset.y + z.x * offset.z,
                                       transform.translation.y + x.y * offset.x + up.y * offset.y + z.y * offset.z,
                                       transform.translation.z + x.z * offset.x + up.z * offset.y + z.z * offset.z,
                                       0.0f};
        mode->fov = safeDef->cameraFov;
        mode->dofIterations = 1;
        cameraMode = mode;
        camera->PushMode(safeDef->cameraBlend, mode, false);
    }
    if (user)
        user->SetVisible(false, true, true, true);
    g_game->SetReturnTreasureHudActive(false);
    g_game->PauseReturnTreasure(true);
    if (timerHud)
        timerHud->ShowText(-1.0f, "", true);

    controlBag.Clear();
    for (int32_t c = kControl_LeftStick; c <= kControl_Tilt; ++c)
        controlBag.PushBack(c);
    activeDial = 0;
    timeLeft = safeDef->timeLimit;
    control = DrawControl();
    promptPending = true;
    promptShown = false;
    SetAiEntitiesVisible(false);
    EntitySetState(this, 0, 1);
    return true;
}

} // namespace entity
} // namespace sly
