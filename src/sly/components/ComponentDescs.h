// openTIT - Component descriptions parsed from entity XML.
//
// An entity definition lists its components as child elements (<Anim>,
// <ControlAI>, <Scoring>, ...). The element name picks a parser from the
// table at 0x00AD4000 (dispatcher 0x00250190); each parser builds the
// description of one component type, registers it in the named template
// table under its "name" attribute and reads the type's attributes.
//
// Every parser returns null when the element has no "name"; most of them first
// fetch the document path for an error message that is compiled out.
#pragma once

#include "sly/entity/Entity.h"

#include "ape/container/ApeTSortedMap.h"
#include "ape/memory/ApeTArray.h"
#include "ape/xml/ApeXml.h"

#include <cstdint>

namespace ape { namespace xml { class XmlElement; } }

namespace sly {
namespace components {

using entity::ComponentDesc;

// Component type numbers of the descriptions below (ComponentDesc::type).
enum ComponentDescType : uint32_t {
    kCompDesc_Physics        = 0x00,
    kCompDesc_Targeting      = 0x01,
    kCompDesc_Anim           = 0x02,
    kCompDesc_Combat         = 0x03,
    kCompDesc_ContextualMarker = 0x04,
    kCompDesc_ControlAI      = 0x06,
    kCompDesc_AnimMap        = 0x0E,
    kCompDesc_ProcAnim       = 0x0F,
    kCompDesc_ProcEyes       = 0x10,
    kCompDesc_IK             = 0x11,
    kCompDesc_FacePlayer     = 0x16,
    kCompDesc_FX             = 0x17,
    kCompDesc_Radar          = 0x18,
    kCompDesc_RadarBlip      = 0x19,
    kCompDesc_Interact       = 0x1B,
    kCompDesc_Mover          = 0x1D,
    kCompDesc_OffscreenMarker = 0x1E,
    kCompDesc_SimplePhysics  = 0x1F,
    kCompDesc_JumpRope       = 0x21,
    kCompDesc_AeSpline       = 0x22,
    kCompDesc_Buoyancy       = 0x20,
    kCompDesc_Wheelchair     = 0x24,
    kCompDesc_Tracker        = 0x26,
    kCompDesc_Vortex         = 0x27,
    kCompDesc_Scoring        = 0x28,
    kCompDesc_SystemCracker  = 0x29,
    kCompDesc_MagNet         = 0x2A,
    kCompDesc_SpeedBoost     = 0x2B,
    kCompDesc_WallClimb      = 0x2C,
    kCompDesc_BallRoll       = 0x2D,
    kCompDesc_MonsterCloset  = 0x2E,
    kCompDesc_DamageSequence = 0x2F,
};

// <PHYSICS> (vtable 0x00B1BFC8, 0xA0 bytes): the entity's rigid body.
struct PhysicsComponentDesc : ComponentDesc {
    explicit PhysicsComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Physics) {}
    void* world = nullptr;       // 0x14 level physics world when parsed
    uint32_t field18[2] = {};    // 0x18 NOTE: not set by the parser
    ape::Matrix34 transform = {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f},
                               {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}};  // 0x20 identity
    uint32_t field60 = 0;        // 0x60
    float mass = 100.0f;         // 0x64
    ape::Vec3 size = {0.65f, 1.0f, 0.5f, 0.0f};    // 0x70
    ape::Vec3 offset = {0.0f, 0.0f, 0.0f, 0.0f};   // 0x80
    bool enabled = true;         // 0x90
    bool flag91 = false;         // 0x91 attribute 0x8BB0F765. NOTE: name unknown
};

