// openTIT - Ape engine level definitions (ApeLevelDef) and their XML loader.
//
// Every playable level is described by an ApeLevelDef: which scene, director,
// animation, zone, GUI and setting files make it up, plus per level
// rendering parameters (camera, fog, light, post effects, ...).
//
// Level definitions come from XML. A level list ("AssetBuilderConfig /
// Levels / Level" entries) names each level and its folder; the level's own
// "Scripts/Levels/<path>/Levels.xmb" then fills in the details. Parsing is
// table driven: ApeLevelDefXMLLoader objects form a tree keyed by element
// name ("LevelData.Level.Camera" -> handler), and ProcessElements() walks the
// document dispatching each element to the handler registered for it.
#pragma once

#include "ape/container/ApeAVL.h"
#include "ape/math/ApeMath.h"
#include "ape/xml/ApeXml.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ape {

// Handler called for one XML element. Returning false aborts the walk of
// the current element list.
using LevelDefXmlCallback = bool (*)(xml::XmlElement* element);

// One node of the element-name dispatch tree.
struct ApeLevelDefXMLLoader {
    LevelDefXmlCallback callback = nullptr;
    bool processChildren = false;   // walk the element's children with this node as context
    AVL<uint32_t, ApeLevelDefXMLLoader*> children;  // keyed by HashNameNoCase(element name)
    ApeLevelDefXMLLoader* parent = nullptr;
};

// Scene file reference (a .san file, possibly a combined cooked one).
struct LevelSceneFile {
    std::string name;
    Vec3 worldOffset{0.0f, 0.0f, 0.0f, 0.0f};
};

// A geometry template: an object derived from a base object with some of its
// textures swapped.
struct LevelGeomTemplate {
    char name[100] = {};            // truncated to 99 characters
    uint32_t nameHash = 0;
    uint32_t baseObjHash = 0;
    AVL<uint32_t, uint32_t> textureSwaps;   // old texture hash -> new texture hash
};

// A group of files that is streamed in together.
struct LevelAsyncFile {
    uint32_t typeHash = 0;
    uint32_t idHash = 0;
    std::string fileName;           // empty if the XML gave none
};

struct LevelAsyncFileGroup {
    uint32_t idHash = 0;
    uint32_t typeHash = 0;
    std::vector<LevelAsyncFile> files;
};

enum DesaturationBlendMode : int32_t {
    kDesatBlend_Opaque      = 0,
    kDesatBlend_Blend       = 2,   // also the default when 'blendmode' is absent
    kDesatBlend_Additive    = 3,
    kDesatBlend_Subtractive = 4,
};

enum DesaturationType : int32_t {
    kDesatType_Monochrome = 0,
    kDesatType_Negative   = 1,
    kDesatType_Solid      = 2,
};

enum ShadowAlgorithm : int32_t {
    kShadow_ZPass = 0,
    kShadow_ZFail = 1,
    kShadow_Auto  = 2,
};

class ApeLevelDef {
public:
    ApeLevelDef();
    ~ApeLevelDef();
    ApeLevelDef(const ApeLevelDef&) = delete;
    ApeLevelDef& operator=(const ApeLevelDef&) = delete;

    // Loads "Scripts/Levels/<path>/Levels.xmb" once. Returns the 'loaded'
    // flag (also set when the file is missing, so it is only tried once).
    bool LoadLevelData();

    // Named lists of strings attached to the level by game code.
    void ClearStringLists();
    void AddStringToList(uint32_t key, const char* str);
    uint16_t GetStringListCount(uint32_t key) const;
    // Null if the list does not exist or index is out of range.
    const char* GetStringFromList(uint32_t key, int index) const;

