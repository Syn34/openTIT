// openTIT - Upgrade shop tuning and the table of upgrade records.
#include "sly/game/Upgrades.h"

#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"
#include "sly/entity/Awards.h"
#include "ape/world/ApeObj.h"
#include "sly/entity/Entity.h"
#include "sly/game/SpawnManager.h"
#include "sly/game/hud/HudHooks.h"
#include "sly/game/hud/HudManager.h"
#include "sly/entity/EntityHooks.h"
#include "sly/gameplay/Sequencer.h"
#include "sly/levels/LevelServices.h"
#include "sly/levels/script/ScriptVars.h"
#include "sly/game/goals/JobManager.h"

#include <cstring>

namespace sly {

// @func 0x0049E78C StaticInit_UpgradeTuning
// @func 0x0049E738 StaticDestroy_UpgradeTuning
// Compiler-generated initialiser/destructor pair for the globals of this file
// (the two tuning values and the zero-initialised upgrade table).
ape::TuningInt g_upgradePurchaseFirstAwardNum("Game.Upgrades.PurchaseFirstAwardNum", 0x11);
ape::TuningInt g_upgradePurchaseAllAwardNum("Game.Upgrades.PurchaseAllAwardNum", 0x2A);

static UpgradeEntry s_upgrades[kUpgradeCount];   // 0x010BE8BC

// @func 0x0049E704 GetUpgradeEntry
UpgradeEntry* GetUpgradeEntry(uint32_t index)
{
    return index < kUpgradeCount ? &s_upgrades[index] : nullptr;
}

uint32_t& LoadedUpgradeCount()
{
    static uint32_t count = 0;   // 0x010C1A9C
    return count;
}

uint8_t& UpgradeState(uint32_t index)
{
    static uint8_t states[kUpgradeCount] = {};   // game world +0xFBC
    static uint8_t dummy = 0;
    return index < kUpgradeCount ? states[index] : (dummy = 0);
}

// @func 0x0049E330 CollectAvailableUpgrades
void CollectAvailableUpgrades(ape::ApeTArray<uint32_t>& out)
{
    for (uint32_t i = 0; i < LoadedUpgradeCount() && i < kUpgradeCount; ++i) {
        if (s_upgrades[i].field_00 != 0 && (UpgradeState(i) & kUpgrade_Available))
            out.PushBack(i);
    }
}

// @func 0x0049E860 TextField::Clear
void TextField::Clear()
{
    Reset(0);
}

// @func 0x0049E8A8 TextField::Reset
void TextField::Reset(uint32_t newId)
{
    id = newId;
    flagA = kBinding_None;
    flagB = false;
    std::memset(text, 0, sizeof(text));
}

// @func 0x0049EA74 ScriptBindingTypeFromName
ScriptBindingType ScriptBindingTypeFromName(uint32_t nameHash)
{
    switch (nameHash) {
    case 0x00014B07u: return kBinding_Var;           // "var"
    case 0x7A30D7E3u: return kBinding_Entity;        // "entity"
    case 0xEB9EE198u: return kBinding_Trigger;       // "trigger"
    case 0xB2E59948u: return kBinding_Spawner;       // "Spawner"
    case 0xE89DF217u: return kBinding_SpawnerGroup;  // "SpawnerGroup"
    case 0x044DC6ABu: return kBinding_Laser;         // "laser"
    case 0xAB27550Fu: return kBinding_LaserField;    // "LaserField"
    case 0xAB39AA94u: return kBinding_LaserGroup;    // "laserGroup"
    case 0x0021B90Eu: return kBinding_Hash;          // "hash"
    case 0x92685411u: return kBinding_String;        // "string"
    case 0x03FE2A3Cu: return kBinding_Float;         // "float"
    case 0x0027BF5Au: return kBinding_UInt;          // "uint"
    case 0x001F32EAu: return kBinding_Bool;          // "bool"
    case 0x0028233Fu: return kBinding_Vec3;          // "vec3"
    case 0x87208BC1u: return kBinding_Matrix;        // "matrix"
    default: return kBinding_None;
    }
}

// @func 0x0049EC08 ParseScriptBinding
bool ParseScriptBinding(ape::xml::XmlElement* element, TextField* binding)
{
    constexpr uint32_t kAttrName = 0x0024728Bu;    // NOTE: the variable name attribute
    constexpr uint32_t kAttrValue = 0x04DA9951u;   // NOTE: the value attribute
    using ape::xml::kQuery_Ok;
    if (!element || !binding)
        return false;
    const ScriptBindingType type = ScriptBindingTypeFromName(element->GetNameHash());
    if (type == kBinding_None)
        return false;
    uint32_t name = 0;
    if (element->GetHash(kAttrName, &name, 0) != kQuery_Ok || name == 0)
        return false;
    binding->id = name;
    binding->flagA = type;

    switch (type) {
    case kBinding_Var:
    case kBinding_Spawner:
    case kBinding_SpawnerGroup:
    case kBinding_Laser:
    case kBinding_LaserField:
    case kBinding_LaserGroup:
    case kBinding_Hash:
        return element->GetHash(kAttrValue, &binding->hash, 0) == kQuery_Ok && binding->hash != 0;
    case kBinding_Entity:
    case kBinding_Trigger:
        return element->GetHash(kAttrValue, &binding->entity.name, 0) == kQuery_Ok && binding->entity.name != 0;
    case kBinding_Float:
        return element->GetFloat(kAttrValue, &binding->floatValue, 0.0f) == kQuery_Ok;
    case kBinding_UInt: {
        int32_t value = 0;
        const bool ok = element->GetInt(kAttrValue, &value, 0) == kQuery_Ok;
        binding->uintValue = uint32_t(value);
        return ok;
    }
    case kBinding_Bool:
        return element->GetBool(kAttrValue, &binding->boolValue, false) == kQuery_Ok;
    case kBinding_Vec3: {
        float value[3] = {};
        const float zero[3] = {};
        if (element->GetVec3(kAttrValue, value, zero) != kQuery_Ok)
            return false;
        std::memcpy(binding->vec3, value, sizeof(value));
        return true;
    }
    case kBinding_Matrix:
        return true;
    case kBinding_String: {
        const char* text = element->GetString(kAttrValue);
        if (!text || !*text)
            return false;
        ape::StrCopyBounded(binding->text, text, 0x40);
        return true;
    }
    default:
        return false;
    }
}

// @func 0x0049FD84 AddEntityBinding
void AddEntityBinding(std::vector<TextField>& list, uint32_t varName, WorldObject* entity)
{
    if (!varName || !entity)
        return;
    TextField binding;
    binding.id = varName;
    binding.entity.object = entity;
    binding.flagA = kBinding_Entity;
    list.push_back(binding);
}

// @func 0x0049FE90 AddUIntBinding
void AddUIntBinding(std::vector<TextField>& list, uint32_t varName, uint32_t value)
{
    if (!varName || !value)
        return;
    TextField binding;
    binding.id = varName;
    binding.uintValue = value;
    binding.flagA = kBinding_UInt;
    list.push_back(binding);
}

// Stores the current value of a script variable in a binding record:
// floats, ints and hashes in the first word, a vector in the first three
// floats, a matrix as four rows of (x, y, z, -). Other variable types are
// left alone.
// @func 0x0049E8E4 CopyScriptVarToBinding
void CopyScriptVarToBinding(TextField& out, const ape::ApeAnimScriptVar* var)
{
    if (!var)
        return;
    switch (var->GetType()) {
    case ape::AnimScriptVarType::Hash:
        out.hash = var->GetHash();
        break;
    case ape::AnimScriptVarType::Matrix: {
        const ape::Matrix34& m = static_cast<const ape::ApeAnimScriptMatrixVar*>(var)->GetMatrix();
        const ape::Vec3* rows[4] = {&m.xAxis, &m.yAxis, &m.zAxis, &m.translation};
        for (int r = 0; r < 4; ++r) {
            out.matrix[r * 4 + 0] = rows[r]->x;
            out.matrix[r * 4 + 1] = rows[r]->y;
            out.matrix[r * 4 + 2] = rows[r]->z;
        }
        break;
    }
    case ape::AnimScriptVarType::Vec3: {
        const ape::Vec3& v = static_cast<const ape::ApeAnimScriptVec3Var*>(var)->GetVec3();
        out.vec3[0] = v.x;
        out.vec3[1] = v.y;
        out.vec3[2] = v.z;
        break;
    }
    case ape::AnimScriptVarType::Int:
        out.uintValue = uint32_t(var->GetInt());
        break;
    case ape::AnimScriptVarType::Float:
        out.floatValue = var->GetFloat();
        break;
    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// Applying bindings to a script instance
// ---------------------------------------------------------------------------
namespace {

using ape::AnimScriptVarType;
using ape::ApeAnimScriptVar;

// Object variables hold the entity's (or HUD element's) world object.
// NOTE: the console also stores the animation controller pointer in the
// variable's word +0x24; 64-bit pointers do not fit there, it stays 0.
void SetObjectVar(ApeAnimScriptVar* var, ape::ApeRefCounted* object)
{
    auto* objectVar = static_cast<ape::ApeAnimScriptObjectVar*>(var);
    objectVar->SetObject(object);
    objectVar->SetField24(0);
}

template <class Var, class T>
bool SetHandleVar(ApeAnimScriptVar* var, int type, T value)
{
    if (!value || int(var->GetType()) != type)
        return false;
    static_cast<Var*>(var)->SetValue(value);
    return true;
}

// The entity of an entity / trigger binding: the stored one, else found by
// name (among the sequence's entities when `sequence` is set).
WorldObject* BindingEntity(const TextField& binding, bool sequence)
{
    if (binding.entity.object)
        return binding.entity.object;
    if (sequence)
        return gameplay::FindSequenceEntity(binding.entity.name);
    return reinterpret_cast<WorldObject*>(entity::FindEntity(binding.entity.name));
}

// A `var` binding: the value of variable `binding.hash` of `source`.
void CopyFromSource(ApeAnimScriptVar* var, const ApeAnimScriptVar* from)
{
    const AnimScriptVarType type = from->GetType();
    if (type != var->GetType())
        return;
    switch (type) {
    case AnimScriptVarType::Object:
        static_cast<ape::ApeAnimScriptObjectVar*>(var)->CopyReference(
            *static_cast<const ape::ApeAnimScriptObjectVar*>(from));
        break;
    case AnimScriptVarType::Int:
        static_cast<ape::ApeAnimScriptIntVar*>(var)->SetInt(from->GetInt());
        break;
    case AnimScriptVarType::Float:
        static_cast<ape::ApeAnimScriptFloatVar*>(var)->SetFloat(from->GetFloat());
        break;
    default:
        if (int(type) == levels::kVarType_String)
            static_cast<levels::ScriptStringVar*>(var)->SetString(
                static_cast<const levels::ScriptStringVar*>(from)->GetString());
        break;
    }
}

} // namespace

// @func 0x0049F080 ApplyScriptBinding
void ApplyScriptBinding(ape::ApeAnimScriptInstance* instance, const TextField& binding,
                        ape::ApeAnimScriptInstance* source, bool sequenceEntities, bool quiet)
{
    if (!instance || binding.flagA == kBinding_None)
        return;
    ApeAnimScriptVar* var = instance->FindVar(binding.id);
    if (!var)
        return;
    const int type = int(var->GetType());
    bool bound = true;
    switch (binding.flagA) {
    case kBinding_Var:
        if (source) {
            if (const ApeAnimScriptVar* from = source->FindVar(binding.hash))
                CopyFromSource(var, from);
        }
        break;
    case kBinding_Entity: {
        auto* entity = reinterpret_cast<entity::Entity*>(BindingEntity(binding, sequenceEntities));
        bound = entity && type == int(AnimScriptVarType::Object);
        if (bound)
            SetObjectVar(var, entity::EntityWorldObjectRef(entity));
        break;
    }
    case kBinding_Trigger: {
        WorldObject* trigger = BindingEntity(binding, sequenceEntities);
        auto* entity = reinterpret_cast<entity::Entity*>(trigger);
        // Only trigger entities (type bit 22) fit a trigger variable.
        bound = entity && entity->HasTypeBit(22) &&
                SetHandleVar<levels::ScriptTriggerVar>(var, levels::kVarType_Trigger, trigger);
        break;
    }
    case kBinding_Spawner:
        bound = g_spawnManager && SetHandleVar<levels::ScriptSpawnerVar>(var, levels::kVarType_Spawner,
                                                                         g_spawnManager->FindSpawner(binding.hash));
        break;
    case kBinding_SpawnerGroup:
        bound = g_spawnManager && SetHandleVar<levels::ScriptSpawnGroupVar>(var, levels::kVarType_SpawnGroup,
                                                                            g_spawnManager->FindGroup(binding.hash));
        break;
    case kBinding_Laser:
        bound = binding.hash != 0 &&
                SetHandleVar<levels::ScriptLaserVar>(var, levels::kVarType_Laser, levels::FindLaser(binding.hash));
        break;
    case kBinding_LaserField:
        bound = binding.hash != 0 && SetHandleVar<levels::ScriptLaserFieldVar>(
                                         var, levels::kVarType_LaserField, levels::FindLaserField(binding.hash));
        break;
    case kBinding_LaserGroup:
        bound = binding.hash != 0 && SetHandleVar<levels::ScriptLaserGroupVar>(
                                         var, levels::kVarType_LaserGroup, levels::FindLaserGroup(binding.hash));
        break;
    case 9: {
        // A HUD element by name: its scene object.
        if (binding.hash != 0 && type == int(AnimScriptVarType::Object)) {
            if (hud::HudObject** element = hud::Hud().objects.Find(binding.hash); element && *element)
                SetObjectVar(var, hud::ObjectScriptRef((*element)->object));
        }
        break;
    }
    case kBinding_Hash:
    case kBinding_UInt:
        if (type == int(AnimScriptVarType::Int))
            static_cast<ape::ApeAnimScriptIntVar*>(var)->SetInt(int32_t(binding.uintValue));
        break;
    case kBinding_Bool:
        if (type == int(AnimScriptVarType::Int))
            static_cast<ape::ApeAnimScriptIntVar*>(var)->SetInt(binding.boolValue ? 1 : 0);
        break;
    case kBinding_String:
        if (type == levels::kVarType_String)
            static_cast<levels::ScriptStringVar*>(var)->SetString(binding.text);
        break;
    case kBinding_Float:
        if (type == int(AnimScriptVarType::Float))
            static_cast<ape::ApeAnimScriptFloatVar*>(var)->SetFloat(binding.floatValue);
        break;
    case kBinding_Vec3:
        if (type == int(AnimScriptVarType::Vec3))
            static_cast<ape::ApeAnimScriptVec3Var*>(var)->SetVec3(
                ape::Vec3{binding.vec3[0], binding.vec3[1], binding.vec3[2], 0.0f});
        break;
    case kBinding_Matrix:
        if (type == int(AnimScriptVarType::Matrix)) {
            ape::Matrix34 m;
            ape::Vec3* rows[4] = {&m.xAxis, &m.yAxis, &m.zAxis, &m.translation};
            for (int r = 0; r < 4; ++r)
                *rows[r] = ape::Vec3{binding.matrix[r * 4], binding.matrix[r * 4 + 1], binding.matrix[r * 4 + 2], 0.0f};
            static_cast<ape::ApeAnimScriptMatrixVar*>(var)->SetMatrix(m);
        }
        break;
    default:
        break;
    }
    if (!bound && !quiet)
        (void)ape::TempHexString(binding.hash);   // compiled-out warning about the unresolved binding
}

// @func 0x0049FCC0 ApplyScriptBindings
void ApplyScriptBindings(ape::ApeAnimScriptInstance* instance, const TextField* bindings, size_t count,
                         ape::ApeAnimScriptInstance* source, bool sequenceEntities, bool quiet)
{
    if (!instance)
        return;
    for (size_t i = 0; i < count; ++i)
        ApplyScriptBinding(instance, bindings[i], source, sequenceEntities, quiet);
}

// qsort comparator of string pointers, case-insensitive.
// @func 0x0049B4E0 CompareStringPointers
int CompareStringPointers(const void* a, const void* b)
{
    return ape::StrCaseCompare(*static_cast<const char* const*>(a), *static_cast<const char* const*>(b));
}

// ---------------------------------------------------------------------------
// Upgrade shop
// ---------------------------------------------------------------------------
namespace {

uint32_t s_upgradeCount = 0;   // 0x010C1A9C

constexpr uint32_t kElem_Upgrades = 0x838099D7u;   // "Upgrades"
constexpr uint32_t kElem_Upgrade = 0x1D0425FCu;    // "Upgrade"
constexpr uint32_t kAttr_Id = 0x0000091Bu;         // "id"
constexpr uint32_t kAttr_Item = 0x08F12FA5u;       // NOTE: name unknown
constexpr uint32_t kAttr_Type = 0x0027873Au;       // "type"
constexpr uint32_t kAttr_Cost = 0x001FA7CDu;       // "cost"
constexpr uint32_t kAttr_UnlockJob = 0xC87D4DFCu;  // NOTE: name unknown
constexpr uint32_t kAttr_Name = 0xDCE08553u;       // NOTE: name unknown
constexpr uint32_t kAttr_Desc = 0x77FC9F6Cu;       // NOTE: name unknown
constexpr uint32_t kAttr_NotAbility = 0xC5E921C2u; // NOTE: name unknown

// Trophies of the shop. NOTE: award names unknown.
constexpr uint32_t kAward_FirstUpgrade = 0xE5FEEEF1u;
constexpr uint32_t kAward_AllUpgrades = 0x4E8EB622u;

// Notification bit raised when an upgrade becomes new.
constexpr uint32_t kNotify_NewUpgrade = 0x80;

uint8_t& StateOf(uint32_t index)
{
    return game::CurrentProgress().upgradeStates[index];
}

// Outside an episode the shop is unrestricted.
bool InEpisode()
{
    game::JobManager* jobs = game::GetJobManager();
    return jobs && jobs->currentJob != nullptr;
}

// Index of the upgrade with `id`, or -1.
int FindUpgrade(uint32_t id)
{
    for (uint32_t i = 0; i < s_upgradeCount; ++i)
        if (s_upgrades[i].id == id)
            return int(i);
    return -1;
}

bool Affordable(uint32_t index)
{
    if (!(StateOf(index) & kUpgrade_Unlocked) && InEpisode())
        return false;
    return int32_t(game::CurrentProgress().coins) >= s_upgrades[index].cost;
}

} // namespace

// @func 0x0049DC70 LoadUpgrades
bool LoadUpgrades(const char* path)
{
    s_upgradeCount = 0;
    ape::xml::XmlDocument doc(path);
    if (!doc.LoadFile(0))
        return false;
    ape::xml::XmlElement* root = doc.FirstChildElement(kElem_Upgrades);
    if (!root)
        return true;
    for (ape::xml::XmlElement* e = root->FirstChildElement(kElem_Upgrade); e && s_upgradeCount + 1 < kUpgradeCount;
         e = e->NextSiblingElement(kElem_Upgrade)) {
        UpgradeEntry& entry = s_upgrades[s_upgradeCount];
        e->GetHash(kAttr_Id, &entry.id, entry.id);
        e->GetHash(kAttr_Item, &entry.item, entry.item);
        e->GetHash(kAttr_Type, &entry.type, entry.type);
        e->GetInt(kAttr_Cost, &entry.cost, entry.cost);
        e->GetHash(kAttr_UnlockJob, &entry.unlockJob, entry.unlockJob);
        if (const char* name = e->GetString(kAttr_Name))
            ape::StrCopyBounded(entry.name, name, sizeof entry.name);
        if (const char* desc = e->GetString(kAttr_Desc))
            ape::StrCopyBounded(entry.desc, desc, sizeof entry.desc);
        e->GetBool(kAttr_NotAbility, &entry.notAbility, entry.notAbility);
        StateOf(s_upgradeCount) &= uint8_t(~(kUpgrade_Unlocked | kUpgrade_Bought));
        ++s_upgradeCount;
    }
    return true;
}

// @func 0x0049E038 UnlockUpgradesOfJob
void UnlockUpgradesOfJob(uint32_t jobId)
{
    for (uint32_t i = 0; i < s_upgradeCount; ++i) {
        if (s_upgrades[i].unlockJob != jobId)
            continue;
        uint8_t& state = StateOf(i);
        if (!(state & kUpgrade_Unlocked)) {
            state |= kUpgrade_Unlocked | kUpgrade_New;
            game::CurrentProgress().notifyBits |= kNotify_NewUpgrade;
        }
    }
}

// @func 0x0049E0DC BuyUpgrade
bool BuyUpgrade(uint32_t id)
{
    const int index = FindUpgrade(id);
    if (index < 0 || !Affordable(uint32_t(index)))
        return false;
    uint8_t& state = StateOf(uint32_t(index));
    if (state & kUpgrade_Bought)
        return false;
    state |= kUpgrade_Unlocked | kUpgrade_Bought;
    game::CurrentProgress().coins -= uint32_t(s_upgrades[index].cost);

    uint32_t bought = 0;
    for (uint32_t i = 0; i < s_upgradeCount; ++i)
        if (StateOf(i) & kUpgrade_Bought)
            ++bought;
    if (s_upgradeCount != 0 && bought != 0)
        AwardTrophyByName(kAward_FirstUpgrade);
    if (bought >= s_upgradeCount)
        AwardTrophyByName(kAward_AllUpgrades);
    return true;
}

// @func 0x0049E268 CanBuyUpgrade
bool CanBuyUpgrade(uint32_t id)
{
    const int index = FindUpgrade(id);
    return index >= 0 && Affordable(uint32_t(index));
}

// @func 0x0049E330 CollectUnlockedUpgrades
void CollectUnlockedUpgrades(std::vector<uint32_t>& out)
{
    for (uint32_t i = 0; i < s_upgradeCount; ++i)
        if (s_upgrades[i].id != 0 && (StateOf(i) & kUpgrade_Unlocked))
            out.push_back(i);
}

// @func 0x0049E43C CollectBoughtItems
void CollectBoughtItems(uint32_t type, std::vector<uint32_t>& out)
{
    for (uint32_t i = 0; i < s_upgradeCount; ++i) {
        const UpgradeEntry& entry = s_upgrades[i];
        const uint8_t state = StateOf(i);
        if (entry.type == type && entry.item != 0 && entry.id != 0 && !entry.notAbility &&
            (state & kUpgrade_Unlocked) && (state & kUpgrade_Bought))
            out.push_back(entry.item);
    }
}

// @func 0x0049E5AC AnyNewUpgrade
bool AnyNewUpgrade()
{
    for (uint32_t i = 0; i < s_upgradeCount; ++i)
        if (s_upgrades[i].id != 0 && (StateOf(i) & kUpgrade_New))
            return true;
    return false;
}

// @func 0x0049E628 UpgradeOwned
bool UpgradeOwned(uint32_t type, uint32_t item)
{
    if (!InEpisode())
        return true;
    if (type == 0 || item == 0)
        return false;
    for (uint32_t i = 0; i < s_upgradeCount; ++i) {
        if (s_upgrades[i].type == type && s_upgrades[i].item == item) {
            const uint8_t state = StateOf(i);
            return (state & kUpgrade_Unlocked) && (state & kUpgrade_Bought);
        }
    }
    return false;
}

} // namespace sly