// <targeting> (vtable 0x00B13C74, 0xE0 bytes): how the entity can be
// targeted, one range block per targeting kind (child elements named after
// the kinds at 0x00AD4D34: two unknown, Throw, Interact, Carry, Collect).
struct TargetingRange {
    int32_t maxTargets = 0;      // +0x00
    float radius = 0.0f;         // +0x04
    float angle = 90.0f;         // +0x08
    float heightRange[2] = {-3.4028235e38f, 3.4028235e38f};   // +0x0C attribute 0x9C819064. NOTE: name unknown
    float value14 = 0.5f;        // +0x14 attribute 0xF5DFB24A. NOTE: name unknown
};
struct TargetingComponentDesc : ComponentDesc {
    explicit TargetingComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Targeting) {}
    TargetingRange ranges[6];    // 0x14
    bool simple = false;         // 0xA4
    bool raycast = false;        // 0xA5
    bool useCamera = false;      // 0xA6
    uint32_t boneName = 0;       // 0xA8
    uint32_t partName = 0;       // 0xAC NOTE: uninitialised on the console before the read
    ape::Vec3 offsetPos = {0.0f, 0.0f, 0.0f, 0.0f};   // 0xB0
    ape::Vec3 vecC0 = {0.0f, 0.0f, 0.0f, 0.0f};       // 0xC0 attribute 0xC333F453. NOTE: name unknown
    bool flagD0 = false;         // 0xD0 attribute 0x69EC8F92. NOTE: name unknown; uninitialised on the console
};

// <Anim> (vtable 0x00B1434C, 0x1C bytes).
struct AnimComponentDesc : ComponentDesc {
    explicit AnimComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Anim) {}
    uint32_t animSet = 0;        // 0x14 "set"
    uint32_t animTree = 0;       // 0x18 "tree"
};

// <Combat> (vtable 0x00B1C040, 0x48 bytes): attack definitions and the
// reaction animation set per reaction type (<Reaction Type=... Name=...>).
struct CombatComponentDesc : ComponentDesc {
    explicit CombatComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Combat) {}
    // 0x14..0x28: two groups of three hashes (attributes 0xFB92D658,
    // 0xFB92E002, 0xFB929D5A and 0xBF173D65, 0xBF17470F, 0xBF170467).
    // NOTE: names unknown.
    uint32_t hashes[6] = {};
    float value2C = 1.0f;        // 0x2C attribute 0x7CDB801C. NOTE: name unknown
    // 0x30 reaction type -> name; type "default" is stored as 0; lookups
    // of missing types give 0xFFFFFFFF.
    ape::ApeTSortedMap<uint32_t> reactions{0x10, nullptr, 0xFFFFFFFFu};
};

// <ContextualMarker> (vtable 0x00B1C0BC, 0x70 bytes): a HUD marker
// attached to the entity, scaled with distance.
struct ContextualMarkerComponentDesc : ComponentDesc {
    explicit ContextualMarkerComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_ContextualMarker) {}
    ape::Vec4 attachRot = {0.0f, 0.0f, 0.0f, 1.0f};       // 0x20 quaternion of AttachRot (degrees)
    ape::Vec3 attachOffset = {0.0f, 0.0f, 0.0f, 0.0f};    // 0x30
    ape::Vec3 attachCameraOffset = {0.0f, 0.0f, 0.0f, 0.0f};   // 0x40
    uint32_t attachPart = 0;     // 0x50
    uint32_t geomDef = 0;        // 0x54
    float scaleMin = -1.0f;      // 0x58
    float scaleMax = -1.0f;      // 0x5C
    float scaleDistMin = -1.0f;  // 0x60
    float scaleDistMax = -1.0f;  // 0x64
    float scaleFactorFOV = 0.0f; // 0x68
    bool hasScaleFactorFOV = false; // 0x6C the XML gave ScaleFactorFOV
    bool applyRotation = false;  // 0x6D NOTE: uninitialised on the console before the read
    bool startEnabled = false;   // 0x6E NOTE: uninitialised on the console before the read
};

// <ControlAI> (vtable 0x00B1432C, 0x18 bytes).
struct ControlAIComponentDesc : ComponentDesc {
    explicit ControlAIComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_ControlAI) {}
    uint32_t behavior = 0;       // 0x14
};

