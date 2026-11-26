// openTIT - Security lasers (0x00398E64-0x003A233C).
#include "sly/gameplay/Lasers.h"

#include "agl/AglRenderTarget.h"
#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/core/ApeString.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/sys/ApeGeometry.h"
#include "ape/setting/ApeSetting.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeObj.h"
#include "sly/entity/Entity.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/game/AudioSystem.h"
#include "sly/game/GameMath.h"
#include "sly/game/SAISettings.h"
#include "sly/game/SlyGame.h"
#include "sly/levels/nav/NavGraph.h"
#include "sly/tuning/LaserFieldTuning.h"

#include <cfloat>
#include <cmath>
#include <new>

namespace sly {

namespace {

LaserManager* g_laserManager = nullptr;   // 0x00F657FC

// Script states.
constexpr uint32_t kScript_End = 0x00010CBBu;          // sent before a script is let go
constexpr uint32_t kScript_PointStart = 0x7A281983u;   // direction scripts
constexpr uint32_t kScriptVar_Object = 0x0027469Eu;    // object variable of the direction scripts
constexpr uint32_t kJoint_FlickerStart = 0x942B571Cu;
constexpr uint32_t kLifetime_FlickerStart = 0x066F8774u;
constexpr uint32_t kLifetime_On = 0xCADB1721u;
constexpr uint32_t kLifetime_Off = 0x3ECC2A7Cu;
constexpr uint32_t kJoint_Disabled = 0xF13F8BB2u;
constexpr uint32_t kJoint_DisabledAgain = 0xA86F8ED1u;
constexpr uint32_t kTriggerTypeBit = 0x00400000u;      // NOTE: entity type bit of the trigger classes
constexpr float kFirstScriptStep = 0.0177f;            // NOTE: 0x3F9119CE as a float

// Lets a script instance run out on its own: the end state, then the
// instance manager deletes it once finished.
void ReleaseScript(ape::ApeAnimScriptInstance* script)
{
    script->SetState(kScript_End);
    script->SetRemoveWhenFinished(true);
    ape::ApeAnimScriptInstance::Register(script);
}

ape::ApeAnimScriptInstance* EntityScript(entity::Entity* entity)
{
    return entity ? static_cast<ape::ApeAnimScriptInstance*>(entity->GetAnimScript()) : nullptr;
}

uint32_t ObjectName(ape::world::ApeObj* object)
{
    const uint32_t hash = object->GetNameHash();
    return hash ? hash : ape::HashNameNoCase(object->GetName());
}

void TagObject(ape::world::ApeObj* object, uint32_t tag, void* user)
{
    object->field_114 = reinterpret_cast<void*>(uintptr_t(tag));
    object->field_118 = user;
}

// Takes the next free element of a pool (-1 when it is full).
template <class T>
int16_t AllocSlot(ape::ApeTFixedArray<T>& pool)
{
    const uint16_t used = pool.Used();
    if (used >= pool.Capacity())
        return -1;
    if (pool.Handles())
        pool.Handles()[used] = 0;
    pool.SetUsed(uint16_t(used + 1));
    return int16_t(used);
}

// The sphere is inside every plane of the view frustum (planes stored
// transposed, four per group).
bool SphereInFrustum(const agl::AglFrustum& frustum, const ape::Sphere& sphere)
{
    for (int group = 0; group < 2; ++group) {
        const ape::Vec4* soa = &frustum.soa[group * 4];
        const float dist[4] = {
            soa[0].x * sphere.x + soa[1].x * sphere.y + soa[2].x * sphere.z + soa[3].x,
            soa[0].y * sphere.x + soa[1].y * sphere.y + soa[2].y * sphere.z + soa[3].y,
            soa[0].z * sphere.x + soa[1].z * sphere.y + soa[2].z * sphere.z + soa[3].z,
            soa[0].w * sphere.x + soa[1].w * sphere.y + soa[2].w * sphere.z + soa[3].w,
        };
        for (float d : dist)
            if (!(sphere.w + d >= 0.0f))
                return false;
    }
    return true;
}

// Point instance of the loaded level by name.
ape::world::ApeObj* FindPoint(uint32_t nameHash)
{
    return ape::world::ApeTInstanceList<ape::world::ApePointInstance>::Find(nameHash);
}

// The world object an object script variable refers to. NOTE: the port's
// script variables hold engine references, not scene objects; until the
// two are unified there is none.
ape::world::ApeObj* ScriptVarObject(ape::ApeAnimScriptObjectVar* var)
{
    (void)var;
    return nullptr;
}

} // namespace

// @func 0x00399AB0 ComputeLaserBeamFrame
// Frame of a beam geometry `geometryLength` long (its half extent along y)
// stretched over `length` from `start` along `dir` (unit): y follows the
// beam, x is perpendicular to it and to z (or to y for vertical beams), and
// the geometry is scaled along y.
void ComputeLaserBeamFrame(float length, float geometryLength, ape::Matrix34& frame, ape::Vec3& scale,
                           const ape::Vec3& start, const ape::Vec3& dir)
{
    const ape::Vec3 up{0.0f, 1.0f, 0.0f, 0.0f};
    const ape::Vec3 forward{0.0f, 0.0f, 1.0f, 0.0f};
    frame.yAxis = dir;
    const float dotForward = dir.x * forward.x + dir.y * forward.y + dir.z * forward.z;
    ape::Vec3& x = frame.xAxis;
    if (std::fabs(std::fabs(dotForward) - 1.0f) < 1e-05f || std::fabs(dotForward + 1.0f) < 1e-05f) {
        x = {up.y * dir.z - up.z * dir.y, up.z * dir.x - up.x * dir.z, up.x * dir.y - up.y * dir.x, 0.0f};
    } else {
        x = {dir.y * forward.z - dir.z * forward.y, dir.z * forward.x - dir.x * forward.z,
             dir.x * forward.y - dir.y * forward.x, 0.0f};
    }
    const ape::Vec3& y = frame.yAxis;
    frame.zAxis = {x.y * y.z - x.z * y.y, x.z * y.x - x.x * y.z, x.x * y.y - x.y * y.x, 0.0f};
    ape::Matrix34NormalizeAxes(frame);
    const float half = length * 0.5f;
    frame.translation = {dir.x * half + start.x, dir.y * half + start.y, dir.z * half + start.z, 0.0f};
    scale.x = 1.0f;
    scale.z = 1.0f;
    scale.y = length / (geometryLength * 2.0f);
}

// ===========================================================================
// Laser

// @func 0x0039BFC4 Laser::Laser
Laser::Laser()
{
    ape::Matrix34 identity;
    ape::Matrix34FromTranslation(identity, {0.0f, 0.0f, 0.0f, 0.0f});
    for (LaserEndpoint& p : points)
        p.transform = identity;
    beam = identity;
    geometryLength = 1.0f;
    damageType = g_gadgetLaserForceElectricDamage.Get() ? 1u : 0u;
    flags = kFlag_Visible;
}

// @func 0x0039BEDC Laser::Reset
void Laser::Reset()
{
    points[0].splinePos = 0.0f;
    points[1].splinePos = 0.0f;
}

// @func 0x003992AC Laser::SetEnabled
void Laser::SetEnabled(bool enabled, bool force, bool fromTrigger)
{
    const bool wasEnabled = (flags & kFlag_Enabled) != 0;
    if (!force && wasEnabled == enabled)
        return;
    const uint32_t old = flags;
    flags = enabled ? (flags | kFlag_Enabled) : (flags & ~kFlag_Enabled);
    if (fromTrigger) {
        // A flickering laser already handed to its trigger keeps flickering.
        if ((old & kFlag_Triggered) && initType == kInit_Flicker)
            return;
        flags |= kFlag_Triggered;
    }
    flags = enabled ? (flags | kFlag_Visible) : (flags & ~kFlag_Visible);

    auto showPoints = [this](bool show) {
        for (LaserEndpoint& p : points) {
            if (!p.script || !p.scriptObject)
                continue;
            if (ape::world::ApeObj* object = ScriptVarObject(p.scriptObject))
                object->SetVisible(show, true);
        }
    };
    auto stopSound = [this]() {
        if (!audio::SoundNameHasDef(sound))
            return false;
        audio::StopVoice(voice, true);
        voice = 0;
        return true;
    };

    if (initType == kInit_Flicker) {
        // The switch takes effect after a flicker; meanwhile the beam shows
        // the state it had.
        if (geometry)
            geometry->SetVisible(false, true);
        showPoints((flags & kFlag_ShowPoints) != 0);
        stopSound();
        if (((old & kFlag_Enabled) != 0) != enabled) {
            flickerDelay = g_laserFlickerDelayTimeMax.Get();
            flickerTime = g_laserFlickerTimeMax.Get();
            flags = enabled ? (flags & ~kFlag_Visible) : (flags | kFlag_Visible);
            if (geometry)
                geometry->SetVisible(!enabled, true);
            showPoints((flags & kFlag_ShowPoints) != 0 || !enabled);
            if (!(flags & kFlag_Enabled))
                flags |= kFlag_80000000;
        }
        return;
    }

    if (geometry)
        geometry->SetVisible(enabled, true);
    showPoints((flags & kFlag_ShowPoints) != 0 || enabled);
    if (stopSound() && enabled)
        voice = entity::PlayEntitySound(sound, &beam.translation, nullptr);
}

// @func 0x0039A110 Laser::SetGeometry
void Laser::SetGeometry(const char* geomName)
{
    if (geometry) {
        geometry->Release();
        geometry = nullptr;
    }
    const char* name = (geomName && *geomName) ? geomName : g_laserDefaultGeom.Get();
    if (!name || !*name)
        return;
    const uint32_t hash = ape::HashNameNoCase(name);
    if (ape::world::ApeObj* placed = ape::world::FindGeomInstance(hash)) {
        geometry = placed->Clone(ape::world::kClone_Children | ape::world::kClone_Def | ape::world::kClone_04);
    } else if (auto* def = ape::world::ApeTDefList<ape::world::ApeGeomDef>::Find(hash)) {
        geometry = ape::world::CreateGeomInstance(def, false);
    }
    if (geometry) {
        geometry->SetVisible(true, true);
        geometryLength = geometry->GetDef()->GetBounds().max.y;
    }
}

// @func 0x00399DA8 Laser::StartPointScripts
void Laser::StartPointScripts(const char* const scriptNames[2])
{
    for (int i = 0; i < 2; ++i) {
        LaserEndpoint& p = points[i];
        if (p.script) {
            ReleaseScript(p.script);
            p.script = nullptr;
            p.scriptObject = nullptr;
        }
        // NOTE: both end points fall back to the first default script; the
        // second default (Entity.Laser.DefaultDirScriptPt2) is never read.
        const char* name = (scriptNames[i] && *scriptNames[i]) ? scriptNames[i] : g_laserDefaultDirScriptPt1.Get();
        if (!name || !*name)
            continue;
        ape::ApeAnimScriptLibrary* library = ape::ApeAnimScriptLibrary::Get(0);
        ape::ApeAnimScript* source = library ? library->FindScript(ape::HashNameNoCase(name)) : nullptr;
        if (!source)
            continue;
        auto* script = new ape::ApeAnimScriptInstance(source);
        script->SetRemoveWhenFinished(false);
        script->Update(kFirstScriptStep);
        script->SetState(kScript_PointStart);
        ape::ApeAnimScriptVar* var = script->FindVar(kScriptVar_Object);
        if (var && var->GetType() == ape::AnimScriptVarType::Object) {
            auto* objectVar = static_cast<ape::ApeAnimScriptObjectVar*>(var);
            if (ape::world::ApeObj* object = ScriptVarObject(objectVar))
                object->SetLocalTransform(p.transform);
            p.scriptObject = objectVar;
        }
        p.script = script;
    }
}

// @func 0x003998BC Laser::ApplyBeam
void Laser::ApplyBeam()
{
    if (!geometry)
        return;
    geometry->SetLocalTransform(beam);
    const ape::Vec3& scale = geometry->GetScale();
    if (scale.x != beamScale.x || scale.y != beamScale.y || scale.z != beamScale.z)
        geometry->SetScale(beamScale, false);
    // NOTE: the console then reads the loop voice's request and clears its
    // two bus pointers in the local copy only; nothing reaches the voice.
    audio::SoundParams params;
    entity::GetVoiceParams(voice, params);
}

void Laser::UpdateBeam()
{
    const ape::Vec3& a = points[0].transform.translation;
    const ape::Vec3& b = points[1].transform.translation;
    ape::Vec3 dir{b.x - a.x, b.y - a.y, b.z - a.z, 0.0f};
    // NOTE: a degenerate beam keeps the squared length (console behaviour).
    float length = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
    if (length >= 1e-05f) {
        length = std::sqrt(length);
        const float inv = 1.0f / length;
        dir = {dir.x * inv, dir.y * inv, dir.z * inv, 0.0f};
    }
    ComputeLaserBeamFrame(length, geometryLength, beam, beamScale, a, dir);
}

// ===========================================================================
// Laser group

// @func 0x00398ED0 LaserGroup::LaserGroup
LaserGroup::LaserGroup()
{
    bounds.min = {FLT_MAX, FLT_MAX, FLT_MAX, 0.0f};
    bounds.max = {-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f};
}

Laser* LaserGroup::FindLaser(uint32_t nameHash)
{
    return g_laserManager ? g_laserManager->LaserAt(*lasers.Find(nameHash)) : nullptr;
}

// @func 0x00398FF8 LaserGroup::AddNavObstacle
void LaserGroup::AddNavObstacle()
{
    if (!setting || !setting->Value(sai_params::GroupNavObstacle).b || !(bounds.min.x < FLT_MAX))
        return;
    levels::NavManager* nav = levels::GetNavManager();
    if (!nav)
        return;
    levels::NavObstacleDesc desc;
    ape::Vec3 center;
    ape::AabbCenter(bounds, center);
    ape::Matrix34FromTranslation(desc.transform, center);
    desc.extents = {bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y, bounds.max.z - bounds.min.z, 0.0f};
    desc.name = "laser";
    desc.mask = 0xFFFF;
    navObstacle = levels::AddNavObstacle(*nav, desc);
}

// @func 0x003991A0 LaserGroup::FinishBounds
void LaserGroup::FinishBounds()
{
    ape::Vec3& lo = bounds.min;
    ape::Vec3& hi = bounds.max;
    const bool tiny = std::fabs(lo.x - hi.x) < 1e-05f && std::fabs(lo.y - hi.y) < 1e-05f &&
                      std::fabs(lo.z - hi.z) < 1e-05f;
    if (tiny || lo.x == hi.x || lo.y == hi.y || lo.z == hi.z) {
        lo = {lo.x - 0.005f, lo.y - 0.005f, lo.z - 0.005f, lo.pad};
        hi = {hi.x + 0.005f, hi.y + 0.005f, hi.z + 0.005f, hi.pad};
    }
    ape::SphereFromAabb(sphere, bounds);
    AddNavObstacle();
}

// @func 0x00399758 LaserGroup::SetEnabled
void LaserGroup::SetEnabled(bool enabled, bool force, bool fromTrigger)
{
    if (!force && ((flags & kFlag_Enabled) != 0) == enabled)
        return;
    flags = enabled ? (flags | kFlag_Enabled) : (flags & ~kFlag_Enabled);
    for (uint16_t i = 0; i < lasers.Count(); ++i)
        if (Laser* laser = g_laserManager->LaserAt(lasers.Data()[i].value))
            laser->SetEnabled(enabled, force, fromTrigger);
    if (setting && setting->Value(sai_params::GroupNavObstacle).b) {
        if (levels::NavManager* nav = levels::GetNavManager())
            levels::RemoveNavObstacle(*nav, navObstacle);
        navObstacle = nullptr;
    }
    if (flags & kFlag_Enabled)
        AddNavObstacle();
}

// @func 0x0039BC48 LaserGroup::Start
void LaserGroup::Start()
{
    for (uint16_t i = 0; i < lasers.Count(); ++i) {
        Laser* laser = g_laserManager->LaserAt(lasers.Data()[i].value);
        if (!laser)
            continue;
        laser->flickerDelay = 0.0f;
        laser->flickerTime = 0.0f;
        laser->flickerToggle = 0.0f;
        laser->flags &= ~(Laser::kFlag_80000000 | Laser::kFlag_Triggered);
        switch (laser->setting->def->GetTypeHash()) {
        case kLaserSetting_Spline:
            LaserSetupSpline(*laser, laser->setting, laser->point);
            break;
        case kLaserSetting_Anchor:
            LaserSetupAnchor(*laser, laser->setting, laser->point);
            break;
        case kLaserSetting_Fixed:
            LaserSetupFixed(*laser, laser->setting, laser->point);
            break;
        default:
            break;
        }
    }
    if (!setting) {
        SetEnabled(true, true, false);
        return;
    }
    const char* triggerName = setting->Value(sai_params::GroupActivationTrigger).str;
    if (triggerName && *triggerName) {
        if (entity::Entity* found = entity::FindEntity(ape::HashNameNoCase(triggerName)))
            trigger = (found->typeBits[0] & kTriggerTypeBit) ? found : nullptr;
    }
    SetEnabled(setting->Value(sai_params::GroupStartEnabled).b, true, false);
}

// @func 0x0039BEFC LaserGroup::ResetLasers
// NOTE: the console repeats the pass once per laser; one pass does the same.
void LaserGroup::ResetLasers()
{
    for (uint16_t i = 0; i < lasers.Count(); ++i)
        if (Laser* laser = g_laserManager->LaserAt(lasers.Data()[i].value))
            laser->Reset();
}

// ===========================================================================
// Laser field

// @func 0x0039CB9C LaserField::LaserField
// NOTE: the colour starts as the engine's transparent entry (0x01164CA4).
LaserField::LaserField() = default;

// @func 0x0039FEBC LaserField::Name
uint32_t LaserField::Name() const
{
    return point ? point->GetNameHash() : 0;
}

// @func 0x0039CD68 LaserField::Release
void LaserField::Release()
{
    if (point)
        TagObject(point, kLaserFieldObjectTag, nullptr);
    polygon.Clear();
    triangles.Clear();
    damagePhantom = nullptr;
    fxPhantom = nullptr;
    if (lifetimeScript) {
        ReleaseScript(lifetimeScript);
        lifetimeScript = nullptr;
    }
    for (MaterialSet& set : materialSets) {
        for (uint32_t i = 0; i < set.count; ++i) {
            if (set.materials[i]) {
                set.materials[i]->Release();
                set.materials[i] = nullptr;
            }
        }
        set.count = 0;
    }
    for (ape::world::ApeObj* outline : outlines)
        if (outline)
            outline->Release();
    outlines.Clear();
    for (entity::Entity* joint : joints)
        if (joint)
            joint->Release(true);
    joints.Clear();
}

// @func 0x0039CF9C LaserField::SetShown
void LaserField::SetShown(bool shown)
{
    flags = shown ? (flags | kFlag_Shown) : (flags & ~kFlag_Shown);
    for (ape::world::ApeObj* outline : outlines)
        if (outline)
            outline->SetVisible(shown, true);
}

// @func 0x0039D054 LaserField::Start
void LaserField::Start()
{
    flags |= kFlag_Started;
    if (initType != 1)
        return;
    // A flickering field comes up hidden and flickers in.
    flags &= ~kFlag_Shown;
    for (ape::world::ApeObj* outline : outlines)
        if (outline)
            outline->SetVisible(false, true);
    for (entity::Entity* joint : joints)
        if (ape::ApeAnimScriptInstance* script = EntityScript(joint))
            script->SetStateAndUpdate(kJoint_FlickerStart);
    if (lifetimeScript)
        lifetimeScript->SetStateAndUpdate(kLifetime_FlickerStart);
    flickerDelay = g_laserFlickerDelayTimeMax.Get();
    flickerTime = g_laserFlickerTimeMax.Get();
}

// @func 0x0039D1D4 LaserField::SetActive
void LaserField::SetActive(bool active)
{
    if (damagePhantom)
        LaserPhantomSetEnabled(damagePhantom, active);
    if (fxPhantom)
        LaserPhantomSetEnabled(fxPhantom, active);
    if (lifetimeScript)
        lifetimeScript->SetStateAndUpdate(active ? kLifetime_On : kLifetime_Off);
    SetShown(active);
}

// @func 0x0039D32C LaserField::SetEnabled
void LaserField::SetEnabled(bool enabled, bool force, bool applyNow)
{
    const bool wasEnabled = (flags & kFlag_Enabled) != 0;
    if (!force && wasEnabled == enabled)
        return;
    // Switching also forgets the trigger and start state.
    flags &= ~(kFlag_Enabled | kFlag_Occupied | kFlag_Started | kFlag_Flickering);
    if (enabled)
        flags |= kFlag_Enabled;
    if (!enabled) {
        const uint32_t jointState = wasEnabled == enabled ? kJoint_DisabledAgain : kJoint_Disabled;
        if (wasEnabled == enabled || initType != 1) {
            for (entity::Entity* joint : joints)
                if (ape::ApeAnimScriptInstance* script = EntityScript(joint))
                    script->SetStateAndUpdate(jointState);
        }
    }
    if (trigger && !applyNow) {
        SetActive(false);
        return;
    }
    if (initType == 1) {
        SetActive(false);
        if (!(flags & kFlag_Enabled) && wasEnabled != enabled) {
            // Switched off: show the field and flicker it out.
            SetShown(true);
            flags |= kFlag_Flickering;
            flickerDelay = g_laserFlickerDelayTimeMax.Get();
            flickerTime = g_laserFlickerTimeMax.Get();
        }
    } else {
        SetActive((flags & kFlag_Enabled) != 0);
    }
    if (flags & kFlag_Enabled)
        Start();
}

// @func 0x0039FEE0 LaserField::SetOutline
void LaserField::SetOutline(const ape::ApeTArray<ape::Vec3>* outline)
{
    triangles.Clear();
    if (outline && outline->Count() != 0)
        TriangulatePolygonXZ(*outline, triangles);
}

// @func 0x0039FF34 LaserField::SetScriptState
void LaserField::SetScriptState(uint32_t state)
{
    if (lifetimeScript)
        lifetimeScript->SetState(state);
    if (state == 0)
        return;
    for (entity::Entity* joint : joints)
        if (ape::ApeAnimScriptInstance* script = EntityScript(joint))
            script->SetStateAndUpdate(state);
}

// ===========================================================================
// Manager

LaserManager* GetLaserManager() { return g_laserManager; }

// @func 0x003A0010 LaserManager::LaserManager
LaserManager::LaserManager()
{
    groups.Init(kMaxGroups, nullptr, ape::GetDefaultAllocator());
    lasers.Init(kMaxLasers, nullptr, ape::GetDefaultAllocator());
    fields.Init(kMaxFields, nullptr, ape::GetDefaultAllocator());
    g_laserManager = this;
}

// @func 0x003A01E4 LaserManager::~LaserManager
LaserManager::~LaserManager()
{
    g_laserManager = nullptr;
}

LaserGroup* LaserManager::FindGroup(uint32_t nameHash)
{
    const int16_t index = *groupNames.Find(nameHash);
    return index < 0 ? nullptr : &groups[size_t(index)];
}

Laser* LaserManager::FindLaser(uint32_t nameHash)
{
    for (uint16_t i = 0; i < groups.Used(); ++i)
        if (Laser* laser = groups[i].FindLaser(nameHash))
            return laser;
    return nullptr;
}

LaserField* LaserManager::FindField(uint32_t nameHash)
{
    for (uint16_t i = 0; i < fields.Used(); ++i)
        if (fields[i].Name() == nameHash)
            return &fields[i];
    return nullptr;
}

// @func 0x003A0380 LaserManager::StartLevel
void LaserManager::StartLevel()
{
    for (uint16_t i = 0; i < groups.Used(); ++i) {
        groups[i].Start();
        groups[i].FinishBounds();
    }
    for (uint16_t i = 0; i < fields.Used(); ++i)
        LaserFieldSetup(fields[i]);
}

// @func 0x003A0470 LaserManager::Update
void LaserManager::Update(float dt)
{
    const bool always = g_laserAlwaysUpdate.Get();
    for (uint16_t i = 0; i < groups.Used(); ++i) {
        LaserGroup& group = groups[i];
        if (group.trigger)
            group.SetEnabled(always || entity::TriggerOccupantCount(group.trigger) != 0, false, true);
        if (!always && !(group.flags & LaserGroup::kFlag_Enabled))
            continue;
        const bool inView = g_game && g_game->mainView && SphereInFrustum(g_game->mainView->frustum, group.sphere);
        group.flags = inView ? (group.flags | LaserGroup::kFlag_InView) : (group.flags & ~LaserGroup::kFlag_InView);
        for (uint16_t j = 0; j < group.lasers.Count(); ++j) {
            Laser* laser = LaserAt(group.lasers.Data()[j].value);
            if (!laser || laser->initType == Laser::kInit_Flicker)
                continue;
            laser->flags = inView ? (laser->flags | Laser::kFlag_Visible) : (laser->flags & ~Laser::kFlag_Visible);
        }
    }

    for (uint16_t i = 0; i < fields.Used(); ++i) {
        LaserField& field = fields[i];
        if ((field.flags & LaserField::kFlag_Enabled) && field.trigger) {
            const bool occupied = entity::TriggerOccupantCount(field.trigger) != 0;
            if (!(field.flags & (LaserField::kFlag_Started | LaserField::kFlag_Occupied)) && occupied)
                field.Start();
            field.flags = occupied ? (field.flags | LaserField::kFlag_Occupied)
                                   : (field.flags & ~LaserField::kFlag_Occupied);
            if (field.initType != 1)
                field.SetActive(occupied);
        }
        LaserFieldUpdate(field, dt);
    }

    if (lasers.Used() == 0 || !(dt > 0.0f))
        return;
    updated.Clear();
    for (uint16_t i = 0; i < lasers.Used(); ++i)
        if (LaserUpdate(lasers[i], dt))
            updated.PushBack(&lasers[i]);
    if (updated.Count() != 0)
        UpdateBeams();
}

// @func 0x0039C8F0 LaserManager::UpdateBeams
// NOTE: the console copies the lasers in batches of 256 to a scratch buffer
// (job style) and back; the beams are computed in place here.
void LaserManager::UpdateBeams()
{
    for (Laser* laser : updated)
        laser->UpdateBeam();
}

// @func 0x003A0924 LaserManager::ApplyBeams
void LaserManager::ApplyBeams()
{
    for (Laser* laser : updated)
        laser->ApplyBeam();
}

// @func 0x003A13C8 LaserManager::UpdateFieldMaterials
void LaserManager::UpdateFieldMaterials()
{
    for (uint16_t i = 0; i < fields.Used(); ++i) {
        LaserField& field = fields[i];
        if (!(field.flags & LaserField::kFlag_Shown))
            continue;
        const ape::ApeTArray<ape::Vec3>& points = field.triangles.Count() == 0 ? field.polygon : field.triangles;
        for (LaserField::MaterialSet& set : field.materialSets)
            LaserFieldColorMaterials(points, field.color, set);
    }
}

// @func 0x003A1500 LaserManager::Unload
void LaserManager::Unload()
{
    for (uint16_t i = 0; i < lasers.Used(); ++i) {
        Laser& laser = lasers[i];
        if (laser.point)
            TagObject(laser.point, kLaserObjectTag, nullptr);
        if (laser.geometry) {
            laser.geometry->Release();
            laser.geometry = nullptr;
        }
        for (LaserEndpoint& p : laser.points) {
            if (p.script) {
                ReleaseScript(p.script);
                p.script = nullptr;
                p.scriptObject = nullptr;
            }
        }
        audio::StopVoice(laser.voice, true);
        laser.voice = 0;
        laser.navObstacle = nullptr;   // NOTE: the NavPower handle is dropped (0x00638498)
    }
    for (uint16_t i = 0; i < fields.Used(); ++i) {
        fields[i].Release();
        fields[i].~LaserField();
    }
    for (uint16_t i = 0; i < groups.Used(); ++i)
        groups[i].~LaserGroup();
    groups.SetUsed(0);
    lasers.SetUsed(0);
    fields.SetUsed(0);
    groupNames.Clear();
}

// @func 0x003A1808 UnloadLasers
void UnloadLasers()
{
    if (g_laserManager)
        g_laserManager->Unload();
}

// @func 0x003A187C LaserManager::FindOrAddGroup
LaserGroup* LaserManager::FindOrAddGroup(uint32_t nameHash, ape::world::ApeObj* point)
{
    (void)point;   // named in the "out of group slots" warning
    if (nameHash)
        if (LaserGroup* group = FindGroup(nameHash))
            return group;
    const int16_t index = AllocSlot(groups);
    if (index < 0)
        return nullptr;
    groupNames.Add(nameHash, index);
    return new (&groups[size_t(index)]) LaserGroup();
}

// @func 0x003A1BFC LaserManager::AddLaser
void LaserManager::AddLaser(LaserGroup* group, ape::world::ApeObj* point, ape::setting::Setting* setting)
{
    if (!group)
        return;
    const uint32_t name = ObjectName(point);
    const int16_t index = AllocSlot(lasers);
    // NOTE: the console goes on with an invalid slot when the pool is full;
    // the callers check for room first.
    if (index < 0)
        return;
    Laser* laser = new (&lasers[size_t(index)]) Laser();
    laser->setting = setting;
    laser->point = point;
    laser->group = group;
    if (point)
        TagObject(point, kLaserObjectTag, laser);
    group->lasers.Add(name, index);
}

// @func 0x003A1DC4 LaserManager::AddLaserForPoint
void LaserManager::AddLaserForPoint(ape::world::ApeObj* point, ape::setting::Setting* setting)
{
    if (!point || !point->GetParent())
        return;
    ape::world::ApeObj* parent = point->GetParent();
    if (const uint32_t name = ObjectName(point)) {
        for (uint16_t i = 0; i < groups.Used(); ++i) {
            if (groups[i].FindLaser(name))
                return;   // "Duplicate laser found!"
        }
    }
    if (lasers.Used() >= lasers.Capacity())
        return;   // "Ran out of laser slots"
    if (LaserGroup* group = FindOrAddGroup(ObjectName(parent), parent))
        AddLaser(group, point, setting);
}

// @func 0x003A20F8 LaserManager::AddField
void LaserManager::AddField(ape::world::ApeObj* point, ape::setting::Setting* setting)
{
    if (!point)
        return;
    if (const uint32_t name = point->GetNameHash()) {
        for (uint16_t i = 0; i < fields.Used(); ++i)
            if (fields[i].Name() == name)
                return;   // "Duplicate laser field found!"
    }
    const int16_t index = AllocSlot(fields);
    if (index < 0)
        return;   // "Ran out of laser field slots"
    LaserField* field = new (&fields[size_t(index)]) LaserField();
    field->point = point;
    ape::world::ApeObj* child = point->GetFirstChild();
    if (child && child->GetType() == ape::world::kObjType_SplineInstance)
        field->outlineSpline = child;
    if (setting->def->GetTypeHash() == kLaserSetting_Field)
        field->setting = setting;
    TagObject(point, kLaserFieldObjectTag, field);
}

namespace {

// Deferred halves of the associations: the point instances exist now.
// @func 0x003A1A84 CreateLaserGroup
void CreateLaserGroup(void* settingPtr, uint32_t objectName)
{
    auto* setting = static_cast<ape::setting::Setting*>(settingPtr);
    ape::world::ApeObj* point = FindPoint(objectName);
    if (!point || !g_laserManager)
        return;   // "Fail to find the associate object (%s) from the scene"
    LaserGroup* group = g_laserManager->FindOrAddGroup(ObjectName(point), point);
    if (group && setting->def->GetTypeHash() == kLaserSetting_Group)
        group->setting = setting;
}

// @func 0x003A200C CreateLaser
void CreateLaser(void* settingPtr, uint32_t objectName)
{
    ape::world::ApeObj* point = FindPoint(objectName);
    if (point && g_laserManager)
        g_laserManager->AddLaserForPoint(point, static_cast<ape::setting::Setting*>(settingPtr));
}

// @func 0x003A233C CreateLaserField
void CreateLaserField(void* settingPtr, uint32_t objectName)
{
    ape::world::ApeObj* point = FindPoint(objectName);
    if (point && g_laserManager)
        g_laserManager->AddField(point, static_cast<ape::setting::Setting*>(settingPtr));
}

using DeferredCallback = void (*)(void*, uint32_t);

void QueueAssociation(ape::setting::Setting* setting, const char* objectName, DeferredCallback callback)
{
    if (!objectName)
        return;
    entity::QueueDeferredEvent(setting, ape::HashNameNoCase(objectName), reinterpret_cast<uintptr_t>(callback));
}

// The laser settings reach the manager through the association hooks.
struct InstallAssociations {
    InstallAssociations()
    {
        g_saiAssociationHooks.laserGroup = &AssociateLaserGroup;
        g_saiAssociationHooks.laser = &AssociateLaser;
        g_saiAssociationHooks.laserField = &AssociateLaserField;
    }
} s_installAssociations;

} // namespace

// @func 0x003A1828 AssociateLaserGroup
void AssociateLaserGroup(ape::setting::Setting* setting, const char* objectName, bool reload)
{
    (void)reload;
    QueueAssociation(setting, objectName, &CreateLaserGroup);
}

// @func 0x003A1BA8 AssociateLaser
void AssociateLaser(ape::setting::Setting* setting, const char* objectName, bool reload)
{
    (void)reload;
    QueueAssociation(setting, objectName, &CreateLaser);
}

// @func 0x003A20A4 AssociateLaserField
void AssociateLaserField(ape::setting::Setting* setting, const char* objectName, bool reload)
{
    (void)reload;
    QueueAssociation(setting, objectName, &CreateLaserField);
}

// ===========================================================================
// Hooks (modules not ported yet)

void LaserPhantomSetEnabled(void* phantom, bool enabled)
{
    (void)phantom;
    (void)enabled;
}

// TODO(0x0039C1C8 LaserUpdate): end point movement and beam hits.
bool LaserUpdate(Laser& laser, float dt)
{
    (void)laser;
    (void)dt;
    return false;
}

// TODO(0x0039FBD8 LaserFieldUpdate)
void LaserFieldUpdate(LaserField& field, float dt)
{
    (void)field;
    (void)dt;
}

// TODO(0x0039F840 LaserFieldColorMaterials)
void LaserFieldColorMaterials(const ape::ApeTArray<ape::Vec3>& points, const uint8_t color[4],
                              LaserField::MaterialSet& set)
{
    (void)points;
    (void)color;
    (void)set;
}

// TODO(0x0039A2B8 LaserSetupFixed)
void LaserSetupFixed(Laser& laser, ape::setting::Setting* setting, ape::world::ApeObj* point)
{
    (void)laser;
    (void)setting;
    (void)point;
}

// TODO(0x0039AD48 LaserSetupAnchor)
void LaserSetupAnchor(Laser& laser, ape::setting::Setting* setting, ape::world::ApeObj* point)
{
    (void)laser;
    (void)setting;
    (void)point;
}

// TODO(0x0039B5A0 LaserSetupSpline)
void LaserSetupSpline(Laser& laser, ape::setting::Setting* setting, ape::world::ApeObj* point)
{
    (void)laser;
    (void)setting;
    (void)point;
}

// TODO(0x0039F3D8 LaserFieldSetup)
void LaserFieldSetup(LaserField& field)
{
    (void)field;
}

} // namespace sly