    char name[0x80] = {};                       // +0x000
    char path[0x80] = {};                       // +0x080 folder under Scripts/Levels/
    std::vector<uint32_t> includedLevels;       // +0x100 name hashes of included levels
    std::vector<std::string> atmospherics;      // +0x114
    std::vector<LevelSceneFile> sceneFiles;     // +0x128
    std::vector<std::string> directorFiles;     // +0x13C
    std::vector<std::string> animFiles;         // +0x150
    std::vector<uint32_t> animLibraryIds;       // +0x164 parallel to animFiles
    std::vector<std::string> knowledgeScriptFiles; // +0x178
    std::vector<std::string> settingFiles;      // +0x18C
    std::vector<std::string> preCacheFiles;     // +0x1A0
    std::vector<std::string> guiFiles;          // +0x1B4
    std::vector<std::string> zoneFiles;         // +0x1C8
    // NOTE: +0x1DC is a pointer array the loader never fills (SoundFile
    // elements are accepted and ignored); its purpose is unknown.
    std::vector<std::string> animClipFiles;     // +0x1F0
    std::vector<std::unique_ptr<LevelGeomTemplate>> geomTemplates;   // +0x204
    std::vector<std::unique_ptr<LevelAsyncFileGroup>> asyncFileGroups; // +0x218
    bool loaded = false;                        // +0x22C
    std::map<uint32_t, std::vector<std::string>> stringLists;        // +0x230

    // While a combined cooked scene file is being parsed its children are
    // already part of it; these nesting counters suppress the individual
    // entries.
    uint8_t suppressIncludes = 0;               // +0x248
    uint8_t suppressDirectorFiles = 0;          // +0x249
    uint8_t suppressSceneFiles = 0;             // +0x24A
    uint8_t suppressAnimFiles = 0;              // +0x24B
    uint8_t suppress_24C = 0;                   // +0x24C NOTE: no reader known

    // Stereo3D
    bool stereoEnable = false;                  // +0x24D
    float stereoDistortion = 0.0f;              // +0x250 clamped to [0.01, 0.1] when read
    float stereoDepthOfScreen = 4.0f;           // +0x254

    // Camera
    bool hasCamera = false;                     // +0x258
    float cameraFov = 55.0f;                    // +0x25C
    float cameraNearZ = 1.0f;                   // +0x260
    float cameraFarZ = 1000.0f;                 // +0x264
    xml::Color8 cameraColor{0, 0, 0, 0};        // +0x268
    Vec3 cameraPos{0.0f, 0.0f, 0.0f, 0.0f};     // +0x270
    Vec3 cameraTarget{0.0f, 0.0f, 0.0f, 0.0f};  // +0x280

    // Light
    bool hasLight = false;                      // +0x290
    float defaultFadeDist = 300.0f;             // +0x294
    float defaultFadeRange = 50.0f;             // +0x298
    float noRescanDist = 0.0f;                  // +0x29C
    float noUpdateDist = 0.0f;                  // +0x2A0

    // Shadow
    bool hasShadow = false;                     // +0x2A4
    xml::Color8 shadowColor{0xCC, 0xCC, 0xCC, 0xFF}; // +0x2A5 NOTE: not read from XML
    float shadowIntensity = 1.0f;               // +0x2AC
    float shadowFarZCull = 10.0f;               // +0x2B0
    int32_t shadowAlgorithm = kShadow_Auto;     // +0x2B4
    float shadowAngleDeviation = 0.0f;          // +0x2B8
    Vec3 shadowVector{0.0f, -1.0f, 0.0f, 0.0f}; // +0x2C0 normalised when read

    // Fog (two groups)
    static constexpr int kFogGroups = 2;
    struct FogGroup {
        bool enable = false;
        float nearZ = 0.0f;
        float farZ = 0.0f;
        xml::Color8 color{0, 0, 0, 0};
        int32_t numPlanes = 0;
    };
    bool hasFog = false;                        // +0x2D0
    FogGroup fog[kFogGroups];                   // +0x2D1.. (stored field by field on PS3)

    // Bloom
    bool hasBloom = false;                      // +0x2F4
    bool bloomEnable = false;                   // +0x2F5
    float bloomIntensity = 0.0f;                // +0x2F8
    xml::Color8 bloomThreshold{0, 0, 0, 0};     // +0x2FC