// <AnimMap> (vtable 0x00B1C618, 0x24 bytes).
struct AnimMapComponentDesc : ComponentDesc {
    explicit AnimMapComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_AnimMap) {}
    uint32_t map = 0;            // 0x14
    uint32_t tree = 0;           // 0x18
    uint32_t blendMap = 0;       // 0x1C
    uint32_t animSet = 0;        // 0x20 "set"
};

// <ProcAnim> (vtable 0x00B1C568, 0x80 bytes): spring-driven bone chain.
struct ProcAnimComponentDesc : ComponentDesc {
    explicit ProcAnimComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_ProcAnim) {}
    // Flag word 0x20.
    bool flag31 = false;         // bit 31 attribute 0x002480E8. NOTE: name unknown
    bool flag30 = false;         // bit 30 attribute 0x724F4DDA. NOTE: name unknown
    uint8_t iterate = 2;         // bits 22..29
    float spring = 0.05f;        // 0x24
    float dampingWorld = 0.1f;   // 0x28
    float dampingLocal = 0.1f;   // 0x2C
    float angleMax = 30.0f;      // 0x30
    ape::Vec3 gravity = {0.0f, -9.81f, 0.0f, 0.0f};   // 0x40
    ape::Vec3 vec50 = {0.0f, 0.0f, 0.0f, 0.0f};       // 0x50 attribute 0x34BF0B12. NOTE: name unknown
    ape::ApeTArray<uint32_t> bones;                    // 0x60 boneName of each child (tag 0xA0702B13)
};

// <ProcEyes> (vtable 0x00B1C5C0, 0x28 bytes): procedural eyes, one <Eye>
// element per eye.
struct ProcEye {
    uint32_t name = 0;           // +0x00 required
    uint32_t material = 0;       // +0x04 required
    uint32_t hash08 = 0;         // +0x08 attribute 0xE79D4302, optional. NOTE: name unknown
    uint32_t bone = 0;           // +0x0C required
    uint32_t eyelidBone = 0;     // +0x10 required
    uint32_t hash14 = 0;         // +0x14 attribute 0x1A1D3D81, required. NOTE: name unknown
    uint32_t hash18 = 0;         // +0x18 attribute 0x98247D0B, required. NOTE: name unknown
    bool enableBlink = false;    // +0x1C required
    uint8_t byte1D = 0;          // +0x1D
    float blinkMin = 0.0f;       // +0x20 attribute 0xD9DE2696 ("...Min"), read with blinking. NOTE: name unknown
    float blinkMax = 0.0f;       // +0x24 attribute 0xD9DE25A8 ("...Max"). NOTE: name unknown
    float blink28 = 0.0f;        // +0x28 attribute 0xD211F8D0. NOTE: name unknown
    float upperRotPivot[2] = {0.5f, 0.5f};   // +0x2C UpperRotPivotU/V
    float lowerRotPivot[2] = {0.5f, 0.5f};   // +0x34 LowerRotPivotU/V
};
struct ProcEyesComponentDesc : ComponentDesc {
    explicit ProcEyesComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_ProcEyes) {}
    bool flag11 = false;         // 0x11 attribute 0x2A9B2CB7 (in the base's padding). NOTE: name unknown
    ape::ApeTArray<ProcEye> eyes;   // 0x14
};

// <IK> (vtable 0x00B1C7D0, 0x60 bytes): names of the IK solvers the
// component drives, from its child elements.
struct IKComponentDesc : ComponentDesc {
    explicit IKComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_IK) {}
    uint32_t jointTwoSolvers[8] = {};    // 0x14 <JointTwoSolver Name=...>
    // 0x34 <JointThreeSolver Name=...>. NOTE: the console leaves unused slots
    // uninitialised.
    uint32_t jointThreeSolvers[8] = {};
    uint32_t footSolvers[2] = {};        // 0x54 <FootSolver Name=...>
    uint32_t lookAtSolver = 0;           // 0x5C <LookAtSolver Name=...> NOTE: uninitialised without one
};

