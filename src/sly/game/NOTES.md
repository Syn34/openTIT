# sly/game notes for the lead

## Changes needed outside this directory
* `src/sly/SlyApp.h`: the app struct really runs to 0x5E40 bytes (frame stack +0xA0,
  frame time +0xA4, two 0x2EC0-byte player blocks from +0xB4 with the pad at +0xCC,
  cursors at +0x5E30, pause request +0x5E38/+0x5E3C). Until it is extended these live in
  `sly/game/AppRuntime.h` (`GetAppRuntime()`).
* `src/sly/SlyApp.cpp`: `AppEndFrame` (already tagged) should call
  `g_game->ResetFrameLists()` (0x00458B50, in `sly/game/SlyGame.cpp`) for phases 1 and 2
  instead of its TODO comment. I only changed the stub bodies (AppInit, AppShutdown) and
  added the includes they need.
* `src/sly/SlyMain.cpp` `GetFrameRing(SlyGame*)` can use a member of `sly::SlyGame`
  (`sly/game/SlyGame.h`) once the frame ring is added there.
* `sly/game/WorldInterface.cpp` holds TODO stubs for world-object accessors (positions, ids,
  alive flag, kill/release, level instance lookup, camera visibility). They should forward to
  the world module when it lands.
* `audio::BusAddRef/BusRelease` (AudioSystem.cpp) are no-ops until the engine audio bus object
  (use count at +0xB2, virtual Release slot 10) exists.
* Linking currently fails only because of `src/ape/world` (Matrix34FromBasis/Matrix34Mul are
  missing); nothing in this directory is involved.

## Open TODOs in this directory
* 0x0045CC18 SlyGame::SlyGame (25 KB) and 0x0045B858 AppTick: not done.
* 0x00452058 Spawner::SpawnOne, 0x00453A68 SpawnGroup::PickSpawner, 0x00455038 character
  spawner loader (subclass ctor at 0x00061810).
* 0x00445A40 SoundQueue::Update, 0x00447860 mix apply, 0x004480D0 / 0x00447D88 sound zone and
  mix state XML loaders, 0x004AC820 WindInstance::Update (gust part).

## Uncertain identifications
* Game event types are only known by number (InitEventNN).
* Spawner setting properties are named from use; the property descriptor globals are listed
  next to each field in `SpawnerSettings`/`SpawnGroupSettings`.
* Tween easing 2 ("Overshoot") name is a guess from its curve.
* AppShutdown calls pad virtual slot 8 with 0; read as `StopRumble(0)`.

## File streaming / effect manager (agent2)
* `FileStreaming.*` (0x002E80C0-0x002EC2F8): stream groups (packed "Streaming/<name>.ftd"
  groups from the ASTF/ASTH/ASTD table, plain-file groups from the level definition's async
  file groups), per-type callbacks and factories (`RegisterStreamType`, called from
  SlyGame::SlyGame, not ported), the character pack prefetcher. The group prerequisite
  (`StreamBlob`, +0x14) is never created here; its layout is inferred. Group type 0x63C971C0
  (kept by CancelNonCharacterGroups) has no known name. `UpdateFileStreaming` should be
  called per frame (0x0047C2B8 / 0x00491710), `ClearStreamGroups` at level unload
  (0x0047E578), `CreateStreamGroupsFromLevel` from 0x0047C8A0.
* `FxManager.*`: walkers of the effect handle list and the camera shake queue of the
  manager at 0x00F64C38. `gameplay::StartCameraShake` and the LevelServices camera shake
  stub could forward to `FxManager::AddCameraShake`. Not done: 0x002EF7B8 (argument order
  unclear), the effect classes, effect creation (0x002F0E0C), shake update (0x002F27D0).

## Job manager records and loader (agent3, 0x002D9740-0x002DE1F4)
* The console's terms are the other way round from the port's type names: the 0x35C-byte
  records (`Job`) are its episodes (<Episode>: timePeriod, start/end movies, hub and hideout
  levels, collectible weights) and the 0x218-byte records (`Episode`) its jobs (<Job>:
  jobScript, movieOnJobComplete). The names were kept to avoid churn; see JobManager.h.
* `JobRecords.cpp`: per-profile progress accessors (state bits 3-4, completed 0x40, flags
  0x80/0x100 of unknown meaning, safe state bits 0-2, current stage byte), collectible counts
  (bottles +0x34 words, masks +0x64, treasures +0x7C, job states +0x1D0 of the profile
  record), the completion formula (0x002DB040) and stage advancement. `PlayerProgress` was
  corrected: 12 episode slots (0x1A0..0x1D0), not 0x20.
