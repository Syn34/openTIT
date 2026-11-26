// openTIT - Security lasers (0x00398E64-0x003A233C).
//
// The laser manager (0x00F657FC, created with the game world) owns three
// fixed pools: laser groups (32), lasers (256) and laser fields (16). They
// are created from the level's tag settings: a "Sly_LaserGroup" setting
// names a point instance; every "Sly_LaserFixed/Anchor/Spline" setting names
// a point whose parent is the group's point; "Sly_LaserField" names a point
// whose first child spline outlines the field.
//
// A laser is a beam between two end points (each optionally running a
// direction script and moving along a spline). Groups switch their lasers
// on and off together (optionally while an activation trigger is occupied)
// and can block navigation with an obstacle around their bounds. Fields are
// flat polygons that hurt whoever stands in them.
#pragma once

#include "ape/container/ApeTFixedArray.h"
#include "ape/container/ApeTSortedMap.h"
#include "ape/math/ApeBounds.h"
#include "ape/math/ApeMath.h"
#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace ape { class ApeAnimScriptInstance; class ApeAnimScriptObjectVar; class ApeRefCounted; }
namespace ape::setting { class Setting; }
namespace ape::world { class ApeObj; class ApeSplineInstance; }
namespace sly::audio { struct SoundName; }
namespace sly::entity { class Entity; }

namespace sly {

struct Laser;
struct LaserGroup;

// Kinds of laser settings (their definition's type hash).
enum LaserSettingType : uint32_t {
    kLaserSetting_Group  = 0xA2A8CB13u,   // "Sly_LaserGroup"
    kLaserSetting_Fixed  = 0xC6CD059Du,   // NOTE: by elimination "Sly_LaserFixed"
    kLaserSetting_Anchor = 0xA7F58081u,   // NOTE: by elimination "Sly_LaserAnchor"
    kLaserSetting_Spline = 0xA296BC08u,   // NOTE: by elimination "Sly_LaserSpline"
    kLaserSetting_Field  = 0xA296758Eu,   // "Sly_LaserField"
};

// Tags a point instance carries (ApeObj +0x114) while a laser or field uses
// it; the object (+0x118) points back at the user.
constexpr uint32_t kLaserObjectTag = 0x044DC6ABu;        // "Laser"
constexpr uint32_t kLaserFieldObjectTag = 0xAB27550Fu;

// One end of a beam (0x60 bytes).
struct LaserEndpoint {
    enum Flags : uint32_t {
        kMoveTypeShift = 30,              // bits 30-31: spline movement type (0..2)
        kReverse       = 0x20000000u,     // moving backwards along the spline
        kMoveable      = 0x10000000u,     // has a moveable object
    };
    ape::Matrix34 transform{};            // +0x00 the point's frame (translation = position)
    ape::world::ApeSplineInstance* spline = nullptr;   // +0x40 spline the point follows
    int32_t splineSegment = 0;            // +0x44 last evaluated segment
    uint32_t field_48 = 0;                // +0x48
    ape::ApeAnimScriptInstance* script = nullptr;      // +0x4C direction script
    ape::ApeAnimScriptObjectVar* scriptObject = nullptr;   // +0x50 its object variable 0x0027469E
    float speed = 1.0f;                   // +0x54 movement speed along the spline
    float splinePos = 0.0f;               // +0x58 0..1
    uint32_t flags = 0;                   // +0x5C Flags
};

// A beam (0x150 bytes).
struct Laser {
    enum InitType : uint32_t { kInit_None = 0, kInit_Flicker = 1 };
    enum Flags : uint32_t {
        kFlag_80000000   = 0x80000000u,   // NOTE: set when a flickering laser is switched off
        kFlag_Triggered  = 0x40000000u,   // switched by its group's trigger
        kFlag_ShowPoints = 0x20000000u,   // the end point objects stay visible ("AlwaysShowEndPoints")
        kFlag_Visible    = 0x10000000u,   // beam in view (the group's sphere is inside the frustum)
        kFlag_Enabled    = 0x08000000u,
    };

