// openTIT - Agl shader classes: the material types and the techniques that draw them.
//
// Every material names a shader class ("Diffuse", "SlyCharacter", ...). A
// shader class owns a list of techniques, one per combination of features it
// supports (skinning, number of lights, texture layers, UV animation ...),
// and picks the technique for each draw. A technique is a vertex/fragment
// program pair (AglShader) plus the code that feeds it: BeginMaterial() when
// the material changes, Draw() for every mesh, End() when switching away.
//
// The classes are static objects; RegisterShaderClasses() creates their
// techniques at start-up and makes them findable by name hash.
#pragma once

#include "agl/AglShader.h"

#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace agl {

class AglMaterial;
class AglSubMesh;
struct AglMaterialAnim;

// Lights affecting a draw.
struct AglLightSet {
    // NOTE: used as a 0/1 selector by every shader class (probably "the
    // main light casts shadows").
    int32_t variant = 0;
};

// One draw: geometry, material and the per-draw state.
struct AglRenderItem {
    uint8_t field_0[0xC] = {};
    AglSubMesh* geometry = nullptr;          // 0x0C
    uint8_t field_10[0xC] = {};
    AglMaterial* material = nullptr;         // 0x1C
    uint8_t field_20[0xC] = {};
    const AglLightSet* lights = nullptr;     // 0x2C
    float position[3] = {};                  // 0x30
    uint8_t field_3C[4] = {};
    int32_t field_40 = 0;
    uint8_t flags = 0;                       // 0x44
    uint8_t lightMask = 0;                   // 0x45: lights the technique uses (set by SelectTechnique)
    uint8_t field_46 = 0;
    int8_t field_47 = -1;
};

// Description of a material as stored in level data (the parts SetupMaterial reads).
struct AglMaterialDesc {
    uint32_t field_0 = 0;
    uint32_t diffuseHash = 0;                // 0x04
    uint32_t detailHash = 0;                 // 0x08 detail layer texture
    uint32_t detail2Hash = 0;                // 0x0C second detail layer texture
    uint32_t textureHash = 0;                // 0x10
    uint8_t field_14[0x38] = {};
    uint8_t wrapMode = 0;                    // 0x4C
    uint8_t field_4D[3] = {};
    // 0x50: set by retired classes to the name hash of the class that should
    // draw the material instead (their SetupMaterial then returns false).
    mutable uint32_t replacementClass = 0;
    uint8_t paramCount = 0;                  // 0x54: number of 'params' exported
    uint8_t field_55[3] = {};
    // 0x58: class specific parameters. Some classes store integers here
    // (read them with ParamBits()).
    float params[8] = {};

    uint32_t ParamBits(int index) const;
};

// A vertex/fragment program pair with the code that drives it.
class AglTechnique : public AglShader {
public:
    ~AglTechnique() override;
    // Material dependent state (called when the material changes).
    virtual void BeginMaterial(AglMaterial* material);
    // Per draw state and the draw itself.
    virtual void Draw(AglRenderItem* item);
    // Restores state when the renderer switches to another technique.
    virtual void End(AglMaterial* material);
};

// Entry of a technique variant table: a program pair plus the skinned (or
// UV animated) vertex program that shares its fragment program. A null
// name keeps the slots empty.
struct AglShaderVariant {
    const char* name;
    const char* altName;
    const char* fragmentName;
};

class AglShaderClass {
public:
    explicit AglShaderClass(const char* className);
    virtual ~AglShaderClass();

    AglShaderClass(const AglShaderClass&) = delete;
    AglShaderClass& operator=(const AglShaderClass&) = delete;

    // Loads the programs of every technique.
    virtual void CreateTechniques() = 0;
    // Fills a new material from its description; false rejects it.
    virtual bool SetupMaterial(AglMaterial* material, const AglMaterialDesc* desc);
    // Technique to draw 'item' with (may set item->lightMask).
    virtual AglTechnique* SelectTechnique(AglMaterial* material, AglRenderItem* item);

    const char* GetName() const { return m_name; }
    uint32_t GetNameHash() const { return m_nameHash; }
    bool IsRegistered() const { return m_registered; }
    void SetRegistered() { m_registered = true; }
    uint32_t GetTechniqueCount() const { return m_techniques.Count(); }
    AglTechnique* GetTechnique(uint32_t index) const { return m_techniques[index]; }

protected:
    void AddTechnique(AglTechnique* technique) { m_techniques.PushBack(technique); }

    // Creates one technique of type T for a program pair.
    template <class T>
    void AddShader(const char* name, const char* fragmentName, const AglGlobalShaderParam* params)
    {
        T* technique = new T();
        ShaderLoad(technique, name, fragmentName, params);
        AddTechnique(technique);
    }

    // Adds the entries [first, count) of a variant table: one technique per
    // entry, or two (name, altName) when 'pairs' is set.
    template <class T>
    void AddVariants(const AglShaderVariant* table, int first, int count, bool pairs,
                     const AglGlobalShaderParam* params)
    {
        for (int i = first; i < count; ++i) {
            const AglShaderVariant& v = table[i];
            if (!v.name) {
                AddTechnique(nullptr);
                if (pairs)
                    AddTechnique(nullptr);
                continue;
            }
            AddShader<T>(v.name, v.fragmentName, params);
            if (pairs)
                AddShader<T>(v.altName, v.fragmentName, params);
        }
    }

private:
    const char* m_name;
    uint32_t m_nameHash;
    ape::ApeTArray<AglTechnique*> m_techniques;
    bool m_registered = false;
};

// Creates the techniques of 'shaderClass' and makes it findable.
bool RegisterShaderClass(AglShaderClass* shaderClass);
// Registers the built-in classes (the game specific sets can be disabled).
bool RegisterShaderClasses();
void SetShaderClassSetsDisabled(bool basic, bool sac, bool sly);
// True when the Sly specific shader classes are switched off (0x010C5ABE).
bool AreSlyShadersDisabled();
// Registered class with the given name hash, following the aliases of
// retired class names; null if unknown.
AglShaderClass* FindShaderClass(uint32_t nameHash);

// Static class lists (AglShaderClasses.cpp), each terminated by null.
AglShaderClass* const* GetUnlistedShaderClasses();
AglShaderClass* const* GetCoreShaderClasses();
AglShaderClass* const* GetBasicShaderClasses();
AglShaderClass* const* GetSacShaderClasses();
AglShaderClass* const* GetSlyShaderClasses();

// Camera position of the view being drawn (0x010DD8B0), used for
// distance based technique selection.
const float* GetViewEyePosition();
void SetViewEyePosition(const float position[3]);

// Uploads the 2x4 texture coordinate transform of a material animation
// (or a plain scale when there is none) to vertex constant register 'reg'.
void UploadUVTransform(AglMaterialAnim* anim, uint32_t reg, float scaleU, float scaleV);
// UV transform rows for 'anim' scaled by (scaleU, scaleV).
const float* BuildUVTransform(AglMaterialAnim* anim, float scaleU, float scaleV);

// Helpers shared by the classes' SelectTechnique implementations.
bool IsSkinned(const AglRenderItem* item);
// Lit by dynamic lights: the item's material accepts them and it has some.
bool IsLit(const AglRenderItem* item);

} // namespace agl