* The mask (0x0106819C) and treasure (0x01068184) tables are `ApeTSortedMap<CollectibleRecord*>`;
  nothing fills them yet (their loader is outside this range). A mask counts when its bit is
  set in the mask word of the episode named by its `foundIn` field.
* `JobManagerLoad.cpp`: constructor (12 episode / 144 job records), Scripts/Episodes.xml and
  episode file parsers, movie unlockables (`Unlockable`, replacing the old stubs), movie
  entries and the entity index table (element/attribute names marked NOTE where unknown),
  job script loading into library 0x7719A548, level start. Not ported: the package streaming
  of a job (0x002D9A14-0x002DA5E0, load state +0x1FC), 0x002DDAB0 ClearGoals (still the
  `JobManagerCleanup` stub), 0x002DE258 HUD bottle counter, 0x002DE41C-0x002DEA50 goal
  selection/teleport (game world player block).
* Job texts: ShowVarText (0x00089048) stores its text in the running job and ShowText
  (0x0007AE74) reads it back (`Episode::StoreText/LoadText`); the levels services
  `TextDisplayShow*` now forward there.
* 0x00F64C24 (30) is the default clue bottle count (`g_defaultBottleCount`).
* Collectible spawning (0x002DF010 masks, 0x002DFAA0 / 0x002DFC2C treasures and safes) uses
  spawn groups 0xFB673D64 (treasures) and 0x5E69B571 (safes); their names are unknown. The
  safe table is 0x010681B4 (`SafeTable`).

## Goals: base class and type ids (agent1, 0x00769xxx-0x0076Bxxx)
* `goals/Goal.cpp`: the base constructor 0x00769BD8 now copies the full parse
  description (`GoalDesc`, laid out from what the constructor reads; the parser 0x00322D88
  that fills it is not ported) and the destructor 0x00769480 frees the same members. The
  console goal also derives from the event-listener base (vtable 0x00B21A30, ctor 0x0048F27C,
  dtor 0x0048F6B0, not ported); `GoalStopListening` stands in for its destructor.
  `JobTakeGoalSaveSlot` (GoalHooks) is the job's goal counter (Job +0x04 top 12 bits).
* `goals/GoalTypeIds.cpp`: every goal class's type id (slot 3), GoalSet, the penguin goal
  base and the health monitor / score destructors. The two goal classes with ids 0x22
  (vtable 0x00B1EDE8) and 0x23 (0x00B1EFD0) are not ported.

## Job manager and goal parser (agent3, later pass)
* `goals/GoalParser.*`: the goal XML parser 0x00322D88 (`ParseGoal`: common attributes, nine
  checkpoints, hints, `<Script>` children with their text bindings, cycling texts, factory
  table by element name), group options 0x00321D3C, script slots 0x00322AC0, GoalSet,
  GoalScore, OperationMousetrap and fishing factories. Still TODO: 0x003373E8 (class of vtable
  0x00B1EDE8), penguin montage 0x003396A8, binocucom photo 0x0032B358, shooting gallery
  0x0033DC0C, bartender 0x003292C8, QTE 0x0033A520 (their `Create*` stubs return null).
  Attribute names were recovered by hashing string pairs; the rest keep their hash.
* GoalDesc: `field_70` holds checkpoint names [0..8], respawn entities [9..17] and Y offsets
  [18..26] (floats); `checkpoints` (+0xEC) are the checkpoints' "associatedPlayer"; the four
  `blobs` are nine-bit sets (respawnAt / attachToSpline / teleportTo / attachToWall).
  `field_134` is the float "hintChance". Hint element kinds: trigger 0, Interactive 1,
  Spawner 2, SpawnerGroup 3, laser 4, LaserField 5, laserGroup 6, teleporter 7 (the enum names
  in Goal.h for 4-6 may be swapped).
* `goals/JobStreaming.cpp`: streaming of a job's goal file and script files (0x002D9A14-
  0x002DA718); `JobManager::LoadJobScripts` now reads the streamed copies.
* JobManager: ClearGoals, goal selection/placement (0x002DE41C-0x002DEDB4), job start
  bookkeeping and pending start state machine (0x002E0008-0x002E1F44, 0x002E0C38), character
  and hideout queries, completion percentages, trophies (entity found, penguins, arcade
  scores, treasures), job completion 0x002E5414, goal failure 0x002E5DD8 (wired to the goal
  hook), respawn 0x002E3AEC. The game world parts (players block, playable characters, point
  instances, state machine, level requests) are hook stubs in JobManager.cpp.
