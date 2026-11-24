# sly/gameplay notes for the lead

## Contents
* `Sequencer.*`: the gameplay sequencer core (0x0042AEE8-0x0042CB0C): sequences,
  the command base class, the entity takeover table, the debug tuning switches.
* `SequenceCommands.*`, `SequenceUiCommands.*`: 60 sequencer command classes
  (0x0042CC10-0x0043AE04).
* `Minigame.*`: the level's active game-mode object (game world +0x1281F4), only
  the request/intro helpers used by the sequencer (0x003A4000-0x003AAB54).
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

## Open TODOs in this range
* Sequencer commands not done (engine-heavy): 0x0042CCD4/0x0042CE48 (spawn at
  rotation), 0x0042D20C, 0x0042D464, 0x0042D8BC, 0x0042DAB4, 0x0042E3AC,
  0x0042E618, 0x0042E868, 0x0042F588, 0x0042FACC, 0x0042FF28 (camera look-at),
  0x004306F4, 0x00430A64, 0x00431748, 0x004321A4 (play anim with blend),
  0x0043267C (dialog stream), 0x004342DC (SeqPlaySound), 0x00434E0C,
  0x0043554C, 0x004360CC (prologue time machine), 0x00437F24, 0x00438348,
  0x004390AC, 0x00439224 (SetPosition), 0x00439E4C (spawn), 0x0043A84C.
* Laser groups/lasers/fields (0x003A1500-0x003A233C): the pools and object
  constructors live below 0x003A0000; not started.
* Minigames (PingPong, penguin games, SystemCracker, SparkRunner, AlterEgo),
  hideout UI, super attack clips, the debug network server (0x00426CE0-
  0x004294F8, needs a socket layer in `src/platform`), level property-class
  registrations (0x0043B450-0x0043EB48): not started.

## Uncertain identifications
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
