// openTIT - Ape engine level definitions (ApeLevelDef) and their XML loader.
#include "ape/level/ApeLevelDef.h"

#include "ape/level/LevelUtil.h"

#include "ape/core/ApeString.h"
#include "ape/core/ApeSystem.h"
#include "ape/file/ApeFile.h"
#include "platform/Platform.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace ape {

using xml::XmlDocument;
using xml::XmlElement;

float g_levelNoRescanDistDefault = 40.0f;
float g_levelNoUpdateDistDefault = 75.0f;

namespace {

// Handler tree roots and the level registry (0x010C2BC0 / 0x010C2CAC).
AVL<uint32_t, ApeLevelDefXMLLoader*> s_rootLoaders;
AVL<uint32_t, ApeLevelDef*> s_levelDefs;

// Parse state shared by the handlers.
ApeLevelDef* s_currentLevelDef = nullptr;           // 0x010C2C28
ApeLevelDefXMLLoader* s_currentLoader = nullptr;    // 0x00F668D0
char s_currentFile[0x80] = {};                      // 0x010C2C2C
// A <Level> entry without a name still becomes the current definition but is
// never registered; the PS3 build leaked it, here it is kept until replaced.
std::unique_ptr<ApeLevelDef> s_unnamedLevelDef;

const xml::Color8 kWhite{0xFF, 0xFF, 0xFF, 0xFF};

void ReadVec3(XmlElement* element, const char* name, Vec3& out, const Vec3& defaultValue)
{
    const float def[3] = {defaultValue.x, defaultValue.y, defaultValue.z};
    float value[3];
    element->GetVec3(name, value, def);
    out.x = value[0];
    out.y = value[1];
    out.z = value[2];
}

uint8_t ReadByteFromInt(XmlElement* element, const char* name)
{
    int32_t value = 0;
    element->GetInt(name, &value, 0);
    return uint8_t(value);
}

// Copies the "name" attribute onto the end of a string list.
bool AppendName(XmlElement* element, std::vector<std::string>& list, const char* attribute = "name")
{
    const char* value = element->GetString(attribute);
    if (!value)
        return false;
    list.emplace_back(value);
    return true;
}

template <class Item, class Fn>
void ForEachInOrder(const typename AVL<uint32_t, Item>::Node* node, Fn&& fn)
{
    if (!node)
        return;
    ForEachInOrder<Item>(node->left, fn);
    fn(node->item);
    ForEachInOrder<Item>(node->right, fn);
}

void DeleteLoaderTree(const AVL<uint32_t, ApeLevelDefXMLLoader*>::Node* node)
{
    ForEachInOrder<ApeLevelDefXMLLoader*>(node, [](ApeLevelDefXMLLoader* loader) {
        DeleteLoaderTree(loader->children.Root());
        delete loader;
    });
}

// Walks the children of 'element' as if they belonged to the parent of the
// current handler (used by elements that only group others).
bool ProcessChildrenWithParentContext(XmlElement* element)
{
    ApeLevelDefXMLLoader* saved = s_currentLoader;
    s_currentLoader = saved ? saved->parent : nullptr;
    ProcessLevelDefElements(element->FirstChildElement());
    s_currentLoader = saved;
    return true;
}

// Loads a "LevelData" document and dispatches its contents.
bool LoadLevelDataFile(const char* path)
{
    XmlDocument doc(path);
    s_currentLoader = nullptr;
    if (!doc.LoadFile(0)) {
        plat::Log(plat::LogLevel::Warning, "Fail to load up the level xml file (%s)!\n", path);
        return true;
    }
    StrCopyBounded(s_currentFile, path, sizeof(s_currentFile));
    XmlElement* root = doc.FirstChildElement("LevelData");
    return root ? ProcessLevelDefElements(root) : false;
}

// ---------------------------------------------------------------------------
// Element handlers
// ---------------------------------------------------------------------------

// Level list entry: creates and registers a definition.
// @func 0x004ED58C LevelDefXml_Level
bool OnLevel(XmlElement* element)
{
    auto* def = new ApeLevelDef();
    s_currentLevelDef = def;
    const char* name = element->GetString("name");
    if (!name) {
        s_unnamedLevelDef.reset(def);
        return true;
    }
    StrCopyBounded(def->name, name, sizeof(def->name));
    s_levelDefs.Add(HashNameNoCase(name), def);
    if (const char* path = element->GetString("path"))
        StrCopyBounded(def->path, path, sizeof(def->path));
    return true;
}

// @func 0x004ED1B8 LevelDefXml_LevelData
bool OnLevelData(XmlElement*)
{
    return true;
}

// Pulls in another LevelData file, then restores the parse state.
// @func 0x004ED1C0 LevelDefXml_Include
bool OnInclude(XmlElement* element)
{
    if (s_currentLevelDef && s_currentLevelDef->suppressIncludes)
        return true;
    const char* fileName = element->GetString("fileName");
    if (!fileName)
        return false;

    char savedFile[0x80];
    char path[0x80];
    StrCopyBounded(savedFile, s_currentFile, sizeof(savedFile));
    StrCopyBounded(path, fileName, sizeof(path));
    ApeLevelDefXMLLoader* savedLoader = s_currentLoader;
    ApeLevelDef* savedDef = s_currentLevelDef;

    const bool result = LoadLevelDataFile(path);

    s_currentLoader = savedLoader;
    s_currentLevelDef = savedDef;
    StrCopyBounded(s_currentFile, savedFile, sizeof(s_currentFile));
    return result;
}

// <Level name include>: selects (creating if needed) the definition and
// records an included level.
// @func 0x004ED368 LevelDefXml_LevelDataLevel
bool OnLevelDataLevel(XmlElement* element)
{
    if (const char* name = element->GetString("name")) {
        const uint32_t hash = HashNameNoCase(name);
        ApeLevelDef* existing = nullptr;
        // NOTE: an already registered level does not become current; the
        // definition being loaded stays current (as on PS3).
        if (!s_levelDefs.Find(hash, existing)) {
            auto* def = new ApeLevelDef();
            s_currentLevelDef = def;
            StrCopyBounded(def->name, name, sizeof(def->name));
            s_levelDefs.Add(hash, def);
        }
    }
    if (const char* include = element->GetString("include")) {
        if (s_currentLevelDef)
            s_currentLevelDef->includedLevels.push_back(HashNameNoCase(include));
    }
    return true;
}

// @func 0x004ED6A0 LevelDefXml_LevelInclude
bool OnLevelInclude(XmlElement* element)
{
    if (s_currentLevelDef && s_currentLevelDef->suppressIncludes)
        return true;
    const char* level = element->GetString("level");
    if (!level || !s_currentLevelDef)
        return false;
    s_currentLevelDef->includedLevels.push_back(HashNameNoCase(level));
    return true;
}

// @func 0x004ED7B0 LevelDefXml_Stereo3D
bool OnStereo3D(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    element->GetBool("enable", &def->stereoEnable, false);
    element->GetFloat("depthOfScreen", &def->stereoDepthOfScreen, def->stereoDepthOfScreen);
    element->GetFloat("distortion", &def->stereoDistortion, 0.01f);
    def->stereoDistortion = Clamp(def->stereoDistortion, 0.01f, 0.1f);
    return true;
}

// @func 0x004ED8A8 LevelDefXml_Camera
bool OnCamera(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    element->GetFloat("fov", &def->cameraFov, 0.0f);
    element->GetFloat("nearz", &def->cameraNearZ, 0.0f);
    element->GetFloat("farz", &def->cameraFarZ, 0.0f);
    element->GetColor("color", &def->cameraColor, &kWhite);
    // NOTE: the PS3 code passes an uninitialised stack vector as the default
    // for pos/tgt; zero is used here.
    const Vec3 zero{0.0f, 0.0f, 0.0f, 0.0f};
    ReadVec3(element, "pos", def->cameraPos, zero);
    ReadVec3(element, "tgt", def->cameraTarget, zero);
    def->hasCamera = true;
    return true;
}

// @func 0x004EDA10 LevelDefXml_Light
bool OnLight(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    element->GetFloat("defaultFadeDist", &def->defaultFadeDist, 300.0f);
    element->GetFloat("defaultFadeRange", &def->defaultFadeRange, 50.0f);
    element->GetFloat("noRescanDist", &def->noRescanDist, g_levelNoRescanDistDefault);
    element->GetFloat("noUpdateDist", &def->noUpdateDist, g_levelNoUpdateDistDefault);
    def->hasLight = true;
    return true;
}

// @func 0x004EDB00 LevelDefXml_Fog
bool OnFog(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    int32_t group = 0;
    element->GetInt("group", &group, 0);
    // NOTE: the PS3 code does not range check 'group'.
    if (group >= 0 && group < ApeLevelDef::kFogGroups) {
        ApeLevelDef::FogGroup& fog = def->fog[group];
        element->GetBool("enable", &fog.enable, false);
        element->GetFloat("nearz", &fog.nearZ, 0.0f);
        element->GetFloat("farz", &fog.farZ, 0.0f);
        element->GetColor("color", &fog.color, &kWhite);
        element->GetInt("NumPlanes", &fog.numPlanes, 0);
    }
    def->hasFog = true;
    return true;
}

// A combined scene: when its cooked file is available it replaces the
// individual include/director/scene/anim entries listed inside it.
// @func 0x004EDC64 LevelDefXml_CombinedSan
bool OnCombinedSan(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    const char* name = element->GetString("name");
    if (!name)
        return false;

    const bool trustCooked = g_engineConfig.field_0A2;
    if (!trustCooked && g_engineConfig.field_0A1)
        return ProcessChildrenWithParentContext(element);

    char cookedPath[0x100];
    std::snprintf(cookedPath, sizeof(cookedPath), "%s%s", name, ".cooked");
    if (!trustCooked) {
        if (!file::FileExists(cookedPath))
            return ProcessChildrenWithParentContext(element);
        // The file is opened once to make sure it is really readable.
        file::File* file = file::OpenFile(cookedPath, "rb");
        if (!file)
            file = file::OpenFile(cookedPath, "rb");
        if (!file)
            return ProcessChildrenWithParentContext(element);
        file->Close();
    }

    LevelSceneFile scene;
    scene.name = cookedPath;
    ReadVec3(element, "worldOffset", scene.worldOffset, Vec3{0.0f, 0.0f, 0.0f, 0.0f});
    def->sceneFiles.push_back(scene);

    ++def->suppressIncludes;
    ++def->suppressDirectorFiles;
    ++def->suppressSceneFiles;
    ++def->suppressAnimFiles;
    ++def->suppress_24C;
    ProcessChildrenWithParentContext(element);
    --def->suppressIncludes;
    --def->suppressDirectorFiles;
    --def->suppressSceneFiles;
    --def->suppressAnimFiles;
    --def->suppress_24C;
    return true;
}

// @func 0x004EDFA4 LevelDefXml_SceneFile
bool OnSceneFile(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    if (def->suppressSceneFiles)
        return true;
    const char* name = element->GetString("name");
    if (!name)
        return false;
    LevelSceneFile scene;
    if (g_engineConfig.field_0A3)
        scene.name = std::string(name) + ".cooked";
    else
        scene.name = name;
    def->sceneFiles.push_back(scene);
    return true;
}

// @func 0x004EE164 LevelDefXml_AnimFile
bool OnAnimFile(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    if (def->suppressAnimFiles)
        return true;
    if (!AppendName(element, def->animFiles))
        return false;
    uint32_t libraryId = 0;
    element->GetHash("libraryid", &libraryId, 0);
    def->animLibraryIds.push_back(libraryId);
    return true;
}

// @func 0x004EE320 LevelDefXml_DirectorFile
bool OnDirectorFile(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    if (def->suppressDirectorFiles)
        return true;
    return AppendName(element, def->directorFiles);
}

// @func 0x004EE45C LevelDefXml_ZoneFile
bool OnZoneFile(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    return def && AppendName(element, def->zoneFiles);
}

// Accepted but ignored by this build.
// @func 0x004EE584 LevelDefXml_SoundFile
bool OnSoundFile(XmlElement*)
{
    return true;
}

// @func 0x004EE590 LevelDefXml_Bloom
bool OnBloom(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    element->GetBool("enable", &def->bloomEnable, false);
    element->GetFloat("intensity", &def->bloomIntensity, 0.0f);
    element->GetColor("threshold", &def->bloomThreshold, &kWhite);
    def->hasBloom = true;
    return true;
}

// @func 0x004EE650 LevelDefXml_Dof
bool OnDof(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    element->GetBool("enable", &def->dofEnable, false);
    element->GetFloat("startingDistance", &def->dofStartingDistance, 0.0f);
    element->GetFloat("range", &def->dofRange, 0.0f);
    element->GetFloat("intensity", &def->dofIntensity, 0.0f);
    def->dofPasses = ReadByteFromInt(element, "numberOfPasses");
    def->hasDof = true;
    return true;
}

// @func 0x004EE760 LevelDefXml_Desaturation
bool OnDesaturation(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    element->GetBool("enable", &def->desatEnable, false);
    element->GetFloat("intensity", &def->desatIntensity, 0.0f);
    element->GetColor("tint", &def->desatTint, &kWhite);

    // Unknown names leave the previous value untouched.
    if (const char* mode = element->GetString("blendmode")) {
        if (level_detail::EqualsNoCase(mode, "OPAQUE"))
            def->desatBlendMode = kDesatBlend_Opaque;
        else if (level_detail::EqualsNoCase(mode, "BLEND"))
            def->desatBlendMode = kDesatBlend_Blend;
        else if (level_detail::EqualsNoCase(mode, "ADDITIVE"))
            def->desatBlendMode = kDesatBlend_Additive;
        else if (level_detail::EqualsNoCase(mode, "SUBTRACTIVE"))
            def->desatBlendMode = kDesatBlend_Subtractive;
    } else {
        def->desatBlendMode = kDesatBlend_Blend;
    }

    if (const char* type = element->GetString("type")) {
        if (level_detail::EqualsNoCase(type, "MONOCHROME"))
            def->desatType = kDesatType_Monochrome;
        else if (level_detail::EqualsNoCase(type, "NEGATIVE"))
            def->desatType = kDesatType_Negative;
        else if (level_detail::EqualsNoCase(type, "SOLID"))
            def->desatType = kDesatType_Solid;
    } else {
        def->desatType = kDesatType_Monochrome;
    }
    def->hasDesaturation = true;
    return true;
}

// @func 0x004EE990 LevelDefXml_RadialBlur
bool OnRadialBlur(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    element->GetBool("enable", &def->radialBlurEnable, false);
    def->radialBlurTimes = ReadByteFromInt(element, "times");
    element->GetFloat("alphaStart", &def->radialBlurAlphaStart, 0.0f);
    element->GetFloat("UVIncrement", &def->radialBlurUVIncrement, 0.0f);
    def->hasRadialBlur = true;
    return true;
}

// @func 0x004EEA80 LevelDefXml_Blur
bool OnBlur(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    element->GetBool("enable", &def->blurEnable, false);
    def->blurPasses = ReadByteFromInt(element, "numberOfPasses");
    element->GetFloat("intensity", &def->blurIntensity, 0.0f);
    def->hasBlur = true;
    return true;
}

// @func 0x004EEB40 LevelDefXml_WarpScreen
bool OnWarpScreen(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    element->GetBool("enable", &def->warpEnable, false);
    element->GetFloat("amplitudeX", &def->warpAmplitudeX, 0.0f);
    element->GetFloat("amplitudeY", &def->warpAmplitudeY, 0.0f);
    element->GetInt("gridDimX", &def->warpGridDimX, 0);
    element->GetInt("gridDimY", &def->warpGridDimY, 0);
    element->GetFloat("frequencyX", &def->warpFrequencyX, 0.0f);
    element->GetFloat("frequencyY", &def->warpFrequencyY, 0.0f);
    element->GetColor("tint", &def->warpTint, &kWhite);
    return true;
}

// @func 0x004EECB0 LevelDefXml_Shadow
bool OnShadow(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    element->GetFloat("intensity", &def->shadowIntensity, 0.0f);
    element->GetFloat("farZCull", &def->shadowFarZCull, 10.0f);
    if (const char* algorithm = element->GetString("algorithm")) {
        if (level_detail::EqualsNoCase(algorithm, "ZFail"))
            def->shadowAlgorithm = kShadow_ZFail;
        if (level_detail::EqualsNoCase(algorithm, "ZPass"))
            def->shadowAlgorithm = kShadow_ZPass;
        if (level_detail::EqualsNoCase(algorithm, "Auto"))
            def->shadowAlgorithm = kShadow_Auto;
    }
    def->hasShadow = true;
    element->GetFloat("angle_deviation", &def->shadowAngleDeviation, 0.0f);
    ReadVec3(element, "vector", def->shadowVector, Vec3{0.0f, -1.0f, 0.0f, 0.0f});
    // NOTE: the PS3 code divides by the length unconditionally.
    if (Vec3LengthSq(def->shadowVector) > 0.0f)
        Vec3Normalize(def->shadowVector);
    return true;
}

// @func 0x004EEE68 LevelDefXml_Atmospherics
bool OnAtmospherics(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    return def && AppendName(element, def->atmospherics, "include");
}

// @func 0x004EEF90 LevelDefXml_GeomTemplate
bool OnGeomTemplate(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    const char* name = element->GetString("name");
    const char* baseObj = element->GetString("baseObj");
    if (!name || !baseObj)
        return false;

    auto geom = std::make_unique<LevelGeomTemplate>();
    geom->nameHash = HashNameNoCase(name);
    geom->baseObjHash = HashNameNoCase(baseObj);
    StrCopyBounded(geom->name, name, sizeof(geom->name));
    // NOTE: the PS3 code meant to accept only <TextureSwap> children but its
    // test is always true, so every child element is read as one.
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const uint32_t oldTexture = HashNameNoCase(child->GetString("oldTexture"));
        const uint32_t newTexture = HashNameNoCase(child->GetString("newTexture"));
        geom->textureSwaps.Add(oldTexture, newTexture);
    }
    def->geomTemplates.push_back(std::move(geom));
    return true;
}

