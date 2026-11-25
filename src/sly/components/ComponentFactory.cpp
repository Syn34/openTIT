// openTIT - Component factory: builds a component from its description.
#include "sly/components/ComponentDescs.h"
#include "sly/components/ComponentDescs2.h"
#include "sly/components/Components.h"
#include "sly/components/ContextualMarker.h"
#include "sly/components/ControlComponents.h"
#include "sly/components/DamageSequence.h"
#include "sly/components/InventoryDescs.h"
#include "sly/components/MonsterCloset.h"
#include "sly/components/OffscreenMarker.h"
#include "sly/components/PulseField.h"
#include "sly/components/SystemCracker.h"
#include "sly/components/TrackerComponent.h"
#include "sly/entity/EntityHooks.h"
#include "sly/levels/ai/AIBossGrizz.h"
#include "sly/levels/ai/AIComponent.h"

namespace sly {
namespace entity {

using namespace components;

// TODO(0x002635E8 CreateComponent): the console switch covers every
// component type (0x00-0x2F, AI components through the AI factory); the
// port builds the classes ported so far and returns null for the others.
EntityComponent* CreateComponent(ComponentDesc* desc)
{
    if (!desc)
        return nullptr;
    switch (desc->type) {
    case kCompDesc_ContextualMarker:
        return new ContextualMarkerComponent(static_cast<ContextualMarkerComponentDesc*>(desc));
    case 0x05:
        return new ControlPlayerComponent(static_cast<ControlComponentDesc*>(desc));
    case 0x07:
        return new ControlPassiveComponent(desc);
    case 0x08:
        return new ControlScriptedA(desc);
    case 0x09:
        return new ControlVehicleComponent(static_cast<ControlComponentDesc*>(desc));
    case 0x0A:
        return new ControlScriptedB(desc);
    case 0x0B:
        return new ControlRadarComponent(static_cast<ControlComponentDesc*>(desc));
    case 0x0C:
        return new ControlScriptedC(desc);
    case 0x0D:
        return new levels::ai::AIComponent(static_cast<levels::ai::AIComponentDesc*>(desc));
    case 0x30:
        return new levels::ai::BossGrizzComponent(static_cast<levels::ai::BossGrizzDesc*>(desc));
    case kCompDesc_ControlAI:
        return new ControlAIComponent(static_cast<ControlAIComponentDesc*>(desc));
    case kCompDesc_AnimMap:
        return new AnimMapComponent(static_cast<AnimMapComponentDesc*>(desc));
    case kCompDesc_IK:
        return new IKComponent(static_cast<IKComponentDesc*>(desc));
    case kCompDesc_Inventory:
        return new InventoryComponent(static_cast<InventoryComponentDesc*>(desc));
    case kCompDesc_RadarBlip:
        return new RadarBlipComponent(static_cast<RadarBlipComponentDesc*>(desc));
    case kCompDesc_Mover:
        return new MoverComponent(static_cast<MoverComponentDesc*>(desc));
    case kCompDesc_OffscreenMarker:
        return new OffscreenMarkerComponent(static_cast<OffscreenMarkerComponentDesc*>(desc));
    case 0x23:
        return new ControlExtComponent(desc);
    case kCompDesc_Tracker:
        return new TrackerComponent(static_cast<TrackerComponentDesc*>(desc));
    case kCompDesc_Vortex:
        return new VortexComponent(static_cast<VortexComponentDesc*>(desc));
    case kCompDesc_Scoring:
        return new ScoringComponent(static_cast<ScoringComponentDesc*>(desc));
    case kCompDesc_SystemCracker:
        return new SystemCrackerComponent(static_cast<SystemCrackerComponentDesc*>(desc));
    case kCompDesc_MagNet:
        return new MagNetComponent(static_cast<MagNetComponentDesc*>(desc));
    case kCompDesc_SpeedBoost:
        return new SpeedBoostComponent(static_cast<SpeedBoostComponentDesc*>(desc));
    case kCompDesc_MonsterCloset:
        return new MonsterClosetComponent(static_cast<MonsterClosetComponentDesc*>(desc));
    case kCompDesc_DamageSequence:
        return new DamageSequenceComponent(static_cast<DamageSequenceComponentDesc*>(desc));
    default:
        return nullptr;
    }
}

} // namespace entity
} // namespace sly
