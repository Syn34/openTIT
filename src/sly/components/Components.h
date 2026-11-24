// openTIT - Entity components built from component descriptions.
//
// Every component derives from entity::EntityComponent (console vtable
// 0x00B1BE30). The base part copies the description's name, owner and type
// and enters the component in the live component registry (0x01056F1C,
// name hash -> component); the destructor takes it out again and drops it
// from the entity update list.
//
// The component factory (0x002635E8, a switch over the description type)
// is not ported yet; the classes below are the simple component kinds.
#pragma once

#include "sly/components/ComponentDescs.h"
#include "sly/entity/Entity.h"

#include "ape/container/ApeTSortedMap.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace sly {
namespace components {

using entity::EntityComponent;

// Live components by name hash, in creation order (0x01056F1C, capacity 0x100).
std::vector<std::pair<uint32_t, EntityComponent*>>& LiveComponents();
// Anim maps by name (0x0105AE30, capacity 0x20), filled by the anim map
// loader. NOTE: the value type is the anim system's map object.
ape::ApeTSortedMap<void*>& AnimMaps();

// Common base of the control components (vtable 0x00B1C118): the input
// state the controller fills in each frame.
class ControlComponent : public EntityComponent {
public:
    explicit ControlComponent(const ComponentDesc* desc) : EntityComponent(desc) {}
    // NOTE: the base destructor is at 0x00763174 (outside this range).
    ~ControlComponent() override = default;

    uint8_t inputFlags[4] = {};      // 0x18
    uint8_t input[0x30] = {};        // 0x20 controller input record. NOTE: layout unknown
};

// <ControlAI> (type 6, vtable 0x00B1C1B8, 0x60 bytes): input produced by the
// owner's AI brain.
class ControlAIComponent : public ControlComponent {
public:
    explicit ControlAIComponent(const ControlAIComponentDesc* desc);
    ~ControlAIComponent() override;
    void OnOwnerSpawned(entity::Entity* owner) override;
    void Update(float dt) override;

    uint32_t behavior = 0;           // 0x50 from the description
    void* brain = nullptr;           // 0x54 the owner's AI object (entity +0xF4)
};

// Type 0x23 control component (vtable 0x00B1C518, 0xC0 bytes). NOTE: read
// as the "ControlExt" controller (an externally driven character: the
// element name table points "ControlExt" at no real parser and the
// EntityComp.ControlExt.* tuning belongs to the same module); its input
// block is three 16-byte vectors.
class ControlExtComponent : public ControlComponent {
public:
    explicit ControlExtComponent(const ComponentDesc* desc);
    ~ControlExtComponent() override;
    void OnOwnerSpawned(entity::Entity* owner) override;
    // TODO(0x00283888 ControlExtComponent::Update): steers towards the
    // target point (vector code not reimplemented yet).
    void Update(float dt) override;

    uint32_t field_50 = 0;           // 0x50
    uint32_t field_54 = 0;           // 0x54
    ape::Vec3 vec60 = {0.0f, 0.0f, 0.0f, 0.0f};   // 0x60
    uint32_t field_70[3] = {};       // 0x70
    float field_7C = 1.0f;           // 0x7C
    ape::Vec3 vec80 = {0.0f, 0.0f, 0.0f, 0.0f};   // 0x80
    float field_90 = 10.0f;          // 0x90
    float field_94 = 5.0f;           // 0x94
    float field_98 = 1.0f;           // 0x98
    float field_9C = 1.0f;           // 0x9C
    float field_A0 = 15.0f;          // 0xA0
    float field_A4 = 0.0f;           // 0xA4
    float field_A8 = 10.0f;          // 0xA8
    float field_AC = 200.0f;         // 0xAC
    float field_B0 = 10.0f;          // 0xB0
    uint32_t field_B4 = 0;           // 0xB4
    uint32_t field_B8 = 0;           // 0xB8
    bool flagBC = false;             // 0xBC bit 31
};

// <AnimMap> (type 0x0E, vtable 0x00B1C628, 0x28 bytes).
class AnimMapComponent : public EntityComponent {
public:
    explicit AnimMapComponent(const AnimMapComponentDesc* desc);
    ~AnimMapComponent() override;

    void* map = nullptr;             // 0x18 anim map registered under the description's map name
    uint32_t animSet = 0;            // 0x1C
    void* tree = nullptr;            // 0x20 anim tree (0x003482B0)
    void* blendMap = nullptr;        // 0x24 blend map (0x00348650)
};

struct JointTwoSolverDef;
struct JointThreeSolverDef;
struct FootSolverDef;
struct LookAtSolverDef;

// <IK> (type 0x11, vtable 0x00B1C7E0, 0x68 bytes): the solvers named by
// the description, resolved in the IK solver registries.
class IKComponent : public EntityComponent {
public:
    explicit IKComponent(const IKComponentDesc* desc);
    ~IKComponent() override;
    void Update(float dt) override;

    JointTwoSolverDef* jointTwo[8] = {};       // 0x18
    JointThreeSolverDef* jointThree[8] = {};   // 0x38
    FootSolverDef* foot[2] = {};               // 0x58
    LookAtSolverDef* lookAt = nullptr;         // 0x60
    uint32_t field64 = 0;                      // 0x64
};

// <RadarBlip> (type 0x19, vtable 0x00B1C930, 0x1C bytes).
class RadarBlipComponent : public EntityComponent {
public:
    explicit RadarBlipComponent(const RadarBlipComponentDesc* desc);
    ~RadarBlipComponent() override;
    void Update(float dt) override;

    uint32_t blipType = 0;           // 0x18
};

// Vortex (type 0x27, vtable 0x00B1CBB8, 0x20 bytes).
class VortexComponent : public EntityComponent {
public:
    explicit VortexComponent(const VortexComponentDesc* desc);
    ~VortexComponent() override;
    void Update(float dt) override;

    float radius = 0.0f;             // 0x18
    float strength = 0.0f;           // 0x1C
};

// <Scoring> (type 0x28, vtable 0x00B1CC80, 0x40 bytes).
class ScoringComponent : public EntityComponent {
public:
    explicit ScoringComponent(const ScoringComponentDesc* desc);
    ~ScoringComponent() override;
    void Update(float dt) override;

    float score = 0.0f;              // 0x18
    ape::Vec3 endPos = {0.0f, 0.0f, 0.0f, 0.0f};   // 0x20
    float startVel = 0.0f;           // 0x30
    float damp = 0.0f;               // 0x34
    float speed = 0.0f;              // 0x38
    bool triggered = false;          // 0x3C NOTE: role assumed
};

// <Mover> (type 0x1D, vtable 0x00B1CED0, 0xC0 bytes): moves the owner
// through up to eight points, pausing a random time at each.
class MoverComponent : public EntityComponent {
public:
    explicit MoverComponent(const MoverComponentDesc* desc);
    ~MoverComponent() override;

    static constexpr int kMaxPoints = 8;
    uint32_t pathType = 0;           // 0x18
    uint32_t motionType = 0;         // 0x1C
    float speed = 0.0f;              // 0x20
    float pauseDelayMin = 0.0f;      // 0x24
    float pauseDelayMax = 0.0f;      // 0x28
    float pauseTimer = 0.0f;         // 0x2C
    bool flag30 = false;             // 0x30
    bool flag31 = false;             // 0x31
    int32_t pointCount = 0;          // 0x34
    int32_t current = -1;            // 0x38 point being approached (-1 = none)
    ape::Vec3 points[kMaxPoints] = {};   // 0x40 world positions
};

} // namespace components
} // namespace sly
