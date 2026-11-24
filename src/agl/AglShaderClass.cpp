// openTIT - Agl shader classes: registry and the base class behaviour.
#include "agl/AglShaderClass.h"

#include "agl/AglResources.h"

#include "ape/core/ApeString.h"

namespace agl {

namespace {

// Registered classes (0x010F0BA8, created on first use with room for 512).
ape::ApeTArray<AglShaderClass*>& Registry()
{
    static ape::ApeTArray<AglShaderClass*> registry(0x200);
    return registry;
}

// The game specific class sets can be switched off (0x010C5ABC..0x010C5ABE).
bool g_basicClassesDisabled = false;
bool g_sacClassesDisabled = false;
bool g_slyClassesDisabled = false;

// Classes that were renamed or merged: materials exported with the old
// name hash are drawn by the replacement.
struct ShaderClassAlias {
    uint32_t from;
    uint32_t to;
};
constexpr ShaderClassAlias kAliases[] = {
    { 0xCD407FA6u, 0xD08FCDADu },
    { 0x3C7C5C8Bu, 0xD08FCDADu },
    { 0xDECB5640u, 0x91B76D20u },
    { 0x7CE30EBDu, 0x8DB50B42u },
};

void RegisterList(AglShaderClass* const* list)
{
    for (; *list; ++list)
        RegisterShaderClass(*list);
}

} // namespace

// --- AglShaderClass ---

// The classes are static objects. The console reserved room for two
// techniques here; the array is left empty so that nothing is allocated
// before the memory system is up (it grows on the first AddTechnique).
AglShaderClass::AglShaderClass(const char* className)
    : m_name(className), m_nameHash(ape::HashNameNoCase(className))
{
}

AglShaderClass::~AglShaderClass()
{
    for (AglTechnique* technique : m_techniques)
        delete technique;
}

bool AglShaderClass::SetupMaterial(AglMaterial* /*material*/, const AglMaterialDesc* /*desc*/)
{
    return true;
}

AglTechnique* AglShaderClass::SelectTechnique(AglMaterial* /*material*/, AglRenderItem* /*item*/)
{
    return m_techniques[0];
}

// --- AglTechnique ---

AglTechnique::~AglTechnique() = default;

void AglTechnique::BeginMaterial(AglMaterial* /*material*/)
{
}

void AglTechnique::Draw(AglRenderItem* /*item*/)
{
}

void AglTechnique::End(AglMaterial* /*material*/)
{
}

// --- registry ---

// @func 0x0050AFEC AglShaderClass::Register
bool RegisterShaderClass(AglShaderClass* shaderClass)
{
    shaderClass->CreateTechniques();
    Registry().PushBack(shaderClass);
    shaderClass->SetRegistered();
    return true;
}

bool AreSlyShadersDisabled()
{
    return g_slyClassesDisabled;
}

void SetShaderClassSetsDisabled(bool basic, bool sac, bool sly)
{
    g_basicClassesDisabled = basic;
    g_sacClassesDisabled = sac;
    g_slyClassesDisabled = sly;
}

// @func 0x0050B138 AglShaderClass::RegisterAll
bool RegisterShaderClasses()
{
    // NOTE: the console also pointed a global (0x00F66FB0) into default
    // texture 2 here; nothing in Agl reads it.
    RegisterList(GetCoreShaderClasses());
    if (!g_basicClassesDisabled)
        RegisterList(GetBasicShaderClasses());
    if (!g_sacClassesDisabled)
        RegisterList(GetSacShaderClasses());
    if (!g_slyClassesDisabled)
        RegisterList(GetSlyShaderClasses());
    return true;
}

// @func 0x004FE990 AglShaderClass::Find
AglShaderClass* FindShaderClass(uint32_t nameHash)
{
    for (AglShaderClass* shaderClass : Registry()) {
        if (shaderClass->GetNameHash() == nameHash)
            return shaderClass;
    }
    for (const ShaderClassAlias& alias : kAliases) {
        if (alias.from == nameHash)
            return FindShaderClass(alias.to);
    }
    return nullptr;
}

// --- selection helpers ---

bool IsSkinned(const AglRenderItem* item)
{
    return item->geometry->skinMatrices != nullptr;
}

bool IsLit(const AglRenderItem* item)
{
    return (item->material->flags & AglMaterial::kFlag_Lit) && item->lights;
}

} // namespace agl