// @func 0x004EF274 LevelDefXml_AnimClipFile
bool OnAnimClipFile(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    return def && AppendName(element, def->animClipFiles);
}

// @func 0x004EF368 LevelDefXml_AsyncFileGroup
bool OnAsyncFileGroup(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    const char* id = element->GetString("id");
    if (!id)
        return false;

    auto group = std::make_unique<LevelAsyncFileGroup>();
    element->GetHash("id", &group->idHash, group->idHash);
    element->GetHash("type", &group->typeHash, group->typeHash);

    for (XmlElement* child = element->FirstChildElement("AsyncFile"); child;
         child = child->NextSiblingElement("AsyncFile")) {
        LevelAsyncFile file;
        child->GetHash("type", &file.typeHash, 0);
        child->GetHash("id", &file.idHash, 0);
        if (const char* fileName = child->GetString("name"))
            file.fileName = fileName;
        else
            plat::Log(plat::LogLevel::Warning,
                      "ApeLevelDef::AsyncFileCB async file group (%s) contains a file with no filename provided\n",
                      id);
        group->files.push_back(std::move(file));
    }
    // A group without files is dropped.
    if (!group->files.empty())
        def->asyncFileGroups.push_back(std::move(group));
    return true;
}

// @func 0x004EF7F0 LevelDefXml_Audio
bool OnAudio(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    element->GetInt("extraBankMemKB", &def->audioExtraBankMemKB, 0);
    return true;
}