* `SlyGame` gained the player goal list (+0x10C0) and the job state word (+0x1074).
* `PlayerProgress` grew the fields these use (costume choices +0x160, arcade tables +0x260,
  unlock entries +0xD60, notification word +0x15C, counters); `Reset` (0x000ADE04) builds a new
  profile. `jobStates` holds 144 entries (it overlapped the arcade tables before).
* Not done here: HUD bottle counter 0x002DE258 and the other HUD text users (need the HudObject
  text fields 0x003632C8 / 0x00363A3C), 0x002DF198, 0x002E4040, 0x002E4750 (end-of-job state),
  0x002E4AAC, 0x002E58C8, 0x002E5C68, 0x002E70B8, 0x002E74C8, 0x002E7588, 0x002E7694.

## HUD elements (agent3, later pass, 0x0035695C-0x00381758)
* `hud/HudManager.*`: HudObject now carries the real base layout (+0x0C name .. +0xA0 state)
  plus the fields of the common element (+0xA4 text .. +0x18C state word) that the existing code
  already used; base constructor/destructor, Update (placement), Refresh, SetActive, stack offset
  slides and priority insertion (EnterStack), hide queue (0x0106C240), the show/intro/outro state
  machine (PlayState, BeginShow, ShowTimed, ShowText, PlayIntro), text field helpers, busy
  element, enable-all. `GuiIsPlaying` was really "object visible" (ApeObj flags bit 15): renamed
  `ObjectVisible`; HudObject::gui is now `object` (ape::world::ApeObj*).
* Element classes: `HudElement` (0x00B1F6B0, common element with text fields, spring and hold
  time), `HudPopup` (0x20), `HudScreen` (2), `HudPrompt` (0x40), `HudSubtitle` (0x4000, timed
  lines), `HudArElement` (0x20000). The type-bit factory 0x0035AC7C is not ported (needs the other
  seven constructors: 0x00356BA0, 0x00357DC4, 0x00358760, 0x00358B64, 0x00359284, 0x003599C4,
  0x00359E08, 0x0035A3A0).
* Definition stand-in `HudObjectDef`: several kinds reuse the same offsets for different data
  (text fields vs prompt objects, defaultFade vs the screen's hide flag); those overlays are
  separate members with NOTEs.
* `JobManager::UpdateBottleHud` (0x002DE258) uses the HUD text; the counter is "BottleCounter".
* Not done: meter elements (vtables 0x00B1F758/0x00B1F790, created by 0x00364980/0x003651F0),
  the remaining element classes' Update functions (2-5 KB each), the factory, 0x00365F28 /
  0x00366710 (HUD per-frame update), 0x00367C20, 0x00368360.. (the 0x214-state element family).
* Hooks: `hud/HudHooks.*` (world callback lists, placement query, anchor stack refresh, object
  cloning, LOD, sounds, confirm button, continue, player HUD colour).

## agent2: game messages, callback lists, audio controls
* `GameMessages.*`: game message queue (128 records, 0x010BA138) and listener registry (256
  nodes 0x010BAB38, key map 0x010BB738): Register/Unregister (0x0048FA54/0x0048F298),
  HasListeners 0x0048FC68, Post 0x004705F8, PostToListeners 0x00470660, in range 0x0048F90C,
  Dispatch 0x0048F800, `GameEventListener` (vtable 0x00B21A30, 0x0048F27C/0x0048F3F8/
  0x0048F6B0/0x0048F548). `entity::EntityMessage` is the queue record (sender, receiver, id,
  argument, toEntity). Hooks in EntityHooks/AIHooks/KnowledgeHooks/GameServices/
  ComponentHooks now post through it; SystemCracker's listener derives from GameEventListener.
  Goals and MinigameListener still use their own stubs (not converted). Nothing calls
  `DispatchGameMessages` yet (the game update 0x00491710 is not ported).
* `SlyGame` callback lists (+0x128480, 13 lists, 5..11 cleared per frame): AddCallback
  0x0046B5F8, RemoveCallback 0x0046B7A4, ForgetEntity 0x0046B6CC, RunCallbacks 0x0045900C.
  NOTE: the console calls the callback with the context in r4 and dt in f1 (r3 is not set up);
  the port calls fn(nullptr, context).
* `ObjectHelpers.*`: geometry material lookup 0x004A59B4/0x004A5A48, depth-always 0x004A5B80,
  hierarchy LOD 0x004A5C20 (+visitor 0x004B00EC), name search 0x004A5AE8 (+visitor 0x004AFCB4).
* `AudioControls.cpp`: voice registers/position/level, pause groups, output channel count, bus
  fades, category/tracked voice queries, audio heap (0x0044D880). New AudioBackend virtuals for
  the library calls; Voice gained the volume/position fields 0x5C..0xD0.
