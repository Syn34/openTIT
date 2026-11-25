# sly/components notes (range 0x00250000-0x002E0000)

## Contents
* `ComponentDescs.*`: the XML component description parsers selected by element name from
  the table at 0x00AD4000 (dispatcher 0x00250190). Done: PHYSICS, Anim, ContextualMarker,
  ControlAI, AnimMap, ProcAnim, IK, FacePlayer, RadarBlip, Buoyancy, OffscreenMarker,
  SimplePhysics, JumpRope, AeSpline, tracker, Vortex, Scoring, SystemCracker, MagNet,
  SpeedBoost, WallClimb, BallRoll, MonsterCloset, DamageSequence, Mover, Combat, Interact,
  targeting, ProcEyes, FX, Wheelchair, Radar, Receptacle.
* `IKSolverDefs.*`: the IK file's solver definitions (JointTwo/JointThree/LookAt/Foot) and
  their four registries, plus the file-level parser 0x00292F00.
* `ComponentDescs2.*`: Projector (+ its part-source helper 0x00288C3C), CustomModel, Weapon,
  Binocucom, Shooting, the null parser of element 0xCE95B536 (0x0028363C) and the element
  dispatcher 0x00250190 (`ParseComponentDesc`, table 0x00AD4000 as a static array).
* `InventoryDescs.*`: inventory group file (0x00266C3C), group/slot parser (0x002650D8, the
  two slot classes 0x00B1BEA0 entity / 0x00B1BEC8 loot table with their accessors
  0x00264D70..0x00264F58), item parser (0x00265D60), the <Inventory> description (0x002666C0)
  and its priority insert (0x00266570); registries 0x0105BE04/18/30 as function-local statics.
* `ComponentTuning.*`: the module's static initialiser 0x002D6A40 (303 tuning variables,
  extracted from the disassembly; the registries it also constructs live with their users).
* `Components.*`: the component base (`entity::EntityComponent` ctor 0x00250298 / dtor
  0x00250384 + base copy 0x002505A0 / deleting 0x00250480, live registry 0x01056F1C) and the
  simple component classes: ControlAI (0x00B1C1B8), type-0x23 control (0x00B1C518, read as
  ControlExt), AnimMap (0x00B1C628), IK (0x00B1C7E0, ctor 0x0025E960), RadarBlip
  (0x00B1C930), Vortex (0x00B1CBB8), Scoring (0x00B1CC80), Mover (0x00B1CED0, ctor
  0x00258008), Inventory (0x00B1BF00, ctor 0x0025E7E4, dtor 0x0026694C; Update 0x00267058 and
  the spawn helpers 0x0026621C.. are not ported). The simple classes' destructors are
  byte-identical copies of the base destructor, each tagged
  on its own class.
* `ComponentHooks.*`: TODO stubs for calls out of this directory (level physics world).
* `sly/entity/Entity.h`: `ComponentDesc` gained the registering constructor and the
  scope byte (+0x10) the parsers fill in.

## Attribute names
Attribute and element hashes are `HashNameNoCase` of the name; names were recovered by
hashing the executable's strings and word pairs. Hashes with no match keep a
`NOTE: name unknown` constant.

## Uncertain / notes (agent3 pass)
* The dispatcher table points ControlPlayer/ControlNet/ControlExt at the dispatcher itself
  (it would recurse forever on the console); the port returns null for them.
