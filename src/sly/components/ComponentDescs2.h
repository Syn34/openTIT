// openTIT - Component descriptions parsed from entity XML (second part):
// Projector, CustomModel, Weapon, Binocucom and Shooting, plus the element
// name dispatcher. See ComponentDescs.h for the shared conventions.
#pragma once

#include "sly/entity/Entity.h"

#include "ape/math/ApeMath.h"
#include "ape/xml/ApeXml.h"

#include <cstdint>

namespace sly {
namespace components {

using entity::ComponentDesc;

constexpr uint32_t kCompDesc_Weapon      = 0x13;
constexpr uint32_t kCompDesc_Projector   = 0x14;
constexpr uint32_t kCompDesc_CustomModel = 0x15;
constexpr uint32_t kCompDesc_Binocucom   = 0x1A;
constexpr uint32_t kCompDesc_Shooting    = 0x25;

// <Projector> (vtable 0x00B1C670, 0x120 bytes): a light/texture projector
// attached to a part, with an optional light source block.
enum ProjectionKind : uint32_t {
    kProjection_Perspective  = 0,   // child 0xBBD2B6A2: nearz, farz, FOV, AspectRatio
    kProjection_Orthographic = 1,   // child 0xF5A8C1B6: nearz, farz, Width, Height (default)
};
// NOTE: kind names assumed from the attributes each block reads.
enum ProjectorSourceKind : uint32_t {
    kProjectorSource_Part  = 0,     // child 0x671ED2BA: attached to a part, with radius
    kProjectorSource_1     = 1,     // child 0x328B6165
    kProjectorSource_2     = 2,     // child 0x580A9337
    kProjectorSource_None  = 3,
};
struct ProjectorComponentDesc : ComponentDesc {
    ~ProjectorComponentDesc() override;
    explicit ProjectorComponentDesc(uint32_t nameHash);
    uint32_t part = 0;                 // 0x14 "Part"
    ape::Vec3 offsetPos = {0.0f, 0.0f, 0.0f, 0.0f};   // 0x20 "OffsetPos"
    ape::Vec3 vec30 = {0.0f, 0.0f, 0.0f, 0.0f};       // 0x30 attribute 0xC333F453. NOTE: name unknown
    uint32_t lightPart = 0;            // 0x40 "LightPart"
    ape::Vec3 vec50 = {0.0f, 0.0f, 0.0f, 0.0f};       // 0x50 attribute 0x6D1FA88B. NOTE: name unknown
    uint32_t hash60 = 0;               // 0x60 attribute 0x7AB1927A. NOTE: name unknown
    ape::Vec3 vec70 = {0.0f, 0.0f, 0.0f, 0.0f};       // 0x70 attribute 0x7A3E769A. NOTE: name unknown
    float pivotOffset[2] = {0.0f, 0.0f};   // 0x80 "PivotOffset"
    float scale = 1.0f;                // 0x88 "Scale"
    bool flag8C = false;               // 0x8C attribute 0x69EC8F92. NOTE: name unknown
    bool checkAlert = false;           // 0x8D "CheckAlert"
    uint32_t textureName = 0;          // 0x90 "TextureName"
    uint32_t renderOption = 0;         // 0x94 "RenderOption"
    ape::xml::Color8 alertColor;       // 0x98 "AlertColor" (default EntityComp.Projector.AlertColor)
    ape::xml::Color8 normColor;        // 0x9C "NormColor" (default EntityComp.Projector.NormColor)
    uint32_t projection = kProjection_Orthographic;   // 0xA0 ProjectionKind
    float nearZ = 0.1f;                // 0xA4 "nearz"
    float farZ = 10.0f;                // 0xA8 "farz"
    float fovOrWidth = 4.0f;           // 0xAC "FOV" (perspective) / "Width" (orthographic)
    float aspectOrHeight = 4.0f;       // 0xB0 "AspectRatio" / "Height"
    float cullDist = 70.0f;            // 0xB4 "CullDist"
    float cullRange = 30.0f;           // 0xB8 "CullRange"
    float valueBC = 0.0f;              // 0xBC projection attribute 0x3ED04CF9. NOTE: name unknown
    float valueC0 = 0.0f;              // 0xC0 projection attribute 0x9A95F70A. NOTE: name unknown
    float valueC4 = 0.0f;              // 0xC4 attribute 0x66C871E2. NOTE: name unknown
    // Light source block.
    uint32_t sourceKind = kProjectorSource_None;   // 0xC8 ProjectorSourceKind
    uint32_t sourceTexture = 0;        // 0xCC "TextureName"
    ape::xml::Color8 sourceAlertColor; // 0xD0 "AlertColor"
    ape::xml::Color8 sourceNormColor;  // 0xD4 "NormColor"
    float fadeDist = 5.0f;             // 0xD8 "FadeDist"
    float fadeRange = 2.0f;            // 0xDC "fadeRange"
    int32_t valueE0 = 16;              // 0xE0 attribute 0x72BD3841. NOTE: name unknown
    // 0xE4..0x104 are only written by a source block (uninitialised on the console otherwise).
    uint32_t targetPart = 0;           // 0xE4 "TargetPart"
    ape::Vec3 sourceVec = {0.0f, 0.0f, 0.0f, 0.0f};  // 0xE8 attribute 0xD883FE50. NOTE: name unknown
    union {
        uint32_t sourcePart;           // 0xF4 kind 0: "Part"
        float sourceScale;             // 0xF4 kinds 1/2: "Scale" (default 1)
    };
    ape::Vec3 sourceOffsetPos = {0.0f, 0.0f, 0.0f, 0.0f};  // 0xF8 kind 0: "OffsetPos"
    float sourceRadius = 0.0f;         // 0x104 kind 0: "Radius"
    bool flickerEnable = false;        // 0x108 "FlickerEnable"
    float flicker10C = 1.0f;           // 0x10C attribute 0xCE0BA767. NOTE: name unknown
    float flicker110 = 1.0f;           // 0x110 attribute 0xCD9F7715. NOTE: name unknown
    float flickerPeriod = 1.0f;        // 0x114 "FlickerPeriod"

