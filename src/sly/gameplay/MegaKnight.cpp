// openTIT - Mega knight boss entity (0x0030C018-0x0031158C).
#include "sly/gameplay/MegaKnight.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/math/ApeBounds.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeRegions.h"
#include "ape/world/ApeSkeleton.h"
#include "sly/entity/ScriptedEntity.h"
#include "sly/camera/CameraModes.h"
#include "sly/entity/GameCamera.h"
#include "sly/game/GameEvent.h"
#include "sly/game/SpawnManager.h"
#include "sly/levels/LevelServices.h"
#include "sly/gameplay/GameplayWorld.h"
#include "sly/gameplay/MegaKnightHooks.h"
#include "sly/tuning/MegaKnightDragonTuning.h"

#include <cmath>
#include <cstring>

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
// Floor ripple

// @func 0x0030419C MegaKnightFloorRipple::Init
void MegaKnightFloorRipple::Init(MegaKnight* owner, uint32_t entityName)
{
    boss = owner;
    if (entity::Entity* found = entity::FindEntity(entityName))
        entity = (found->classBits & 0x4) ? found : nullptr;
    if (!entity || !entity->object)
        return;
    for (ape::world::ApeObj* ring = entity->object->GetFirstChild(); ring; ring = ring->GetNextSibling())
        rings.PushBack(ring);
}

// @func 0x0030C5C8 MegaKnightFloorRipple::HeightAt
// The rings are sorted outwards: the first one within 1.5 units of the
// target's distance from the centre decides.
float MegaKnightFloorRipple::HeightAt(const entity::Entity* target) const
{
    if (!target)
        return 0.0f;
    const ape::Vec3& p = target->transform.translation;
    if (p.x == 0.0f && p.y == 0.0f && p.z == 0.0f)
        return 0.0f;
    const float radius = std::sqrt(p.x * p.x + p.z * p.z);
    for (uint32_t i = 0; i < rings.Count(); ++i) {
        const ape::Vec3& r = rings[i]->GetWorld().translation;
        const float ringRadius = std::sqrt(r.x * r.x + r.y * r.y + r.z * r.z);
        if (std::fabs(ringRadius - radius) <= 1.5f)
            return r.y < p.y ? 0.0f : r.y;
        if (radius < ringRadius)
            return 0.0f;
    }
    return 0.0f;
}

// @func 0x0030C6F8 MegaKnightFloorRipple::Update
void MegaKnightFloorRipple::Update(float dt)
{
    static const ape::Vec3 kUp = {0.0f, 1.0f, 0.0f, 0.0f};   // console global 0x01164B60
    if (!boss || !entity)
        return;
    cooldown -= dt;
    if (cooldown < 0.0f)
        cooldown = 0.0f;

    ape::ApeTArray<entity::Entity*>& spawned = boss->spawned;
    for (int i = 0; i < int(spawned.Count()); ++i) {
        entity::Entity* e = spawned[uint32_t(i)];
        if (HeightAt(e) <= 0.0f)
            continue;
        e->Release(false);
        if (e && (e->classBits & entity::kClass_20))
            MKRippleKnock(e);
        auto* scripted = (e->classBits & 0x2) ? static_cast<entity::ScriptedEntity*>(e) : nullptr;
        scripted->Damage(scripted->maxHealth, nullptr, 0);
        spawned.RemoveAtSwap(uint32_t(i));
        --i;
    }

    entity::Entity* player = MKPlayerCharacter();
    const float height = HeightAt(player);
    if (height <= 0.0f || cooldown != 0.0f)
        return;
    float t = height / g_megaKnightFloorRippleMaxHeight.Get();
    if (1.0f - t < 0.0f)
        t = 1.0f;
    if (t < 0.0f)
        t = 0.0f;
    const float impulse = g_megaKnightFloorRippleImpulseStr.Get() * t;
    if (player && player->IsAlive() && !static_cast<entity::ScriptedEntity*>(player)->IsDead() &&
        g_megaKnightFloorRippleDamageEnabled.Get()) {
        boss->HurtPlayer(boss, 0, g_megaKnightFloorRippleDamage.Get(), dt, kUp, impulse);
        cooldown = g_megaKnightFloorRippleHitCooldown.Get();
    }
}

// ===========================================================================
// Camera controller

// @func 0x00305828 MegaKnightCamera::MegaKnightCamera
MegaKnightCamera::MegaKnightCamera(MegaKnight* owner, entity::GameCamera* camera,
                                   const MegaKnightCameraDef* defaultDef)
    : boss(owner), gameCamera(camera), defaultCamera(defaultDef)
{
    hitFraction.Set(1.0f, 1.0f, g_megaKnightCameraHitFractionSpeed.Get());
    SetCamera(0.0f, nullptr, true);
    auto* fixed = new camera::FixedCameraMode(camera);
    fixed->transform = camera->transform;
    fixed->fov = camera->fov;
    fixed->dofEnabled = false;
    fixed->dofIterations = 1;
    mode = fixed;
}

