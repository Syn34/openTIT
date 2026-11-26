# sly/gameplay notes for the lead

## Contents
* `Sequencer.*`: the gameplay sequencer core (0x0042AEE8-0x0042CB0C): sequences,
  the command base class, the entity takeover table, the debug tuning switches.
* `SequenceCommands.*`, `SequenceUiCommands.*`: 60 sequencer command classes
  (0x0042CC10-0x0043AE04).
* `Minigame.*`: the level's active game-mode object (game world +0x1281F4), only
  the request/intro helpers used by the sequencer (0x003A4000-0x003AAB54).
* `SequenceCommands2.*` (second batch): place entity, camera view/release,
  character actions 1-4, hit, player control, HUD screen/layer, skin attach,
  play clip, anim script, render flag, set active, anim set swap,
  SeqPlaySound, SeqDialogStream (ctor/dtor only), child-name matcher
  0x0043B02C.
* `BvpFighter.*`, `BvpTuning.*`, `StateMachine.h`: Bentley vs Penelope boss
  fight (0x003AABBC-0x003B3014): fighter base (clip pairs, damage, range
  checks), Bentley/Penelope overrides, battle object (camera blends,
  message routing) and the 240 "Mechanic.BvP" tuning variables
  (0x004227F0/0x00421474). `StateMachine.*` is the engine state machine
  base (vtable 0x00B219D0; ctor 0x0045CAA0, members 0x00464A24-0x00464D1C).
  Slot 9's default (0x007435B8, `return 1`) is a shared stub and not tagged.
* `DebugNetServer.*`: debug network server 0x00426CE0-0x004285B8.
* `PenguinMinigames.*`: the six penguin minigame actor-record destructors.
* `GameplayWorld.*`: every world/engine call the above needs, as free functions
  with TODO stubs (safe defaults). Each hook names the console member, virtual
  slot or function it stands for.

## Changes needed outside this directory
* `sly/game/SlyGame.h`: the game world members used here live in
  `gameplay::WorldFlags` until SlyGame models them: +0x28 (bit 1 HUD hidden,
  bit 2 input locked), +0x2D bit 5 (cutscene), +0x2E bits 6/7 (AI/player pause).