// @func 0x004EF850 LevelDefXml_Texture
bool OnTexture(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    element->GetFloat("gamma", &def->gamma, 0.0f);
    def->hasGamma = true;
    return true;
}

// @func 0x004EF8B8 LevelDefXml_OcclPlane
bool OnOcclPlane(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    element->GetInt("numToChoose", &def->occlPlanesToChoose, 64);
    def->hasOcclPlanes = true;
    return true;
}

// @func 0x004EF920 LevelDefXml_LOD
bool OnLOD(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    element->GetFloat("yInfluence", &def->lodYInfluence, 1.0f);
    def->lodYInfluence = Clamp(def->lodYInfluence, -0.5f, 1.0f);
    return true;
}

// Copies the "name" attribute into a fixed buffer of the current definition.
template <size_t N>
bool CopyNameInto(XmlElement* element, char (ApeLevelDef::*field)[N])
{
    ApeLevelDef* def = s_currentLevelDef;
    if (!def)
        return false;
    if (const char* name = element->GetString("name"))
        StrCopyBounded(def->*field, name, N);
    return true;
}

// @func 0x004EF9E0 LevelDefXml_BGMusic
bool OnBGMusic(XmlElement* element)
{
    return CopyNameInto(element, &ApeLevelDef::bgMusic);
}

