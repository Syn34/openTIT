// openTIT - IK solver definitions.
//
// An IK file lists named solvers by kind (<JointTwoSolver>, <JointThreeSolver>,
// <LookAtSolver>, <FootSolver>); the <IK> component description refers to
// them by name. Each kind has its own registry keyed by the name hash.
#pragma once

#include "ape/container/ApeTSortedMap.h"
#include "ape/math/ApeMath.h"

#include <cstdint>

namespace ape { namespace xml { class XmlElement; } }

namespace sly {
namespace components {

// Kinds of solver target ("Type", names at 0x00AD4FD4).
enum IKTargetType : uint32_t {
    kIKTarget_Target = 0,        // "Target"
    kIKTarget_Layer  = 1,        // "layer"
};

// <JointTwoSolver> (0xD0 bytes, 16-byte aligned).
struct JointTwoSolverDef {
    char name[0x20];             // 0x00
    uint32_t targetType;         // 0x20 IKTargetType. NOTE: uninitialised on the console without "Type"
    uint32_t layer;              // 0x24
    char firstJoint[0x20];       // 0x28
    char secondJoint[0x20];      // 0x48
    char endJoint[0x20];         // 0x68
    char effectorJoint[0x20];    // 0x88
    ape::Vec3 axis;              // 0xB0 attribute 0xEDAE5B2C, normalised. NOTE: name unknown
    float gain;                  // 0xC0
    float minAngle;              // 0xC4
    float maxAngle;              // 0xC8
};

// <JointThreeSolver> (0xC0 bytes).
struct JointThreeSolverDef {
    char name[0x20];             // 0x00
    uint32_t targetType;         // 0x20
    uint32_t layer;              // 0x24 always 0
    char firstJoint[0x20];       // 0x28
    char secondJoint[0x20];      // 0x48
    char thirdJoint[0x20];       // 0x68
    char endJoint[0x20];         // 0x88
    ape::Vec3 axis;              // 0xB0 attribute 0xEDAE5B2C (not normalised). NOTE: name unknown
};

// <LookAtSolver> (0xA0 bytes).
struct LookAtSolverDef {
    char name[0x20];             // 0x00
    uint32_t layer;              // 0x20
    char headJoint[0x20];        // 0x24
    ape::Vec3 vec50;             // 0x50 attribute 0xE4127843. NOTE: name unknown
    ape::Vec3 vec60;             // 0x60 attribute 0x799BEF43. NOTE: name unknown
    ape::Vec3 vec70;             // 0x70 attribute 0xDE06974A. NOTE: name unknown
    float limitAngle;            // 0x80
    float limitAngleLeft;        // 0x84
    float limitAngleRight;       // 0x88
    float limitAngleUp;          // 0x8C
    float limitAngleDown;        // 0x90
    float range94[2];            // 0x94 attribute 0x63F2322B. NOTE: name unknown
};

// <FootSolver> (0xC0 bytes).
struct FootSolverDef {
    char name[0x20];             // 0x00
    char hipJoint[0x20];         // 0x20
    char kneeJoint[0x20];        // 0x40
    char ankleJoint[0x20];       // 0x60
    ape::Vec3 axis;              // 0x80 unit Z
    ape::Vec3 toeOffset;         // 0x90
    float valueA0;               // 0xA0 attribute 0x055F7F61. NOTE: name unknown
    float valueA4;               // 0xA4 attribute 0xF2ADE153. NOTE: name unknown
    float minAnkle;              // 0xA8 NOTE: name from a hash match
    float maxAnkle;              // 0xAC NOTE: name from a hash match
    float minAngle;              // 0xB0
    float maxAngle;              // 0xB4
    float downDist;              // 0xB8
    float upDist;                // 0xBC
};

// Registries (0x01057874 look-at, 0x0105788C foot, 0x010578A4 joint-two,
// 0x010578BC joint-three).
ape::ApeTSortedMap<LookAtSolverDef*>& LookAtSolverDefs();
ape::ApeTSortedMap<FootSolverDef*>& FootSolverDefs();
ape::ApeTSortedMap<JointTwoSolverDef*>& JointTwoSolverDefs();
ape::ApeTSortedMap<JointThreeSolverDef*>& JointThreeSolverDefs();

LookAtSolverDef* ParseLookAtSolverDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem);
FootSolverDef* ParseFootSolverDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem);
JointTwoSolverDef* ParseJointTwoSolverDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem);
JointThreeSolverDef* ParseJointThreeSolverDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem);
// Parses every solver of an IK file; returns `result` unchanged.
uint8_t ParseIKSolverDefs(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t result);

} // namespace components
} // namespace sly