    // Depth of field
    bool hasDof = false;                        // +0x300
    bool dofEnable = false;                     // +0x301
    float dofStartingDistance = 0.0f;           // +0x304
    float dofRange = 2.0f;                      // +0x308
    float dofIntensity = 1.0f;                  // +0x30C
    uint8_t dofPasses = 0;                      // +0x310

    // Desaturation
    bool hasDesaturation = false;               // +0x311
    bool desatEnable = false;                   // +0x312
    float desatIntensity = 0.0f;                // +0x314
    xml::Color8 desatTint{0, 0, 0, 0};          // +0x318
    int32_t desatBlendMode = 0;                 // +0x31C DesaturationBlendMode
    int32_t desatType = 0;                      // +0x320 DesaturationType

    // Radial blur
    bool hasRadialBlur = false;                 // +0x324
    bool radialBlurEnable = false;              // +0x325
    uint8_t radialBlurTimes = 0;                // +0x326
    float radialBlurAlphaStart = 0.0f;          // +0x328
    float radialBlurUVIncrement = 0.0f;         // +0x32C

    // Blur
    bool hasBlur = false;                       // +0x330
    bool blurEnable = false;                    // +0x331
    uint8_t blurPasses = 0;                     // +0x332
    float blurIntensity = 0.0f;                 // +0x334

    // Warp screen
    bool hasWarpScreen = false;                 // +0x338 NOTE: never set by the loader
    bool warpEnable = false;                    // +0x339
    float warpAmplitudeX = 0.0f;                // +0x33C
    float warpAmplitudeY = 0.0f;                // +0x340
    float warpFrequencyX = 0.0f;                // +0x344
    float warpFrequencyY = 0.0f;                // +0x348
    int32_t warpGridDimX = 0;                   // +0x34C
    int32_t warpGridDimY = 0;                   // +0x350
    xml::Color8 warpTint{0xFF, 0xFF, 0xFF, 0xFF}; // +0x354

    int32_t audioExtraBankMemKB = 0;            // +0x358
    bool hasGamma = false;                      // +0x35C
    float gamma = 0.0f;                         // +0x360
    bool hasOcclPlanes = false;                 // +0x364
    int32_t occlPlanesToChoose = 64;            // +0x368
    float lodYInfluence = 1.0f;                 // +0x36C clamped to [-0.5, 1]
    char levelSettingName[0x80] = {};           // +0x370
    char levelSndGroupName[0x20] = {};          // +0x3F0
    char bgMusic[0x80] = {};                    // +0x410
};

// -- level registry -----------------------------------------------------------

// Registers a handler for a dotted element path ("LevelData.Level.Camera").
// Every parent path must already be registered; otherwise nothing happens.
void RegisterLevelDefXMLLoader(const char* path, LevelDefXmlCallback callback, bool processChildren);
// Registers all the handlers of the level XML format.
void RegisterLevelDefXMLLoaders();

// Dispatches 'element' and its following siblings to the handlers of the
// current loader context (the root table when there is none). Returns false
// as soon as a handler fails.
bool ProcessLevelDefElements(xml::XmlElement* element);

// Reads a level list file (AssetBuilderConfig/Levels/Level entries) and
// creates a level definition for every entry.
bool LoadLevelList(const char* path);

// Deletes every level definition.
void ShutdownLevelDefs();
// Clears both registries (end of program).
void LevelDefModuleShutdown();

// Level definition being loaded or played (the target of the XML handlers
// and of the level file chunks that attach strings to it), or null.
ApeLevelDef* GetCurrentLevelDef();

uint32_t GetLevelDefCount();
// Level definitions in name hash order.
ApeLevelDef* GetLevelDefByIndex(uint32_t index);
ApeLevelDef* FindLevelDef(uint32_t nameHash);

// Engine-wide fallbacks for the Light element's noRescanDist/noUpdateDist.
// NOTE: on PS3 these are EngineConfig floats at +0x070/+0x074 (40 and 75 when
// left at 0). They are kept here until EngineConfig gains the fields.
extern float g_levelNoRescanDistDefault;
extern float g_levelNoUpdateDistDefault;

} // namespace ape