// @func 0x004EFA4C LevelDefXml_KnowledgeScriptFile
bool OnKnowledgeScriptFile(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    return def && AppendName(element, def->knowledgeScriptFiles);
}

// @func 0x004EFB40 LevelDefXml_SettingFile
bool OnSettingFile(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    return def && AppendName(element, def->settingFiles);
}

// @func 0x004EFC34 LevelDefXml_PreCacheFiles
bool OnPreCacheFiles(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    return def && AppendName(element, def->preCacheFiles);
}

// @func 0x004EFD28 LevelDefXml_LevelSettingName
bool OnLevelSettingName(XmlElement* element)
{
    return CopyNameInto(element, &ApeLevelDef::levelSettingName);
}

// @func 0x004EFD94 LevelDefXml_LevelSndGroupName
bool OnLevelSndGroupName(XmlElement* element)
{
    return CopyNameInto(element, &ApeLevelDef::levelSndGroupName);
}

// @func 0x004EFE00 LevelDefXml_GuiFile
bool OnGuiFile(XmlElement* element)
{
    ApeLevelDef* def = s_currentLevelDef;
    return def && AppendName(element, def->guiFiles);
}

} // namespace

// ---------------------------------------------------------------------------
// ApeLevelDef
// ---------------------------------------------------------------------------

