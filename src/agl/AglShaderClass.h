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
// One local (point) light of a light set, as uploaded to the shaders.
struct AglLocalLight {
    float position[4];        // NOTE: order of the three vectors from the uploads
    float color[4];           // multiplied by the material colour when uploaded
    float attenuation[4];
};

// Lights affecting a render item (+0x2C).
struct AglLightSet {
    // Number of local lights (0 or 1 in practice); every lit shader class
    // picks the technique of a pair with it.
    int32_t variant = 0;
    uint32_t field_4[3] = {};
    float ambient[4] = {};            // 0x10
    float directionalColor[4] = {};   // 0x20
    float direction[4] = {};          // 0x30
    AglLocalLight locals[1] = {};     // 0x40, 'variant' entries
};

// Stencil reference tagging of item draws (0x010DDA60/0x010DDA61): each lit
// draw writes ((item flag 0x20) ? 0x80 : 0) | material field_65, masked by
// writeMask, as its stencil reference. The renderer loop sets both bytes.
struct AglItemStencil {
    uint8_t ref = 0;
    uint8_t writeMask = 0;
};
AglItemStencil& GetItemStencil();

// Animation clock of the renderer (0x010DD5A8): the step queued for the
// next frame, the last step and the accumulated time that animated
// materials (texture warps, water) use. Advanced by BeginRenderFrame
// (0x0051008C).
struct AglAnimationClock {
    float pendingStep = 0.0f;
    float lastStep = 0.0f;
    float time = 0.0f;
    float field_C = 1.0f;
};
AglAnimationClock& GetAnimationClock();

// Render item light mask bits (+0x45), set by SelectTechnique.
// NOTE: names inferred from the Draw functions.
enum AglItemLightBits : uint8_t {
    kItemLight_Lit      = 0x01,   // light constants are uploaded
    kItemLight_PerPixel = 0x02,   // as fragment constants (else vertex constants 0x10..)
    kItemLight_Eye      = 0x04,   // the eye position in object space is needed
};

// One draw: geometry, material and the per-draw state. The first 0x40
// bytes are the world transform (rows x, y, z, translation); on the console
// the unused fourth lane of each row holds one of the pointers.
struct AglRenderItem {
    float axisX[3] = {};                     // 0x00
    AglSubMesh* geometry = nullptr;          // 0x0C
    float axisY[3] = {};                     // 0x10
    AglMaterial* material = nullptr;         // 0x1C
    float axisZ[3] = {};                     // 0x20
    const AglLightSet* lights = nullptr;     // 0x2C
    float position[3] = {};                  // 0x30 translation
    const float* bonePalette = nullptr;      // 0x3C skinning matrices (3 rows of 4 floats per bone)
    // NOTE: per-class extra data: an RGBA byte colour for debug draws, the
    // silhouette pass for outlines.
    const void* field_40 = nullptr;
    uint8_t flags = 0;                       // 0x44
    uint8_t lightMask = 0;                   // 0x45: lights the technique uses (set by SelectTechnique)
    uint8_t field_46 = 0;
    int8_t field_47 = -1;
    // NOTE: RGB tint bytes read by the unlit Sac skinned draw.
    uint8_t field_48[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
};

// Fog of a draw: vertex constant 0x20 gets the layer's start, 1 / (end -
// start) and whether fog is on; fragment constant 'colorConstant' gets the
// fog colour. Fog is on when 'enable' is set and the layer of the render
// target being drawn has fog.
void ApplyFog(int layer, bool enable, uint32_t colorConstant);
// ApplyFog for a render item: layer from item flag 0x40, enabled by
// material flag 0x40 (NOTE: "receives fog").
void ApplyItemFog(const AglRenderItem* item, uint32_t colorConstant);

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
    // Retired classes also rewrite the parameters for their replacement.
    mutable uint8_t paramCount = 0;          // 0x54: number of 'params' exported
    uint8_t field_55[3] = {};
    // 0x58: class specific parameters. Some classes store integers here
    // (read them with ParamBits()).
    mutable float params[24] = {};       // 0x58..0xB7

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
// The out-of-line lookup game code calls (same result as FindShaderClass).
AglShaderClass* GetShaderClass(uint32_t nameHash);

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
// Vertex constant `reg` = the animation's quantised UV offset (0 without one).
void UploadUVOffset(const AglMaterialAnim* anim, uint32_t reg);
// UV transform rows for 'anim' scaled by (scaleU, scaleV).
const float* BuildUVTransform(AglMaterialAnim* anim, float scaleU, float scaleV);

// Helpers shared by the classes' SelectTechnique implementations.
bool IsSkinned(const AglRenderItem* item);
// Lit by dynamic lights: the item's material accepts them and it has some.
bool IsLit(const AglRenderItem* item);

} // namespace agl
