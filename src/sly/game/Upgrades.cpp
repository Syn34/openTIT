// openTIT - Upgrade shop tuning and the table of upgrade records.
#include "sly/game/Upgrades.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"

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

} // namespace sly
