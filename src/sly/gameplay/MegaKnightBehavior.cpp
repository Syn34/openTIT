// openTIT - Mega knight boss behaviour and stamp data (0x00302DA8-0x00303CEC).
#include "sly/gameplay/MegaKnightBehavior.h"

#include "ape/core/ApeString.h"
#include "ape/memory/ApeMem.h"
#include "ape/xml/ApeXml.h"
#include "platform/Platform.h"

#include <cstring>

namespace sly {
namespace gameplay {

namespace {

constexpr uint32_t kHash_Name     = 0x0024728Bu;   // "Name"
constexpr uint32_t kHash_Sequence = 0x7F1667C1u;   // "Sequence"
constexpr uint32_t kHash_Time     = 0x00274ACDu;   // "Time"
constexpr uint32_t kHash_Delay    = 0x03DEC163u;   // "Delay"
constexpr uint32_t kHash_Count    = 0x03D558EFu;   // "count"
constexpr uint32_t kHash_EaseIn   = 0x79792193u;   // "EaseIn"
constexpr uint32_t kHash_A = 0x41u, kHash_R = 0x52u, kHash_X = 0x58u, kHash_Y = 0x59u;

// Stamp element names.
constexpr uint32_t kStamp_Pause  = 0x04862DD6u;    // "Pause"
constexpr uint32_t kStamp_Angle  = 0x03B880B3u;    // "angle"
constexpr uint32_t kStamp_Offset = 0x8ACA5893u;    // "offset"

// Action element names.
constexpr uint32_t kAction_Camera      = 0x760CB725u;   // "Camera"
constexpr uint32_t kAction_Move        = 0x6A8A0D06u;
constexpr uint32_t kAction_Count       = 0x651BDBA8u;
constexpr uint32_t kAction_GroundSmash = 0x60BB2C95u;   // "GroundSmash"
constexpr uint32_t kAction_Pause       = 0x04862DD6u;   // "Pause"
constexpr uint32_t kAction_Laser       = 0x044DC6ABu;   // "Laser"
constexpr uint32_t kAction_StampMove   = 0xD9ABF25Du;

// NOTE: start-up globals whose values are not known (0x01069B38 laser time,
// 0x01069EF8 last movement parameter); assumed 0.
constexpr float kDefaultLaserTime = 0.0f;
constexpr float kDefaultMoveParam = 0.0f;

// The console copies names unbounded into 32-byte buffers.
void CopyName(char (&dst)[0x20], const char* src)
{
    std::strncpy(dst, src, sizeof dst - 1);
    dst[sizeof dst - 1] = '\0';
}

uint32_t ElementHash(XmlElement* element)
{
    return element->IsValid() ? element->GetNameHash() : 0;
}

} // namespace

// @func 0x00302DA8 MegaKnightStamp::Parse
void MegaKnightStamp::Parse(XmlElement* element)
{
    const uint32_t kind = ElementHash(element);
    if (kind == kStamp_Pause) {
        type = kPause;
    } else if (kind == kStamp_Angle) {
        element->GetFloat(kHash_A, &a, a);
        element->GetFloat(kHash_R, &b, b);
        type = kAngle;
    } else if (kind == kStamp_Offset) {
        element->GetFloat(kHash_X, &a, a);
        element->GetFloat(kHash_Y, &b, b);
        type = kOffset;
    } else {
        plat::Log(plat::LogLevel::Warning, "unknown megaknight stamp type '%u'\n", kind);
    }
    element->GetFloat(kHash_Delay, &delay, delay);
    element->GetBool(0x042B7A7Bu, &flag, flag);
}

// @func 0x00302F14 MegaKnightGroundSmashAction::Parse
void MegaKnightGroundSmashAction::Parse(XmlElement* element)
{
    if (const char* name = element->GetString(kHash_Sequence))
        CopyName(sequence, name);
}

// @func 0x00302F6C MegaKnightLaserAction::Parse
void MegaKnightLaserAction::Parse(XmlElement* element)
{
    element->GetFloat(kHash_Time, &time, kDefaultLaserTime);
}

void MegaKnightMoveParams::Parse(XmlElement* element)
{
    element->GetFloat(0x94A773B8u, &field_30, field_30);
    element->GetFloat(0x7B738C62u, &field_34, field_34);
    element->GetFloat(0xC687E664u, &field_38, field_38);
    element->GetFloat(0xC3C5F05Bu, &field_3C, field_3C);
    element->GetFloat(0x85AEB11Cu, &field_40, field_40);
    element->GetFloat(0x8BB08F02u, &field_44, field_44);
}

// @func 0x00302FB0 MegaKnightMoveAction::Parse
void MegaKnightMoveAction::Parse(XmlElement* element)
{
    int32_t ignored = 0;
    element->GetInt(0xF95DFD73u, &ignored, 0);
    field_04 = 0;
    params.Parse(element);
}

// @func 0x003030E8 MegaKnightStampMoveAction::Parse
void MegaKnightStampMoveAction::Parse(XmlElement* element)
{
    element->GetHash(0x1E78B4AEu, &target, target);
    element->GetFloat(0x1D424785u, &field_08, field_08);
    element->GetFloat(0x81D650F9u, &field_0C, field_0C);
    element->GetBool(0x78833C19u, &field_64, field_64);
    if (XmlElement* child = element->FirstChildElement())
        stamp.Parse(child);
    params.Parse(element);
}

// @func 0x00303298 MegaKnightCountAction::Parse
void MegaKnightCountAction::Parse(XmlElement* element)
{
    int32_t ignored = 0;
    element->GetInt(kHash_Count, &ignored, 0);
    count = 0;
}

// @func 0x003032EC MegaKnightPauseAction::Parse
void MegaKnightPauseAction::Parse(XmlElement* element)
{
    element->GetFloat(kHash_Time, &time, time);
}

// @func 0x00303328 MegaKnightCameraAction::Parse
void MegaKnightCameraAction::Parse(XmlElement* element)
{
    element->GetHash(kHash_Name, &name, name);
    element->GetFloat(kHash_EaseIn, &easeIn, easeIn);
}

// Reads the name and one action per child element.
// @func 0x0030339C MegaKnightBehavior::Parse
void MegaKnightBehavior::Parse(XmlElement* element)
{
    if (const char* text = element->GetString(kHash_Name))
        CopyName(name, text);
    element->GetHash(kHash_Name, &nameHash, nameHash);
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (!child->IsValid())
            continue;
        MegaKnightAction* action = nullptr;
        switch (child->GetNameHash()) {
        case kAction_Camera: action = new MegaKnightCameraAction(); break;
        case kAction_Move: {
            auto* move = new MegaKnightMoveAction();
            move->params.field_44 = kDefaultMoveParam;
            action = move;
            break;
        }
        case kAction_Count: action = new MegaKnightCountAction(); break;
        case kAction_GroundSmash: action = new MegaKnightGroundSmashAction(); break;
        case kAction_Pause: action = new MegaKnightPauseAction(); break;
        case kAction_Laser: action = new MegaKnightLaserAction(); break;
        case kAction_StampMove: {
            auto* move = new MegaKnightStampMoveAction();
            move->params.field_44 = kDefaultMoveParam;
            action = move;
            break;
        }
        default:
            plat::Log(plat::LogLevel::Warning, "unknown megaknight behavior action '%u'\n",
                      child->GetNameHash());
            continue;
        }
        action->Parse(child);
        actions.PushBack(action);
    }
}

// Reads the name and one stamp per "offset" / "angle" / "Pause" child.
// @func 0x0030386C MegaKnightStampSet::Parse
void MegaKnightStampSet::Parse(XmlElement* element)
{
    const char* text = element->GetString(kHash_Name);
    CopyName(name, text ? text : "NoName");
    element->GetHash(kHash_Name, &nameHash, nameHash);
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (!child->IsValid())
            continue;
        const uint32_t kind = child->GetNameHash();
        if (kind != kStamp_Angle && kind != kStamp_Pause && kind != kStamp_Offset) {
            plat::Log(plat::LogLevel::Warning, "Unknown Stamp Command in stamp: '%s'\n", name);
            continue;
        }
        MegaKnightStamp stamp;
        stamp.Parse(child);
        stamps.PushBack(stamp);
    }
}

// @func 0x00303A4C MegaKnightDef::~MegaKnightDef
// @func 0x00303CEC MegaKnightDef::~MegaKnightDef (deleting)
MegaKnightDef::~MegaKnightDef()
{
    for (MegaKnightCameraDef*& camera : cameras) {
        ape::MemFree(camera);
        camera = nullptr;
    }
    cameras.Clear();
    for (MegaKnightBehavior*& behavior : behaviors) {
        if (!behavior)
            continue;
        for (MegaKnightAction*& action : behavior->actions) {
            delete action;
            action = nullptr;
        }
        delete behavior;
        behavior = nullptr;
    }
    behaviors.Clear();
    for (MegaKnightStampSet*& set : stampSets) {
        delete set;
        set = nullptr;
    }
    stampSets.Clear();
}

} // namespace gameplay
} // namespace sly
