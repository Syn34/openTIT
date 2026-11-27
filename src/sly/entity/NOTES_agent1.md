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

## Third pass (agent1): one definition model
* `EntityDef.h`: the single `EntityDef` (0x8C bytes, vtable 0x00B12230: slot 3 GetBounds,
  7 Spawn(placement), 8 InstanceName(placement)), its description `EntityDefDesc`, the placement
  base `EntityPlacement` (vtable 0x00B12260; the old `SpawnParams` was the same record) and the
  instance description `EntityInstanceDesc` (full constructor 0x000CDD28). The real definition
  constructor (0x000C26D8) resolves the geometry/template object, builds the component list from
  the named templates, the Havok bone map, and registers the definition.
* The real entity base (vtable 0x00B12278, 71 slots, constructor 0x000D3578, destructor
  0x000D7024) is what triggers derive from; 0x00B12550 (79 slots, destructor 0x000DF8B4) is a
  derived class that adds the interaction slots 71-78. The port keeps one `Entity` class.
  `Entity(const EntityInstanceDesc&)` is ported; world object set-up 0x000D2F04 and the
  attachment records 0x000D1B10 are hooks.
* Definition flag word +0x20: desc flag i is bit 31-i; bit 26 (kDefFlag_Desc5) becomes placement
  bit 29 and hides new entities, bit 25 the highlight flag, bit 27 "sleeps away", bit 30 the
  instance description's bit 14. Placement bits84 bits 21-28 are the player slot (0xFF none);
  placement +0x60 / description +0x70 is the instance scale; +0x70 the source object.
* Entity type bits as the triggers test them: 1 scripted (ScriptedEntity::IsDead is slot 80),
  3 character, 4 excluded from update-enable tracking, 49 pushable.
* Definitions ported on the new model: ScriptedEntityDef (0x000C2E8C, description
  `ScriptedEntityDefDesc`), ActorDef (0x000C79C0; now derives from ScriptedEntityDef; the Havok
  body of each part comes from the hook `ActorPartCreateBody`), ChainDef (desc parser 0x001A7878,
  loader entry 0x001A7AB0). The definition loader 0x000CA038 and the big shared description
  parsers (physical 0x000C4020, actor 0x000C85A8) are still TODO; MegaKnightDef (gameplay)
  should derive from ActorDef once its loader is ported.
* Placements (sly/objects/EntityPlacements.cpp, PlacementLoader.cpp): scripted placement
  constructor/parser (0x000CC188/0x000CBE10; the 0x4C-byte override records are script variable
  bindings = `TextField`), every class loader called by the placement loader 0x000CC920 except
  the input mover (0x001D7D98, its parser 0x001D7470 reuses the record block per kind) and the
  loader itself (element dispatch table, common attributes, placement map 0x01013B1C,
  attachment records 0x000C9AB8). Placement and instance description fields were retyped from
  word arrays to the floats/hashes/bools the parsers read.
* `GetPlayerCount`/`PlayerCount`/`IsSinglePlayerMode` read the game's local player count;
  `TriggerSetEnabled`/`TriggerOccupantCount` go to TriggerBase.
* Camera: Track update (0x001139C0). The console passes dt and Camera.Track.Interp to the
  approach helpers in swapped positions (kept). Hooks: `DampedApproach` (0x004A8778),
  `CharacterAttachedSpline` (0x0012919C).
* AI: manager tick (0x00019AF0, sly/levels/ai/AIManagerTick.cpp), critter FindPerch (0x00020100)
  with its spline filter (0x000239E4) and the engine's closest-spline query (0x005A8CF8) plus the
  spline property table accessors (0x005A8BA8/0x005A8BE4; the table itself is filled by the
  level loader, not ported). The surround record update 0x000182C0 is TODO.
* Scripted instance description (0x000DFDF8, `objects::ScriptedInstanceDesc`): base description
  without placement defaults, type bits 0/1, defaults read from the ScriptedEntityDef.
* AI surround records (0x000182C0, `AIManager::UpdateRecord50`): the record's spot (last player
  position + direction * radius) is snapped onto the nav mesh; the point counts as good while it
  is near the spot, the target is visible from it and it lies inside the main view frustum
  (`AglFrustum::soa`, margin 1). The line of sight test 0x000138B0 is a Havok ray cast on the
  level physics world (not ported) and still reports "not visible"; who fills `controller` and
  `radius` is not found yet.

