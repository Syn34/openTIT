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
* `script/CommandsK..Z.cpp`, `script/CommandsAA..AD.cpp` (second agent): commands #83-#269.
  `ScriptCommands.h` registers them all.
* `ScriptServices.h/.cpp`: the services those commands need (same idea as GameServices,
  each declaration names the console function/field; stubs return "missing"). The
  "Seq..." commands go through `SequenceAdd*` services: on the console their Execute
  builds a sly/gameplay sequencer command (0x0042C000..0x0043B000) and appends it with
  0x0042C138; the sequencer port should implement those services.
* `ai/AIPriorityTargets.*`: the priority target list of the level AI object (0x00F557F0).
* `PlayerNotifyFlags.*`: the named per-player notification flags (0x000AE2BC/0x000AE35C);
  `PlayerNotifyFlagIsSet` in GameServices.cpp now uses it.
* `DeferredRequests.*`: the request queue 0x00F9CDA8 filled by commands 0x83F48558 and
  0x2C1E0CC1 (0x000AB954/0x000ABA14).
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
* The sequencer port (sly/gameplay) should provide the `SequenceAdd*` functions of
  `ScriptServices.h` (or the stubs there should forward to it). Several of its class
  names (SeqHudScreenCommand for the spawner switch, SeqAmbientTintCommand for the camera
  weight, SeqTimeScaleCommand for the group volume, SeqHideHudCommand/SeqLockInputCommand
  for "PauseEntities"/"PauseHavok") look mismatched with the script names that build them.
* `PlayerNotifyFlagsOf` (PlayerNotifyFlags.cpp) and `GetAIPriorityTargets` need the game
  world / level AI object to hand out their records.
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

* Second agent (#83-#269):
  * Command names: only some are known from strings/hash matches; classes otherwise carry
    the registered hash. "NOTE: name from a hash match" as before. `GamePlayAnim` (#88) is
    a real string but breaks the alphabetical order.
  * `SplineFollowDetachCommand` (0xAFAF3B97) reads its float from the "object" attribute
    (the console passes the same hash twice).
  * `SetTargetingDurationCommand`: with "RefreshDuration" present the console stores 1.0
    instead of the value and does not set the "has refresh" bit (kept).
  * `SeqCameraWeightCommand`: the console passes default 0.0 for "Exponent" (so the
    "Curve" default is overwritten when Exponent is missing). The older
    `CameraWeightCommand` (CommandsJ) keeps the curve value; one of the two may be wrong.
  * Default globals assumed: 0x01164AF4 Vec2 (1,1), 0x01164AFC Vec2 (0,0), 0x01164B90
    light direction (0,-1,0), 0x01164CA4 marker colour transparent black, 0x01164C90
    identity quaternion. The capacity global 0x00F65C7C (QueueCutscene texts) assumed 16.
    Note that agl/AglDisplay.h lists 0x01164CA0 as (0x80,0x80,0x80,0xFF), not white.
  * Exact-copy groups: 0x0009A398 (SeqActivateMixState::Link) = 0x0006BB10, and
    0x0009CF8C / 0x000A555C (SeqEnqueueSound / SeqSetMusic Link) = 0x000740B0. They are
    separate classes, each tagged on its own Link. 0x000891E4 equals 0x004384E8 (other
    module, not tagged here).
  * `SetEntityFlagCommand` (0x06F47B91) uses its own 7-entry 64-bit flag table
    (0x00AC51A0), different from Get/SetEntityFlag's 31-name table.

## Done (tagged)
Commands #1-#82 of the registration list (constructors, parsers, destructors in range,
Link, debug elements, and the Execute functions that are self-contained or need only a
named service), SendEvent/BroadcastEvent + `ResolveEventEntity` (0x0009A190), nav graph
(0x000106EC, 0x0001087C, 0x00010980, 0x000110EC, 0x000112D8), proxy points (0x00064D48,
0x00064F48), Bezier (0x00064C88).

Second agent: commands #83-#269 (601 tags: constructors, parsers, Create, in-range
destructors, Link, debug elements and most Execute functions), priority targets
(0x0001B238/74/E8), notify flags (0x000AE2BC/0x000AE35C), deferred requests
(0x000AB954/0x000ABA14), ResolveEntityByObjectName (0x0009A284).

## Not done in the range
* Execute functions marked `TODO(...)` in the command files (entity/physics/sequence
  builders with large argument lists).
* Commands not ported at all (TODO comments): #152 SetPos (0x0008DE44..), #160
  SetSoundRegister (0x00091178 parser), #227 dialogue line parser (0x000A0558), #230 text
  panel (0x000A116C..), #232 (0x000A1CEC..), #238 (0x000A3108), #240 (0x000A3688..),
  #260 SeqSpawnEntity, #265 SyncSequence; light setting helpers 0x00090C00/0x00090D00;
  smoke cloud 0x0001B590; MsDecibel range 0x0005BA08; 0x00099FE0/0x0009A074.
