// openTIT - Mega knight boss: behaviour and stamp data loaded from XML
// (0x00302DA8-0x00303CEC).
//
// The mega knight definition (vtable 0x00B1DA78, 0x170 bytes) keeps named
// behaviours (lists of actions: camera, ground smash, laser, pause, ...) and
// named stamp sets (lists of stamps: where and when the knight's feet come
// down). The boss entity and the code that runs these lists are not ported.
#pragma once

#include "ape/memory/ApeTArray.h"
#include "sly/entity/Entity.h"

#include <cstdint>

namespace ape { namespace xml { class XmlElement; } }

namespace sly {
namespace gameplay {

using ape::xml::XmlElement;

// One stamp (0x14 bytes, 0x11 used).
struct MegaKnightStamp {
    enum Type : int32_t { kOffset = 1, kAngle = 2, kPause = 3 };
    int32_t type = 0;      // +0x00
    float a = 0.0f;        // +0x04 offset X / angle A
    float b = 0.0f;        // +0x08 offset Y / angle R
    float delay = 0.0f;    // +0x0C "Delay"
    bool flag = false;     // +0x10 bool attribute 0x042B7A7B (name unknown)

    // Reads a stamp element ("offset", "angle" or "Pause") (0x00302DA8).
    void Parse(XmlElement* element);
};

// Base of the behaviour actions (vtable 0x00B1D9B8; the actions 0x00B1D9D0..
// 0x00B1DA60).
class MegaKnightAction {
public:
    // @func 0x007686B0 MegaKnightAction::~MegaKnightAction
    // @func 0x007686C0 MegaKnightAction::~MegaKnightAction (deleting)
    virtual ~MegaKnightAction() = default;
    // slot 2
    // @func 0x00768704 MegaKnightAction::Parse
    virtual void Parse(XmlElement* element) { (void)element; }
    // slot 3: kind of action (-1 for the base).
    // @func 0x00768708 MegaKnightAction::Kind
    virtual int Kind() const { return -1; }
};

// "GroundSmash" (0x24 bytes): the sequence to play.
class MegaKnightGroundSmashAction : public MegaKnightAction {
public:
    ~MegaKnightGroundSmashAction() override;
    int Kind() const override;
    void Parse(XmlElement* element) override;   // 0x00302F14
    char sequence[0x20] = {};                   // +0x04
};

// "Laser" (8 bytes).
class MegaKnightLaserAction : public MegaKnightAction {
public:
    ~MegaKnightLaserAction() override;
    int Kind() const override;
    void Parse(XmlElement* element) override;   // 0x00302F6C
    float time = 0.0f;                          // +0x04
};

// Shared tail of the two movement actions: six tuning floats (+0x30..+0x44).
struct MegaKnightMoveParams {
    float field_30 = 0.0f;     // hash 0x94A773B8
    float field_34 = 0.0f;     // hash 0x7B738C62
    float field_38 = 0.0f;     // hash 0xC687E664
    float field_3C = 0.0f;     // hash 0xC3C5F05B
    float field_40 = 0.0f;     // hash 0x85AEB11C
    float field_44 = 0.0f;     // hash 0x8BB08F02 (default: start-up global 0x01069EF8)
    void Parse(XmlElement* element);
};

// Action 0x6A8A0D06 (0x50 bytes). NOTE: name unknown; the console reads an
// int (0xF95DFD73) and drops it.
class MegaKnightMoveAction : public MegaKnightAction {
public:
    ~MegaKnightMoveAction() override;
    int Kind() const override;
    void Parse(XmlElement* element) override;   // 0x00302FB0
    uint32_t field_04 = 0;
    ape::Vec3 positionA{};                      // +0x10 (zero vector)
    ape::Vec3 positionB{};                      // +0x20
    MegaKnightMoveParams params;                // +0x30
};

// Action 0xD9ABF25D (0x70 bytes): a move ending with a stamp. NOTE: name unknown.
class MegaKnightStampMoveAction : public MegaKnightAction {
public:
    ~MegaKnightStampMoveAction() override;
    int Kind() const override;
    void Parse(XmlElement* element) override;   // 0x003030E8
    uint32_t target = 0;                        // +0x04 hash attribute 0x1E78B4AE
    float field_08 = 0.0f;                      // +0x08 0x1D424785
    float field_0C = 0.0f;                      // +0x0C 0x81D650F9
    ape::Vec3 positionA{};                      // +0x10
    ape::Vec3 positionB{};                      // +0x20
    MegaKnightMoveParams params;                // +0x30
    MegaKnightStamp stamp;                      // +0x50 from the first child
    bool field_64 = false;                      // +0x64 bool 0x78833C19
};

// Action 0x651BDBA8 (8 bytes). NOTE: reads "count" but stores 0.
class MegaKnightCountAction : public MegaKnightAction {
public:
    ~MegaKnightCountAction() override;
    int Kind() const override;
    void Parse(XmlElement* element) override;   // 0x00303298
    int32_t count = 0;
};

// "Pause" (8 bytes).
class MegaKnightPauseAction : public MegaKnightAction {
public:
    ~MegaKnightPauseAction() override;
    int Kind() const override;
    void Parse(XmlElement* element) override;   // 0x003032EC
    float time = 0.0f;
};

// "Camera" (0xC bytes): camera by name with an ease-in time.
class MegaKnightCameraAction : public MegaKnightAction {
public:
    ~MegaKnightCameraAction() override;
    int Kind() const override;
    void Parse(XmlElement* element) override;   // 0x00303328
    uint32_t name = 0;
    float easeIn = 0.0f;
};

// A named behaviour (0x38 bytes).
struct MegaKnightBehavior {
    char name[0x20] = {};                           // +0x00
    uint32_t nameHash = 0;                          // +0x20
    ape::ApeTArray<MegaKnightAction*> actions;      // +0x24
    void Parse(XmlElement* element);                // 0x0030339C
};

// A named stamp set (0x38 bytes).
struct MegaKnightStampSet {
    char name[0x20] = {};
    uint32_t nameHash = 0;
    ape::ApeTArray<MegaKnightStamp> stamps;         // +0x24
    void Parse(XmlElement* element);                // 0x0030386C
};

// The mega knight's definition (only what the destructor touches).
class MegaKnightDef : public entity::EntityDef {
public:
    ~MegaKnightDef() override;                      // 0x00303A4C / 0x00303CEC
    ape::ApeTArray<char*> strings;                  // +0x130 NOTE: role unknown
    ape::ApeTArray<MegaKnightBehavior*> behaviors;  // +0x144
    ape::ApeTArray<MegaKnightStampSet*> stampSets;  // +0x158
};

} // namespace gameplay
} // namespace sly
