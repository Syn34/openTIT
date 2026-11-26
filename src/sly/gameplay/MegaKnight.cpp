// openTIT - Mega knight boss entity (0x0030C018-0x0031158C).
#include "sly/gameplay/MegaKnight.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/math/ApeBounds.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeRegions.h"
#include "sly/game/SpawnManager.h"
#include "sly/gameplay/GameplayWorld.h"
#include "sly/gameplay/MegaKnightHooks.h"
#include "sly/tuning/MegaKnightDragonTuning.h"

#include <cmath>

namespace sly {
namespace gameplay {

namespace {
// Marker record values of the boss's marked targets.
constexpr uint32_t kMarkerType_Boss = 0x7DDC7E2Du;
} // namespace

// ===========================================================================
// Object pairs

void MegaKnightObjectPair::Apply(int index, bool on)
{
    if (on == shown[index])
        return;
    shown[index] = on;
    if (entity::Entity* object = objects[index]) {
        object->SetVisible(on, true, true, true);
        object->SetFlag80(!on, -1);
    }
}

// @func 0x0030C018 MegaKnightObjectPair::SetShown
void MegaKnightObjectPair::SetShown(bool on)
{
    Apply(1, on);
    Apply(0, on);
}

// @func 0x0030C14C MegaKnightObjectPair::Show
void MegaKnightObjectPair::Show(bool on, bool first)
{
    if (!on) {
        SetShown(false);
        return;
    }
    Apply(first ? 0 : 1, true);
}


// ===========================================================================
// Platform

namespace {
constexpr uint32_t kGeom_Platform = 0x6F3D51EAu;
constexpr uint32_t kScript_PlatformA = 0x454F68A7u;
constexpr uint32_t kScript_PlatformB = 0x048D2280u;
constexpr uint32_t kScriptVar_Object = 0x0027469Eu;
constexpr uint32_t kScriptState_Spawn = 0x04B6FB5Bu;     // "Spawn"
constexpr uint32_t kScriptState_PlatformOff = 0x4A5CA6A4u;
// States of the boss's platform script (+0x3AC).
constexpr uint32_t kBossPlatform_Idle = 0x0590D9BBu;
constexpr uint32_t kBossPlatform_Full = 0xD6E3A3F3u;

// A script instance bound to `object` through the object variable 0x0027469E.
ape::ApeAnimScriptInstance* StartObjectScript(uint32_t scriptName, ape::world::ApeObj* object)
{
    ape::ApeAnimScript* found = ape::ApeAnimScriptLibrary::FindScriptAnywhere(scriptName, 0, true);
    if (!found)
        return nullptr;
    // NOTE: the console also clears bit 9 of the instance's flag word.
    auto* instance = new ape::ApeAnimScriptInstance(found);
    if (ape::ApeAnimScriptVar* var = instance->FindVar(kScriptVar_Object)) {
        if (var->GetType() == ape::AnimScriptVarType::Object)
            static_cast<ape::ApeAnimScriptObjectVar*>(var)->SetObject(MKObjectScriptRef(object));
    }
    instance->SetStateAndUpdate(kScriptState_Spawn);
    return instance;
}

void SetScriptStateOnce(ape::ApeAnimScriptInstance* instance, uint32_t state)
{
    if (!instance)
        return;
    const uint32_t current = instance->GetState() ? instance->GetState()->GetNameHash() : 0;
    if (current != state)
        instance->SetStateAndUpdate(state);
}
} // namespace

// @func 0x00303D38 MegaKnightPlatform::MegaKnightPlatform
MegaKnightPlatform::MegaKnightPlatform(MegaKnight* owner)
    : boss(owner)
{
    const ape::Vec3 one = {1.0f, 1.0f, 1.0f, 0.0f};   // console global 0x01164B30, NOTE: assumed (1,1,1)
    position = one;
    field_30 = one;
    object = MKGeometryObject(kGeom_Platform);
    if (object) {
        object->SetVisible(true, true);
        ape::Aabb box;
        box.min = {3.4028235e38f, 3.4028235e38f, 3.4028235e38f, 0.0f};
        box.max = {-3.4028235e38f, -3.4028235e38f, -3.4028235e38f, 0.0f};
        ape::world::GetObjectWorldBounds(*object, box);
        height = std::fabs(box.min.y) + std::fabs(box.max.y);
        if (height <= 0.0f)
            height = 1.0f;
    }
    scriptA = StartObjectScript(kScript_PlatformA, object);
    scriptB = StartObjectScript(kScript_PlatformB, object);
}

MegaKnightPlatform::~MegaKnightPlatform()
{
    if (object) {
        object->Release();
        object = nullptr;
    }
    delete scriptB;
    scriptB = nullptr;
    delete scriptA;
    scriptA = nullptr;
}

// @func 0x00308F5C MegaKnightPlatform::Reset
void MegaKnightPlatform::Reset(bool full)
{
    field_40 = 0.0f;
    if (scriptB)
        scriptB->SetStateAndUpdate(kScriptState_PlatformOff);
    if (scriptA)
        scriptA->SetStateAndUpdate(kScriptState_PlatformOff);
    SetScriptStateOnce(boss->script3AC, full ? kBossPlatform_Full : kBossPlatform_Idle);
    if (full) {
        field_68 = false;
        up = {0.0f, 1.0f, 0.0f, 0.0f};   // console global 0x01164B60
        field_4C = 0.0f;
    }
    if (object) {
        object->SetVisible(full, true);
        if (!full) {
            if (scriptB)
                scriptB->SetStateAndUpdate(kScriptState_PlatformOff);
            if (scriptA)
                scriptA->SetStateAndUpdate(kScriptState_PlatformOff);
        }
    }
    field_69 = false;
}

// ===========================================================================
// The boss

// @func 0x0030CB10 MegaKnight::~MegaKnight
// @func 0x0030CC8C MegaKnight::~MegaKnight (deleting)
// @func 0x0030CD94 MegaKnight::~MegaKnight (thunk, +0x17C)
// @func 0x0030CD20 MegaKnight::~MegaKnight (deleting thunk, +0x17C)
// @func 0x0030CD6C MegaKnight::~MegaKnight (thunk, +0x180)
// @func 0x0030CCD4 MegaKnight::~MegaKnight (deleting thunk, +0x180)
// The members release their storage; the helper objects went in Release.
MegaKnight::~MegaKnight() = default;

// @func 0x0030CDD0 MegaKnight::Release
bool MegaKnight::Release(bool kill)
{
    if (kill) {
        if (cameraController) {
            MKDeleteCameraController(cameraController);
            cameraController = nullptr;
        }
        for (MegaKnightPhase*& phase : phases) {
            delete phase;
            phase = nullptr;
        }
        ClearMarkedTargets();
        if (stampManager) {
            MKDeleteStampManager(stampManager);
            stampManager = nullptr;
        }
        delete platform;
        platform = nullptr;
        for (uint32_t i = 0; i < spawned.Count(); ++i) {
            entity::Entity* e = spawned[i];
            e->Release(false);
            if (e->IsAlive())
                e->Release(true);
        }
        spawned.Clear();
        if (entity::Entity* player = MKPlayerCharacter())
            MKClearPlayerTargetingFlag(player);
        delete script3AC;
        script3AC = nullptr;
        for (ape::ApeAnimScriptInstance*& s : scripts3B0) {
            delete s;
            s = nullptr;
        }
        MKClearCharacterHooks(this);
        MKStopListening(this);
    }
    return ActorEntity::Release(kill);
}

// @func 0x00311578 MegaKnight::OnHitCategory0
void MegaKnight::OnHitCategory0(entity::EntityHitRecord& hit) { (void)hit; }
// @func 0x00311580 MegaKnight::OnHitCategory1
void MegaKnight::OnHitCategory1(entity::EntityHitRecord& hit) { (void)hit; }
// @func 0x00311584 MegaKnight::OnHitCategory2
void MegaKnight::OnHitCategory2(entity::EntityHitRecord& hit) { (void)hit; }
// @func 0x00311588 MegaKnight::OnHitCategory3
void MegaKnight::OnHitCategory3(entity::EntityHitRecord& hit) { (void)hit; }
// @func 0x0031157C MegaKnight::OnHitCategory4
void MegaKnight::OnHitCategory4(entity::EntityHitRecord& hit) { (void)hit; }

// @func 0x00311570 MegaKnight::Interface17CSlot5
// @func 0x00311574 MegaKnight::Interface17CSlot5 (thunk, +0x17C)
void MegaKnight::Interface17CSlot5(const void* event) { (void)event; }

int MegaKnight::CurrentPhaseIndex() const
{
    return phaseIndex;
}

MegaKnightPhase* MegaKnight::CurrentPhase() const
{
    const uint32_t index = uint32_t(CurrentPhaseIndex());
    return index < kMKPhase_Count ? phases[index] : nullptr;
}

// @func 0x0030D2B8 MegaKnight::SwitchPhase
void MegaKnight::SwitchPhase(uint32_t phase)
{
    if (phase == kMKPhase_None || g_megaKnightStateLocked.Get() || phase >= kMKPhase_Count || !phases[phase])
        return;
    if (MegaKnightPhase* current = CurrentPhase())
        current->OnExit(phase);
    phaseIndex = int32_t(phase & 0xFF);
    phases[phase]->OnEnter();
}

// @func 0x0030932C MegaKnight::PhaseEventA
void MegaKnight::PhaseEventA()
{
    if (MegaKnightPhase* current = CurrentPhase())
        current->OnEventA();
    gameplay::ShowButtonHint(0xEB4F48A0u);
}

// @func 0x0030C3F0 MegaKnight::AddMarkedTarget
void MegaKnight::AddMarkedTarget(entity::Entity* target, const ape::Vec3& offset)
{
    if (!target)
        return;
    for (uint32_t i = 0; i < markedTargets.Count(); ++i)
        if (markedTargets[i] == target)
            return;
    markedTargets.PushBack(target);
    if (entity::Entity* player = MKPlayerCharacter())
        MKAddTargetMarker(player, target, offset);
}

// @func 0x0030C2B4 MegaKnight::RemoveMarkedTarget
void MegaKnight::RemoveMarkedTarget(entity::Entity* target)
{
    if (!target)
        return;
    for (uint32_t i = 0; i < markedTargets.Count(); ++i) {
        if (markedTargets[i] == target) {
            markedTargets.RemoveAtSwap(i);
            break;
        }
    }
    if (entity::Entity* player = MKPlayerCharacter())
        MKRemoveTargetMarker(player, target);
}

void MegaKnight::ClearMarkedTargets()
{
    while (markedTargets.Count() != 0)
        RemoveMarkedTarget(markedTargets[0]);
}

// @func 0x0030DBF0 MegaKnight::SetCamera
void MegaKnight::SetCamera(float ease, uint32_t name, bool force)
{
    if (!tracking.fightActive.Get() && !force)
        return;
    if (name == currentCamera)
        return;
    const MegaKnightCameraDef* camera = nullptr;
    if (name != 0) {
        const MegaKnightDef* knightDef = static_cast<const MegaKnightDef*>(def);
        for (uint32_t i = 0; i < knightDef->cameras.Count(); ++i) {
            if (knightDef->cameras[i]->name == name) {
                camera = knightDef->cameras[i];
                break;
            }
        }
    }
    if (ease < 0.0f && camera)
        ease = camera->easeTime;
    if (cameraController) {
        MKCameraControllerSet(cameraController, camera, ease);
        currentCamera = name;
    }
}

void MegaKnight::PlayAnim(uint32_t clip, float speedScale, bool loop)
{
    if (clip == currentAnim)
        return;
    const float blend = skipBlend ? 0.0f : g_megaKnightAnimBlend.Get();
    MKPlayClip(this, clip, speedScale * g_megaKnightAnimSpeedMultiplier.Get(), blend, loop);
    skipBlend = false;
    currentAnim = clip;
}

void MegaKnight::BlendGlowTo(float target)
{
    glow.to = target;
    glow.from = glow.value;
    glow.elapsed = 0.0f;
    glow.SetDuration(std::fabs(target - glow.value) * 0.5f);
}

void MegaKnight::ResetRopes()
{
    for (entity::Entity* rope : ropes) {
        if (rope) {
            MKRopeReset(rope);
            rope->SetVisible(true, false, false, false);
        }
    }
    tracking.ropesHeld.Force(0);
}

// @func 0x00304298 MegaKnight::ClearArmMarkers
void MegaKnight::ClearArmMarkers()
{
    for (uint32_t arm = 0; arm < 3; ++arm) {
        if (armSpawners[0][arm])
            armSpawners[0][arm]->Deactivate(true);
        if (armSpawners[1][arm])
            armSpawners[1][arm]->Deactivate(true);
    }
    if (armObject)
        retiredArmObjects.PushBack(armObject);
    armObject = nullptr;
    while (retiredArmObjects.Count() != 0) {
        entity::Entity* object = retiredArmObjects[retiredArmObjects.Count() - 1];
        object->Release(false);
        if (object->IsAlive())
            object->Release(true);
        retiredArmObjects.PopBack();
    }
}

// TODO(0x0030E1E0 MegaKnight::ChooseNextPhase): runs the behaviour lists
// (camera, stamp and move actions) and returns the phase they ask for.
uint32_t MegaKnight::ChooseNextPhase()
{
    return kMKPhase_Idle;
}

// TODO(0x00311590 MegaKnight::SetArmMarker): swaps the marker object of an
// arm (spawned through 0x00307CC8).
void MegaKnight::SetArmMarker(uint32_t arm, bool on)
{
    (void)arm;
    (void)on;
}

} // namespace gameplay
} // namespace sly
