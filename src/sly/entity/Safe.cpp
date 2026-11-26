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
#include "sly/objects/EntityPlacements.h"

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

} // namespace entity
} // namespace sly