// <FacePlayer> (vtable 0x00B1C720, 0x20 bytes): turns a part towards the player.
struct FacePlayerComponentDesc : ComponentDesc {
    explicit FacePlayerComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_FacePlayer) {}
    uint32_t part = 0;           // 0x14
    float rotSpeed = 0.0f;       // 0x18 radians per second (degrees in the XML)
    bool lockY = false;          // 0x1C
    bool flag1D = false;         // 0x1D attribute 0x05AB7B55. NOTE: name unknown
};

// <FX> (vtable 0x00B1C778, 0xE0 bytes): effects played on the entity for
// the gadget powers (one child element per power; all fields start zero).
struct PowerFx {
    uint32_t objName = 0;        // +0x00
    uint32_t script = 0;         // +0x04
    uint32_t defName = 0;        // +0x08
    float timeout = 0.0f;        // +0x0C
};
struct FxComponentDesc : ComponentDesc {
    explicit FxComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_FX) {}
    PowerFx cloak;               // 0x20 <Cloak>
    float cloakPowerCost = 0.0f; // 0x30
    PowerFx electricity;         // 0x34 <Electricity>
    PowerFx fire;                // 0x44 <fire>
    PowerFx poison;              // 0x54 <Poison>
    PowerFx ice;                 // 0x64 <Ice>
    uint32_t pad74[3] = {};      // 0x74
    // 0x80: child element 0x654ADD4F. NOTE: element name unknown.
    uint32_t glowObjName = 0;    // 0x80
    float glow84 = 0.0f;         // 0x84 attribute 0x71D1F169 (default 0.8). NOTE: name unknown
    uint32_t pad88[2] = {};
    ape::Vec3 glowScale = {0.0f, 0.0f, 0.0f, 0.0f};   // 0x90 attribute 0x1BAD0B40 (default 1.05). NOTE: name unknown
    float colorScale[4] = {};    // 0xA0 ColorScaleR/G/B/A (defaults 1.1, 1.1, 1.1, 1.5)
    bool glowB0 = false;         // 0xB0 attribute 0xA621B465. NOTE: name unknown
    bool glowB1 = false;         // 0xB1 attribute 0x6E828D95. NOTE: name unknown
    uint8_t padB2[14] = {};
    // 0xC0 <SlowTime>.
    float slowTimeRadius = 0.0f;     // 0xC0
    float slowTimeTimeout = 0.0f;    // 0xC4
    float slowTimeScale = 0.0f;      // 0xC8
    float slowTimePowerCost = 0.0f;  // 0xCC
    uint32_t slowTimeHashD0 = 0;     // 0xD0 attribute 0x4F113DAE. NOTE: name unknown
    float slowTimeD4 = 0.0f;         // 0xD4 attribute 0x173BA787 (default 1). NOTE: name unknown
    float slowTimeD8 = 0.0f;         // 0xD8 attribute 0x4E1FBF3A. NOTE: name unknown
};

// <Radar> (vtable 0x00B1C8C8, 0x60 bytes): Binocucom radar target.
struct RadarDefEntry {
    uint32_t def = 0;            // +0x00 "Def"
    uint32_t value = 0;          // +0x04 attribute 0x7BA7CDEF. NOTE: name unknown
};
struct RadarComponentDesc : ComponentDesc {
    explicit RadarComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Radar) {}
    // 0x14: marker type per radar mode (location, entity, binoPhoto,
    // binoRecord, hacking, vehicle, blueprint, follow, switch).
    ape::ApeTArray<uint32_t> markerTypes;
    ape::ApeTArray<RadarDefEntry> defs;   // 0x28 child elements 0xDE886EE4
    uint32_t initScript = 0;         // 0x3C
    uint32_t onActivateScript = 0;   // 0x40
    uint32_t hash44 = 0;             // 0x44 attribute 0x7C4EBDF7. NOTE: name unknown
    float compassScale = 1.0f;       // 0x48
    uint32_t hash4C = 0;             // 0x4C attribute 0xC9796122. NOTE: name unknown
    uint32_t hash50 = 0;             // 0x50 attribute 0xA7FC59B2. NOTE: name unknown
    float activeDuration = 0.0f;     // 0x54 default EntityComp.Radar.DefaultActiveDuration
    float refreshDuration = 0.0f;    // 0x58 default EntityComp.Radar.DefaultRefreshDuration
    int32_t ignoreEntities = 0;      // 0x5C 1 when IgnoreEntities is set
};