// @func 0x004EAD20 ApeLevelDef::ApeLevelDef
ApeLevelDef::ApeLevelDef() = default;

// @func 0x004EB11C ApeLevelDef::~ApeLevelDef
ApeLevelDef::~ApeLevelDef() = default;

// @func 0x004EBCF4 ApeLevelDef::LoadLevelData
bool ApeLevelDef::LoadLevelData()
{
    char filePath[0x100];
    std::snprintf(filePath, sizeof(filePath), "Scripts/Levels/%s/Levels.xmb", path);
    s_currentLevelDef = this;
    if (loaded)
        return true;
    if (LoadLevelDataFile(filePath))
        loaded = true;
    return loaded;
}

// @func 0x004ECE04 ApeLevelDef::ClearStringLists
void ApeLevelDef::ClearStringLists()
{
    stringLists.clear();
}

// @func 0x004ECF08 ApeLevelDef::AddStringToList
void ApeLevelDef::AddStringToList(uint32_t key, const char* str)
{
    stringLists[key].emplace_back(str ? str : "");
}

// @func 0x004ED0CC ApeLevelDef::GetStringListCount
uint16_t ApeLevelDef::GetStringListCount(uint32_t key) const
{
    auto it = stringLists.find(key);
    return it == stringLists.end() ? 0 : uint16_t(it->second.size());
}

