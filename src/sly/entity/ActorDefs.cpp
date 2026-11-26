// openTIT - Destructors of the actor variant definitions (see ActorDefs.h).
// All of them only run the base destructor (and free their own arrays,
// which are members here).
#include "sly/entity/ActorDefs.h"

#include "sly/entity/GameEntityHooks.h"

namespace sly {
namespace entity {

// @func 0x007511B8 ActorGroupDef::~ActorGroupDef
// @func 0x007511E8 ActorGroupDef::~ActorGroupDef (deleting)
ActorGroupDef::~ActorGroupDef() = default;

// @func 0x00751948 HitCounterActorDef::~HitCounterActorDef
// @func 0x00751978 HitCounterActorDef::~HitCounterActorDef (deleting)
HitCounterActorDef::~HitCounterActorDef() = default;

// @func 0x00751F4C WatchActorDef::~WatchActorDef
// @func 0x00751F7C WatchActorDef::~WatchActorDef (deleting)
WatchActorDef::~WatchActorDef() = default;

// @func 0x00752550 SignalActorDef::~SignalActorDef
// @func 0x00752580 SignalActorDef::~SignalActorDef (deleting)
SignalActorDef::~SignalActorDef() = default;

// @func 0x00752B54 ActorB15E60Def::~ActorB15E60Def
// @func 0x00752B84 ActorB15E60Def::~ActorB15E60Def (deleting)
ActorB15E60Def::~ActorB15E60Def() = default;

// @func 0x00753AAC ScriptObjectActorDef::~ScriptObjectActorDef
// @func 0x00753ADC ScriptObjectActorDef::~ScriptObjectActorDef (deleting)
ScriptObjectActorDef::~ScriptObjectActorDef() = default;

// @func 0x00755FF8 ActorB17990Def::~ActorB17990Def
// @func 0x00756028 ActorB17990Def::~ActorB17990Def (deleting)
ActorB17990Def::~ActorB17990Def() = default;

// @func 0x007565FC InteractivePropAUseDef::~InteractivePropAUseDef
// @func 0x0075662C InteractivePropAUseDef::~InteractivePropAUseDef (deleting)
InteractivePropAUseDef::~InteractivePropAUseDef() = default;

// @func 0x00757390 ObjectActorDef::~ObjectActorDef
// @func 0x007573C0 ObjectActorDef::~ObjectActorDef (deleting)
ObjectActorDef::~ObjectActorDef() = default;

// @func 0x00757418 ObjectActorDef::Spawn
Entity* ObjectActorDef::Spawn(EntityPlacement* placement)
{
    (void)placement;
    return nullptr;
}

// @func 0x00757420 TweenActorDef::~TweenActorDef
// @func 0x00757450 TweenActorDef::~TweenActorDef (deleting)
TweenActorDef::~TweenActorDef() = default;

// @func 0x0075757C InteractivePropBDef::~InteractivePropBDef
// @func 0x007575AC InteractivePropBDef::~InteractivePropBDef (deleting)
InteractivePropBDef::~InteractivePropBDef() = default;

// @func 0x007540B0 SoundActorB16678Def::~SoundActorB16678Def
// @func 0x007540E0 SoundActorB16678Def::~SoundActorB16678Def (deleting)
SoundActorB16678Def::~SoundActorB16678Def() = default;

// @func 0x00754430 SoundActorB168E0Def::~SoundActorB168E0Def
// @func 0x00754460 SoundActorB168E0Def::~SoundActorB168E0Def (deleting)
SoundActorB168E0Def::~SoundActorB168E0Def() = default;

// @func 0x007547A8 SoundActorB16B48Def::~SoundActorB16B48Def
// @func 0x0075483C SoundActorB16B48Def::~SoundActorB16B48Def (deleting)
SoundActorB16B48Def::~SoundActorB16B48Def() = default;

// @func 0x00754A18 SoundActorB16DB0Def::~SoundActorB16DB0Def
// @func 0x00754A48 SoundActorB16DB0Def::~SoundActorB16DB0Def (deleting)
SoundActorB16DB0Def::~SoundActorB16DB0Def() = default;

// @func 0x00754D90 SoundActorB17008Def::~SoundActorB17008Def
// @func 0x00754DC0 SoundActorB17008Def::~SoundActorB17008Def (deleting)
SoundActorB17008Def::~SoundActorB17008Def() = default;

// @func 0x00755034 SoundActorB17270Def::~SoundActorB17270Def
// @func 0x00755064 SoundActorB17270Def::~SoundActorB17270Def (deleting)
SoundActorB17270Def::~SoundActorB17270Def() = default;

// @func 0x00755190 SoundActorB174C0Def::~SoundActorB174C0Def
// @func 0x007551C0 SoundActorB174C0Def::~SoundActorB174C0Def (deleting)
SoundActorB174C0Def::~SoundActorB174C0Def() = default;

// @func 0x00755218 SoundActorB17728Def::~SoundActorB17728Def
// @func 0x00755248 SoundActorB17728Def::~SoundActorB17728Def (deleting)
SoundActorB17728Def::~SoundActorB17728Def() = default;

// @func 0x0075399C EntityDefRope::~EntityDefRope
// @func 0x007539CC EntityDefRope::~EntityDefRope (deleting)
EntityDefRope::~EntityDefRope() = default;

// @func 0x00759328 EntityDefB19378::~EntityDefB19378
// @func 0x00759358 EntityDefB19378::~EntityDefB19378 (deleting)
EntityDefB19378::~EntityDefB19378() = default;

// @func 0x00757A3C TargetableCLinkedDef::~TargetableCLinkedDef
// @func 0x00758A3C TargetableCLinkedDef::~TargetableCLinkedDef (deleting)
TargetableCLinkedDef::~TargetableCLinkedDef() = default;

// @func 0x00750EAC EntityDefB14FD8::~EntityDefB14FD8
// @func 0x00750F14 EntityDefB14FD8::~EntityDefB14FD8 (deleting)
EntityDefB14FD8::~EntityDefB14FD8() = default;

// @func 0x007549BC SoundActorB16B48::Slot65
bool SoundActorB16B48::Slot65() { return EntityGetState(this, 0) == 1; }

} // namespace entity
} // namespace sly
