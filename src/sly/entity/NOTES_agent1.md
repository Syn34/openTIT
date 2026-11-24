# sly/entity additions for 0x000F0000-0x00200000 (agent1)

## Class model
* `EntityInterfaces.h`: the two listener interfaces at +0x17C/+0x180 (own vtables
  0x00B1200C/0x00B12528). They are mixed in by the *derived* classes (physical entity,
  actors, props), not by Entity itself: the derived destructors restore the interfaces'
  own vtables before calling the base. Thunk destructors are tagged on the class
  destructor ("thunk, +0x17C/+0x180").
* Stand-ins (untagged, minimal): `ScriptedEntity` (0x00B126B8, ctor 0x000E02D8),
  `ScriptedEntityDef` (0x00B11D98), `ActorEntity` (0x00B12898, dtor 0x007476B0),
  `ActorDef` (0x00B12838, dtor 0x000E281C), `PhysicalEntity` (0x00B12CD8, dtor 0x000EB0F8),
  `PhysicalEntityDef` (0x00B12C88, dtor 0x000E9820). Whoever ports the entity module
  (0x000E0000-0x000F0000, 0x0074xxxx) should replace them.
* Class names are descriptive where the originals are unknown (RigidProp, AlterEgoEntity,
  TargetableA/B/CBase, SoundActor*, InteractiveProp*, SearchlightActor, ...); each header
  gives the console vtable.
* `Character.h`: Character (0x00B14168, 0x550 bytes) and CharacterDef (0x00B14138);
  only teardown and small helpers are ported. CharacterDef's identical second destructor
  (0x0011BCF4) and 0x00121E6C are tagged as "base object" variants.
* `EntityTuning.cpp`: the entity module's static initialiser (0x000F86DC/0x000F81E4) with
  its seven tuning variables and three key tables; the definition map, entity lists and
  deferred event queue it also constructs live in Entity.cpp/EntityDefs.cpp.

## Hooks
`GameEntityHooks.*` holds the TODO stubs (entity slots not declared on Entity, audio
requests, anim script/driver bits, Havok frees, minigame flag, game camera lookup).