## Fourth pass (agent1)
* `EntityDefLoader.cpp`: the definition loader (0x000CA038): shared attributes, "from" parent
  defaults, component template names, attachment records, then a table of class factories by
  element hash (unported factories are listed with their console address and leave the
  definition out). The actor class is built in place (`ActorDefDesc(loader)` = the console
  initialiser 0x00745D88 with its constant arguments); `ActorDefDesc::Parse` (0x000C85A8) is
  still a TODO that reads only the scripted attributes; the physical definition
  (0x000C4020/0x000C30E8) is not ported. `EntityDefDesc` +0x2C is the load scope, +0x3C a float,
  +0x44 the attachment records.
* `ScriptedEntityDefDesc::Parse` (0x000C3910, used by 14 parsers) and the scripted entity
  constructor (0x000E01C0, lifetime script start 0x000E0000). Def class bits: 2 scripted, 4 actor,
  0x2000 physical (`kDefClass_*`).
* `sly/physics/PhysicsQuery.*`: ray casts against the game physics world resolved to entities
  (stubs until a physics layer exists). The AI line of sight (0x000138B0) uses it; controller
  slot 72 is `IsSeeThroughLayer(layer)` (was misnamed IsStateInterruptible).
* `EntityUpdateJobs.cpp`: the per-frame selection job drivers (suspend/resume, delete, update,
  post-update; 0x000CE5D4..0x000CFF4C) and the module's level start (0x000CE0A8). The SPU
  selection programs are hooks (`EntityJobSelect*`); the update ones select every live entity.
* `EntityRenderLists.cpp`: instance render lists B/C (fade pass + component placement), list D
  ("s_compSyncList"), the end-of-frame visibility restore (type bit 3 characters keep their fade
  copy, type bit 77 is SimpleWheeledVehicle).
* `RopeEntity.*` (vtable 0x00B162C8): destructor, slots 5/10/12/22, segment helpers. Segments are
  0x60-byte records (axis +0x10, push +0x40, object +0x50); NOTE: ChainEntity's links share that
  layout (its `Link` array really starts at +0x160, not +0x1B0; count at +0x762 is read by the
  chain push 0x001A97D8).
* AI: CritterAI::EnterState (0x000215D8) and ClampToWanderArea (0x000212B8),
  AIController::BeginMove (0x00014DA8), TargetReachable/IsAtNavPoint/ClipEndsAtNavPoint
  (NavPower behind `NavCreatePoint`/`NavPathToPlayerExists`/`NavPointGroundPosition` hooks), EnemyAI
  TargetInAttackRange and the patrol interaction pair.
* Not done (looked at): ActorEntity constructor 0x000E38F0 (part body cloning, NavPower
  obstacles), ActorDef::Spawn 0x000E4314 and the actor instance description 0x000E40E0 (layer 4,
  0x80 bytes of state, type bits 0-2, C8/CC from the caller), the spawn definition loader
  0x000D5C00 (Ghidra drops the map inserts), entity sound play 0x000DB348/0x000DBA84 (SoundParams
  layout), the Spline camera constructor 0x00108AF8 (per-spline tag table lookups).

## Fifth pass (agent1)
* Definition factories (`ActorDefLoaders.cpp`, `PhysicalEntityDefs.cpp`): `ActorDefDesc::Parse`
  (0x000C85A8; part records are `ActorPartFields` + body name / body), the physical description
  (0x000C4020 parse, 0x000C30E8 constructor; `PhysicalDefFields` shared by description and
  definition, `PhysicalEntityDef` now derives from `ScriptedEntityDef`), and the class factories:
  ActorGroup, HitCounter, Watch, Signal, B15E60, ObjectActor ("Teleporter"), InteractivePropA use
  definition, InteractivePropB, Searchlight, TweenActor, ScriptObjectActor, SoundActorDef and its
  B17728 / sentry light (B174C0) / switch (B16678) / B168E0 / trigger-volume (B17008) / light
  (B16B48) subclasses, Projectile, ProjectileB, EntityProjectileMine. Each class description keeps
  the class bit in `typeBits` (or `registryFlags`, the second type word) and takes its defaults
  from a "from" parent of the same class. Not done: B16DB0 (0x001C49E8), InteractivePropA
  definition 0x00B17DB0 (0x001DD3F8), Safe (0x001E6300), input mover (0x001D62A8), the plain
  0x000C7104 family, the non-actor classes (chains, ropes, items, laser field...).
* The initialisers 0x00745D88 / 0x0074FD80 (actor) and 0x0074596C (physical) are the description
  constructors (outside this range); the physical one takes stack arguments not recovered.
