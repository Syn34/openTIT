# sly/levels notes (range 0x00010000-0x000B0000)

## Layout
* `script/`: the game's own anim script commands (about 270 classes, vtables
  0x00B0F068..0x00B10E68, code 0x0006B6E0..0x000A8000). Commands registered #1-#82 of
  the registration list are covered (plus SendEvent/BroadcastEvent), see "Done" below.
  * `ScriptCommand.h/.cpp`: shared helpers (entity operand lookup through the "pEntity"
    user-data tag, variable helpers, debug element, warnings switch 0x010AEC60, entity
    flag name table 0x00AC4C28).
  * `CommandsA/E/F/G/G2/H/I/J/Event.cpp`: commands in registration order. Every file ends
    with a `RegisterScriptCommandsX()`; `ScriptCommands.h` has
    `RegisterLevelScriptCommands()` which adds them all under their element hashes.
* `nav/NavGraph.*`: NavGraph.bfx_res loading, nav mesh snapping/proximity, obstacle list.
  NavPower is reached only through `NavPowerInterface` (null default).
* `ai/AIProxyPoints.*`: AI proxy point table (0x00F557F4) and a cubic Bezier helper.
* `GameServices.h/.cpp`: every call into systems outside this range (entities, player,
  camera, HUD, goals, trophies by name, audio lookups, game events, level changes) as
  free functions with TODO stubs that return the "not found" value. Each names the
  console function or field it stands for.

## How the command list was recovered
* Prototypes are registered in `SlyGame::SlyGame` (0x0045CC18): `MemAlloc(size)`, default
  constructor, then `CommandPrototypes().Add(hash, proto)` (0x00785BC4). The list is in
  alphabetical order of the element names, which helped name commands.
* Names come from strings when present, otherwise from matching the registered hash
  against words of the binary's strings. Names marked "NOTE: name from a hash match"
  are 2-3 word matches that fit the alphabetical position; the rest are descriptive names
  with the hash in the comment.
* Vtable layout (7 slots): dtor, deleting dtor, Release (0x0073B890, shared), Execute,
  CreateDebugElement (default 0x0073B8F0 returns null), Link (default 0x0073B8F8), Create.
  Many trivial destructors live at 0x0073xxxx (outside the range) and are not tagged.
* `CreateDebugElement` of IfCollected/IfEntity allocates an `XmlElement`; the release
  build's `XmlElement` constructor ignores the name it is passed, so these are ported
  as `new XmlElement()` (`NewDebugElement`). The ape/anim port left its equivalents as
  TODOs; they could use the same approach.

## Changes needed outside this directory
* `SlyGame::SlyGame` (0x0045CC18, not ported) should call
  `sly::levels::RegisterLevelScriptCommands()` after `ape::RegisterBuiltins()`.
* `GameServices.cpp` stubs should forward to the entity/world/HUD/camera modules when they
  land. The ones that matter most: `FindEntity` (0x000D6A28: entity list 0x01013B34,
  alive bit 18 of +0xAC, name hash +0xA8), `WorldObjectUserTag/UserData` (+0x114/+0x118
  of world objects, `ape::world::ApeObj::field_114/118`), `FindMixState` (0x0044C43C),
  `FindSoundName` (0x00446984), `TextFieldParse` (0x0049EC08).
* `sly/game/WorldInterface.h` `WorldObjectKey`/`WorldObjectName` are used by the proxy
  point table.

## Uncertain identifications / behaviour notes
* Ghidra drops the stored value of int/bool reads through stack temporaries and many call
  arguments; the affected parsers (Blast, GadgetTurret, Gadget, HudHealthMeter, EnableSpawner
  defaults, IfCollected/IfJob enums, FadeDesaturation blend modes) were decoded from the
  disassembly.
* Default colours 0x01164CA0/0x01164CA8 are start-up globals; assumed opaque white/black.
  Default direction 0x01164B70 assumed (0,0,1). Default quaternion assumed identity.
* `CharacterSpecialCommand` (0xB8B01DB7): the console stores the default direction, not
  the value read (kept). A "CharSpecialCmd" hash match exists but breaks the sort order.
* `EnableDarkScreen` passes default 0 to all its getters (so a missing "Ramp" stores 0).
* `SendEvent/BroadcastEvent`: the console overwrites the entity-name payload with the
  resolved entity pointer on execution; the port keeps the name and resolves each time.
* `CloseBinoccucom` is spelled with a double "c" in the data (hash 0x7F37BF6C).
* `IfJob` releases its nested commands first-to-last (others last-to-first).
* `RemoveNavObstacle`: console handles are ref counted; raw handles here.
* The obstacle description (`NavObstacleDesc`) is a guess (transform + extents).

## Done (tagged)
Commands #1-#82 of the registration list (constructors, parsers, destructors in range,
Link, debug elements, and the Execute functions that are self-contained or need only a
named service), SendEvent/BroadcastEvent + `ResolveEventEntity` (0x0009A190), nav graph
(0x000106EC, 0x0001087C, 0x00010980, 0x000110EC, 0x000112D8), proxy points (0x00064D48,
0x00064F48), Bezier (0x00064C88).

## Not done in the range
* Execute functions marked `TODO(...)` in the command files (entity/physics heavy).
* Commands #83-#269 (0x000812C0..0x000A8000): same pattern, not started.
* 0x00010238, 0x000104C8 (player-on-navmesh tracker), 0x00010B18 (nav jump links from
  splines), 0x00017420.., the 75-slot level entity classes (vtables 0x00B0DC70..0x00B0EDA0:
  INST_* objects, glass blocks, electric orbs, jumbotron, shields, El Jefe, music notes),
  AI settings parser 0x00067738 (16 KB), AI spawn 0x00061810, anim events 0x000650B0 /
  0x00065290, AR targets 0x000A9768, loading screen 0x000ABBE0..
* 0x0007AF18 (four-field clear inside the GetEntityFlag block; owner unknown).
