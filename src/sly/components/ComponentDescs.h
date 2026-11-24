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

#include <cstdint>

namespace ape { namespace xml { class XmlElement; } }

namespace sly {
namespace components {

using entity::ComponentDesc;

// Component type numbers of the descriptions below (ComponentDesc::type).
enum ComponentDescType : uint32_t {
    kCompDesc_Anim           = 0x02,
    kCompDesc_ControlAI      = 0x06,
    kCompDesc_AnimMap        = 0x0E,
    kCompDesc_FacePlayer     = 0x16,
    kCompDesc_RadarBlip      = 0x19,
    kCompDesc_SimplePhysics  = 0x1F,
    kCompDesc_JumpRope       = 0x21,
    kCompDesc_AeSpline       = 0x22,
    kCompDesc_Tracker        = 0x26,
    kCompDesc_Vortex         = 0x27,
    kCompDesc_Scoring        = 0x28,
    kCompDesc_SystemCracker  = 0x29,
    kCompDesc_MagNet         = 0x2A,
    kCompDesc_SpeedBoost     = 0x2B,
    kCompDesc_BallRoll       = 0x2D,
    kCompDesc_DamageSequence = 0x2F,
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

// Parsers (root = document element, elem = the component element).
ComponentDesc* ParseAnimComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseControlAIComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseAnimMapComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseFacePlayerComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
ComponentDesc* ParseRadarBlipComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);
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
ComponentDesc* ParseDamageSequenceComponentDesc(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope);

} // namespace components
} // namespace sly
