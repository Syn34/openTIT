# sly/objects notes (range 0x00180000-0x00260000: world entity classes)

## Layout
* `ObjectsBase.h/.cpp`: local stand-ins for the entity framework (the entity module,
  0x000B0000-0x00180000, is being ported in parallel): `EntityDefDesc` (0x4C-byte def
  description), `EntityDef` (vtable 0x00B12230), `EntityPlacement` ("settings", vtable
  0x00B12260), `EntityInstanceDesc` (0x90-byte instance description) and `ObjEntity`
  (entity base, vtable 0x00B12550). Only the fields/slots used here are modelled; the
  functions in ObjectsBase.cpp are untagged minimal versions of 0x000C26D8, 0x000C2DA0,
  0x000C387C, 0x000CBA88, 0x000CDD28, 0x000CDB28.
* `ObjectsHooks.h/.cpp`: every call out of this directory (Havok phantoms, nav obstacles,
  game events, audio, job manager, spline lookup, ...) as TODO stubs naming the console
  function/member.
* `Trigger.*`: trigger volume base (def 0x00B19510, placement 0x00B19540, entity 0x00B19558,
  plain trigger 0x00B19698): XML def/placement parsing, Create, occupant bookkeeping,
  enter/exit report queues, nav obstacle registration, events.
* `TriggerTypes.*`: alert, crawl, filtered (physics state), job, jump-direction and
  update-enable triggers.
* `TriggerSound.*`: polled triggers (0x00B197D8; listener-position polling for sphere/box
  volumes), ambient sound and sound zone triggers.
* `TriggerCamera.*`: camera trigger def/placement/Create/ctor.
* `TriggerEnv.*`: damage trigger (def/placement/Create/ctor) and wind trigger def side.
* Tuning globals: `CharacterMoveTuning` (534 "Entity.Character.*"/"Camera.*"/
  "Mechanic.SystemCracker.*" variables, 0x0019FED0/0x0019D2DC), `ChainTuning`
  ("Entity.Chain/PushBlock/Rope/Springboard/SentryLight.*", 0x001D50C4/0x001D4C28),
  `TriggerTuning` ("Debug.Display.Trigger.*" etc., 0x00226288/0x00226060),
  `VehicleTuning` (0x0024F034/0x0024EEDC). The CharacterMove values were extracted from the
  disassembly (Ghidra lost the float arguments).

## Changes needed outside this directory
* When `sly/entity` lands, `ObjectsBase.h` should be replaced by the real `EntityDef`,
  placement, instance description and `Entity` classes; field offsets are noted on every
  member. Entity slots used here: 2 OnCreated, 4 Update, 7/8 Set/GetState(layer),
  10 Release(bool), 16 physics component, 19 GetName, 24 SetBusy, 40 GetBounds(world),
  43 HandleMessage, 55 ApplyVisibility, 57/58 OnEntered/OnLeftTrigger, 80.
* `EntityDefDesc::typeBits` (+0x04) / `EntityDef::typeBits` (+0x18) and
  `EntityPlacement::typeBits` (+0x08) are per-class type bits (trigger types: push block 1,
  wind 2, crawl 4, alert 8, filtered 0x10, jump dir 0x20, update enable 0x80).
* `TriggerPlacement` constructor is console 0x00746148 (still a TODO: the def attachment list copy is missing;
  the owner of that address can tag it on `TriggerPlacement::TriggerPlacement`).
* Trigger name accessor 0x00759E30 is `TriggerBase::GetName` (tagged by the 0x0072-0x007B range owner).
* Tagged byte-identical copies outside this range (same classes): trigger def destructor
  copies 0x0075AE14..0x0075BE38, 0x0075B6D8; trigger entity destructor copies 0x00759E74,
  0x0075A110, 0x0075BB14, 0x0075A250.
* ObjectsHooks.cpp stubs should forward to: Havok (0x007D48C4/0x007D4EA8 references,
  0x00931D10/0x00931F00 add/remove phantom, 0x0093FE00/0x0093FED8 properties,
  0x00928828/0x009288D8 listeners, 0x00924B90 setAabb, 0x009315F0 filter update),
  nav (0x000110EC/0x000112D8 in sly/levels), 0x0012E824/0x0012E8B0, game events
  (0x0046502C/0x00465124/0x004650A8/0x00464F88 + 0x00482D14), audio (0x00446984,
  0x0044C474, 0x0044C758, 0x0044C404, 0x0044C43C, 0x00441C68/0x00441CB8/0x00441D18,
  0x0044D5BC/0x0044D614), 0x00129894 disguise test, 0x002E1220 job lookup, spline def
  list 0x007335C4. Game world: listener position +0x128260, player +0x1078, player count
  byte +0x1D.