* Inventory attribute names are mostly unknown (hashes kept). Loot chance defaults to 0.0
  (the parser overwrites the constructor's 0.5). `InventoryLootSlotDesc::GetRotation` with no
  entry returns a zero quaternion like the console.
* Weapon: the console does not bound the <Part> count (six slots); the port stops at six. The
  "LaunchAngle" default (45) stays in degrees when the attribute is missing (console bug).
* Shooting: four angle defaults are pi/4 used as degrees; one attribute (0xE9ED1023) is read
  twice into two fields.
* Binocucom attribute names follow the tuning names (without "Default"); the projectile list is
  not bounded on the console (eight slots).
* `ComponentTuning.cpp` registers "UI.Binocucom.Radar.EnemyColor" twice, like the console.
* Components: the per-class slot-4 empty overrides (0x00288970 AnimMap, 0x00293934 IK) and
  the shared empty slot 8 (0x00264D6C) are not tagged (meaning unknown).

## Not done
* BossGrizz description (0x00063D54 parser, 0x00062824 ctor, 0x000632A8 body, 0x00062E20
  action list helper): a pattern-boss action table (types from 0x00F557FC).
* The component factory 0x002635E8 (needs every component class) and the remaining
  component classes (Physics 0x00261FE0, Targeting, Anim, Combat, ContextualMarker, ProcAnim,
  ProcEyes, FX, FacePlayer, CustomModel, Projector, Weapon, Inventory runtime 0x0026621C..
  0x00267FE0, Radar, Binocucom, Interact, Receptacle, Wheelchair, AeSpline, JumpRope,
  Buoyancy, SimplePhysics, OffscreenMarker, Shooting, tracker, SystemCracker, MagNet,
  SpeedBoost, MonsterCloset, BallRoll, WallClimb, DamageSequence and the control family).
  MagNet/SpeedBoost embed an interface object (vtable 0x00B1CD24, base dtor 0x0048F6B0);
  AeSpline unregisters a game-world listener in its destructor (0x0046B7A4).
* Description destructors (0x0076xxxx, plus 0x00738DB0/0x0074EC00/0x0074EC44/0x00749668)
  and the inventory slot defaults are in `ComponentDescDtors.cpp`.

## Fifth pass (agent3)
* New classes: OffscreenMarker, Tracker, SystemCracker, MagNet/SpeedBoost (PulseField.*),
  DamageSequence, ContextualMarker. Their constructors are built inline by the factory
  (0x002635E8, still TODO) and tagged at the factory's helper addresses where one exists.
* Component virtual slot 8 (`EntityComponent::RenderInstance`, default 0x00264D6C) is called
  for components in the instance render list (kList_C, `entity::AddToInstanceRenderList`,
  0x000D6614); ContextualMarker's override (0x0027BE38) is TODO.
* Factory types 5, 7-13 have no XML parser: control components created by code. Types 8, 10
  and 12 embed a `StateMachine` at +0x50 (vtables 0x00B1C2A8/0x00B1C378/0x00B1C444) whose
  handlers drive scripted movement (move to point, follow spline); not ported (vector code
  and Havok character calls).
* ComponentHooks gained HUD, marker colour, material, listener group and geometry hooks
  (`MarkerGeometry`, `EntityFindLocator` are stubs).
* Engine colour table 0x01164CA0: white, transparent, black, red (0xCAC), green (0xCB0),
  blue (0xCB4), purple (0xCB8), orange (0xCBC), grey (0xCC0), yellow (0xCC4), pink (0xCC8),
  cyan (0xCCC).

## Later pass (agent3)
* `MonsterCloset.*`: the MonsterCloset component and its spawn slots (mine projectiles fired
  from the owner's weapon towards a point instance; turret owners aim first). Projectile
  spawning (0x000EDE30), weapon muzzle (0x00250F40), point instances and turret aiming are hooks.
* `ControlComponents.*`: control input record (`ControlComponent::Input`: stick, world move,
  magnitude, two-bit action states), base update 0x0027C290, the action -> pad button tables of
  the player (type 5), vehicle (9) and radar (11) controllers, the scripted controllers 8/10/12
  (state machine embedded at +0x50; the per-state handlers are TODO). `ControlSlot16` is now
  `ButtonForAction`. The pad controllers' Update functions (0x0027C478, 0x0027E508, 0x0027FCB8)
  are TODO (vector code / bad instruction data).