// <RadarBlip> (vtable 0x00B1C920, 0x18 bytes).
struct RadarBlipComponentDesc : ComponentDesc {
    explicit RadarBlipComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_RadarBlip) {}
    uint32_t blipType = 0x03F0D768u;   // 0x14 "Type", default "Enemy"
};

// <Interact> (vtable 0x00B1CE10, 0x60 bytes): how the player interacts
// with the entity.
enum InteractPhysicsState : uint32_t {   // names at 0x00AD3FE4
    kPhysState_None, kPhysState_Ground, kPhysState_Air, kPhysState_Climb,
    kPhysState_Crawl, kPhysState_PlayDead, kPhysState_ClimbWall,
};
struct InteractComponentDesc : ComponentDesc {
    explicit InteractComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Interact) {}
    ape::Vec3 vec20 = {0.0f, 0.0f, 0.0f, 0.0f};    // 0x20 attribute 0x3B6C1F30. NOTE: name unknown
    ape::Vec3 rotation = {0.0f, 0.0f, 0.0f, 0.0f}; // 0x30 attribute 0x98DB3873, radians. NOTE: name unknown
    float interactDistSq = -1.0f;    // 0x40 InteractDist, squared when positive
    float halfAngleThreshold = -1.0f; // 0x44 half of AngleThreshold, radians
    float halfAngle48 = -1.0f;       // 0x48 attribute 0x17A28676, half angle in radians. NOTE: name unknown
    uint32_t scriptLifetime = 0;     // 0x4C NOTE: name from a hash match
    float value50 = 0.0f;            // 0x50 attribute 0x3A20269E. NOTE: name unknown
    uint32_t physicsStates = 1u << kPhysState_Ground;   // 0x54 mask of InteractPhysicsState
    // Flag word 0x58, bits 31..23.
    bool startAvailable = false;     // bit 31
    bool flags[7] = {false, false, false, false, false, false, true};   // bits 30..24. NOTE: names unknown
    bool activeNoCull = false;       // bit 23
    char* text = nullptr;            // 0x5C copy of attribute 0x6E2EE341. NOTE: name unknown
};

// <Mover> (vtable 0x00B1CEC0, 0x130 bytes): moves the entity along a path
// of up to eight child points.
struct MoverPoint {
    uint32_t tag = 0;            // +0x00 element name hash of the child
    uint32_t name = 0;           // +0x04 "Name"
    uint32_t pad[2] = {};
    ape::Vec3 offset = {0.0f, 0.0f, 0.0f, 0.0f};   // +0x10 "Offset"
};
struct MoverComponentDesc : ComponentDesc {
    explicit MoverComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Mover) {}
    uint32_t pathType = 0xE2E902C9u;   // 0x14 NOTE: default name unknown
    uint32_t motionType = 0x85D98325u; // 0x18 default "Linear"
    float speed = 1.0f;          // 0x1C
    float pauseDelayMin = 0.0f;  // 0x20 NOTE: name from a hash match
    float pauseDelayMax = 0.0f;  // 0x24 NOTE: name from a hash match
    bool flag28 = true;          // 0x28 attribute 0x86BE6BD3. NOTE: name unknown
    bool flag29 = true;          // 0x29 attribute 0xE9B00400. NOTE: name unknown
    int32_t pointCount = 0;      // 0x2C
    MoverPoint points[8];        // 0x30
};