// @func 0x004ED140 ApeLevelDef::GetStringFromList
const char* ApeLevelDef::GetStringFromList(uint32_t key, int index) const
{
    auto it = stringLists.find(key);
    if (it == stringLists.end() || index < 0 || index >= int(it->second.size()))
        return nullptr;
    return it->second[size_t(index)].c_str();
}

// ---------------------------------------------------------------------------
// Loader registry and dispatch
// ---------------------------------------------------------------------------

// @func 0x004EB904 ProcessLevelDefElements
bool ProcessLevelDefElements(XmlElement* element)
{
    ApeLevelDefXMLLoader* context = s_currentLoader;
    const AVL<uint32_t, ApeLevelDefXMLLoader*>& table = context ? context->children : s_rootLoaders;
    bool result = true;
    for (; element; element = element->NextSiblingElement()) {
        const uint32_t key = element->IsValid() ? element->GetNameHash() : 0;
        ApeLevelDefXMLLoader* loader = nullptr;
        table.Find(key, loader);
        s_currentLoader = loader;
        if (loader) {
            result = loader->callback(element);
            if (result && loader->processChildren)
                result = ProcessLevelDefElements(element->FirstChildElement());
        }
        if (!result)
            break;
    }
    s_currentLoader = context;
    return result;
}