    // 0x0039BFC4: identity frames, unit scale, no objects.
    Laser();

    // 0x003992AC: switches the beam (and its end point objects and sound);
    // `force` applies the state even when unchanged, `fromTrigger` marks a
    // switch made by the group's trigger.
    void SetEnabled(bool enabled, bool force, bool fromTrigger);
    // 0x0039BEDC: both end points back to the start of their splines.
    void Reset();
    // 0x0039A110: the beam geometry: a placed instance of that name (cloned),
    // else an instance of the geometry definition; the default laser geometry
    // without a name.
    void SetGeometry(const char* geomName);
    // 0x00399DA8: (re)starts the direction script of each end point (the
    // default point script without a name) with the point's frame on the
    // script's object.
    void StartPointScripts(const char* const scriptNames[2]);
    // 0x003998BC: puts the beam geometry at the beam frame and scale.
    void ApplyBeam();
    // The beam frame from the end points (the part of 0x0039C8F0 run per
    // laser).
    void UpdateBeam();

    LaserEndpoint points[2];              // +0x00
    ape::Matrix34 beam{};                 // +0xC0 frame of the beam geometry
    ape::Vec3 beamScale{1.0f, 1.0f, 1.0f, 0.0f};   // +0x100
    ape::setting::Setting* setting = nullptr;      // +0x110
    uint32_t initType = kInit_None;       // +0x114 InitType ("InitType")
    ape::world::ApeObj* point = nullptr;  // +0x118 the setting's point instance
    LaserGroup* group = nullptr;          // +0x11C
    ape::world::ApeObj* geometry = nullptr;   // +0x120 beam geometry instance
    float geometryLength = 0.0f;          // +0x124 length of the geometry (definition +0x24)
    uint32_t damageType = 0;              // +0x128 0 fire, 1 electric ("LaserDamageType")
    float damageAmount = 10.0f;           // +0x12C "LaserDamageAmount"
    audio::SoundName* sound = nullptr;    // +0x130 "LaserSound"
    uint32_t voice = 0;                   // +0x134 the playing loop
    void* navObstacle = nullptr;          // +0x138
    float flickerDelay = 0.0f;            // +0x13C time before a switch takes effect
    float flickerTime = 0.0f;             // +0x140 how long the flicker lasts
    float flickerToggle = 0.0f;           // +0x144 time to the next toggle
    uint32_t flags = 0;                   // +0x148 Flags
};

// Lasers switched together (0x70 bytes).
struct LaserGroup {
    enum Flags : uint32_t {
        kFlag_InView  = 0x80000000u,      // the bounds are inside the main view's frustum
        kFlag_Enabled = 0x40000000u,
    };

    // 0x00398ED0: empty bounds, enabled, room for 256 lasers.
    LaserGroup();

    // 0x00399758: switches every laser of the group (see Laser::SetEnabled)
    // and adds or removes the navigation obstacle.
    void SetEnabled(bool enabled, bool force, bool fromTrigger);
    // 0x00398FF8: blocks navigation around the bounds (when the setting asks
    // for an obstacle and the bounds are valid).
    void AddNavObstacle();
    // 0x003991A0: pads degenerate bounds, derives the sphere and adds the
    // navigation obstacle.
    void FinishBounds();
    // 0x0039BC48: sets every laser up from its setting, finds the activation
    // trigger and applies the start state.
    void Start();
    // 0x0039BEFC: every laser back to its start (see Laser::Reset).
    void ResetLasers();
    // Laser by name, or null.
    Laser* FindLaser(uint32_t nameHash);

