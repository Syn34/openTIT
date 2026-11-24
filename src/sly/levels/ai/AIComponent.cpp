// openTIT - AI component and its description.
#include "sly/levels/ai/AIComponent.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"
#include "sly/levels/ai/AIBehaviours.h"
#include "sly/levels/ai/AIHooks.h"

#include <new>

namespace sly {
namespace levels {
namespace ai {

using ape::xml::XmlElement;

namespace {

constexpr uint32_t kAttr_Name = 0x0024728Bu;       // "name"
constexpr uint32_t kAttr_Type = 0x0027873Au;       // "type"
constexpr uint32_t kAttr_Setting = 0x3A034393u;    // NOTE: attribute name unknown
constexpr uint32_t kAttr_Level = 0x044FA364u;      // "level"
constexpr uint32_t kElem_Pattern = 0xFBA47770u;    // "Pattern"
constexpr uint32_t kElem_State = 0x04B8CC71u;      // "state"

AIStatePatterns g_patterns;

AIController* CreateBehaviour(uint32_t type)
{
    switch (type) {
    case kAIType_Cpu:         return new CpuAI();
    case kAIType_C83BCA3D:    return new BismarqueAI();
    case kAIType_BC6EBE7B:    return new ParadoxAI();
    case kAIType_ElJefe:      return new ElJefeAI();
    case kAIType_Player:      return new PlayerAI();
    case kAIType_7B70E717:    return new SentryAIVariant();
    case kAIType_Critter:     return new CritterAI();
    case kAIType_PatternBoss: return new PatternBossAI();
    case kAIType_4908BDCF:    return new SentryAI();
    case kAIType_MsDecibel:   return new MsDecibelAI();
    default:                  return nullptr;
    }
}

} // namespace

AIStatePatterns& PatternBossPatterns() { return g_patterns; }

// @func 0x00017124 ParseAIStatePatterns
void ParseAIStatePatterns(XmlElement* elem)
{
    // NOTE: the console never empties the count array (0x00F9BCA8); kept.
    g_patterns.levels.Clear();
    g_patterns.states.Clear();
    g_patterns.first.Clear();
    uint32_t first = 0;
    for (XmlElement* pattern = elem->FirstChildElement(); pattern; pattern = pattern->NextSiblingElement()) {
        if (pattern->GetNameHash() != kElem_Pattern)
            continue;
        uint32_t level = 0;
        pattern->GetHash(kAttr_Level, &level, 0);
        for (XmlElement* state = pattern->FirstChildElement(); state; state = state->NextSiblingElement()) {
            if (state->GetNameHash() != kElem_State)
                continue;
            uint32_t name = 0;
            state->GetHash(kAttr_Name, &name, 0);
            g_patterns.states.PushBack(name);
        }
        // NOTE: Ghidra shows 0 stored here; the "level" value read above is
        // assumed.
        g_patterns.levels.PushBack(level);
        g_patterns.first.PushBack(first);
        const uint32_t end = g_patterns.states.Count();
        g_patterns.count.PushBack(end - first);
        first = end;
    }
}

// @func 0x00017420 ParseAIComponentDesc
entity::ComponentDesc* ParseAIComponentDesc(XmlElement* root, XmlElement* elem, uint8_t scope)
{
    const char* name = elem->GetString("name");
    if (!name) {
        (void)root->GetDocumentPath();
        return nullptr;
    }
    auto* desc = new AIComponentDesc(ape::HashNameNoCase(name));
    desc->scope = scope;
    elem->GetHash(kAttr_Type, &desc->aiType, desc->aiType);
    elem->GetHash(kAttr_Setting, &desc->settingsName, desc->settingsName);
    if (desc->aiType == kAIType_PatternBoss)
        ParseAIStatePatterns(elem);
    return desc;
}

// @func 0x000179C0 AIComponent::AIComponent
AIComponent::AIComponent(const AIComponentDesc* desc)
{
    id = desc->name;
    owner = desc->owner;
    type = desc->type;
    settings = desc->settingsName ? FindSettingObject(desc->settingsName) : nullptr;
    controller = CreateBehaviour(desc->aiType);
    if (controller)
        return;

    // Standard enemy; a character spawner may pick the settings by the
    // definition it spawned.
    uint32_t defNames[3] = {};
    const PropertyBlock* spawnerSettings[3] = {};
    if (owner && CharacterSpawnerSettings(owner, defNames, spawnerSettings)) {
        const uint32_t defName = owner->def ? owner->def->nameHash : 0;
        settings = nullptr;
        for (int i = 0; i < 3; ++i) {
            if (defNames[i] == defName) {
                settings = spawnerSettings[i];
                break;
            }
        }
    }
    controller = new EnemyAI();
}

// @func 0x00017F84 AIComponent::~AIComponent
// @func 0x00018018 AIComponent::~AIComponent (deleting)
// @func 0x000180C8 AIComponent::~AIComponent (base object, used by derived components)
AIComponent::~AIComponent()
{
    controller->Detach();
    delete controller;
}

// @func 0x00017F64 CreateAIComponent
AIComponent* CreateAIComponent(AIComponent* storage, const AIComponentDesc* desc)
{
    return new (storage) AIComponent(desc);
}

// @func 0x0001815C AIComponent::OnOwnerSpawned
void AIComponent::OnOwnerSpawned(entity::Entity* newOwner)
{
    controller->Attach(newOwner);
}

// @func 0x0001819C AIComponent::Update
void AIComponent::Update(float dt)
{
    controller->Update(dt);
}

// @func 0x000181DC AIComponent::PostUpdate
void AIComponent::PostUpdate(float dt)
{
    controller->TickEntity(dt);
}

// @func 0x0001821C AIComponent::SetOption
void AIComponent::SetOption(bool value)
{
    if (value != enabled)
        controller->SetActive(value);
    enabled = value;
}

} // namespace ai
} // namespace levels
} // namespace sly
