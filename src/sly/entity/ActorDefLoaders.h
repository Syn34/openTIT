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

} // namespace entity
} // namespace sly