    void* navObstacle = nullptr;          // +0x00
    ape::Aabb bounds{};                   // +0x10 world bounds of the lasers' splines
    ape::Sphere sphere{};                 // +0x30
    ape::setting::Setting* setting = nullptr;   // +0x40 null for groups made by lasers alone
    entity::Entity* trigger = nullptr;    // +0x44 "ActivationTrigger"
    uint32_t flags = kFlag_InView;        // +0x48 Flags
    ape::ApeTSortedMap<int16_t> lasers{0x100, nullptr, int16_t(-1)};   // +0x4C name -> laser index
};

// A damaging floor area (0x170 bytes).
struct LaserField {
    enum Flags : uint32_t {
        kFlag_Shown      = 0x80000000u,
        kFlag_Enabled    = 0x40000000u,
        kFlag_Occupied   = 0x20000000u,   // the activation trigger had someone in it
        kFlag_Started    = 0x10000000u,
        kFlag_Flickering = 0x08000000u,
    };
    // Material set of the field surface (0x4C bytes).
    struct MaterialSet {
        ape::ApeRefCounted* materials[16] = {};   // +0x00
        float uDist = 0.0f;               // +0x40
        float vDist = 0.0f;               // +0x44
        uint32_t count = 0;               // +0x48
    };

    // 0x0039CB9C
    LaserField();

    // 0x0039CD68: releases the objects, joints, scripts and materials.
    void Release();
    // 0x0039CF9C: shows or hides the outline objects.
    void SetShown(bool shown);
    // 0x0039D054: starts the field (flickering fields begin their flicker).
    void Start();
    // 0x0039D1D4: switches the phantoms, the lifetime script state and the
    // outline objects.
    void SetActive(bool active);
    // 0x0039D32C: enables or disables the field; `applyNow` also applies
    // the state when the field waits for its activation trigger.
    void SetEnabled(bool enabled, bool force, bool applyNow);
    // 0x0039FEE0: triangulates the field outline.
    void SetOutline(const ape::ApeTArray<ape::Vec3>* polygon);
    // 0x0039FF34: a state of the lifetime script and of every joint's script.
    void SetScriptState(uint32_t state);
    // 0x0039FEBC: the name of the field's point instance.
    uint32_t Name() const;

    ape::ApeTArray<ape::world::ApeObj*> outlines{0x10};   // +0x00 outline geometry
    ape::ApeTArray<entity::Entity*> joints{0x10};         // +0x14 joint entities at the corners
    ape::ApeTArray<ape::Vec3> polygon{0x10};              // +0x28 outline points
    ape::ApeTArray<ape::Vec3> triangles{0x10};            // +0x3C triangulated outline
    MaterialSet materialSets[3];          // +0x50
    uint8_t color[4] = {};                // +0x134 vertex colour
    ape::setting::Setting* setting = nullptr;   // +0x138
    uint32_t initType = 0;                // +0x13C 1: flicker
    ape::world::ApeObj* point = nullptr;  // +0x140
    ape::world::ApeObj* outlineSpline = nullptr;   // +0x144 the point's first child spline
    void* damagePhantom = nullptr;        // +0x148 "LaserField_DamagePhantom"
    void* fxPhantom = nullptr;            // +0x14C "LaserField_FxPhantom"
    entity::Entity* trigger = nullptr;    // +0x150 activation trigger
    ape::ApeAnimScriptInstance* lifetimeScript = nullptr;   // +0x154
    float flickerDelay = 0.0f;            // +0x158
    float flickerTime = 0.0f;             // +0x15C
    float field_160 = 0.0f;               // +0x160
    uint32_t flags = kFlag_Shown;         // +0x164 Flags
};

class LaserManager {
public:
    static constexpr uint16_t kMaxGroups = 0x20;
    static constexpr uint16_t kMaxLasers = 0x100;
    static constexpr uint16_t kMaxFields = 0x10;

    // 0x003A0010 / 0x003A01E4. The manager registers itself as the level's
    // laser manager.
    LaserManager();
    ~LaserManager();

    // 0x003A0380: starts every group and field.
    void StartLevel();
    // 0x003A037C: nothing to do when the level start is finished.
    void LevelStarted() {}
    // 0x003A0470: groups follow their triggers and the view, fields their
    // triggers; the lasers update and the visible beams are placed.
    void Update(float dt);
    // 0x0039C8F0: the beam frames of the lasers updated this frame.
    void UpdateBeams();
    // 0x003A0924: places the beam geometry of the lasers updated this frame.
    void ApplyBeams();
    // 0x003A13C8: the material vertex colours of every shown field.
    void UpdateFieldMaterials();
    // 0x003A1500: frees the level's lasers, fields and groups.
    void Unload();