* 0x00010238, 0x000104C8 (player-on-navmesh tracker), 0x00010B18 (nav jump links from
  splines), 0x00017420.., the 75-slot level entity classes (vtables 0x00B0DC70..0x00B0EDA0:
  INST_* objects, glass blocks, electric orbs, jumbotron, shields, El Jefe, music notes),
  AI settings parser 0x00067738 (16 KB), AI spawn 0x00061810, anim events 0x000650B0 /
  0x00065290, AR targets 0x000A9768, loading screen 0x000ABBE0..
* 0x0007AF18 (four-field clear inside the GetEntityFlag block; owner unknown).

## Third pass (agent3)
* `script/ScriptVars.*`: the game's eight script variable types registered by SlyGame
  (sound 13, Trigger 15, String 16, Spawner 17, SpawnerGroup 18, Laser 19, LaserField 20,
  LaserGroup 21; element name of the sound type unknown, hash 0x08D5E6EB).
  `RegisterLevelScriptCommands()` now also registers them; `EntityVarValue` reads the
  Trigger variable.
* `LevelServices.h/.cpp`: services needed by the newly ported Execute functions (same
  stub convention as GameServices/ScriptServices). Notable: laser manager lookups,
  interaction components, screen filter fades, sun light settings, attach records,
  targeting markers, track player.
* Executes ported this pass: ActivateInteraction, ObjectMaterial, AnimLayerTime,
  CameraShake, CharacterSpecial, AttachEntity, WaveDistort (+ `sly/gameplay/WaveDistortion`
  table 0x010AA394), EnableCollision/Component/Laser/SentryLight/Spawner/SpawnerGroup,
  EnqueueSound, EntityKillFx/GetCollectInfo/SetEnabled/SetScriptState/SetGadgetState/
  SetNoTarget/SetVisible, the Fade* family, Gadget/GadgetTurret, GetAnimSpeed,
  GetBinocucomVisible, GetEntityFlag, GetJobState, FxTypeQuery, GetRespawnInfo,
  GetSafeState, HudHealthMeter, HudMessage, IfJob, InputMover, MsDecibel, Kill,
  MapMaterialToVar, OverrideColor, ScanObject, AnimLayer (reverse), ScoreEvent,
  EntityStateTarget, ChainParams, SetPlayerSpawn, SetMarker, WorldTrack, SetEntitySpline,
  SetLightSetting (+ presets), SmokeBomb, ChildAnimLayer, SwapDef, SwapWeapon, DefFlags,
  SeqLink, SeqCameraCut, SeqDamage, SeqPause, SeqShowPart.
* Left as TODO (Ghidra reports bad instruction data or the argument lists of the
  sequencer constructors could not be matched reliably): Blast, Impulse, PlaySound,
  VehicleInput, ThrowPlayer, SpawnDebris, FxObjShake/FxTrail (effect descriptor blocks),
  Damage, FireProjectile, Crossfade, GamePlayAnim, SpawnEntity/Rope/Request,
  MapEntityToVar and most Seq* commands with long constructor argument lists.
* `SpawnManager`: `SpawnerDespawnAndReset` is now visible and `SpawnManager::FindGroup`
  was added for the spawner group command.

## Fourth pass (agent3)
* New commands: SetSoundRegister (#160, 0x000910F4..0x0009170C), SyncSequence (#265), the
  button QTE over actors (#238, hash 0x58A0F34C), entity state move (#240, 0xB39D2C46), dialog
  pages (#232, 0x9F8D96A1), speaker dialog (#230, 0x661993C7), the dialogue line (#227,
  0x780316BB: parser, Create, Execute) and SeqSpawnEntity (#260). New Executes: SeqPlayAnim,
  SeqRunScript, StartSequence.
* Sequencer constructors: on this ABI float arguments take a parameter slot (and skip a GPR),
  so registers loaded only to be stored to the stack area are temporaries. With that rule the
  sequencer port's argument lists of SeqDialogStream (0x0043267C) and SeqPlayClip
  (0x004321A4) match the callers positionally; SeqDialogPages (0x00433B2C) takes float
  scale/offset pairs and an int script variable as its "condition" (the gameplay port was
  retyped accordingly), and SeqAnimScript's `param` is the running instance pointer (the
  port's uint32 cannot hold it; 0 is passed).
* New services: AudioSetVoiceRegister/AudioSetGlobalRegister/AudioMusicVoice,
  SequenceAddActorQte (0x0043554C), SequenceAddSpawnEntity (0x00439E4C) — stubs until the
  audio/sequencer ports provide them.
* Still TODO: SetPos (#152, Euler/quaternion parser and two 2.6 KB Executes), SeqEnablePart /
  SeqMoveTo Execute (the sequencer port takes a WorldObject*/uint32 where the console passes
  an ape world object / point instance pointer), the AR debug server (0x000A85C0..0x000ABAD4,
  vtable 0x00B11D80, render target "RenderTarget_SlyAR"), BossGrizz (0x00060100..0x00064B8C).
