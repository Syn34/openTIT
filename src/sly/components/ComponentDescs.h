// openTIT - Component descriptions parsed from entity XML.
//
// An entity definition lists its components as child elements (<Anim>,
// <ControlAI>, <Scoring>, ...). The element name picks a parser from the
// table at 0x00AD4000 (dispatcher 0x00250190); each parser builds the
// description of one component type, registers it in the named template
// table under its "name" attribute and reads the type's attributes.
//
// Every parser returns null when the element has no "name" (the console then
// only fetches the document path for an error message that is compiled out).
#pragma once

#include "sly/entity/Entity.h"

#include "ape/xml/ApeXml.h"

#include <cstdint>

namespace ape { namespace xml { class XmlElement; } }

namespace sly {
namespace components {

using entity::ComponentDesc;

// Component type numbers of the descriptions below (ComponentDesc::type).
enum ComponentDescType : uint32_t {
    kCompDesc_Physics        = 0x00,
    kCompDesc_Anim           = 0x02,
    kCompDesc_ControlAI      = 0x06,
    kCompDesc_AnimMap        = 0x0E,
    kCompDesc_FacePlayer     = 0x16,
    kCompDesc_RadarBlip      = 0x19,
    kCompDesc_OffscreenMarker = 0x1E,
    kCompDesc_SimplePhysics  = 0x1F,
    kCompDesc_JumpRope       = 0x21,
    kCompDesc_AeSpline       = 0x22,
    kCompDesc_Buoyancy       = 0x20,
    kCompDesc_Tracker        = 0x26,
    kCompDesc_Vortex         = 0x27,
    kCompDesc_Scoring        = 0x28,
    kCompDesc_SystemCracker  = 0x29,
    kCompDesc_MagNet         = 0x2A,
    kCompDesc_SpeedBoost     = 0x2B,
    kCompDesc_WallClimb      = 0x2C,
    kCompDesc_BallRoll       = 0x2D,
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

// <Anim> (vtable 0x00B1434C, 0x1C bytes).
struct AnimComponentDesc : ComponentDesc {
    explicit AnimComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_Anim) {}
    uint32_t animSet = 0;        // 0x14 "set"
    uint32_t animTree = 0;       // 0x18 "tree"
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

// <FacePlayer> (vtable 0x00B1C720, 0x20 bytes): turns a part towards the player.
struct FacePlayerComponentDesc : ComponentDesc {
    explicit FacePlayerComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_FacePlayer) {}
    uint32_t part = 0;           // 0x14
    float rotSpeed = 0.0f;       // 0x18 radians per second (degrees in the XML)
    bool lockY = false;          // 0x1C
    bool flag1D = false;         // 0x1D attribute 0x05AB7B55. NOTE: name unknown
};

// <RadarBlip> (vtable 0x00B1C920, 0x18 bytes).
struct RadarBlipComponentDesc : ComponentDesc {
    explicit RadarBlipComponentDesc(uint32_t nameHash) : ComponentDesc(nameHash, kCompDesc_RadarBlip) {}
    uint32_t blipType = 0x03F0D768u;   // 0x14 "Type", default "Enemy"
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
ComponentDesc* ParseAnimComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseControlAIComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseAnimMapComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseFacePlayerComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseRadarBlipComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseOffscreenMarkerComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseSimplePhysicsComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseJumpRopeComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseAeSplineComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseTrackerComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseVortexComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseScoringComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseSystemCrackerComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseMagNetComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseSpeedBoostComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseBallRollComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseBuoyancyComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseWallClimbComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseDamageSequenceComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);

} // namespace components
} // namespace sly