// @func 0x004EC20C RegisterLevelDefXMLLoader
void RegisterLevelDefXMLLoader(const char* path, LevelDefXmlCallback callback, bool processChildren)
{
    // The PS3 code works on a copy truncated to 99 characters.
    std::string fullPath(path ? path : "");
    if (fullPath.size() > 99)
        fullPath.resize(99);

    std::vector<std::string> parts;
    size_t start = 0;
    for (size_t dot; (dot = fullPath.find('.', start)) != std::string::npos; start = dot + 1)
        parts.push_back(fullPath.substr(start, dot - start));
    if (start != fullPath.size())
        parts.push_back(fullPath.substr(start));
    if (parts.empty())
        return;

    // Resolve the parent chain; give up if any parent is missing.
    ApeLevelDefXMLLoader* parent = nullptr;
    for (size_t i = 0; i + 1 < parts.size(); ++i) {
        const AVL<uint32_t, ApeLevelDefXMLLoader*>& table = parent ? parent->children : s_rootLoaders;
        ApeLevelDefXMLLoader* next = nullptr;
        if (!table.Find(HashNameNoCase(parts[i].c_str()), next) || !next)
            return;
        parent = next;
    }

    auto* loader = new ApeLevelDefXMLLoader();
    loader->callback = callback;
    loader->processChildren = processChildren;
    loader->parent = parent;
    AVL<uint32_t, ApeLevelDefXMLLoader*>& table = parent ? parent->children : s_rootLoaders;
    table.Add(HashNameNoCase(parts.back().c_str()), loader);
}

// @func 0x004EC730 RegisterLevelDefXMLLoaders
void RegisterLevelDefXMLLoaders()
{
    RegisterLevelDefXMLLoader("Level", OnLevel, true);
    RegisterLevelDefXMLLoader("LevelData", OnLevelData, true);
    RegisterLevelDefXMLLoader("LevelData.Include", OnInclude, false);
    RegisterLevelDefXMLLoader("LevelData.Level", OnLevelDataLevel, true);
    RegisterLevelDefXMLLoader("LevelData.Level.Include", OnLevelInclude, false);
    RegisterLevelDefXMLLoader("LevelData.Level.Stereo3D", OnStereo3D, true);
    RegisterLevelDefXMLLoader("LevelData.Level.Camera", OnCamera, true);
    RegisterLevelDefXMLLoader("LevelData.Level.Light", OnLight, true);
    RegisterLevelDefXMLLoader("LevelData.Level.Fog", OnFog, true);
    RegisterLevelDefXMLLoader("LevelData.Level.combinedSan", OnCombinedSan, false);
    RegisterLevelDefXMLLoader("LevelData.Level.SceneFile", OnSceneFile, true);
    RegisterLevelDefXMLLoader("LevelData.Level.DirectorFile", OnDirectorFile, true);
    RegisterLevelDefXMLLoader("LevelData.Level.AnimFile", OnAnimFile, true);
    RegisterLevelDefXMLLoader("LevelData.Level.ZoneFile", OnZoneFile, true);
    RegisterLevelDefXMLLoader("LevelData.Level.SoundFile", OnSoundFile, true);
    RegisterLevelDefXMLLoader("LevelData.Level.Atmospherics", OnAtmospherics, true);
    RegisterLevelDefXMLLoader("LevelData.Level.Bloom", OnBloom, true);
    RegisterLevelDefXMLLoader("LevelData.Level.Dof", OnDof, true);
    RegisterLevelDefXMLLoader("LevelData.Level.Desaturation", OnDesaturation, true);
    RegisterLevelDefXMLLoader("LevelData.Level.RadialBlur", OnRadialBlur, true);
    RegisterLevelDefXMLLoader("LevelData.Level.Blur", OnBlur, true);
    RegisterLevelDefXMLLoader("LevelData.Level.WarpScreen", OnWarpScreen, true);
    RegisterLevelDefXMLLoader("LevelData.Level.Shadow", OnShadow, true);
    RegisterLevelDefXMLLoader("LevelData.Level.GeomTemplate", OnGeomTemplate, false);
    RegisterLevelDefXMLLoader("LevelData.Level.AnimClipFile", OnAnimClipFile, false);
    RegisterLevelDefXMLLoader("LevelData.Level.AsyncFileGroup", OnAsyncFileGroup, false);
    RegisterLevelDefXMLLoader("LevelData.Level.Audio", OnAudio, true);
    RegisterLevelDefXMLLoader("LevelData.Level.Texture", OnTexture, true);
    RegisterLevelDefXMLLoader("LevelData.Level.OcclPlane", OnOcclPlane, true);
    RegisterLevelDefXMLLoader("LevelData.Level.LOD", OnLOD, true);
    RegisterLevelDefXMLLoader("LevelData.Level.BGMusic", OnBGMusic, true);
    RegisterLevelDefXMLLoader("LevelData.Level.KnowledgeScriptFile", OnKnowledgeScriptFile, true);
    RegisterLevelDefXMLLoader("LevelData.Level.SettingFile", OnSettingFile, true);
    RegisterLevelDefXMLLoader("LevelData.Level.PreCacheFiles", OnPreCacheFiles, true);
    RegisterLevelDefXMLLoader("LevelData.Level.LevelSettingName", OnLevelSettingName, true);
    RegisterLevelDefXMLLoader("LevelData.Level.LevelSndGroupName", OnLevelSndGroupName, true);
    RegisterLevelDefXMLLoader("LevelData.Level.GuiFile", OnGuiFile, true);
}

