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

## Second pass (agent1, entity/camera/objects)
* Entity slots 49-52 are on `Entity` now: `PlayClip` (0x000DBE78), `PlayClipByHash`,
  `PlayClipByName` (children linked with kChild_ForwardAnim play "<link name>_<clip>")
  and `RootMotionDelta` (0x000DC730; flags2 bits 10/11 scale the step with the object or
  by `rootMotionScale` (+0xE0) / `rootMotionWeight` (+0xF0)). The anim layer's root track
  extraction (0x00352F70) is a hook (`AnimLayerRootMotion`). The child link record starts
  with a name buffer (0x80 bytes assumed). GameCamera's clip slots override the Entity ones;
  the old `EntityPlayClip` hook is gone.
* Physical entities: slot 94 `AimAt` (aim frame = the `carriedTransform` at +0x1C0; the
  carried entity is a marker sitting at the aim point), slot 95 `OnLaunched` (flight time
  +0x228, script variables, state "Start"), slot 101 `UpdateFromBody`, `SweepTo` (body
  sweep hook `RigidBodySweep`, 0x004A41C0), launch point +0x200. ProjectileB flies a
  quadratic Bezier (side/forward scales +0x298/+0x29C, time +0x290/+0x2A0); Projectile
  lobs (def +0x1A4). Def flags 10 (die on hit) and 22 (face the motion).
* Placement copies (slot 2 of the placements, `sly/objects/EntityPlacementCopies.cpp`):
  every scripted/actor/sound actor placement hands its fields to an instance description
  0x20 bytes lower (placement +0xF0 -> description +0xD0). `EntityPlacement::FillInstanceDesc`
  (0x000CDB28) is real now; the scripted placement's override lists are 0x4C-byte records.
  The placement parsers (0x000CBE10, constructor 0x000CC188) still need the objects and
  entity definition models to meet (two `EntityDef` classes exist).
* `ActorEntity::GetAimPoint` (slot 71) uses the first definition part's +0x80 offset.
* Follow camera block (vtable 0x00B14120): Camera.Follow.* tuning (all but Interp); the
  variable addresses were matched through the camera tuning initialiser's constructor calls.
* Camera: Strafe update (0x00115FB0), Vehicle activation (0x00111200).
* Not done: LaunchProjectile 0x000EDE30 (needs a PhysicalEntityPlacement on the stack),
  Projectile slot 101 0x000F30B8, the SPU-filtered entity update drivers 0x000CE5D4..
  0x000CFF4C (the filter runs in an SPU program), the Track/Orbit/Shoulder/Spline updates.
