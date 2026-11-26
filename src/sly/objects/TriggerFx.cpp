// openTIT - FX and push-block trigger volumes.
#include "sly/objects/TriggerFx.h"

#include "ape/anim/ApeAnimScript.h"
#include "sly/levels/script/ScriptCommand.h"
#include "sly/objects/TriggerTuning.h"
#include "sly/objects/TriggerTypes.h"

#include <algorithm>

namespace sly {
namespace objects {

namespace {
constexpr uint32_t kAttr_ScriptLifetimeName = 0xF89FDD94;  // "ScriptLifetime"
constexpr size_t kFxTextCapacity = 16;
// Script variable / states of the lifetime script. NOTE: names unknown.
constexpr uint32_t kScriptVarStaying = 0x2C73353E;
constexpr uint32_t kScriptStateStay = 0xF89061F1;
constexpr uint32_t kScriptStateDisabled = 0xF6239C09;
constexpr uint32_t kScriptStateEnabled = 0xA6CB0022;
// Collision filter of push-block volumes.
constexpr uint32_t kPushBlockFilter = 0x15;
// Entity type bit (+0xC4) of push blocks.

// One "X" value sets min and max; otherwise "XMin"/"XMax" are read.
void ReadRange(ape::xml::XmlElement* elem, uint32_t single, uint32_t minAttr, uint32_t maxAttr,
               float& minValue, float& maxValue)
{
    float value = 0.0f;
    if (elem->GetFloat(single, &value, 0.0f) == ape::xml::kQuery_Ok) {
        minValue = value;
        maxValue = value;
        return;
    }
    elem->GetFloat(minAttr, &minValue, minValue);
    elem->GetFloat(maxAttr, &maxValue, maxValue);
}

void ClampRange(float& minValue, float& maxValue)
{
    minValue = std::max(minValue, 0.0f);
    maxValue = std::max(maxValue, 0.0f);
    if (maxValue < minValue)
        maxValue = minValue;
}
} // namespace

void ParseFxTriggerParams(ape::xml::XmlElement* elem, FxTriggerParams& params, const uint32_t (&flagAttrs)[3])
{
    elem->GetHash(kAttr_ScriptLifetimeName, &params.script, params.script);
    for (uint32_t i = 0; i < 3; ++i) {
        bool value = (params.flags & (1u << i)) != 0;
        elem->GetBool(flagAttrs[i], &value, value);
        if (value)
            params.flags |= 1u << i;
    }
    ReadRange(elem, kAttr_FxDelay, kAttr_FxDelayMin, kAttr_FxDelayMax, params.delayMin, params.delayMax);
    ReadRange(elem, kAttr_FxDelay2, kAttr_FxDelay2Min, kAttr_FxDelay2Max, params.delay2Min, params.delay2Max);
    elem->GetHash(kAttr_FxMessage, &params.message, params.message);
    ClampRange(params.delayMin, params.delayMax);
    ClampRange(params.delay2Min, params.delay2Max);
}

// ---------------------------------------------------------------------------
// Definition

FxTriggerDefDesc::FxTriggerDefDesc(const EntityDefDesc& loader, const char* prefix)
    : TriggerDefDesc(kDefClass_FxTrigger | kDefClass_Trigger, loader, prefix)
{
}

// @func 0x0021DEE0 FxTriggerDefDesc::Parse
void FxTriggerDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                             const EntityDefDesc& loader, const EntityDef* parent)
{
    TriggerDefDesc::Parse(root, elem, loader, parent);
    field_20 = 1;
    // An FX parent supplies the defaults (a non-FX parent only drew a warning).
    if (parent && parent->IsClass(kDefClass_FxTrigger))
        params = static_cast<const FxTriggerDef*>(parent)->params;
    ParseFxTriggerParams(elem, params, kFxFlagAttrs);
}

// @func 0x0021E23C CreateFxTriggerDef
EntityDef* CreateFxTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                              const EntityDefDesc& loader, const EntityDef* parent)
{
    FxTriggerDefDesc desc(loader, "TriggerFX_");
    desc.Parse(root, elem, loader, parent);
    return new FxTriggerDef(desc);
}

