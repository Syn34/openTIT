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