MegaKnightCamera::~MegaKnightCamera()
{
    if (mode) {
        MKCameraRemoveMode(gameCamera, mode);
        delete mode;
        mode = nullptr;
    }
}

// @func 0x003055E8 MegaKnightCamera::SetCamera
void MegaKnightCamera::SetCamera(float ease, const MegaKnightCameraDef* def, bool immediate)
{
    if (!def)
        def = defaultCamera;
    if (ease < 0.0f)
        ease = 0.0f;
    if (def) {
        std::strncpy(cameraName, def->nameText, sizeof(cameraName) - 1);
        cameraName[sizeof(cameraName) - 1] = '\0';
        const float targets[4] = {def->values[0], def->values[1], def->values[2], 0.0f};
        for (int i = 0; i < 4; ++i) {
            FloatTween& t = blends[i];
            if (!immediate && ease != 0.0f) {
                t.from = t.value;
                t.to = targets[i];
                t.elapsed = 0.0f;
                t.SetDuration(ease);
            } else {
                // Value 3 starts at the camera's fourth value and decays.
                t.from = i == 3 ? def->values[3] : targets[i];
                t.to = targets[i];
                t.elapsed = 0.0f;
                t.SetDuration(0.0f);
            }
            t.easing = kEase_Cosine;
        }
    } else {
        cameraName[0] = '\0';
    }
    Update(0.0f);
}

// TODO(0x00304FD0 MegaKnightCamera::Update): blends the four values and
// frames the boss and the player (vector code the disassembler cannot
// follow).
void MegaKnightCamera::Update(float dt)
{
    for (FloatTween& t : blends)
        t.Update(dt);
}

// ===========================================================================
// Stamp manager

// @func 0x00304D94 MegaKnightStampManager::MegaKnightStampManager
MegaKnightStampManager::MegaKnightStampManager(MegaKnight* owner)
    : boss(owner), landed(0x200), pending(0)
{
    leadDist = g_megaKnightMissileLeadDist.Get();
    entity::EntityDef* def = entity::FindEntityDef(0xD4A3F67Au);
    if (def && (def->optionFlags & 0x2000))
        stampDef = def;
    def = entity::FindEntityDef(0xB267EAFBu);
    if (def && (def->optionFlags & 0x2000))
        stampDefB = def;
    effectDef = entity::FindEntityDef(0x8F7C1D39u);
}

MegaKnightStampManager::~MegaKnightStampManager() = default;

// @func 0x0030B4AC MegaKnightStampManager::Clear
void MegaKnightStampManager::Clear()
{
    for (uint32_t i = 0; i < landed.Count(); ++i) {
        MegaKnightStampRecord* stamp = landed[i];
        if (!stamp)
            continue;
        if (entity::Entity* e = stamp->entity) {
            e->Release(false);
            if (e->IsAlive())
                e->Release(true);
            stamp->entity = nullptr;
        }
        if (entity::Entity* e = stamp->effect) {
            e->Release(false);
            e->Release(true);
            stamp->effect = nullptr;
        }
        delete stamp;
    }
    ClearPending();
    landed.Clear();
}

void MegaKnightStampManager::Reset()
{
    field_30 = 0;
    falling = 0;
    field_60 = 0;
    Clear();
}

void MegaKnightStampManager::ClearPending()
{
    for (uint32_t i = 0; i < pending.Count(); ++i)
        delete pending[i];
    pending.Clear();
}

bool MegaKnightStampManager::Settled(uint32_t count) const
{
    return falling == 0 && count >= landed.Count() && pending.Count() == 0;
}

