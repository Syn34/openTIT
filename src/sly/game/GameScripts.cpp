// openTIT - Game helpers around anim script instances.
#include "sly/game/GameScripts.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/core/ApeString.h"

namespace sly {

namespace {

constexpr uint32_t kThisVar = ape::AnimHash("this");
constexpr uint32_t kEndState = ape::AnimHash("end");

ape::ApeAnimScript* FindLibraryScript(uint32_t libraryHash, uint32_t scriptHash)
{
    ape::ApeAnimScriptLibrary* library = ape::ApeAnimScriptLibrary::Get(libraryHash);
    return library ? library->FindScript(scriptHash) : nullptr;
}

// Binds "this" of a new instance.
void BindThis(ape::ApeAnimScriptInstance* instance, ape::ApeRefCounted* object, uint32_t objectData)
{
    if (!object)
        return;
    ape::ApeAnimScriptVar* var = instance->FindVar(kThisVar);
    if (!var || var->GetType() != ape::AnimScriptVarType::Object)
        return;
    auto* objectVar = static_cast<ape::ApeAnimScriptObjectVar*>(var);
    objectVar->SetObject(object);
    objectVar->SetField24(objectData);
}

} // namespace

// @func 0x004A5E58 StartObjectScript
ape::ApeAnimScriptInstance* StartObjectScript(ape::ApeAnimScript* script, ape::ApeRefCounted* object,
                                              uint32_t objectData, bool autoRun)
{
    if (!script)
        return nullptr;
    auto* instance = new ape::ApeAnimScriptInstance(script);
    BindThis(instance, object, objectData);
    if (autoRun) {
        instance->SetRemoveWhenFinished(true);
        ape::ApeAnimScriptInstance::Register(instance);
    }
    return instance;
}

// @func 0x004A5FC8 StartObjectScript(hash)
ape::ApeAnimScriptInstance* StartObjectScript(uint32_t scriptHash, ape::ApeRefCounted* object,
                                              uint32_t objectData, bool autoRun)
{
    if (!scriptHash)
        return nullptr;
    return StartObjectScript(FindLibraryScript(0, scriptHash), object, objectData, autoRun);
}

// @func 0x004A621C ReplaceObjectScript
ape::ApeAnimScriptInstance* ReplaceObjectScript(ape::ApeAnimScriptInstance*& slot, ape::ApeAnimScript* script,
                                                ape::ApeRefCounted* object, uint32_t objectData,
                                                bool stopOnly)
{
    ape::ApeAnimScriptInstance* result = slot;
    if (slot && (script || stopOnly)) {
        slot->SetState(kEndState);
        slot->SetRemoveWhenFinished(true);
        slot = nullptr;
        result = nullptr;
    }
    if (script) {
        result = StartObjectScript(script, object, objectData, true);
        slot = result;
        // The slot keeps the instance; it is not dropped when it finishes.
        if (result)
            result->SetRemoveWhenFinished(false);
    }
    return result;
}

// @func 0x004A63FC ReplaceObjectScript(hash)
ape::ApeAnimScriptInstance* ReplaceObjectScript(ape::ApeAnimScriptInstance*& slot, uint32_t scriptHash,
                                                uint32_t libraryHash, ape::ApeRefCounted* object,
                                                uint32_t objectData, bool stopOnly)
{
    if (!ape::ApeAnimScriptLibrary::Get(libraryHash))
        return nullptr;
    return ReplaceObjectScript(slot, FindLibraryScript(libraryHash, scriptHash), object, objectData, stopOnly);
}

// @func 0x004A6558 ReplaceObjectScript(names)
ape::ApeAnimScriptInstance* ReplaceObjectScript(ape::ApeAnimScriptInstance*& slot, const char* scriptName,
                                                const char* libraryName, ape::ApeRefCounted* object,
                                                uint32_t objectData, bool stopOnly)
{
    return ReplaceObjectScript(slot, ape::HashNameNoCase(scriptName), ape::HashNameNoCase(libraryName),
                               object, objectData, stopOnly);
}

// @func 0x004A65F0 SetScriptObjectVar
bool SetScriptObjectVar(ape::ApeAnimScriptInstance* instance, uint32_t varHash, ape::ApeRefCounted* object)
{
    if (!instance)
        return false;
    ape::ApeAnimScriptVar* var = instance->FindVar(varHash);
    if (!var || var->GetType() != ape::AnimScriptVarType::Object)
        return false;
    static_cast<ape::ApeAnimScriptObjectVar*>(var)->SetObject(object);
    return true;
}

// @func 0x004A674C SetScriptIntVar
bool SetScriptIntVar(ape::ApeAnimScriptInstance* instance, uint32_t varHash, int32_t value)
{
    if (!instance)
        return false;
    ape::ApeAnimScriptVar* var = instance->FindVar(varHash);
    if (!var || var->GetType() != ape::AnimScriptVarType::Int)
        return false;
    static_cast<ape::ApeAnimScriptIntVar*>(var)->SetInt(value);
    return true;
}

} // namespace sly
