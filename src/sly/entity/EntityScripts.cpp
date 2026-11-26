// openTIT - Anim scripts run for an entity (0x000DCA20-0x000DCECC).
#include "sly/entity/Entity.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/anim/ApeAnimScriptVars.h"
#include "ape/core/ApeString.h"
#include "sly/entity/EntityHooks.h"

namespace sly {
namespace entity {

namespace {
constexpr uint32_t kVar_This = 0x0027469Eu;       // "this"
constexpr uint32_t kVar_ThisName = 0x77EB3A29u;   // NOTE: name unknown (hash variable)
constexpr uint32_t kVar_Transform = 0x001E8ECCu;  // NOTE: name unknown (matrix variable)
} // namespace

// @func 0x000DCA20 Entity::StartScript
ape::ApeAnimScriptInstance* Entity::StartScript(ape::ApeAnimScript* script, uint32_t bone, bool autoRun)
{
    if (!script)
        return nullptr;
    auto* instance = new ape::ApeAnimScriptInstance(script);

    ape::ApeAnimScriptVar* self = instance->FindVar(kVar_This);
    if (self && self->GetType() == ape::AnimScriptVarType::Object) {
        auto* objectVar = static_cast<ape::ApeAnimScriptObjectVar*>(self);
        objectVar->SetObject(EntityWorldObjectRef(this));
        // NOTE: the console keeps the animation controller pointer here.
        objectVar->SetField24(0);
    }
    ape::ApeAnimScriptVar* name = instance->FindVar(kVar_ThisName);
    if (name && name->GetType() == ape::AnimScriptVarType::Hash)
        static_cast<ape::ApeAnimScriptHashVar*>(name)->SetHash(nameHash);

    ape::ApeAnimScriptVar* matrix = instance->FindVar(kVar_Transform);
    if (matrix && matrix->GetType() == ape::AnimScriptVarType::Matrix)
        static_cast<ape::ApeAnimScriptMatrixVar*>(matrix)->SetMatrix(EntityBoneWorldTransform(this, bone));

    if (autoRun) {
        instance->SetRemoveWhenFinished(true);
        ape::ApeAnimScriptInstance::Register(instance);
    }
    return instance;
}

// @func 0x000DCD20 Entity::StartScript (by hash)
ape::ApeAnimScriptInstance* Entity::StartScript(uint32_t scriptHash, uint32_t bone, bool autoRun)
{
    if (scriptHash == 0)
        return nullptr;
    ape::ApeAnimScriptLibrary* library = ape::ApeAnimScriptLibrary::Get(0);
    ape::ApeAnimScript* script = library ? library->FindScript(scriptHash) : nullptr;
    return script ? StartScript(script, bone, autoRun) : nullptr;
}

// @func 0x000DCE6C Entity::StartScriptByName
ape::ApeAnimScriptInstance* Entity::StartScriptByName(const char* scriptName, uint32_t bone, bool autoRun)
{
    return StartScript(ape::HashNameNoCase(scriptName), bone, autoRun);
}

// @func 0x000DCECC StartScriptAt
// An anim script that belongs to no entity, with its matrix variable set
// to `transform`.
ape::ApeAnimScriptInstance* StartScriptAt(ape::ApeAnimScript* script, const ape::Matrix34& transform, bool autoRun)
{
    if (!script)
        return nullptr;
    auto* instance = new ape::ApeAnimScriptInstance(script);
    ape::ApeAnimScriptVar* matrix = instance->FindVar(kVar_Transform);
    if (matrix && matrix->GetType() == ape::AnimScriptVarType::Matrix)
        static_cast<ape::ApeAnimScriptMatrixVar*>(matrix)->SetMatrix(transform);
    if (autoRun) {
        instance->SetRemoveWhenFinished(true);
        ape::ApeAnimScriptInstance::Register(instance);
    }
    return instance;
}

} // namespace entity
} // namespace sly