// @func 0x0030BE48 MegaKnightStampManager::QueueStamp
void MegaKnightStampManager::QueueStamp(float delay, float duration, uint32_t kind, const ape::Matrix34& transform,
                                        uint32_t field60, bool flag)
{
    MegaKnightPendingStamp* p = new MegaKnightPendingStamp();
    p->owner = this;
    p->kind = kind;
    p->transform = transform;
    p->timer = 0.0f;
    p->delay = delay <= 0.0f ? 1.0f : delay;
    p->duration = duration <= 0.0f ? 1.0f : duration;
    p->field_5C = 0.0f;
    p->field_60 = field60;
    p->flag = flag;
    pending.PushBack(p);
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
        delete cameraController;
        cameraController = nullptr;
        for (MegaKnightPhase*& phase : phases) {
            delete phase;
            phase = nullptr;
        }
        ClearMarkedTargets();
        if (stampManager) {
            stampManager->Clear();
            delete stampManager;
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
        cameraController->SetCamera(ease, camera, false);
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

// @func 0x0030FB94 MegaKnight::PostUpdate
bool MegaKnight::PostUpdate(float dt)
{
    return ActorEntity::PostUpdate(dt);
}

// @func 0x0030EBF0 MegaKnight::UpdatePhases
bool MegaKnight::UpdatePhases(float dt)
{
    if (CurrentPhaseIndex() != kMKPhase_None) {
        if (MegaKnightPhase* phase = CurrentPhase()) {
            phase->Update(dt);
            if (phase == CurrentPhase() && phase->IsDone()) {
                skipBlend = true;
                SwitchPhase(ChooseNextPhase());
            }
        }
    }
    // Phases that are not running keep playing their "Done" / "Spawn" states.
    for (uint32_t i = 0; i < kMKPhase_Count; ++i) {
        MegaKnightPhase* phase = phases[i];
        if (!phase || int(i) == CurrentPhaseIndex())
            continue;
        ape::ApeAnimScriptInstance* script = phase->script;
        if (script && script->GetState()) {
            const uint32_t state = script->GetState()->GetNameHash();
            if (state == 0x00201B82u /* "Done" */ || state == 0x04B6FB5Bu /* "Spawn" */)
                script->Update(dt);
        }
    }
    if (!resetRequested)
        return true;
    if (!secondHalf)
        ResetFight();
    else
        RestartFight();
    return false;
}

namespace {

// The arena part both resets share (inline twice on the console).
void ResetArena(MegaKnight* boss, uint32_t eventName)
{
    MegaKnightPlatform* platform = boss->platform;
    platform->field_40 = 0.0f;
    platform->Reset(false);
    boss->stampManager->Reset();
    for (uint32_t i = 0; i < boss->spawned.Count(); ++i) {
        entity::Entity* e = boss->spawned[i];
        e->Release(false);
        if (e->IsAlive())
            e->Release(true);
    }
    boss->spawned.Clear();
    boss->ResetRopes();
    for (entity::Entity* rope : boss->ropes)
        if (rope)
            MKRopeRelease(rope);
    boss->ClearArmMarkers();
    if (eventName == 0x5850F805u) {
        for (entity::Entity* rope : boss->ropes)
            if (rope)
                MKSetInteractionState(rope, 4);
    }
    boss->field_298 = false;
    while (boss->markedTargets.Count() != 0)
        boss->RemoveMarkedTarget(boss->markedTargets[0]);
    boss->heartTarget->SetVisible(false, true, true, true);
    if (MegaKnightCamera* camera = boss->cameraController) {
        camera->SetCamera(0.0f, nullptr, true);
        MKCameraRemoveMode(camera->gameCamera, camera->mode);
    }
    GameEvent event;
    InitEvent1C(event, eventName, 0, 0);
    levels::GameWorldPostEvent(0, event);
}

} // namespace

// @func 0x0030D8E0 MegaKnight::ResetFight
void MegaKnight::ResetFight()
{
    ResetArena(this, 0x58516C64u);
}

// @func 0x0030DD50 MegaKnight::RestartFight
void MegaKnight::RestartFight()
{
    for (uint32_t i = 0; i < 5; ++i) {
        if (entity::Entity* e = entity::FindEntity(0x7001C018u + i)) {
            e->SetVisible(false, true, true, true);
            e->Release(true);
        }
    }
    if (MegaKnightPhase* idle = phases[kMKPhase_Idle]) {
        SwitchPhase(kMKPhase_Idle);
        static_cast<MKPhaseIdle*>(idle)->duration = g_megaKnightStartIdle.Get();
        SetCamera(1.0f, 0, false);
    }
    ResetArena(this, 0x5850F805u);
}

// @func 0x003059EC MegaKnight::BindTracking
void MegaKnight::BindTracking()
{
    tracking.camera = MKGameCamera();
    tracking.bone = FindBone(0x0021C5E0u);
}

// @func 0x0030D434 MegaKnight::BoneWorldTransform
void MegaKnight::BoneWorldTransform(int bone, ape::Matrix34& out) const
{
    const ape::world::ApeSkeletonBone* b = GetBone(bone);
    if (!b)
        return;
    ape::Matrix34 model = *b->model;
    ape::world::Matrix34Mul(out, model, object->GetWorld());
}

// @func 0x0030FC90 MegaKnight::SelectBehavior
void MegaKnight::SelectBehavior()
{
    const uint32_t base = secondHalf ? 0x0993D8AAu    // "Phase1Arm0"
                                     : 0x09A1F02Bu;   // "Phase2Arm0"
    const uint32_t armsDown = (armIntact[0] ? 1u : 0u) + (armIntact[2] ? 1u : 0u);
    behavior = nullptr;
    const ape::ApeTArray<MegaKnightBehavior*>& list = static_cast<const MegaKnightDef*>(def)->behaviors;
    for (uint32_t i = 0; i < list.Count(); ++i) {
        if (list[i]->nameHash == base + armsDown) {
            behavior = list[i];
            break;
        }
    }
    behaviorAction = 0;
}

// TODO(0x00309440 MegaKnight::HurtPlayer): damage, knock-back and hit
// reaction of the player (vector code).
void MegaKnight::HurtPlayer(entity::Entity* source, uint32_t flags, float damage, float dt,
                            const ape::Vec3& direction, float impulse)
{
    (void)source; (void)flags; (void)damage; (void)dt; (void)direction; (void)impulse;
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