// <OffscreenMarker> (vtable 0x00B1CF18, 0x2C bytes): HUD marker of an
// entity, with its own colours on and off screen.
struct OffscreenMarkerComponentDesc : ComponentDesc {
    explicit OffscreenMarkerComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_OffscreenMarker) {}
    uint32_t hash14 = 0;         // 0x14 attribute 0x457D8076. NOTE: name unknown
    uint32_t hash18 = 0;         // 0x18 attribute 0x4EA1C7E6. NOTE: name unknown
    float rangeSq = -1.0f;       // 0x1C attribute 0x4852D5AF, squared when positive. NOTE: name unknown
    ape::xml::Color8 onscreenColor = {0xFF, 0xFF, 0xFF, 0xFF};    // 0x20 (shared white 0x01164CA0)
    ape::xml::Color8 offscreenColor = {0xFF, 0xFF, 0xFF, 0xFF};   // 0x24
    // Flag word 0x28 (bits 31..28).
    bool startEnabled = true;    // bit 31
    bool hasOnscreenColor = false;   // bit 30: the XML gave OnscreenColor
    bool hasOffscreenColor = false;  // bit 29: the XML gave OffscreenColor
    bool flag28 = false;         // bit 28 attribute 0xF6DB9816. NOTE: name unknown
};

// <SimplePhysics> (vtable 0x00B1CF70, 0x24 bytes).
struct SimplePhysicsComponentDesc : ComponentDesc {
    explicit SimplePhysicsComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_SimplePhysics) {}
    float radius = 0.5f;         // 0x14
    float height = 1.0f;         // 0x18
    float mass = 0.0f;           // 0x1C
    bool startEnabled = true;    // 0x20
};

// <JumpRope> (vtable 0x00B1CB38, 0x1C bytes).
struct JumpRopeComponentDesc : ComponentDesc {
    explicit JumpRopeComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_JumpRope) {}
    uint32_t materialObject = 0; // 0x14
    uint32_t anchorEntity = 0;   // 0x18
};

// <AeSpline> (vtable 0x00B0EF54, 0x14 bytes): no attributes of its own.
struct AeSplineComponentDesc : ComponentDesc {
    explicit AeSplineComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_AeSpline) {}
};

// <Wheelchair> (vtable 0x00B1CFC8, 0x130 bytes): Bentley's wheelchair, with
// up to two <Arm> elements (root bone, IK bones, slave bones).
struct WheelchairSlave {
    uint32_t name = 0;           // +0x00
    uint32_t parent = 0;         // +0x04
};
struct WheelchairComponentDesc : ComponentDesc {
    explicit WheelchairComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Wheelchair) {}
    int32_t chairType = 0;       // 0x14
    uint32_t chairBone = 0;      // 0x18
    uint32_t wheelBoneL = 0;     // 0x1C
    uint32_t wheelBoneR = 0;     // 0x20
    uint32_t hash24 = 0;         // 0x24 attribute 0xCF9ADB4C. NOTE: name unknown
    uint32_t armRoot[2] = {};    // 0x28
    uint32_t armIkBones[2][16] = {};   // 0x30
    // 0xB0. NOTE: not cleared by the console (unused slots stay uninitialised).
    WheelchairSlave armSlaves[2][8] = {};
};

// <tracker> (vtable 0x00B1D0F0, 0x2C bytes).
struct TrackerComponentDesc : ComponentDesc {
    explicit TrackerComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Tracker) {}
    uint32_t script = 0;         // 0x14
    float minRange = 0.0f;       // 0x18
    float maxRange = 1.0f;       // 0x1C
    float minAngle = 0.0f;       // 0x20
    float maxAngle = 1.0f;       // 0x24
    bool startEnabled = true;    // 0x28
};

// <Vortex> (vtable 0x00B1CBA8, 0x1C bytes). NOTE: the console leaves both
// values uninitialised before the reads (they are the read defaults).
struct VortexComponentDesc : ComponentDesc {
    explicit VortexComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Vortex) {}
    float radius = 0.0f;         // 0x14
    float strength = 0.0f;       // 0x18
};