    // 0x003A187C: the group of that name, created when missing (null when
    // the pool is full). `point` names the group in the warning.
    LaserGroup* FindOrAddGroup(uint32_t nameHash, ape::world::ApeObj* point);
    // 0x003A1BFC: a new laser of `group` for `point`.
    void AddLaser(LaserGroup* group, ape::world::ApeObj* point, ape::setting::Setting* setting);
    // 0x003A1DC4: a laser for `point` (its parent names the group); a
    // laser already known by that name is not added again.
    void AddLaserForPoint(ape::world::ApeObj* point, ape::setting::Setting* setting);
    // 0x003A20F8: a field for `point`.
    void AddField(ape::world::ApeObj* point, ape::setting::Setting* setting);

    LaserGroup* FindGroup(uint32_t nameHash);
    Laser* FindLaser(uint32_t nameHash);
    LaserField* FindField(uint32_t nameHash);
    Laser* LaserAt(int16_t index) { return index < 0 ? nullptr : &lasers[size_t(index)]; }

    ape::ApeTFixedArray<LaserGroup> groups;       // +0x00
    ape::ApeTFixedArray<Laser> lasers;            // +0x10
    ape::ApeTFixedArray<LaserField> fields;       // +0x20
    ape::ApeTArray<Laser*> updated{0x100};        // +0x30 lasers that moved this frame
    ape::ApeTSortedMap<int16_t> groupNames{kMaxGroups, nullptr, int16_t(-1)};   // +0x44 name -> group index
    ape::ApeTArray<void*> field_5C{8};            // +0x5C NOTE: element type unknown
};

// The level's laser manager (0x00F657FC), null without one.
LaserManager* GetLaserManager();

// 0x003A1808: the level's lasers go away (game shell, level unload).
void UnloadLasers();
// 0x00399AB0: frame and scale of a beam geometry (see Lasers.cpp).
void ComputeLaserBeamFrame(float length, float geometryLength, ape::Matrix34& frame, ape::Vec3& scale,
                           const ape::Vec3& start, const ape::Vec3& dir);

// Association callbacks of the laser settings (queued until the level's
// point instances exist).
void AssociateLaserGroup(ape::setting::Setting* setting, const char* objectName, bool reload);
void AssociateLaser(ape::setting::Setting* setting, const char* objectName, bool reload);
void AssociateLaserField(ape::setting::Setting* setting, const char* objectName, bool reload);

// Hooks into modules that are not ported yet.
// Phantom of a laser field (+0x148 / +0x14C): its slot 76 switches it.
void LaserPhantomSetEnabled(void* phantom, bool enabled);
// 0x0039C1C8 (TODO): moves a laser's end points and checks its beam for
// hits; true when the beam must be placed again this frame.
bool LaserUpdate(Laser& laser, float dt);
// 0x0039FBD8 (TODO): the per-frame part of a field (flicker, damage, FX).
void LaserFieldUpdate(LaserField& field, float dt);
// 0x0039F840 (TODO): vertex colours of a field material set over the
// outline (or its triangles).
void LaserFieldColorMaterials(const ape::ApeTArray<ape::Vec3>& points, const uint8_t color[4],
                              LaserField::MaterialSet& set);
// Set-up of a laser from its setting by kind (0x0039A2B8 fixed, 0x0039AD48
// anchor, 0x0039B5A0 spline; TODO).
void LaserSetupFixed(Laser& laser, ape::setting::Setting* setting, ape::world::ApeObj* point);
void LaserSetupAnchor(Laser& laser, ape::setting::Setting* setting, ape::world::ApeObj* point);
void LaserSetupSpline(Laser& laser, ape::setting::Setting* setting, ape::world::ApeObj* point);
// 0x0039F3D8 (TODO): builds a field from its setting (outline, joints,
// materials, phantoms, scripts).
void LaserFieldSetup(LaserField& field);

} // namespace sly