FxTriggerInstanceDesc::FxTriggerInstanceDesc(const char* instanceName, FxTriggerDef* ownerDef)
    : TriggerInstanceDesc(instanceName, ownerDef)
{
    // NOTE: the console relies on the placement to fill these; the
    // definition's values are used when it does not.
    if (ownerDef)
        params = ownerDef->params;
    texts.reserve(kFxTextCapacity);
}

// @func 0x0021E620 FxTriggerDef::Spawn
Entity* FxTriggerDef::Spawn(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->flags & kDefClass_FxTrigger) != 0))
        return nullptr;
    FxTriggerInstanceDesc desc(InstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    FxTrigger* trigger = new ScriptedFxTrigger(desc);
    trigger->OnSpawn();
    return trigger;
}

// ---------------------------------------------------------------------------
// Placement

FxTriggerPlacement::FxTriggerPlacement(const char* placementName)
    : TriggerPlacement(placementName, nullptr, nullptr, true, false, true, false, false, false)
{
    flags |= kDefClass_FxTrigger;
    texts.reserve(kFxTextCapacity);
}

// @func 0x0021E978 ReadFxTriggerPlacement
static void ReadFxTriggerPlacement(ape::xml::XmlElement* elem, FxTriggerPlacement& placement)
{
    ParseTriggerPlacementFlags(elem, placement);
    for (ape::xml::XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (child->GetNameHash() == kElem_FxTexts)
            levels::ParseTextFieldList(child, placement.texts, kFxTextCapacity);
    }
    ParseFxTriggerParams(elem, placement.params, kFxFlagAttrs);
}

// @func 0x0021ED78 ParseFxTriggerPlacement
EntityPlacement* ParseFxTriggerPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                         const char* name)
{
    (void)root;
    FxTriggerPlacement* placement = new FxTriggerPlacement(name);
    ReadFxTriggerPlacement(elem, *placement);
    return placement;
}

// @func 0x0021EEA4 FxTriggerPlacement::FillInstanceDesc
void FxTriggerPlacement::FillInstanceDesc(EntityInstanceDesc& baseDesc) const
{
    TriggerPlacement::FillInstanceDesc(baseDesc);
    FxTriggerInstanceDesc& desc = static_cast<FxTriggerInstanceDesc&>(baseDesc);
    desc.params = params;
    for (const TextField& text : texts)
        desc.texts.push_back(text);
}

// ---------------------------------------------------------------------------
// Entity

// @func 0x0021E3F8 FxTrigger::FxTrigger
FxTrigger::FxTrigger(const FxTriggerInstanceDesc& desc)
    : TriggerBase(desc, 0x20, true), m_params(desc.params)
{
    m_hasTexts = !desc.texts.empty();
    // A message the player has already read is not shown again.
    if (m_params.message != 0 && TextDisplayHasSeen(m_params.message)) {
        m_messageShown = true;
        m_messageSeen = true;
    }
    m_scriptDef = ape::ApeAnimScriptLibrary::FindScriptAnywhere(m_params.script, 0, true);
    if (m_scriptDef) {
        m_script = EntityStartScript(this, m_scriptDef);
        if (m_script) {
            ScriptInstanceSetPaused(m_script, false);
            m_scriptRunning = true;
            ScriptSetTriggerVar(m_script, this);
        }
    }
}

// @func 0x0075B7EC ScriptedFxTrigger::~ScriptedFxTrigger
// @func 0x0075B818 ScriptedFxTrigger::~ScriptedFxTrigger (deleting)
ScriptedFxTrigger::~ScriptedFxTrigger() = default;

// @func 0x0021F088 FxTrigger::~FxTrigger
// @func 0x0021F1EC FxTrigger::~FxTrigger (deleting)
FxTrigger::~FxTrigger()
{
    if (m_script)
        ScriptInstanceSetPaused(m_script, true);
}

