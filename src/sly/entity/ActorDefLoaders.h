// openTIT - Definition factories of the actor classes (ActorDefLoaders.cpp).
// Each takes the loader's shared description and the "from" parent (may be
// null) and returns the new definition.
#pragma once

#include "sly/entity/EntityDef.h"

namespace ape::xml { class XmlElement; }

namespace sly {
namespace entity {

#define SLY_DECLARE_DEF_FACTORY(name)                                                                             \
    EntityDef* name(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,          \
                    const EntityDef* parent)

SLY_DECLARE_DEF_FACTORY(CreateActorGroupDef);        // 0x201FA54F
SLY_DECLARE_DEF_FACTORY(CreateHitCounterActorDef);   // 0xB2221492
SLY_DECLARE_DEF_FACTORY(CreateWatchActorDef);        // 0xB47D3A95
SLY_DECLARE_DEF_FACTORY(CreateSignalActorDef);       // 0x906A55EA
SLY_DECLARE_DEF_FACTORY(CreateActorB15E60Def);       // 0x8CD0E8E8
SLY_DECLARE_DEF_FACTORY(CreateObjectActorDef);       // 0xC475D118 "Teleporter"
SLY_DECLARE_DEF_FACTORY(CreateInteractivePropAUseDef);   // 0x044344C9
SLY_DECLARE_DEF_FACTORY(CreateSearchlightDef);       // 0x6F429D34
SLY_DECLARE_DEF_FACTORY(CreateTweenActorDef);        // 0x55B67D7C
SLY_DECLARE_DEF_FACTORY(CreateInteractivePropBDef);  // 0x6927A078
SLY_DECLARE_DEF_FACTORY(CreateSoundActorDef);        // 0x7CDC084C
SLY_DECLARE_DEF_FACTORY(CreateSoundActorB17728Def);  // 0x633A7385
SLY_DECLARE_DEF_FACTORY(CreateSoundActorB174C0Def);  // 0xEE9D8477 "SentryLight"

} // namespace entity
} // namespace sly