// <Scoring> (vtable 0x00B1CC70, 0x40 bytes).
struct ScoringComponentDesc : ComponentDesc {
    explicit ScoringComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Scoring) {}
    float score = 0.0f;          // 0x14 NOTE: uninitialised on the console before the read
    ape::Vec3 endPos = {0.0f, 0.0f, 0.0f, 0.0f};   // 0x20
    float startVel = 0.0f;       // 0x30
    float damp = 5.0f;           // 0x34
    float speed = 20.0f;         // 0x38
};

// <SystemCracker> (vtable 0x00B1CC00, 0x1C bytes).
struct SystemCrackerComponentDesc : ComponentDesc {
    explicit SystemCrackerComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_SystemCracker) {}
    uint32_t script = 0;         // 0x14
    bool flag18 = false;         // 0x18 attribute 0xCC2ABC8B. NOTE: name unknown
};

// Shared layout of <MagNet> (vtable 0x00B1CCC8) and <SpeedBoost>
// (0x00B1CDA0), 0x2C bytes: a pulsing radial field.
struct PulseFieldComponentDesc : ComponentDesc {
    PulseFieldComponentDesc(uint32_t nameHash, uint32_t componentType) : ComponentDesc(nameHash, componentType) {}
    float radius = 0.0f;         // 0x14
    float strength = 0.0f;       // 0x18
    float damping = 0.0f;        // 0x1C
    float onDuration = 0.0f;     // 0x20
    float offDuration = 0.0f;    // 0x24
    float timeOffset = 0.0f;     // 0x28
};
struct MagNetComponentDesc : PulseFieldComponentDesc {
    explicit MagNetComponentDesc(uint32_t nameHash) : PulseFieldComponentDesc(nameHash, kCompDesc_MagNet) {}
};
struct SpeedBoostComponentDesc : PulseFieldComponentDesc {
    explicit SpeedBoostComponentDesc(uint32_t nameHash) : PulseFieldComponentDesc(nameHash, kCompDesc_SpeedBoost) {}
};

// <BallRoll> (vtable 0x00B1D1D0, 0x30 bytes).
struct BallRollComponentDesc : ComponentDesc {
    explicit BallRollComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_BallRoll) {}
    uint32_t boneName = 0;       // 0x14
    uint32_t entityName = 0;     // 0x18
    float radius = 0.55f;        // 0x1C "Radius", then "RefRadius" (see the parser)
    float field20 = 0.5f;        // 0x20 NOTE: not read from the XML
    bool grow = false;           // 0x24
    uint32_t hash28 = 0;         // 0x28 attribute 0x5D1A6CBA. NOTE: name unknown
    uint32_t hash2C = 0;         // 0x2C attribute 0x2BCD00F1. NOTE: name unknown
};

// <MonsterCloset> (vtable 0x00B1CD48, 0x2C bytes): spawns entities, one
// <Spawn>-style child element (tag hash 0x7480D3BA) per kind.
struct MonsterClosetSpawn {
    uint32_t type = 0;           // 0x00 index of "Type" in {none, "Mine"}
    uint32_t def = 0;            // 0x04 entity definition
    uint32_t spawnPoint = 0;     // 0x08
    float respawnCooldown = 0.0f; // 0x0C
    bool respawnEnabled = true;  // 0x10
};
struct MonsterClosetComponentDesc : ComponentDesc {
    explicit MonsterClosetComponentDesc(uint32_t nameHash)
        : ComponentDesc(nameHash, kCompDesc_MonsterCloset)
    {
        spawns.Init(0x20, nullptr, true);
    }
    ape::ApeTArray<MonsterClosetSpawn> spawns;   // 0x14
    float respawnCooldown = 0.0f;                // 0x28
};

// <DamageSequence> (vtable 0x00B1D258, 0x2C bytes).
struct DamageSequenceComponentDesc : ComponentDesc {
    explicit DamageSequenceComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_DamageSequence) {}
    uint32_t script = 0;         // 0x14
    int32_t int18 = -1;          // 0x18 attribute 0xBF260CF3. NOTE: name unknown
    float initTime = 1.0f;       // 0x1C
    float enabledTime = 1.0f;    // 0x20
    float disabledTime = 1.0f;   // 0x24
    bool startEnabled = true;    // 0x28
    bool flag29 = false;         // 0x29 attribute 0x173A6338. NOTE: name unknown
};

