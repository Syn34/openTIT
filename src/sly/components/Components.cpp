// openTIT - Entity components built from component descriptions.
#include "sly/components/Components.h"

#include "sly/components/ComponentHooks.h"
#include "sly/components/IKSolverDefs.h"

namespace sly {
namespace components {

std::vector<std::pair<uint32_t, EntityComponent*>>& LiveComponents()
{
    static std::vector<std::pair<uint32_t, EntityComponent*>> s_components = [] {
        std::vector<std::pair<uint32_t, EntityComponent*>> v;
        v.reserve(0x100);
        return v;
    }();
    return s_components;
}

ape::ApeTSortedMap<void*>& AnimMaps()
{
    static ape::ApeTSortedMap<void*> s_maps(0x20);
    return s_maps;
}

} // namespace components

namespace entity {

// @func 0x00250298 EntityComponent::EntityComponent
EntityComponent::EntityComponent(const ComponentDesc* desc)
    : id(desc->name), owner(desc->owner), type(desc->type), enabled(true), field_14(0)
{
    components::LiveComponents().emplace_back(id, this);
}

// @func 0x00250384 EntityComponent::~EntityComponent
// @func 0x002505A0 EntityComponent::~EntityComponent (base object copy, called by derived destructors)
// @func 0x00250480 EntityComponent::~EntityComponent (deleting)
EntityComponent::~EntityComponent()
{
    // Order-preserving removal of the first entry for this component.
    auto& live = components::LiveComponents();
    for (auto it = live.begin(); it != live.end(); ++it) {
        if (it->second == this) {
            live.erase(it);
            break;
        }
    }
    UnregisterObject(kList_C, this);
}

} // namespace entity

namespace components {

// ===========================================================================
// ControlAI
// ===========================================================================

// NOTE: built inline by the component factory (0x002635E8).
ControlAIComponent::ControlAIComponent(const ControlAIComponentDesc* desc)
    : ControlComponent(desc), behavior(desc->behavior)
{
}

// @func 0x0027CBB8 ControlAIComponent::~ControlAIComponent
// @func 0x0027CCB4 ControlAIComponent::~ControlAIComponent (deleting)
ControlAIComponent::~ControlAIComponent() = default;

// @func 0x0027CDE0 ControlAIComponent::OnOwnerSpawned
void ControlAIComponent::OnOwnerSpawned(entity::Entity* newOwner)
{
    // The owner's message listener is its AI object.
    brain = newOwner->listener;
}

// @func 0x0027CDEC ControlAIComponent::Update
void ControlAIComponent::Update(float dt)
{
    (void)dt;
    if (brain)
        AIBrainFillInput(brain, input);
}

// ===========================================================================
// ControlExt (type 0x23)
// ===========================================================================

// @func 0x00255AD8 ControlExtComponent::ControlExtComponent
ControlExtComponent::ControlExtComponent(const ComponentDesc* desc) : ControlComponent(desc) {}

// @func 0x00283644 ControlExtComponent::~ControlExtComponent
// @func 0x00283740 ControlExtComponent::~ControlExtComponent (deleting)
ControlExtComponent::~ControlExtComponent() = default;

// @func 0x0028386C ControlExtComponent::OnOwnerSpawned
// Nothing to bind: the controller is fed from outside.
void ControlExtComponent::OnOwnerSpawned(entity::Entity* newOwner) { (void)newOwner; }

void ControlExtComponent::Update(float dt) { (void)dt; }

// ===========================================================================
// AnimMap
// ===========================================================================

// NOTE: built inline by the component factory (0x002635E8).
AnimMapComponent::AnimMapComponent(const AnimMapComponentDesc* desc)
    : EntityComponent(desc),
      map(*AnimMaps().Find(desc->map)),
      animSet(desc->animSet),
      tree(FindAnimTree(desc->tree)),
      blendMap(FindAnimBlendMap(desc->blendMap))
{
}

// @func 0x00287BB8 AnimMapComponent::~AnimMapComponent
// @func 0x00287CB4 AnimMapComponent::~AnimMapComponent (deleting)
AnimMapComponent::~AnimMapComponent() = default;

// ===========================================================================
// IK
// ===========================================================================

// @func 0x0025E960 IKComponent::IKComponent
IKComponent::IKComponent(const IKComponentDesc* desc) : EntityComponent(desc)
{
    for (int i = 0; i < 8; ++i)
        jointTwo[i] = *JointTwoSolverDefs().Find(desc->jointTwoSolvers[i]);
    for (int i = 0; i < 8; ++i)
        jointThree[i] = *JointThreeSolverDefs().Find(desc->jointThreeSolvers[i]);
    lookAt = *LookAtSolverDefs().Find(desc->lookAtSolver);
    for (int i = 0; i < 2; ++i)
        foot[i] = *FootSolverDefs().Find(desc->footSolvers[i]);
}

// @func 0x00292CD8 IKComponent::~IKComponent
// @func 0x00292DD4 IKComponent::~IKComponent (deleting)
IKComponent::~IKComponent() = default;

// TODO(0x002934E0 IKComponent::Update): runs the solvers against the owner's
// animation pose (vector code not reimplemented yet).
void IKComponent::Update(float dt) { (void)dt; }

// ===========================================================================
// RadarBlip
// ===========================================================================

// NOTE: built inline by the component factory (0x002635E8).
RadarBlipComponent::RadarBlipComponent(const RadarBlipComponentDesc* desc)
    : EntityComponent(desc), blipType(desc->blipType)
{
}

// @func 0x0029FC30 RadarBlipComponent::~RadarBlipComponent
// @func 0x0029FD2C RadarBlipComponent::~RadarBlipComponent (deleting)
RadarBlipComponent::~RadarBlipComponent() = default;

// @func 0x0029FE58 RadarBlipComponent::Update
// The radar reads the blip type; nothing to do per frame.
void RadarBlipComponent::Update(float dt) { (void)dt; }

// ===========================================================================
// Vortex
// ===========================================================================

// NOTE: built inline by the component factory (0x002635E8).
VortexComponent::VortexComponent(const VortexComponentDesc* desc)
    : EntityComponent(desc), radius(desc->radius), strength(desc->strength)
{
}

// @func 0x002B0890 VortexComponent::~VortexComponent
// @func 0x002B098C VortexComponent::~VortexComponent (deleting)
VortexComponent::~VortexComponent() = default;

// TODO(0x002B0B10 VortexComponent::Update): pulls nearby physics bodies and
// characters towards the owner.
void VortexComponent::Update(float dt) { (void)dt; }

// ===========================================================================
// Scoring
// ===========================================================================

// NOTE: built inline by the component factory (0x002635E8).
ScoringComponent::ScoringComponent(const ScoringComponentDesc* desc)
    : EntityComponent(desc),
      score(desc->score),
      endPos(desc->endPos),
      startVel(desc->startVel),
      damp(desc->damp),
      speed(desc->speed),
      triggered(false)
{
}

// @func 0x002B1F5C ScoringComponent::~ScoringComponent
// @func 0x002B2058 ScoringComponent::~ScoringComponent (deleting)
ScoringComponent::~ScoringComponent() = default;

// @func 0x002B2184 ScoringComponent::Update
// Scoring is event driven; nothing to do per frame.
void ScoringComponent::Update(float dt) { (void)dt; }

// ===========================================================================
// Mover
// ===========================================================================

namespace {
constexpr uint32_t kMoverPoint_Entity = 0x7A30D7E3u;    // "Entity": a named entity
constexpr uint32_t kMoverPoint_Nearest = 0x70E8DC96u;   // nearest entity of a definition. NOTE: name unknown
constexpr uint32_t kMoverPoint_Owner = 0xA9CDB8F9u;     // relative to the owner. NOTE: name unknown
constexpr uint32_t kMoverPath_Default = 0xE2E902C9u;    // NOTE: name unknown
constexpr uint32_t kMoverPath_Forward = 0x026F1EA5u;    // "Forward"
} // namespace

// @func 0x00258008 MoverComponent::MoverComponent
MoverComponent::MoverComponent(const MoverComponentDesc* desc)
    : EntityComponent(desc),
      pathType(desc->pathType),
      motionType(desc->motionType),
      speed(desc->speed),
      pauseDelayMin(desc->pauseDelayMin),
      pauseDelayMax(desc->pauseDelayMax),
      flag30(desc->flag28),
      flag31(desc->flag29)
{
    // Point positions are fixed now: the anchor's position plus the offset.
    for (int i = 0; i < desc->pointCount; ++i) {
        const MoverPoint& point = desc->points[i];
        const ape::Vec3* anchor = nullptr;
        if (point.tag == kMoverPoint_Entity) {
            if (entity::Entity* found = entity::FindEntity(point.name))
                anchor = &found->Position();
        } else if (point.tag == kMoverPoint_Nearest) {
            if (entity::Entity* found = entity::FindNearestEntityOfDef(point.name, owner->Position()))
                anchor = &found->Position();
        } else if (point.tag == kMoverPoint_Owner) {
            anchor = &owner->Position();
        }
        if (anchor && pointCount < kMaxPoints) {
            points[pointCount++] = {anchor->x + point.offset.x, anchor->y + point.offset.y,
                                    anchor->z + point.offset.z, 0.0f};
        }
    }
    // Both path kinds start at the first point; the default kind does so
    // even without points.
    current = -1;
    if (pathType == kMoverPath_Default || (pathType == kMoverPath_Forward && pointCount > 0))
        current = 0;
    pauseTimer = ape::RandomRange(pauseDelayMin, pauseDelayMax, ape::g_gameRandom);
}

// @func 0x002B9FF8 MoverComponent::~MoverComponent
// @func 0x002BA0F4 MoverComponent::~MoverComponent (deleting)
MoverComponent::~MoverComponent() = default;

} // namespace components
} // namespace sly
