// openTIT - HUD meters.
#include "sly/game/hud/HudMeter.h"

#include "agl/AglResources.h"
#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/core/ApeString.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeObj.h"
#include "sly/anim/AnimController.h"
#include "sly/entity/ScriptedEntity.h"
#include "sly/game/GameScripts.h"
#include "sly/game/goals/JobManager.h"
#include "sly/game/hud/HudHooks.h"

#include <algorithm>
#include <cstdio>

namespace sly {
namespace hud {

namespace {

// Element state values (HudObject::animState >> 1) of the meters.
enum MeterState : uint32_t {
    kMeter_Hidden = 0,
    kMeter_Intro = 1,
    kMeter_Shown = 2,
    kMeter_Outro = 3,
};

constexpr uint32_t kScriptState_Intro = 0x04296E4Cu;    // "Intro"
constexpr uint32_t kScriptState_Idle = 0x002237D4u;     // "Idle"
constexpr uint32_t kScriptState_Outro = 0x048129EBu;    // "Outro"
constexpr uint32_t kScriptState_Hurt = 0xA464660Eu;     // NOTE: name unknown; the health went down
constexpr uint32_t kScriptState_Hidden = 0x95B0708Eu;   // NOTE: name unknown; set when the meter hides
constexpr uint32_t kScriptState_Normal = 0x009BD94Du;   // NOTE: same pair as SetMeterBVariant
constexpr uint32_t kScriptState_Variant = 0xDBDDDABDu;
constexpr uint32_t kScript_SecondBarFull = 0x9C87D029u;   // NOTE: script name unknown
constexpr uint32_t kLevelVar = 0x044FA364u;             // "Level"
// State "Level<n>" of the POW meter script is this hash plus n.
// NOTE: works because the hash of "Level0".."Level9" grows by one per digit.
constexpr uint32_t kLevelStateBase = 0x02B81827u;

uint32_t MeterStateOf(const HudObject* element) { return element->animState >> 1; }

void SetMeterState(HudObject* element, uint32_t state)
{
    element->animState = (element->animState & 1u) | (state << 1);
}

void RunScriptState(HudObject* element, uint32_t state)
{
    if (element->animScript)
        static_cast<ape::ApeAnimScriptInstance*>(element->animScript)->SetStateAndUpdate(state);
}

bool ElementObjectVisible(const HudObject* element)
{
    return element->object && element->object->IsVisible();
}

// UV animation of the first material of a geometry's first LOD.
agl::AglMaterialAnim* FillAnim(ape::world::ApeObj* geom)
{
    if (!geom || !geom->GetDef())
        return nullptr;
    auto* def = static_cast<ape::world::ApeGeomDef*>(geom->GetDef());
    // NOTE: the world layer's opaque AglMesh is the renderer's topology.
    auto* topology = reinterpret_cast<agl::AglTopology*>(def->m_lods[0].mesh);
    agl::AglMaterial* material = topology->meshes[0].material;
    return material->anim;
}

// Looks a fill geometry up and hands its U scroll to the game.
ape::world::ApeObj* FindFillGeometry(uint32_t name)
{
    ape::world::ApeObj* geom = name ? ape::world::FindGeomInstance(name) : nullptr;
    if (!geom || !geom->GetDef())
        return nullptr;
    if (agl::AglMaterialAnim* anim = FillAnim(geom))
        anim->u.SetManual();
    return geom;
}

void SetFillValue(ape::world::ApeObj* geom, float value)
{
    if (!geom || !geom->GetDef())
        return;
    FillAnim(geom)->u.value = value;
}

uint32_t ClipHash(const char* format, const char* prefix, const char* suffix)
{
    char name[0x80];
    std::snprintf(name, sizeof(name), format, prefix, suffix);
    return ape::HashNameNoCase(name);
}

const char* ClipPrefix(const HudObjectDef* def)
{
    return def->animPrefix[0] ? def->animPrefix : def->defName;
}

void SetPartVisible(ape::world::ApeObj* part, bool visible)
{
    if (part && part->IsVisible() != visible)
        part->SetVisible(visible, true);
}

// Registers `element` under its name in the element map.
void RegisterElement(HudObject* element)
{
    Hud().objects.Add(element->RegisteredName(), element);
}

// Unregisters and deletes `element` (if any) and clears the slot.
void DestroyElement(HudObject*& element)
{
    if (!element)
        return;
    Hud().objects.Remove(element->RegisteredName());
    delete element;
    element = nullptr;
}

HudMeter* FindMeterOf(const entity::Entity* owner);

HudObjectDef* FindDef(uint32_t name, uint32_t type)
{
    HudObjectDef** slot = Hud().defs.Find(name);
    HudObjectDef* def = slot ? *slot : nullptr;
    return (def && (def->typeMask & type)) ? def : nullptr;
}

} // namespace

// ---------------------------------------------------------------------------
// Global scripts

// @func 0x00364408 hud::PlayGlobalScript
void PlayGlobalScript(uint32_t script)
{
    ape::ApeAnimScriptInstance* instance = ape::ApeAnimScriptInstance::Create(script);
    if (!instance)
        return;
    instance->Update(1.0f / 60.0f);
    instance->SetRemoveWhenFinished(true);
}

// @func 0x0036E568 hud::PlayGlobalScriptWithLevel
void PlayGlobalScriptWithLevel(uint32_t script, int32_t level)
{
    if (!script)
        return;
    ape::ApeAnimScriptInstance* instance = ape::ApeAnimScriptInstance::Create(script);
    if (!instance)
        return;
    // NOTE: the console writes the variable's value word without checking
    // its type.
    ape::ApeAnimScriptVar* var = instance->FindVar(kLevelVar);
    if (var && var->GetType() == ape::AnimScriptVarType::Int)
        static_cast<ape::ApeAnimScriptIntVar*>(var)->SetInt(level);
    instance->Update(1.0f / 60.0f);
    instance->SetRemoveWhenFinished(true);
}

// ---------------------------------------------------------------------------
// HudMeter

// @func 0x00364EE4 HudMeter::HudMeter
HudMeter::HudMeter(HudObjectDef* def_) : HudObject(0, def_)
{
    const HudObjectDef::MeterDef& info = def->meter;
    useOwner_ = info.useOwner != 0;
    meterOwner = nullptr;
    meterFlagsC0 = uint8_t((info.showExtra ? 0x80u : 0u) | ((info.showFill & 1u) ? 0x40u : 0u));
    meterHoldTime = info.holdTime;
    meterHoldTimer = 0.0f;
    drainTime_ = info.drainTime;
    const ape::Vec3& zero = ape::world::kWorldZero;
    if (def->baseOffset.x == zero.x && def->baseOffset.y == zero.y && def->baseOffset.z == zero.z)
        baseOffset = g_defaultHudOffset;
    typeFlags |= kHud_Type8;
    player_ = info.player;
    fillA_ = FindFillGeometry(info.fillA);
    fillB_ = FindFillGeometry(info.fillB);
    frame_ = info.frame ? ape::world::FindGeomInstance(info.frame) : nullptr;
    secondFillA_ = FindFillGeometry(info.extra);
    secondFillB_ = FindFillGeometry(info.extra2);
    const char* prefix = ClipPrefix(def);
    const char* suffix = info.healthMeter ? "HealthMeter" : "";
    if (prefix && prefix[0]) {
        clips_[1] = ClipHash("%s%s_Intro", prefix, suffix);
        clips_[2] = ClipHash("%s%s_Idle", prefix, suffix);
        clips_[3] = ClipHash("%s%s_Outro", prefix, suffix);
    }
}

entity::Entity* HudMeter::Target() const
{
    if (useOwner_)
        return meterOwner;
    entity::Entity* character = game::PlayerEntityOf(player_);
    return (character && character->HasTypeBit(1)) ? character : nullptr;
}

// @func 0x00364C90 HudMeter::SetFill
void HudMeter::SetFill(float fraction)
{
    const float value = fraction * 0.5f + 0.5f;
    SetFillValue(fillA_, value);
    SetFillValue(fillB_, value);
}

// @func 0x00364D28 HudMeter::Show
void HudMeter::Show()
{
    if (HudFrameTimeFrozen() || MeterStateOf(this) != kMeter_Hidden)
        return;
    SetActive(true);
    SetMeterState(this, kMeter_Intro);
    PlayHudClip(animController, clips_[1], 1.0f, false);
    RunScriptState(this, kScriptState_Intro);
    if (auto* target = static_cast<entity::ScriptedEntity*>(Target())) {
        drainTimer_ = 0.0f;
        meterHoldTimer = 0.0f;
        fraction_ = target->health / target->maxHealth;
        SetFill(fraction_);
    }
}

// @func 0x0036D288 HudMeter::Hide
void HudMeter::Hide()
{
    const uint32_t state = MeterStateOf(this);
    if (state == kMeter_Hidden || state == kMeter_Outro)
        return;
    SetActive(true);
    SetMeterState(this, kMeter_Outro);
    PlayHudClip(animController, clips_[3], 1.0f, false);
    RunScriptState(this, kScriptState_Outro);
}

// @func 0x0036D350 HudMeter::Update
void HudMeter::Update(float dt)
{
    switch (MeterStateOf(this)) {
    case kMeter_Hidden:
        return;
    case kMeter_Intro:
        if (HudClipDone(animController)) {
            SetMeterState(this, kMeter_Shown);
            PlayHudClip(animController, clips_[2], 1.0f, false);
            RunScriptState(this, kScriptState_Idle);
        }
        break;
    case kMeter_Shown: {
        auto* target = static_cast<entity::ScriptedEntity*>(Target());
        if (!target) {
            SetActive(false);
            break;
        }
        const float health = target->health / target->maxHealth;
        if (health < fraction_)
            RunScriptState(this, kScriptState_Hurt);
        if (meterHoldTimer > 0.0f) {
            // A fresh loss waits before the fill moves.
            meterHoldTimer = std::max(meterHoldTimer - dt, 0.0f);
            break;
        }
        if (drainTimer_ > 0.0f) {
            drainTimer_ = std::max(drainTimer_ - dt, 0.0f);
            const float shown = (drainFrom_ - drainTo_) * (drainTimer_ / drainTime_) + drainTo_;
            SetFill(shown);
            if (shown <= 0.0f)
                Hide();
        }
        if (fraction_ != health) {
            if (health < fraction_) {
                if (drainTime_ > 0.0f) {
                    drainTimer_ = drainTime_;
                    drainFrom_ = fraction_;
                    drainTo_ = health;
                } else {
                    drainTimer_ = 0.0f;
                    SetFill(health);
                    if (health <= 0.0f)
                        Hide();
                }
            } else {
                drainTimer_ = 0.0f;
                SetFill(health);
            }
            fraction_ = health;
        }
        break;
    }
    case kMeter_Outro:
        if (HudClipDone(animController)) {
            SetActive(false);
            SetMeterState(this, kMeter_Hidden);
        }
        break;
    default:
        break;
    }
    HudObject::Update(dt);
}

// @func 0x0036D874 HudMeter::Refresh
void HudMeter::Refresh()
{
    SetActive(false);
    const float origin[3] = {ape::world::kWorldZero.x, ape::world::kWorldZero.y, ape::world::kWorldZero.z};
    SetStackOffset(origin, true);
    fraction_ = 1.0f;
    secondFraction_ = 1.0f;
    drainTimer_ = 0.0f;
    meterHoldTimer = 0.0f;
    animState = 1;
}

// @func 0x0036D9E4 HudMeter::UpdateFillVisibility
void HudMeter::UpdateFillVisibility()
{
    if (!fillA_)
        return;
    const bool visible = ElementObjectVisible(this) && (meterFlagsC0 & 0x40);
    SetPartVisible(fillA_, visible);
    SetPartVisible(frame_, visible);
}

// @func 0x0036DAFC HudMeter::SetActive
void HudMeter::SetActive(bool active)
{
    if (object)
        object->SetVisible(active, true);
    UpdateFillVisibility();
    if (secondFillA_)
        secondFillA_->SetVisible(ElementObjectVisible(this) && (meterFlagsC0 & 0x80), true);
    if (ElementObjectVisible(this))
        RunScriptState(this, (meterFlagsC0 & 0x20) ? kScriptState_Variant : kScriptState_Normal);
    if (!active)
        RunScriptState(this, kScriptState_Hidden);
}

// @func 0x0036D900 HudMeter::SetEnabled
void HudMeter::SetEnabled(bool enabled, bool /*flag*/)
{
    if (!enabled) {
        Hide();
        return;
    }
    Show();
    EnterStack();
}

// @func 0x0036DC6C HudMeter::SetSecondBarVisible
void HudMeter::SetSecondBarVisible(bool visible)
{
    const bool was = (meterFlagsC0 & 0x80) != 0;
    meterFlagsC0 = uint8_t((meterFlagsC0 & 0x7Fu) | (visible ? 0x80u : 0u));
    if (was != visible && secondFillA_)
        secondFillA_->SetVisible(object && object->IsVisible() && visible, true);
}

// @func 0x00369680 HudMeter::SetSecondFill
void HudMeter::SetSecondFill(float fraction)
{
    if (!HudFrameTimeFrozen() && fraction >= 1.0f && fraction > secondFraction_)
        StartObjectScript(kScript_SecondBarFull, ObjectScriptRef(object), 0, true);
    secondFraction_ = fraction;
    const float value = fraction * 0.5f + 0.5f;
    SetFillValue(secondFillA_, value);
    SetFillValue(secondFillB_, value);
}

// ---------------------------------------------------------------------------
// HudPowMeter

// @func 0x00364700 HudPowMeter::HudPowMeter
HudPowMeter::HudPowMeter(HudObjectDef* def_) : HudObject(0, def_)
{
    const HudObjectDef::PowMeterDef& info = def->pow;
    holdTime_ = info.holdTime;
    drainTime_ = info.drainTime;
    typeFlags |= kHud_Type10;
    player_ = info.player;
    fillA_ = FindFillGeometry(info.fillA);
    fillB_ = FindFillGeometry(info.fillB);
    for (int i = 0; i < kPipCount; ++i) {
        pips_[i] = ape::world::FindGeomInstance(info.pips[i]);
        pipScripts_[i] = info.pipScripts[i];
    }
    introScript_ = info.introScript;
    field_104 = info.field_F8;
    const char* prefix = ClipPrefix(def);
    // The clip names are fixed; the prefix only has to exist.
    if (prefix && prefix[0]) {
        clips_[1] = ape::HashNameNoCase("POWMeter_Intro");
        clips_[2] = ape::HashNameNoCase("POWMeter_Idle");
        clips_[3] = ape::HashNameNoCase("POWMeter_Outro");
    }
}

// @func 0x00364580 HudPowMeter::PlayIntroScript
void HudPowMeter::PlayIntroScript()
{
    if (introScript_)
        PlayGlobalScript(introScript_);
}

// @func 0x003645B0 HudPowMeter::Show
void HudPowMeter::Show()
{
    if (MeterStateOf(this) != kMeter_Hidden)
        return;
    if (object)
        object->SetVisible(true, true);
    PlayIntroScript();
    SetMeterState(this, kMeter_Intro);
    PlayHudClip(animController, clips_[1], 1.0f, false);
    RunScriptState(this, kScriptState_Intro);
}

// @func 0x0036E210 HudPowMeter::Hide
void HudPowMeter::Hide()
{
    const uint32_t state = MeterStateOf(this);
    if (state == kMeter_Hidden || state == kMeter_Outro)
        return;
    if (object)
        object->SetVisible(true, true);
    SetMeterState(this, kMeter_Outro);
    PlayHudClip(animController, clips_[3], 1.0f, false);
    RunScriptState(this, kScriptState_Outro);
}

// @func 0x0036E498 HudPowMeter::SetFill
void HudPowMeter::SetFill(int32_t value, int32_t max)
{
    const float fill = (float(value) / float(max)) * 0.5f + 0.5f;
    SetFillValue(fillA_, fill);
    SetFillValue(fillB_, fill);
}

// @func 0x0036E714 HudPowMeter::PlayPipScript
void HudPowMeter::PlayPipScript(int index)
{
    if (pipScripts_[index])
        PlayGlobalScript(pipScripts_[index]);
}

// @func 0x0036E9BC HudPowMeter::SetPipCount
void HudPowMeter::SetPipCount(int count)
{
    for (int i = 0; i < kPipCount; ++i) {
        if (pips_[i])
            pips_[i]->SetVisible(i < count, false);
    }
}

// @func 0x0036EA80 HudPowMeter::SetLevel
void HudPowMeter::SetLevel(int32_t level)
{
    level_ = level;
    RunScriptState(this, kLevelStateBase + uint32_t(level));
}

// @func 0x0036DED8 HudPowMeter::Update
void HudPowMeter::Update(float dt)
{
    switch (MeterStateOf(this)) {
    case kMeter_Hidden:
        return;
    case kMeter_Intro:
        if (HudClipDone(animController)) {
            SetMeterState(this, kMeter_Shown);
            PlayHudClip(animController, clips_[2], 1.0f, false);
            RunScriptState(this, kScriptState_Idle);
        }
        break;
    case kMeter_Shown:
        // The first shown frame keeps the object's transform.
        if (object && !transformSaved_) {
            savedTransform_ = object->GetLocal();
            transformSaved_ = true;
        }
        break;
    case kMeter_Outro:
        if (HudClipDone(animController)) {
            if (object)
                object->SetVisible(false, true);
            SetMeterState(this, kMeter_Hidden);
        }
        break;
    default:
        break;
    }
    HudObject::Update(dt);
}

// @func 0x0036E184 HudPowMeter::Refresh
void HudPowMeter::Refresh()
{
    SetActive(false);
    const float origin[3] = {ape::world::kWorldZero.x, ape::world::kWorldZero.y, ape::world::kWorldZero.z};
    SetStackOffset(origin, true);
    fraction_ = 1.0f;
    drainTimer_ = 0.0f;
    holdTimer_ = 0.0f;
    animState = 1;
}

// @func 0x0036E750 HudPowMeter::SetActive
void HudPowMeter::SetActive(bool active)
{
    if (!object)
        return;
    if (!active) {
        hiddenParts_.Clear();
        CollectParts(object, false);
        return;
    }
    for (ape::world::ApeObj* part : hiddenParts_)
        part->SetVisible(true, false);
    object->SetVisible(true, false);
}

// @func 0x0036E868 HudPowMeter::CollectParts
void HudPowMeter::CollectParts(ape::world::ApeObj* part, bool show)
{
    // NOTE: like the console, the walk also takes in the start object's
    // siblings.
    if (ape::world::ApeObj* sibling = part->GetNextSibling())
        CollectParts(sibling, show);
    if (ape::world::ApeObj* child = part->GetFirstChild())
        CollectParts(child, show);
    if (!show && part->IsVisible()) {
        hiddenParts_.Add(part);
        part->SetVisible(false, false);
    }
}

// @func 0x0036E2F0 HudPowMeter::SetEnabled
void HudPowMeter::SetEnabled(bool enabled, bool /*flag*/)
{
    if (!enabled) {
        Hide();
        return;
    }
    Show();
    EnterStack();
}

// ---------------------------------------------------------------------------
// Manager side

// @func 0x003651F0 hud::CreateMeter
HudMeter* CreateMeter(uint32_t name)
{
    if (!name)
        return nullptr;
    HudObjectDef* def = FindDef(name, kHud_Type8);
    if (!def)
        return nullptr;
    auto* meter = new HudMeter(def);
    meter->StartLifetimeScript();
    meter->Show();
    RegisterElement(meter);
    return meter;
}

// @func 0x00364980 hud::UpdatePowMeter
void UpdatePowMeter()
{
    HudManagerState& hud = Hud();
    if (hud.powMeter) {
        if (hud.powMeter->RegisteredName() == hud.powMeterName)
            return;
        DestroyElement(hud.powMeter);
    }
    if (!hud.powMeterName)
        return;
    HudObjectDef* def = FindDef(hud.powMeterName, kHud_Type10);
    if (!def)
        return;
    auto* pow = new HudPowMeter(def);
    hud.powMeter = pow;
    pow->StartLifetimeScript();
    pow->Show();
    RegisterElement(pow);
}

// @func 0x00365360 hud::UpdateMeterA
void UpdateMeterA()
{
    HudManagerState& hud = Hud();
    if (!hud.meterA || hud.meterA->RegisteredName() != hud.contextName) {
        if (hud.meterA) {
            hud.meterA->LeaveStack();
            DestroyElement(hud.meterA);
        }
        hud.meterA = CreateMeter(hud.contextName);
        if (hud.meterA)
            hud.meterA->EnterStack();
    }
    if (auto* meter = static_cast<HudMeter*>(hud.meterA)) {
        meter->useOwner_ = hud.contextEntity != nullptr;
        meter->meterOwner = hud.contextEntity;
    }
}

// @func 0x003654C0 hud::UpdateMeterB
void UpdateMeterB()
{
    HudManagerState& hud = Hud();
    if (hud.meterB) {
        if (hud.meterB->RegisteredName() == hud.pendingScreenName)
            return;
        DestroyElement(hud.meterB);
    }
    hud.meterB = CreateMeter(hud.pendingScreenName);
}

HudMeter* FindMeterOf(const entity::Entity* owner)
{
    for (HudObject* meter : Hud().meters) {
        if (meter->meterOwner == owner)
            return static_cast<HudMeter*>(meter);
    }
    return nullptr;
}

// @func 0x00369910 hud::AddMeterFor
void AddMeterFor(entity::Entity* owner, uint32_t name)
{
    if (!owner || FindMeterOf(owner))
        return;
    if (!name && owner->def)
        name = owner->def->nameHash;
    HudMeter* meter = CreateMeter(name);
    if (!meter)
        return;
    // NOTE: the bit is named kClass_400000 elsewhere; here it marks an
    // entity that has a HUD meter.
    owner->classBits |= entity::kClass_400000;
    Hud().meters.Add(meter);
    meter->EnterStack();
    meter->meterOwner = owner;
    meter->useOwner_ = true;
}

// @func 0x00369A80 hud::RemoveMeterOf
void RemoveMeterOf(entity::Entity* owner)
{
    if (!owner)
        return;
    owner->classBits &= ~uint64_t(entity::kClass_400000);
    auto& meters = Hud().meters;
    for (uint32_t i = 0; i < meters.Count(); ++i) {
        if (meters[i]->meterOwner == owner) {
            Hud().retiredMeters.Add(meters[i]);
            meters.RemoveAtSwap(i);
            return;
        }
    }
}

// @func 0x00369C30 hud::SetSecondFillOf
void SetSecondFillOf(const entity::Entity* owner, float fraction)
{
    if (HudMeter* meter = FindMeterOf(owner))
        meter->SetSecondFill(fraction);
}

// @func 0x003697B0 hud::SetMeterBSecondFill
void SetMeterBSecondFill(float fraction)
{
    if (Hud().meterB)
        static_cast<HudMeter*>(Hud().meterB)->SetSecondFill(fraction);
}

} // namespace hud
} // namespace sly
