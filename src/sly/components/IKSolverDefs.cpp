// openTIT - IK solver definitions.
#include "sly/components/IKSolverDefs.h"

#include "ape/core/ApeString.h"
#include "ape/memory/ApeMem.h"
#include "ape/xml/ApeXml.h"

#include <cmath>
#include <new>

namespace sly {
namespace components {

namespace {

using ape::xml::XmlElement;

constexpr uint32_t kTag_JointTwoSolver   = 0x75215015u;
constexpr uint32_t kTag_JointThreeSolver = 0x6AD11467u;
constexpr uint32_t kTag_LookAtSolver     = 0x5BDDD9A5u;
constexpr uint32_t kTag_FootSolver       = 0x895ACE01u;

constexpr uint32_t kAttr_Type            = 0x0027873Au;
constexpr uint32_t kAttr_Layer           = 0x044DDD31u;
constexpr uint32_t kAttr_Axis            = 0xEDAE5B2Cu;   // NOTE: name unknown
constexpr uint32_t kAttr_Gain            = 0x0021437Fu;
constexpr uint32_t kAttr_MinAngle        = 0xDD33E5C1u;
constexpr uint32_t kAttr_MaxAngle        = 0x471281EFu;
constexpr uint32_t kAttr_LookAt50        = 0xE4127843u;   // NOTE: name unknown
constexpr uint32_t kAttr_LookAt60        = 0x799BEF43u;   // NOTE: name unknown
constexpr uint32_t kAttr_LookAt70        = 0xDE06974Au;   // NOTE: name unknown
constexpr uint32_t kAttr_LimitAngle      = 0xB4C235F8u;
constexpr uint32_t kAttr_LimitAngleLeft  = 0x7C6A11FFu;
constexpr uint32_t kAttr_LimitAngleRight = 0x112E8F04u;
constexpr uint32_t kAttr_LimitAngleUp    = 0x8D0CA293u;
constexpr uint32_t kAttr_LimitAngleDown  = 0x7C66969Au;
constexpr uint32_t kAttr_LookAt94        = 0x63F2322Bu;   // NOTE: name unknown
constexpr uint32_t kAttr_ToeOffset       = 0x81C9963Du;
constexpr uint32_t kAttr_FootA0          = 0x055F7F61u;   // NOTE: name unknown
constexpr uint32_t kAttr_FootA4          = 0xF2ADE153u;   // NOTE: name unknown
constexpr uint32_t kAttr_MinAnkle        = 0xDD33F4C5u;
constexpr uint32_t kAttr_MaxAnkle        = 0x471290F3u;
constexpr uint32_t kAttr_DownDist        = 0x83BD0148u;
constexpr uint32_t kAttr_UpDist          = 0x95934AC1u;

// Target types by "Type" (0x00AD4FD4).
constexpr uint32_t kTargetTypeNames[2] = {0x93116591u /* Target */, 0x044DDD31u /* layer */};

constexpr float kPi = 3.1415927f;
constexpr float kFltMax = 3.4028235e38f;

// Copies a joint/solver name into a 0x20 buffer (31 characters).
// NOTE: the console copies a missing attribute from a null pointer.
void CopyName(char* dst, const char* src)
{
    ape::StrNCopy(dst, src ? src : "", 0x1F);
}

// 16-byte aligned, uninitialised storage from the default memory class.
template <class T>
T* NewDef()
{
    return static_cast<T*>(ape::MemAlloc(sizeof(T), ape::kMemClass_Default, 0x10));
}

void Normalize(ape::Vec3& v)
{
    float scale = 1.0f / std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    v.x *= scale;
    v.y *= scale;
    v.z *= scale;
}

// Index of "Type" in kTargetTypeNames; reports unknown names and leaves
// `type` unchanged then.
void ReadTargetType(XmlElement* root, XmlElement* elem, uint32_t& type)
{
    uint32_t typeName = 0;
    if (elem->GetHash(kAttr_Type, &typeName, 0) != ape::xml::kQuery_Ok)
        return;
    uint32_t index = 0;
    while (index < 2 && kTargetTypeNames[index] != typeName)
        ++index;
    if (index < 2)
        type = index;
    else
        (void)root->GetDocumentPath();   // compiled-out error report
}

} // namespace

ape::ApeTSortedMap<LookAtSolverDef*>& LookAtSolverDefs()
{
    static ape::ApeTSortedMap<LookAtSolverDef*> s_defs;
    return s_defs;
}

ape::ApeTSortedMap<FootSolverDef*>& FootSolverDefs()
{
    static ape::ApeTSortedMap<FootSolverDef*> s_defs;
    return s_defs;
}

ape::ApeTSortedMap<JointTwoSolverDef*>& JointTwoSolverDefs()
{
    static ape::ApeTSortedMap<JointTwoSolverDef*> s_defs;
    return s_defs;
}

ape::ApeTSortedMap<JointThreeSolverDef*>& JointThreeSolverDefs()
{
    static ape::ApeTSortedMap<JointThreeSolverDef*> s_defs;
    return s_defs;
}

// @func 0x00291E6C ParseLookAtSolverDef
LookAtSolverDef* ParseLookAtSolverDef(XmlElement* root, XmlElement* elem)
{
    (void)root;
    const float ones[3] = {1.0f, 1.0f, 1.0f};
    const float zero[3] = {0.0f, 0.0f, 0.0f};
    const char* name = elem->GetString("name");
    const char* headJoint = elem->GetString("headJoint");
    uint32_t layer = 0;
    ape::Vec3 vec50 = {1.0f, 1.0f, 1.0f, 0.0f};
    ape::Vec3 vec60 = {1.0f, 1.0f, 1.0f, 0.0f};
    ape::Vec3 vec70 = {0.0f, 0.0f, 0.0f, 0.0f};
    float limits[5];
    float range[2] = {-kFltMax, kFltMax};
    elem->GetHash(kAttr_Layer, &layer, 0);
    elem->GetVec3(kAttr_LookAt50, &vec50.x, ones);
    elem->GetVec3(kAttr_LookAt60, &vec60.x, ones);
    elem->GetVec3(kAttr_LookAt70, &vec70.x, zero);
    elem->GetFloat(kAttr_LimitAngle, &limits[0], kPi);
    elem->GetFloat(kAttr_LimitAngleLeft, &limits[1], kPi);
    elem->GetFloat(kAttr_LimitAngleRight, &limits[2], kPi);
    elem->GetFloat(kAttr_LimitAngleUp, &limits[3], kPi);
    elem->GetFloat(kAttr_LimitAngleDown, &limits[4], kPi);
    elem->GetVec2(kAttr_LookAt94, range, range);

    LookAtSolverDef* def = NewDef<LookAtSolverDef>();
    if (!def)
        return nullptr;
    def->layer = layer;
    def->vec50 = vec50;
    def->vec60 = vec60;
    def->vec70 = vec70;
    def->limitAngle = limits[0];
    def->limitAngleLeft = limits[1];
    def->limitAngleRight = limits[2];
    def->limitAngleUp = limits[3];
    def->limitAngleDown = limits[4];
    def->range94[0] = range[0];
    def->range94[1] = range[1];
    CopyName(def->name, name);
    CopyName(def->headJoint, headJoint);
    LookAtSolverDefs().Add(ape::HashNameNoCase(def->name), def);
    return def;
}

// @func 0x00292210 ParseFootSolverDef
FootSolverDef* ParseFootSolverDef(XmlElement* root, XmlElement* elem)
{
    (void)root;
    FootSolverDef* def = NewDef<FootSolverDef>();
    if (!def)
        return nullptr;
    def->axis = {0.0f, 0.0f, 1.0f, 0.0f};
    def->toeOffset = {0.0f, 0.0f, 0.0f, 0.0f};
    def->valueA0 = 0.12f;
    def->valueA4 = 0.4f;
    def->minAnkle = -0.3f;
    def->maxAnkle = 0.15f;
    def->minAngle = -1.0f;
    def->maxAngle = 0.0f;
    def->downDist = 0.6f;
    def->upDist = 0.2f;
    CopyName(def->name, elem->GetString("name"));
    CopyName(def->hipJoint, elem->GetString("hipJoint"));
    CopyName(def->kneeJoint, elem->GetString("kneeJoint"));
    CopyName(def->ankleJoint, elem->GetString("ankleJoint"));
    elem->GetVec3(kAttr_ToeOffset, &def->toeOffset.x, &def->toeOffset.x);
    elem->GetFloat(kAttr_FootA0, &def->valueA0, def->valueA0);
    elem->GetFloat(kAttr_FootA4, &def->valueA4, def->valueA4);
    elem->GetFloat(kAttr_MinAnkle, &def->minAnkle, def->minAnkle);
    elem->GetFloat(kAttr_MaxAnkle, &def->maxAnkle, def->maxAnkle);
    elem->GetFloat(kAttr_MinAngle, &def->minAngle, def->minAngle);
    elem->GetFloat(kAttr_MaxAngle, &def->maxAngle, def->maxAngle);
    elem->GetFloat(kAttr_DownDist, &def->downDist, def->downDist);
    elem->GetFloat(kAttr_UpDist, &def->upDist, def->upDist);
    Normalize(def->axis);
    FootSolverDefs().Add(ape::HashNameNoCase(def->name), def);
    return def;
}

// @func 0x00292618 ParseJointTwoSolverDef
JointTwoSolverDef* ParseJointTwoSolverDef(XmlElement* root, XmlElement* elem)
{
    const float unitZ[3] = {0.0f, 0.0f, 1.0f};
    uint32_t targetType = 0;   // NOTE: uninitialised on the console
    uint32_t layer = 0;
    ape::Vec3 axis = {0.0f, 0.0f, 1.0f, 0.0f};
    float gain = 1.0f, minAngle = 1.0f, maxAngle = -1.0f;
    const char* name = elem->GetString("name");
    ReadTargetType(root, elem, targetType);
    elem->GetHash(kAttr_Layer, &layer, layer);
    const char* firstJoint = elem->GetString("firstJoint");
    const char* secondJoint = elem->GetString("secondJoint");
    const char* endJoint = elem->GetString("endJoint");
    const char* effectorJoint = elem->GetString("effectorJoint");
    elem->GetVec3(kAttr_Axis, &axis.x, unitZ);
    elem->GetFloat(kAttr_Gain, &gain, gain);
    elem->GetFloat(kAttr_MinAngle, &minAngle, minAngle);
    elem->GetFloat(kAttr_MaxAngle, &maxAngle, maxAngle);
    Normalize(axis);

    JointTwoSolverDef* def = NewDef<JointTwoSolverDef>();
    if (!def)
        return nullptr;
    def->targetType = targetType;
    def->layer = layer;
    def->axis = axis;
    def->gain = gain;
    def->minAngle = minAngle;
    def->maxAngle = maxAngle;
    CopyName(def->name, name);
    CopyName(def->firstJoint, firstJoint);
    CopyName(def->secondJoint, secondJoint);
    CopyName(def->endJoint, endJoint);
    CopyName(def->effectorJoint, effectorJoint);
    JointTwoSolverDefs().Add(ape::HashNameNoCase(def->name), def);
    return def;
}

// @func 0x002929EC ParseJointThreeSolverDef
JointThreeSolverDef* ParseJointThreeSolverDef(XmlElement* root, XmlElement* elem)
{
    const float unitZ[3] = {0.0f, 0.0f, 1.0f};
    uint32_t targetType = 0;
    ape::Vec3 axis = {0.0f, 0.0f, 1.0f, 0.0f};
    const char* name = elem->GetString("name");
    ReadTargetType(root, elem, targetType);
    const char* firstJoint = elem->GetString("firstJoint");
    const char* secondJoint = elem->GetString("secondJoint");
    const char* thirdJoint = elem->GetString("thirdJoint");
    const char* endJoint = elem->GetString("endJoint");
    elem->GetVec3(kAttr_Axis, &axis.x, unitZ);

    JointThreeSolverDef* def = NewDef<JointThreeSolverDef>();
    if (!def)
        return nullptr;
    def->targetType = targetType;
    def->layer = 0;
    def->axis = axis;
    CopyName(def->name, name);
    CopyName(def->firstJoint, firstJoint);
    CopyName(def->secondJoint, secondJoint);
    CopyName(def->thirdJoint, thirdJoint);
    CopyName(def->endJoint, endJoint);
    JointThreeSolverDefs().Add(ape::HashNameNoCase(def->name), def);
    return def;
}

// @func 0x00292F00 ParseIKSolverDefs
uint8_t ParseIKSolverDefs(XmlElement* root, XmlElement* elem, uint8_t result)
{
    for (XmlElement* e = elem->FirstChildElement(kTag_JointTwoSolver); e; e = e->NextSiblingElement(kTag_JointTwoSolver))
        ParseJointTwoSolverDef(root, e);
    for (XmlElement* e = elem->FirstChildElement(kTag_JointThreeSolver); e; e = e->NextSiblingElement(kTag_JointThreeSolver))
        ParseJointThreeSolverDef(root, e);
    for (XmlElement* e = elem->FirstChildElement(kTag_LookAtSolver); e; e = e->NextSiblingElement(kTag_LookAtSolver))
        ParseLookAtSolverDef(root, e);
    for (XmlElement* e = elem->FirstChildElement(kTag_FootSolver); e; e = e->NextSiblingElement(kTag_FootSolver))
        ParseFootSolverDef(root, e);
    return result;
}

} // namespace components
} // namespace sly