// @func 0x004EBE50 LoadLevelList
bool LoadLevelList(const char* path)
{
    XmlDocument doc(path);
    s_currentLoader = nullptr;
    if (!doc.LoadFile(0)) {
        plat::Log(plat::LogLevel::Warning, "Fail to load up the level xml file (%s)!\n", path);
        return true;
    }
    StrCopyBounded(s_currentFile, path, sizeof(s_currentFile));
    XmlElement* config = doc.FirstChildNode("AssetBuilderConfig");
    XmlElement* levels = config ? config->FirstChildNode("Levels") : nullptr;
    XmlElement* first = levels ? levels->FirstChildElement("Level") : nullptr;
    return first ? ProcessLevelDefElements(first) : false;
}

// @func 0x004EBF5C ShutdownLevelDefs
void ShutdownLevelDefs()
{
    ForEachInOrder<ApeLevelDef*>(s_levelDefs.Root(), [](ApeLevelDef* def) { delete def; });
    s_levelDefs.Clear();
    s_currentLevelDef = nullptr;
    s_unnamedLevelDef.reset();
}

// NOTE: on PS3 this is the module's static destructor; it only empties the two
// trees (definitions and loaders were leaked). Loaders are freed here.
// @func 0x004F30AC LevelDefModuleShutdown
void LevelDefModuleShutdown()
{
    s_levelDefs.Clear();
    DeleteLoaderTree(s_rootLoaders.Root());
    s_rootLoaders.Clear();
    s_currentLoader = nullptr;
}

// @func 0x004ECAC4 GetLevelDefCount
uint32_t GetLevelDefCount()
{
    uint32_t count = 0;
    ForEachInOrder<ApeLevelDef*>(s_levelDefs.Root(), [&](ApeLevelDef*) { ++count; });
    return count;
}

// @func 0x004ECC50 GetLevelDefByIndex
ApeLevelDef* GetLevelDefByIndex(uint32_t index)
{
    ApeLevelDef* found = nullptr;
    uint32_t i = 0;
    ForEachInOrder<ApeLevelDef*>(s_levelDefs.Root(), [&](ApeLevelDef* def) {
        if (i++ == index)
            found = def;
    });
    return found;
}

ApeLevelDef* FindLevelDef(uint32_t nameHash)
{
    ApeLevelDef* def = nullptr;
    s_levelDefs.Find(nameHash, def);
    return def;
}

} // namespace ape