    // Reads a part-attached source block (child 0x671ED2BA); always succeeds.
    bool ReadPartSource(ape::xml::XmlElement* elem);
};

// <CustomModel> (vtable 0x00B1C6C8, 0xB58 bytes): named option groups of
// interchangeable model objects (<group name=...><Option name=... ObjName=.../>).
struct CustomModelOption {
    char name[0x20];                   // +0x00
    uint32_t nameHash;                 // +0x20
    uint32_t objName;                  // +0x24 "ObjName"
};
struct CustomModelGroup {
    char name[0x20];                   // +0x00
    uint32_t nameHash;                 // +0x20
    uint32_t optionCount;              // +0x24
    CustomModelOption options[8];      // +0x28
};
struct CustomModelComponentDesc : ComponentDesc {
    ~CustomModelComponentDesc() override;
    explicit CustomModelComponentDesc(uint32_t nameHash);
    static constexpr uint32_t kMaxGroups = 8;
    static constexpr uint32_t kMaxOptions = 8;
    uint32_t groupCount = 0;           // 0x14
    CustomModelGroup groups[kMaxGroups];   // 0x18
};

// <Weapon> (vtable 0x00B1C828, 0x410 bytes): up to six firing points
// (<Part> children) and the weapon's fire rate.
enum WeaponLaunchType : int32_t {
    kWeaponLaunch_0 = 0,               // 0x0027E3CB. NOTE: name unknown
    kWeaponLaunch_1 = 1,               // 0x5B0F39FA: reads LaunchAngle, MaxRange, PredictPos
    kWeaponLaunch_2 = 2,               // 0x5C0DC16E: reads PredictPos and 0xA65E6A8E
    kWeaponLaunch_3 = 3,               // 0x5D8AAF89
};
struct WeaponFirePoint {
    ape::Vec3 offset;                  // +0x00 attribute 0x04849810. NOTE: name unknown
    ape::Vec3 rot10;                   // +0x10 attribute 0x25DA775A, degrees -> radians. NOTE: name unknown
    ape::Vec3 rot20;                   // +0x20 attribute 0x0A283BAA, degrees -> radians. NOTE: name unknown
    float value30;                     // +0x30 attribute 0x3C395DA0 (default -1). NOTE: name unknown
    ape::Vec3 vec40;                   // +0x40 attribute 0xEE3D09AE. NOTE: name unknown
    uint32_t hash50;                   // +0x50 attribute 0x69ACF13B. NOTE: name unknown
    uint32_t hash54;                   // +0x54 attribute 0xE3D1D420. NOTE: name unknown
    uint32_t hash58;                   // +0x58 attribute 0x3B25897A. NOTE: name unknown
    uint32_t projDefName;              // +0x5C "ProjDefName"
    float speedOverride;               // +0x60 "SpeedOverride" (default -1)
    int32_t launchType;                // +0x64 "LaunchType" (WeaponLaunchType)
    // +0x68 launch-type specific: type 1 {LaunchAngle (radians), MaxRange,
    // PredictPos}; type 2 {PredictPos, attribute 0xA65E6A8E} as two bytes.
    union {
        struct { float launchAngle; float maxRange; bool predictPos; } aimed;
        struct { bool predictPos; bool flag1; } direct;
    };
    float leadTime;                    // +0x74 "LeadTime"
    float value78;                     // +0x78 attribute 0x52A63446. NOTE: name unknown
    float value7C;                     // +0x7C attribute 0x2C394DE0 (default 0.1). NOTE: name unknown
    float value80;                     // +0x80 attribute 0xE776A159 (default -10). NOTE: name unknown
    float range84[2];                  // +0x84 attributes 0xC59F0A85/0xC59F0997 ("...Min"/"...Max"). NOTE: names unknown
    int32_t value8C;                   // +0x8C attribute 0xD97C7082. NOTE: name unknown
    // +0x90 flag bits 31..28.
    bool flag31;                       // attribute 0x9A9BAF78. NOTE: name unknown
    bool flag30;                       // attribute 0xED0B55B1. NOTE: name unknown
    bool requireTarget;                // "RequireTarget"
    bool flattenMatrix;                // "FlattenMatrix"
};
struct WeaponComponentDesc : ComponentDesc {
    ~WeaponComponentDesc() override;
    explicit WeaponComponentDesc(uint32_t nameHash);
    static constexpr uint32_t kMaxFirePoints = 6;
    WeaponFirePoint firePoints[kMaxFirePoints] = {};   // 0x20 (zero-filled by the constructor)
    uint32_t firePointCount = 0;       // 0x3E0
    // Shot intervals (1 / rate): attribute 0xBA5F6FB1 sets both, 0x6807D0E1 /
    // 0x6807CFF3 ("...Min"/"...Max") each one; only positive rates count.
    float intervalMin = 0.0f;          // 0x3E4
    float intervalMax = 0.0f;          // 0x3E8
    ape::Vec3 scatterRange = {0.0f, 0.0f, 0.0f, 0.0f};   // 0x3F0 "ScatterRange", degrees -> radians
    uint32_t fireOrder = 0;            // 0x400 attribute 0x677FEFF1: Sequential, Random, 0xE7EB824F
    uint32_t hash404 = 0;              // 0x404 attribute 0x51D7D9E6. NOTE: name unknown
    int32_t value408 = 0;              // 0x408 attribute 0x887349FF. NOTE: name unknown
    bool flag40C = false;              // 0x40C attribute 0x9941979B. NOTE: name unknown
};

// <Binocucom> (vtable 0x00B1C978, 0x1B4 bytes): the binocucom gadget's
// sounds, UI element paths, scripts, projectiles and vision modes. Defaults
// come from the "EntityComp.Binocucom.Default*" tuning variables.
struct BinocucomProjectile {
    uint32_t def;                      // +0x00 "def"
    float reloadTime;                  // +0x04 "ReloadTime" (default -1)
    int32_t ability;                   // +0x08 index of "Ability" (-1 = none)
    uint32_t iconId;                   // +0x0C "IconID"
};
struct BinocucomVision {
    ape::xml::Color8 colorTint;        // +0x00 "colorTint"
    float visionOffset;                // +0x04 "visionOffset"
    float scanLineFreq;                // +0x08 "scanLineFreq"
    bool mono;                         // +0x0C "mono"
    bool bloom;                        // +0x0D "bloom"
};
struct BinocucomVisionMaterial {
    uint32_t name;                     // +0x00 "name"
    uint32_t replacement;              // +0x04 attribute 0x49AA0D0B. NOTE: name unknown
};
struct BinocucomComponentDesc : ComponentDesc {
    explicit BinocucomComponentDesc(uint32_t nameHash);
    ~BinocucomComponentDesc() override;
    uint32_t hash14 = 0;               // 0x14 attribute 0x1914C72B. NOTE: name unknown
    // Built from "prefixName" (default "Sly").
    uint32_t meterMaterial = 0;        // 0x18 "Mat_%sBicomMeter"
    uint32_t reticleMaterial = 0;      // 0x1C "Mat_%sBicomReticle"
    uint32_t compassMaterial = 0;      // 0x20 "Mat_%sBicomCompass"
    uint32_t igcActorLMaterial = 0;    // 0x24 "Mat_%sBicomIGCActorL"
    uint32_t igcActorRMaterial = 0;    // 0x28 "Mat_%sBicomIGCActorR"
    uint32_t dbStatic = 0;             // 0x2C "STATIC_%sBicomDBStatic"
    uint32_t showSFX = 0;              // 0x30 "ShowSFX"
    uint32_t hideSFX = 0;              // 0x34 "HideSFX"
    uint32_t ambientSFX = 0;           // 0x38 "AmbientSFX"
    uint32_t zoomInSFX = 0;            // 0x3C "ZoomInSFX"
    uint32_t zoomOutSFX = 0;           // 0x40 "ZoomOutSFX"
    uint32_t photoSFX = 0;             // 0x44 "PhotoSFX"
    uint32_t introAnim = 0;            // 0x48 "%sBicom_Intro"
    uint32_t idleAnim = 0;             // 0x4C "%sBicom_Idle"
    uint32_t outroAnim = 0;            // 0x50 "%sBicom_Outro"
    uint32_t introIGCAnim = 0;         // 0x54 "%sBicom_IntroIGC"
    uint32_t idleIGCAnim = 0;          // 0x58 "%sBicom_IdleIGC"
    uint32_t outroIGCAnim = 0;         // 0x5C "%sBicom_OutroIGC"
    uint32_t photo3DAnimGeo = 0;       // 0x60 "3DPhotoAnimGeo"
    uint32_t photo3DAnim = 0;          // 0x64 "3DPhotoAnim"
    // 0x68..0x94 UI element paths (owned copies).
    char* uiParentGroup = nullptr;     // 0x68 "UIParentGroup"
    char* ngReticle = nullptr;         // 0x6C "NGReticle"
    char* partialReticle = nullptr;    // 0x70 "PartialReticle"
    char* fullReticle = nullptr;       // 0x74 "FullReticle"
    char* reticleTitle = nullptr;      // 0x78 "ReticleTitle"
    char* photoParentGroup = nullptr;  // 0x7C "PhotoGroup"
    char* photoImage = nullptr;        // 0x80 "PhotoImage"
    char* photoTitle = nullptr;        // 0x84 "PhotoTitle"
    char* photoDate = nullptr;         // 0x88 "PhotoDate"
    char* targetList = nullptr;        // 0x8C "TargetList"
    char* checkMark = nullptr;         // 0x90 "CheckMark"
    char* checkBox = nullptr;          // 0x94 "CheckBox"
    uint32_t photo2DStartAnim = 0;     // 0x98 "2DPhotoStartAnim"
    uint32_t photo2DEndAnim = 0;       // 0x9C "2DPhotoEndAnim"
    float photoDisplayTime = 0.0f;     // 0xA0 "PhotoDisplayTime"
    uint32_t bicomText = 0;            // 0xA4 "STATIC_%sBicomText"
    char* helpText = nullptr;          // 0xA8 "HelpText"
    char* helpTextSecondary = nullptr; // 0xAC "HelpTextSecondary"
    BinocucomProjectile projectiles[8] = {};   // 0xB0 <Projectile> children
    float value130 = -1.0f;            // 0x130 attribute 0x2B4ED536. NOTE: name unknown
    float angle134 = 0.0f;             // 0x134 attribute 0xADD86E25 (default 20 degrees), radians. NOTE: name unknown
    float angle138 = -1.0f;            // 0x138 attribute 0xD084310C (degrees), radians. NOTE: name unknown
    // 0x13C..0x170 scripts.
    uint32_t inGameShowScript = 0;     // "InGameShowScript"
    uint32_t cinematicShowScript = 0;  // "CinematicShowScript"
    uint32_t tutorialShowScript = 0;   // "TutorialShowScript"
    uint32_t inGameHideScript = 0;     // "InGameHideScript"
    uint32_t cinematicHideScript = 0;  // "CinematicHideScript"
    uint32_t tutorialHideScript = 0;   // "TutorialHideScript"
    uint32_t showLCharScript = 0;      // "ShowLCharacterScript"
    uint32_t showRCharScript = 0;      // "ShowRCharacterScript"
    uint32_t showStaticScript = 0;     // "ShowStaticScript"
    uint32_t staticScript = 0;         // "StaticScript"
    uint32_t alertScript = 0;          // "AlertScript"
    uint32_t fullReticleScript = 0;    // "FullReticleScript"
    uint32_t partialReticleScript = 0; // "PartialReticleScript"
    uint32_t centeredNGReticleScript = 0;   // "CenteredNGReticleScript"
    BinocucomVision visions[2] = {};   // 0x174 Normal, Thermal (<Vision Type=...>)
    BinocucomVisionMaterial visionMaterials[4] = {};   // 0x194 <VisionMaterial> children
};

// <Shooting> (vtable 0x00B1D030, 0x138 bytes): aiming and shooting
// parameters of characters. Angles are read in degrees and kept in radians.
struct ShootingAnim {
    uint32_t projDefName;              // +0x00 "ProjDefName"
    uint32_t hash04;                   // +0x04 attribute 0xE3D1D420. NOTE: name unknown
    float value08;                     // +0x08 attribute 0xC7A1516D. NOTE: name unknown
};
struct ShootingComponentDesc : ComponentDesc {
    ~ShootingComponentDesc() override;
    explicit ShootingComponentDesc(uint32_t nameHash);
    uint32_t attackGround = 0;         // 0x14 "AttackGround"
    uint32_t hash18 = 0;               // 0x18 attribute 0xFB929CF6. NOTE: name unknown
    uint32_t hash1C = 0;               // 0x1C attribute 0x0BE3E3E6. NOTE: name unknown
    uint32_t attackAir = 0;            // 0x20 "AttackAir"
    uint32_t hash24 = 0;               // 0x24 attribute 0xBF170403. NOTE: name unknown
    uint32_t hash28 = 0;               // 0x28 attribute 0x6DF00D33. NOTE: name unknown
    uint32_t attackClimb = 0;          // 0x2C "AttackClimb"
    uint32_t hash30 = 0;               // 0x30 attribute 0xB52544B8. NOTE: name unknown
    uint32_t hash34 = 0;               // 0x34 attribute 0x17163228. NOTE: name unknown
    float angle38 = 0.0137078f;        // 0x38 attribute 0x64C9BAEA (default 0.785398 "degrees", i.e. pi/4 used as degrees), radians. NOTE: name unknown
    float angle3C = 0.0137078f;        // 0x3C attribute 0xA9FE7C7B (default as angle38), radians. NOTE: name unknown
    float aimSticky = 0.7f;            // 0x40 "AimSticky"
    float pitchRange[2] = {0.0137078f, 0.0137078f};   // 0x44 attributes 0x2EC61337/0x2EC61249 (default as angle38), clamped to +-89.9 degrees, radians, ordered
    float value4C = 0.1f;              // 0x4C attribute 0x566B3A00. NOTE: name unknown
    float value50 = 0.1f;              // 0x50 attribute 0xBFC64510. NOTE: name unknown
    float value54 = -1.0f;             // 0x54 attribute 0x6684969D. NOTE: name unknown
    float value58 = -1.0f;             // 0x58 attribute 0x6A0E54B6. NOTE: name unknown
    float angle5C = 0.2617994f;        // 0x5C attribute 0x6326647F (default 15), radians. NOTE: name unknown
    float angle60 = 1.0471976f;        // 0x60 attribute 0x5EBE4369 (default 60), radians. NOTE: name unknown
    uint32_t hash64 = 0;               // 0x64 attribute 0x6350BDB4. NOTE: name unknown
    float angle68 = 0.0174533f;        // 0x68 attribute 0x184920D6 (default 1), radians. NOTE: name unknown
    float angle6C = 0.1396263f;        // 0x6C attribute 0x99DA72E5 (default 8), radians. NOTE: name unknown
    uint32_t aimBone = 0xB3521F36u;    // 0x70 attribute 0xC06D2016 (default "Spine_2"). NOTE: name unknown
    float angle74 = 1.0471976f;        // 0x74 attribute 0x9323107E (default 60, clamped 0..90), radians. NOTE: name unknown
    uint32_t hash78 = 0;               // 0x78 attribute 0xB510DF00. NOTE: name unknown
    float angle7C = 0.0872665f;        // 0x7C attribute 0x93BC69F6 (default 5), radians. NOTE: name unknown
    float value80 = 0.2f;              // 0x80 attribute 0x7142F22E. NOTE: name unknown
    bool flag84 = false;               // 0x84 attribute 0x85F93E26. NOTE: name unknown
    float value88 = 1.0f;              // 0x88 NOTE: not read
    float value8C = 1.0f;              // 0x8C attribute 0x1499581C (clamped 0.2..60). NOTE: name unknown
    uint32_t hash90 = 0;               // 0x90 attribute 0x811B7C68. NOTE: name unknown
    float angle94 = -3.4028235e38f;    // 0x94 attribute 0x0743D16F, radians. NOTE: name unknown
    float angle98 = -1.0f;             // 0x98 attribute 0x456D1135, radians. NOTE: name unknown
    bool flag9C = true;                // 0x9C attribute 0x0AA54FF4. NOTE: name unknown
    bool flag9D = false;               // 0x9D attribute 0x09AED71D. NOTE: name unknown
    bool flag9E = true;                // 0x9E attribute 0xEB1C4F07. NOTE: name unknown
    float angleA0 = 0.0872665f;        // 0xA0 attribute 0xA459F408 (default 5), radians. NOTE: name unknown
    uint32_t countA4 = 10;             // 0xA4 attribute 0xEAF4CDB3 (at most 16). NOTE: name unknown
    float valueA8 = 0.8f;              // 0xA8 NOTE: not read
    float valueAC = 0.05f;             // 0xAC attribute 0x11220C44 (clamped 0.001..1000). NOTE: name unknown
    float valueB0 = 0.05f;             // 0xB0 attribute 0x9D761FB7 (clamped 0.001..1000). NOTE: name unknown
    float valueB4 = 2.0f;              // 0xB4 attribute 0x0C617229 (clamped 0.001..1000). NOTE: name unknown
    float valueB8 = 5.0f;              // 0xB8 attribute 0x013F745E (clamped 0.1..60). NOTE: name unknown
    float angleBC = 1.0471976f;        // 0xBC attribute 0x1DF5AEF3 (default 60, clamped 1..89), radians. NOTE: name unknown
    float valueC0 = 1.0f;              // 0xC0 attribute 0xF2D1211B (clamped 0..1). NOTE: name unknown
    uint32_t hashC4 = 0;               // 0xC4 attribute 0x651DE034. NOTE: name unknown
    uint32_t hashC8 = 0;               // 0xC8 attribute 0x320E57BA. NOTE: name unknown
    uint32_t hashCC = 0;               // 0xCC attribute 0xAE60D046. NOTE: name unknown
    float valueD0 = -1.0f;             // 0xD0 attribute 0x6B64F0FC. NOTE: name unknown
    float valueD4 = -1.0f;             // 0xD4 attribute 0xE9ED1023. NOTE: name unknown
    float valueD8 = 999999.0f;         // 0xD8 attribute 0xE9ED1023 again (console reads the same name twice)
    float angleDC = 0.7853982f;        // 0xDC attribute 0x32B2FC7E, radians. NOTE: name unknown
    float angleE0 = 0.7853982f;        // 0xE0 attribute 0xA29DA90F, radians. NOTE: name unknown
    float angleE4 = 120.0f;            // 0xE4 attribute 0xFBBD7BFB (clamped 45..720 degrees), radians. NOTE: name unknown
    uint32_t hashE8 = 0;               // 0xE8 attribute 0x23136842. NOTE: name unknown
    uint32_t hashEC = 0;               // 0xEC attribute 0x236B23E1. NOTE: name unknown
    float valueF0 = 0.25f;             // 0xF0 attribute 0x05AD2DE4. NOTE: name unknown
    float valueF4 = 0.8f;              // 0xF4 attribute 0x56700203. NOTE: name unknown
    float valueF8 = 1.5f;              // 0xF8 NOTE: not read
    // 0xFC per-"Type" entries (default, CHARGE, 0x838099B5, 0x838099B6).
    ShootingAnim anims[4] = {{0, 0, 100.0f}, {0, 0, 100.0f}, {0, 0, 100.0f}, {0, 0, 100.0f}};
    float angle12C = 5.0f;             // 0x12C attribute 0x7B3B035B (clamped 0..89.9 degrees), radians. NOTE: name unknown
    float value130 = 1.0f;             // 0x130 attribute 0x8833485D. NOTE: name unknown
    uint32_t hash134 = 0;              // 0x134 attribute 0x78727291. NOTE: name unknown
};

ComponentDesc* ParseProjectorComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseCustomModelComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseWeaponComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseBinocucomComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseShootingComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
// Element 0xCE95B536: a component kind whose parser builds nothing.
ComponentDesc* ParseNullComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);

// Builds the description of one component element, picking the parser by
// the element's name; null for unknown names.
ComponentDesc* ParseComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);

} // namespace components
} // namespace sly