* `sly/game/AudioSystem.h`: `Requests()` is commented as 0x010A8C40, but every
  sequencer caller passes 0x010ACC40 as the queue. One of the two is a typo.
  `SoundQueue::FindPlaying` really takes a `SoundName*` (it compares the
  request's def with the first word of the argument); the current
  double-pointer signature gives the same result when called with `&name`,
  which is how this module calls it.
* Audio functions still missing in `sly/game` and stubbed in GameplayWorld.cpp:
  0x00449950 StopAllSounds, 0x0044D2E0 IsSoundCategoryPlaying, 0x0044D4C8
  IsTrackedVoicePlaying, 0x00449E48 PlayMusicStream, 0x004431A0 (unknown, see
  `StreamSystemIdle`).
* GameplayWorld.cpp stubs should forward to the world module (entity virtual
  slots 10/22/24/32/44/45/76/88, render node flags, components, movers), the
  camera manager (0x00F64C38 shakes/effects, 0x00F557F0 mover), the HUD/GUI
  (0x00368DB0, 0x003679C8, 0x004D5544, 0x004E2EF0/0x004E3168, 0x004890A0,
  0x00489328, 0x00489A60) and the game world blocks at +0x1282A4..+0x1283A8
  (depth of field, focus pull, exposure, colour filter) once those exist.
* The build currently fails to link because of `agl/AglResources.cpp`
  (AglMaterial ctor) and `sly/levels/script/ScriptCommand.cpp`; nothing in this
  directory is involved. All files here compile warning-free.

* Sockets: `GameplayWorld.h` has `Net*` hooks (socket/bind/listen/accept/
  send/sendto/recvfrom/close, cellNetCtl IP query) for DebugNetServer; they
  belong in `src/platform` once it has a socket layer.
* `GameplayWorld.h` hook naming: virtual slot 88 (0x160) is damage
  (amount, source, type), now `ActorDamage`; the older `EntityDespawn`
  hook names the same slot and should be merged.
* `AnimScriptBindVariables` (0x0049FCC0) is a TODO stub in
  SequenceCommands2.cpp; it belongs with the anim-script glue.

## Open TODOs in this range
* Sequencer commands still not done: 0x0042FF28 (camera look-at),
  0x00430A64 (camera orbit), 0x00432AB4/0x00432E08 (dialog stream
  Execute/Update), 0x00434E0C (wait for player look), 0x0043554C (ramp
  entities; 0x00435B30 has bad instruction data), 0x004360CC (prologue time
  machine), 0x00437F24 (rider), 0x00439224 (SetPosition), 0x00439E4C
  (spawn).
* BvP: per-state handlers 0x003AD270 / 0x003AECB0, battle phases
  0x003B0BA8 / 0x003B1E88 / 0x003B2718 (TODO stubs), 0x003AC298,
  0x003AC560, 0x003AC8C0, 0x003ABB98, 0x003ACB90 (transform math).
* Water/ripple grid module 0x004286D8-0x0042AD9C (render dependent).
* Laser groups/lasers/fields (0x003A1500-0x003A233C): the pools and object
  constructors live below 0x003A0000; not started.
* Minigames (PingPong, penguin games, SystemCracker, SparkRunner, AlterEgo),
  hideout UI, super attack clips, the debug network server (0x00426CE0-
  0x004294F8, needs a socket layer in `src/platform`), level property-class
  registrations (0x0043B450-0x0043EB48): not started.

## Uncertain identifications
* Character actions 1-4: numbered because the action ids of the scripted
  state machine are not decoded; parameter blocks are modelled as small
  structs (`CharacterStartAction` returns them).
* "BvpFighter" was first taken for a generic super-attack class; the tuning
  reads (Mechanic.BvP.*) identify it as the Bentley vs Penelope fight.
  B20398 = Bentley, B203E0 = Penelope is inferred from the tuning names
  each one reads.
* Penguin record class names are taken from the nearest game string; the
  sumo record's two arrays are kept opaque.
* BvpTuning's static init also sets up a zero vector, a NaN sentinel, a
  list head and an array (no users); not modelled.
* Command class names are descriptive guesses except where the original
  warnings name them (EnableComponent, EnablePhysics, EntitySuppressAudio,
  SeqEnqueueStream, SetEntityVisible, SetEntityUpdate, SeqPlaySound).
* `SequenceEntity` flag names (show/hide on release, render flags 10/12,
  collision) follow how RegisterSequenceEntity/RestoreSequenceEntity use them.
* The "Minigame" object at game world +0x1281F4 is the active game-mode object
  (the bartender minigame installs one); request bit names are guesses.
* SeqCameraShakeCommand copies the caller's second value into the third slot
  (the original does this too); kept on purpose.
* `FindSequenceEntity` is called with an extra flag by some commands; the
  original ignores it (it only fed a compiled-out warning).

## Mega knight data (agent2)
* `MegaKnightBehavior.*` (0x00302DA8-0x00303CEC): behaviour actions (Camera, GroundSmash,
  Laser, Pause, two movement actions and a counter known only by hash), stamps and stamp sets
  from XML, and the definition's destructor. The loader that calls the parsers (0x0030743C),
  the boss entity and the action runtime (0x00305A44.., 0x003081C8..) are not ported. Two
  start-up defaults (0x01069B38, 0x01069EF8) are assumed 0.

## Mega knight boss (agent3)
* `MegaKnight.*`, `MegaKnightPhases.cpp`: the boss entity (no constructor yet: 0x00305A44),
  its camera controller, platform, stamp manager, floor ripple, the twelve phase classes and
  the per-frame update. Calls into unported systems go through `MegaKnightHooks.*`.
* Still TODO: the constructor and the def's Spawn (0x003072A8), the action runtime
  (0x003081C8..0x0030B6D8), ChooseNextPhase (0x0030E1E0), UpdateTracking (0x0030EEE0),
  camera update (0x00304FD0), HurtPlayer (0x00309440), SetArmMarker (0x00311590), the aim IK
  callback (0x00311A60), fight start (0x0030FD68/0x003104D0), message handler (0x00310980)
  and the vector-heavy phase updates (laser, monitor/shoot platform, rope attached).
* `secondHalf` (+0x299) selects the "Phase1ArmN" behaviour lists, so its name may be
  backwards. `lookAt` (+0x3D0) is an ApproachVec3 whose target is the boss's focus point.

## Lasers (agent3, sixth pass)
* `Lasers.*`: the laser manager (0x00F657FC, pools of 32 groups / 256 lasers / 16 fields),
  the setting associations (`Sly_LaserGroup/Fixed/Anchor/Spline/Field`, installed into
  `g_saiAssociationHooks` by a static installer in Lasers.cpp), group/laser/field enable,
  flicker, end point movement along splines, beam frames, the fixed/anchor/spline set-ups,
  field set-up and joints, level start/update/unload. `LevelServices` and the goal hint hooks
  now forward to it.
* The goal hint kinds 4/5/6 were mislabelled: 4 is a laser, 5 a laser field, 6 a laser group
  (GoalHint::Apply calls 0x003992AC / 0x0039D32C / 0x00399758 for them).
* The setting type hashes: 0xA296BC08 Sly_LaserFixed, 0xA7F58081 Sly_LaserAnchor,
  0xC6CD059D Sly_LaserSpline.
* Not ported: field visuals 0x0039DA30 (materials from the 0x00F8F114 material list, outline
  segments, joints), phantoms 0x0039E6F8 (Havok + DamageTrigger), the field draw 0x0039F840,
  the player-vs-beam test 0x003A09C0 (vector code), RFireProjectile::Execute 0x00398040.
* The direction scripts' object variable holds a scene object on the console; the port's
  object variables hold engine references, so `ScriptVarObject` returns null for now.
* Both end points default to Entity.Laser.DefaultDirScriptPt1 (Pt2 is never read).

## Dragon boss (agent3, sixth pass)
* Release 0x002FEEA0, encounter reset 0x002FEA20, head targets 0x002F9AC0, phase selection
  0x002FEFB4. The head target entities (+0x3B8) are of an unidentified class (transform at
  +0x160, active bit +0x1BC); `DragonHeadTargetSet` is a hook. `DragonBossDef` models only
  the phase list (+0x130).