* The phantom listener callbacks (0x0075A6xx-0x0075ACxx, outside range) should call
  `TriggerBase::QueueEnter/QueueExit`.

## Uncertain identifications / behaviour notes
* Trigger flags 0x1EF4F9F5 ("players only") and 0x2D407057 ("events for every entity")
  have no recovered names; their meaning follows how the instance uses them.
* Placement bit 7 (instance flag bit 27) is read as "AABB phantom fed by the listener";
  clear means a shape phantom whose overlaps are polled each frame.
* `TriggerBase::Update` reproduces the console's index walk over the occupant list while
  OnExit swap-removes (an occupant can be skipped for one frame).
* `TriggerBase::GetBounds`: for phantom types outside 0..2 the console copies an
  uninitialised box; the port keeps the zero-point box.
* `ExitAll`/`Release` guard against an OnExit override that does not remove the occupant
  (the console would loop forever).
* Camera states: Ground 1, Air 2, Climb 4, Crawl 8; a volume allowing Climb never
  excludes Air.
* Damage instance flag slot 2 is never fed by the def or the placement (console skips it).
* Damage orientation is a quaternion from XYZ Euler half angles (FastCos/FastSin).
* Wind types: "Directional" + two unnamed hashes (0x9D9722F0, 0xDCC384C1).

## TODO in this range (not tagged)
* 0x0020FD30 PolledTrigger::IsInside (box branch is undecodable vector code; reconstructed),
  0x00210D80 ambient sound parameters, 0x00213870 sound zone fade weight, 0x0020D7E8 trigger
  SetTransform, 0x0020F400/0x0020F914 global deferred report queue (0x01055B80),
  0x002108A8/0x00213398 phantom shape rebuild, 0x00211780/0x00211E40/0x00214640/0x00225E8C
  sound trigger shape set-up, sound zone Update 0x00213C78.
* Trigger types not started: death (0x0021B898..0x0021D3C8), FX (0x0021DEE0..0x002203EC),
  push block (0x00221760..0x00221F38), weather (0x00221F3C..0x00223C9C), camera behavior
  (0x00216AB0..0x002198A4), wind instance (0x00224170..0x0022515C), update-enable/crawl/
  camera instance logic (Update/OnEnter beyond what is tagged), job OnEnter 0x00220E64.
* Everything outside the trigger family: character state code (0x00180150-0x0019FB9C),
  breakable chains/chain physics (0x001A44E0-0x001A9C80, 0x001BBC40..), ropes/EntityDefRope
  (0x001B9B70..), push blocks (0x001B37E8..), sentry lights (0x001D0450..), safes
  (0x001E5398..), costume prompt/smoke bomb (0x001978F0), turrets, item/remote code,
  vehicles (0x00226A28..0x0024EEDC), entity components (0x00250000..). These are built on
  three def families of the entity module (plain 0x000C2B90, actor 0x000E27FC, physical
  0x000C3078) that would need the same stand-in treatment.

## Build state
* All files here compile warning-free and the full tree builds and links (an earlier
  break was only in in-progress `src/sly/entity` files).

## Third pass (agent3)
* New trigger classes: `TriggerDeath.*` (death volumes; OnEnter 0x0021C8B8 still TODO),
  `TriggerFx.*` (FX and push-block volumes; Update/OnEnter/OnExit TODO),
  `TriggerWeather.*` (def and entity basics; per-player blend TODO),
  `TriggerCameraBehavior.*` (entity side; def parsing needs the camera module).
* Wind trigger instance (`TriggerEnv.cpp`): constructor, wind field calls, lifetime script
  states ("Spawn", "OnEnter", "OnExit"), SetEnabled. The wind field object is a
  `sly::WindSystem` built by the wind module (hooks `WindField*`).
* Update-enable triggers track player characters (`UpdateEnableTrigger::Update/OnEnter/
  OnExit`); job triggers offer their job on entry (prompt or immediate start).
* Camera trigger OnEnter/OnExit/ReleaseCameraMode (camera mode calls are hooks).
* `InheritDefDesc` (0x000C387C) is now tagged: the stand-in is the complete function.
* New hooks in ObjectsHooks.h: entity scripts, script variables/states, volume shape
  queries, wind fields, jobs, text display, camera modes, `EntityClass2`.
