# sly/components notes (range 0x00250000-0x002E0000)

## Contents
* `ComponentDescs.*`: the XML component description parsers selected by element name from
  the table at 0x00AD4000 (dispatcher 0x00250190). Done: PHYSICS, Anim, ContextualMarker,
  ControlAI, AnimMap, ProcAnim, IK, FacePlayer, RadarBlip, Buoyancy, OffscreenMarker,
  SimplePhysics, JumpRope, AeSpline, tracker, Vortex, Scoring, SystemCracker, MagNet,
  SpeedBoost, WallClimb, BallRoll, MonsterCloset, DamageSequence.
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
* Radar (0x00299A60: Ghidra loses the values pushed per child and the 9-entry name table
  0x00F64BF0), Receptacle (0x002B8328: 0x90-byte slot records built on the stack),
  targeting, Combat, ProcEyes, Projector, CustomModel, Weapon, FX, Binocucom, Interact,
  Mover, Wheelchair, Shooting, and the 0xCE95B536 element (0x0028363C).
* IK solver definitions (0x00291E6C head, 0x00292210 leg, 0x00292618 / 0x002929EC joint
  solvers): Ghidra drops the copies of the read values; they need the disassembly.
* Description destructors live at 0x0076xxxx (outside this range).
