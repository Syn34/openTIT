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
* Radar reads two tuning defaults (`EntityComp.Radar.Default*Duration`) that belong to the
  unported component static initialiser 0x002D6A40; they are defined in `ComponentDescs.cpp`
  for now and must move there when that initialiser is ported.
* `ComponentHooks.*`: TODO stubs for calls out of this directory (level physics world).
* `sly/entity/Entity.h`: `ComponentDesc` gained the registering constructor and the
  scope byte (+0x10) the parsers fill in.

## Attribute names
Attribute and element hashes are `HashNameNoCase` of the name; names were recovered by
hashing the executable's strings and word pairs. Hashes with no match keep a
`NOTE: name unknown` constant.

## Not done
* Dispatcher 0x00250190: needs every table entry first.
* Inventory (0x002666C0 with item parser 0x00265D60, sorted insert 0x00266570 and the
  item/group registries 0x0105BE30 / 0x0105BE18; group parser 0x002650D8).
* Inventory: group file parser 0x00266C3C, group parser 0x002650D8 (two slot classes,
  vtables 0x00B1BEA0 / 0x00B1BEC8), item parser 0x00265D60 and the component 0x002666C0.
* Projector,
  CustomModel, Weapon, Binocucom (its defaults are tuning globals of 0x002D6A40), Shooting,
  and the 0xCE95B536 element (0x0028363C, a parser that returns null).
* Description destructors live at 0x0076xxxx (outside this range).