* `objects::ActorInstanceDesc` (0x000E40E0): four state layers, 0x80 bytes of state; the actor
  instance descriptions of the placements derive from it. `ActorEntity(desc)` (0x000E38F0) clones
  the part bodies through `ActorPartCloneBody`; `ActorDef::Spawn` builds `PlainActor`. Class
  spawns + constructors: ActorGroup (+PostUpdate), HitCounter, Watch, Signal, ScriptObject,
  InteractivePropA/B, Searchlight (+Update/InteractionCancel), teleporter (`Teleporter.cpp`).
* TweenActor is the target range (its camera uses Entity.TgtRange.CamPopTime): owned entity
  release helpers, TakeUser (pose hook through the character controller), Release, Reset,
  ReleaseCamera.
* ObjectActor is the teleporter (setting association 0x001EB9CC queues a deferred event whose
  data word is the callback; the entity module's frame step 0x000D4D30 calls it).
* AI: nav jump links (`AssociateNavJumpLink`), nav agent creation/snap/path following
  (`NavAgentPathStep` hook; slot 63 is `OnPlatformLink`), vision cone helper (0x000167F0),
  `FindEnemyInCone`, Bismarque constructor.
* Later in the pass: B16DB0 and Safe definitions, the laser field contact callback
  (0x000C21EC), the breakable chain entity release, RacerAI move heading (0x0002D8C8), Ms Decibel
  spline stretch (0x0005BA08), Paradox script commands (0x0005FA08). Next candidates: the Safe
  constructor 0x001E5CD8 (combination from the game RNG 0x01164888), InteractivePropA definition
  0x001DD3F8 + constructor 0x001DCBA0, the input mover definition 0x001D62A8 and placement
  0x001D7D98, InteractivePropB::SpawnAttached 0x001F50E4 (needs Entity::Attach 0x000D15C0).

## Sixth pass (agent1)
* `Safe.cpp`: the combination safe is complete except its per-frame callback 0x001E9228 (camera /
  tilt sensor, vector code): definition constructor, spawn, constructor, part lookup, reset,
  scramble, prompt, update (timer with the countdown beep, dial input per control, solving,
  opening and reward spawn), input reader and InteractionBegin (fixed camera behind the safe).
  The safe's random draws use the game RNG exactly as the console (three fresh samples; the
  first dial's seed test in Reset can never hold). SafeDef/SafePlacement fields are named.
* `SoundActors.cpp`: SoundActor constructor and slot-state lookup; spawns of SoundActor and the
  switch (B16678), B168E0, light (B16B48), B17728 and the gadget turret (B17270, with its
  constructor, aim parts/frames and weapon shot callback). `SoundActor::kSlotNames` is public;
  `slotTimer` is the script's float variable. Still missing: B16DB0 (0x001C5A80) and B17008
  (0x001C8DB8) constructors, sentry light constructor 0x001D1170.
* Turret definition (B17270) parse/constructor/factory and input mover (ActorB17990)
  definition parse/constructor/factory, entity constructor and spawn are registered in the
  definition table. `InputMoverControl` (InputMoverControl.h) is the 0x44-byte control block
  shared by definition, placement and instance description (a union per control mode).
  Not done: the input mover placement loader 0x001D7D98 and its parse 0x001D7470 (the mode-3
  branch differs from the definition's).
* `ActorHooks.h/.cpp`: new TODO hooks (interaction stop 0x002B7704, safe callback queue,
  motion sensor samples, skin owner group / shot callback).
* AI: Ms Decibel (counter attack check, damage, glass blocks), Paradox (attach, detach, strafe
  camera, phase one), Bismarque (retreat point, fight visuals, QTE button restart, attack
  clean-up, barrier throw), pattern boss steps, El Jefe arena triggers. AIDamageInfo gained
  the hit source (+0x4C); Bismarque's records are `QteButton`s (+0x134).
* `0x000496CC`, `0x001882F8` and other static helpers are free functions; Character gained
  LeaveInteractState (0x0012D290), CountStealthTakedown (0x0013D530), IdleStateValue
  (0x001293C0).
* The player turret prop's definition (C++ name InteractivePropADef, class bit 0x40000000):
  parse 0x001DD3F8, constructor 0x001DC5F8, factory 0x001DE610 (registered), and the prop's
  aim frame 0x001DC988 / camera release 0x001DEC60.
* Next candidates: the remaining sound actor constructors, InteractivePropC (turret prop)
  constructor 0x001DCBA0 (two Havok phantoms) and spawn 0x001DD20C, the
  TargetableC family factory chain (0x001F85F0/0x001F9078/0x001F9BA0), breakable chain
  definition (its parse loses the piece loop in the reference).