// <Buoyancy> (vtable 0x00B1CAA0, 0x64 bytes): floating on water.
struct BuoyancyComponentDesc : ComponentDesc {
    explicit BuoyancyComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Buoyancy) {}
    float maxDepth = 4.0f;       // 0x14
    float floatSpeed = 2.0f;     // 0x18
    float sinkSpeed = 1.0f;      // 0x1C
    float value20 = 0.4f;        // 0x20 attribute 0xFFF7B927. NOTE: name unknown
    float value24 = 0.5f;        // 0x24 attribute 0x7F154347. NOTE: name unknown
    float value28 = 0.6f;        // 0x28 attribute 0x827EF485. NOTE: name unknown
    ape::Vec3 field2C{};         // 0x2C..0x38 NOTE: not set by the parser (left uninitialised)
    float value3C = 3.0f;        // 0x3C attribute 0x4801705E. NOTE: name unknown
    float value40 = 0.2f;        // 0x40 attribute 0xCD0D2AF5. NOTE: name unknown
    float value44 = 3.0f;        // 0x44 attribute 0x708A4E60. NOTE: name unknown
    float tiltSpeed = 1.0f;      // 0x48
    float value4C = 1.0f;        // 0x4C attribute 0xC4F41CF6. NOTE: name unknown
    float maxRotationX = 30.0f;  // 0x50
    float maxRotationY = 0.0f;   // 0x54
    float maxRotationZ = 30.0f;  // 0x58
    uint32_t contactScript = 0;  // 0x5C
    float contactInterval = 1.0f; // 0x60
};

// <WallClimb> (vtable 0x00B1D148, 0x70 bytes). Angles are read in degrees.
struct WallClimbComponentDesc : ComponentDesc {
    explicit WallClimbComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_WallClimb) {}
    float grabAngleH = 60.0f;    // 0x14 radians after parsing
    float grabDistance = 8.0f;   // 0x18
    float maxAngleV = 15.0f;     // 0x1C radians after parsing
    float maxAngleH = 30.0f;     // 0x20 radians after parsing
    float maxDistance = 0.5f;    // 0x24
    float stickThreshold = 0.3f; // 0x28
    uint32_t field2C = 0;        // 0x2C NOTE: not set by the parser
    ape::Vec3 offset30 = {0.8f, 1.4f, 0.8f, 0.0f};    // 0x30 NOTE: role unknown, not read
    ape::Vec3 offset40 = {0.0f, 0.0f, -0.45f, 0.0f};  // 0x40 NOTE: role unknown, not read
    float value50 = 0.0f;        // 0x50 attribute 0x977C8AB2. NOTE: name unknown
    float value54 = 15.0f;       // 0x54 attribute 0xB081C3B4. NOTE: name unknown
    float value58 = 9.8f;        // 0x58 attribute 0x01B279B3. NOTE: name unknown
    float field5C = 0.4f;        // 0x5C NOTE: not read
    float value60 = 0.5f;        // 0x60 attribute 0x009DAD78. NOTE: name unknown
};

// Parsers (root = document element, elem = the component element).
ComponentDesc* ParsePhysicsComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseTargetingComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseAnimComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseCombatComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseContextualMarkerComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseControlAIComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseAnimMapComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseProcAnimComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseProcEyesComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseIKComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseFacePlayerComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseFxComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseRadarComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseRadarBlipComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseInteractComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseMoverComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseOffscreenMarkerComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseSimplePhysicsComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseJumpRopeComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseAeSplineComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseWheelchairComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseTrackerComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseVortexComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseScoringComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseSystemCrackerComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseMagNetComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseSpeedBoostComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseBallRollComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseBuoyancyComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseWallClimbComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseMonsterClosetComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseDamageSequenceComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);

} // namespace components
} // namespace sly