// @func 0x00220024 FxTrigger::OnStay
void FxTrigger::OnStay(Entity* entity)
{
    if (!m_script)
        return;
    ScriptSetEntityVar(m_script, kScriptVarStaying, entity);
    ScriptSetState(m_script, kScriptStateStay);
}

// @func 0x00220190 FxTrigger::SetEnabled
void FxTrigger::SetEnabled(bool enabled)
{
    if (enabled != IsEnabled() && m_script)
        ScriptSetState(m_script, enabled ? kScriptStateEnabled : kScriptStateDisabled);
    ChangeEnabled(enabled);
}

// @func 0x00220158 FxTrigger::IsDebugDisplayed
bool FxTrigger::IsDebugDisplayed() const
{
    return TriggerDebugSwitch(g_debugDisplayTriggerFX.Get());
}

// @func 0x002202BC FxTrigger::RestartScript
void FxTrigger::RestartScript()
{
    if (m_params.script == 0 || (m_scriptDef && m_script))
        return;
    if (m_script) {
        ScriptInstanceSetPaused(m_script, true);
        m_script = nullptr;
    }
    m_scriptDef = ape::ApeAnimScriptLibrary::FindScriptAnywhere(m_params.script, 0, true);
    if (!m_scriptDef)
        return;
    m_script = EntityStartScript(this, m_scriptDef);
    if (m_script) {
        ScriptInstanceSetPaused(m_script, false);
        m_scriptRunning = true;
        ScriptSetTriggerVar(m_script, this);
    }
}

// ---------------------------------------------------------------------------
// Push block

// @func 0x00221760 PushBlockTriggerDefDesc::Parse
static void ParsePushBlockDefDesc(FxTriggerDefDesc& desc, ape::xml::XmlElement* root,
                                  ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                                  const EntityDef* parent)
{
    desc.Parse(root, elem, loader, parent);
    desc.field_20 = 1;
    // (A parent that is not a push-block trigger only drew a warning.)
}

// @func 0x002217E0 CreatePushBlockTriggerDef
EntityDef* CreatePushBlockTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                     const EntityDefDesc& loader, const EntityDef* parent)
{
    FxTriggerDefDesc desc(loader, "TriggerPushBlock_");
    desc.typeBits = kTrigType_PushBlock;
    ParsePushBlockDefDesc(desc, root, elem, loader, parent);
    return new PushBlockTriggerDef(desc);
}

// @func 0x002219A8 PushBlockTriggerDef::Spawn
Entity* PushBlockTriggerDef::Spawn(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->typeBits & kTrigType_PushBlock) != 0))
        return nullptr;
    FxTriggerInstanceDesc desc(InstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    PushBlockTrigger* trigger = new PushBlockTrigger(desc);
    trigger->OnSpawn();
    return trigger;
}

PushBlockTrigger::PushBlockTrigger(const FxTriggerInstanceDesc& desc) : FxTrigger(desc)
{
    if (m_volume)
        VolumeSetCollisionFilter(m_volume, kPushBlockFilter);
}

// @func 0x00221D44 PushBlockTrigger::~PushBlockTrigger
// @func 0x00221D70 PushBlockTrigger::~PushBlockTrigger (deleting)
PushBlockTrigger::~PushBlockTrigger() = default;

// @func 0x00221DD0 PushBlockTrigger::OnEnter
void PushBlockTrigger::OnEnter(Entity* entity)
{
    if (entity && (entity->HasTypeBit(kType_Pushable)))
        FxTrigger::OnEnter(entity);
}

// @func 0x00221E44 PushBlockTrigger::OnExit
void PushBlockTrigger::OnExit(Entity* entity)
{
    if (!entity || !(entity->HasTypeBit(kType_Pushable)))
        return;
    // NOTE: the console runs the FX leave helper (0x0021FB38, not ported)
    // unless the message has already been seen; only the base part remains.
    FxTrigger::OnExit(entity);
}

} // namespace objects
} // namespace sly
