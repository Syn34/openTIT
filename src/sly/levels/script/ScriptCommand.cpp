// openTIT - Shared helpers of the game's anim script commands.
#include "sly/levels/script/ScriptCommand.h"

#include "ape/core/ApeString.h"
#include "sly/levels/GameServices.h"

namespace sly {
namespace levels {

bool g_scriptWarnings = false;

void ScriptWarning(const char* format, uint32_t hash)
{
    if (g_scriptWarnings)
        ape::TempFormat(format, ape::TempHexString(hash));
}

WorldObject* EntityFromObject(const ape::ApeRefCounted* object)
{
    if (!object || WorldObjectUserTag(object) != kUserTag_Entity)
        return nullptr;
    return static_cast<WorldObject*>(WorldObjectUserData(object));
}

ape::ApeAnimScriptObjectVar* FindObjectVar(ApeAnimScriptInstance* instance, uint32_t varHash)
{
    if (varHash == 0)
        return nullptr;
    ApeAnimScriptVar* var = instance->FindVar(varHash);
    if (!var || var->GetType() != AnimScriptVarType::Object)
        return nullptr;
    return static_cast<ape::ApeAnimScriptObjectVar*>(var);
}

ape::ApeRefCounted* ObjectFromVar(ApeAnimScriptInstance* instance, uint32_t varHash)
{
    ape::ApeAnimScriptObjectVar* var = FindObjectVar(instance, varHash);
    return var ? var->GetObject() : nullptr;
}

WorldObject* EntityFromVar(ApeAnimScriptInstance* instance, uint32_t varHash)
{
    return EntityFromObject(ObjectFromVar(instance, varHash));
}

WorldObject* ResolveEntity(ApeAnimScriptInstance* instance, uint32_t objectVar, uint32_t entityName)
{
    if (WorldObject* entity = EntityFromVar(instance, objectVar))
        return entity;
    return entityName ? FindEntity(entityName) : nullptr;
}

float ResolveFloat(ApeAnimScriptInstance* instance, uint32_t varHash, float fallback)
{
    if (varHash == 0)
        return fallback;
    ApeAnimScriptVar* var = instance->FindVar(varHash);
    if (var && var->GetType() == AnimScriptVarType::Float)
        return var->GetFloat();
    return fallback;
}

int32_t ResolveInt(ApeAnimScriptInstance* instance, uint32_t varHash, int32_t fallback)
{
    if (varHash == 0)
        return fallback;
    ApeAnimScriptVar* var = instance->FindVar(varHash);
    if (var && var->GetType() == AnimScriptVarType::Int)
        return var->GetInt();
    return fallback;
}

ape::ApeRefCounted* ResolveWorldObject(ApeAnimScriptInstance* instance, uint32_t nameHash)
{
    if (nameHash == 0)
        return nullptr;
    if (ape::ApeRefCounted* object = instance->ResolveObject(nameHash, false))
        return object;
    return instance->ResolveObject(nameHash, true);
}

XmlElement* NewDebugElement(const char* commandName)
{
    (void)commandName;
    return new XmlElement();
}

void RunCommands(const ape::AnimScriptCommandList& list, ApeAnimScriptInstance* instance,
                 ApeAnimScriptState* state)
{
    for (ApeAnimScriptCommand* command : list)
        command->Execute(instance, state);
}

void LinkCommands(const ape::AnimScriptCommandList& list)
{
    for (ApeAnimScriptCommand* command : list)
        command->Link();
}

} // namespace levels
} // namespace sly
