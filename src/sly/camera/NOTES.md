# sly/camera notes (range 0x000F0000-0x00200000, agent1)

## Contents
* `CameraSprings.*`: the three spring types the game camera eases with (vector, quaternion,
  scalar; 0x000F8C70-0x000F9380). Their own vtables (0x00B13988/98/A8) and destructors live
  in the helper range 0x00749xxx (not tagged).
* `CameraSettings.*`: the per-frame view (`CameraViewState`, GameCamera +0x1E0) and the
  follow-camera settings block (`CameraSettings`, 0x80 bytes; character def +0x3F0,
  vehicle def +0x1B0, character +0x4B0) with defaults/blend (0x000FD5F0-0x000FDB88).
* `CameraMode.h`: the mode base (root vtable 0x00B0DAEC). Slot meanings come from
  GameCamera's use: 4-6 update stages, 7 Activate, 8 ActivateFrom(view), 9 Deactivate,
  10 Finish, 11 ResetSettings, 12 Tick, 13 PostUpdate, 14 GetTarget, 15 DebugDraw,
  16 GetDebugText. Type ids/names: Spline 2, Fixed 3, Anim 5, Binocucom 6, Vehicle 7,
  Spotlight 8, Havok vehicle 9, Crawl 10, Default 11, Shooter 12, Ortho 13, Shoulder 14,
  Track 15, Orbit 16, Strafe 17. The GetType/GetName one-liners are at 0x00749xxx.
* `CameraModes.*`: the modes. Fully ported: Anim, Fixed (update), Ortho, Shooter, Crawl,
  Spotlight (minus constructor). Partly: Default (settings, overrides, teardown, tick),
  Shoulder, Spline (setters, finish, debug text), Binocucom, Vehicle, Havok vehicle,
  Track, Orbit, Strafe.
* `CameraHooks.*`: TODO stubs for everything outside (entity slots not declared on
  Entity, Havok phantoms/entities, post effect parameters, pads, anim controllers).
* `sly/entity/GameCamera.*`: the camera entity: mode stack (push/remove with blends),
  blend update, view application (render target view/fov, DOF and colour filter),
  additive camera animation, split-screen viewports.

## Changes needed outside this directory
* `sly/entity/Entity.h`: `UpdateTransform` now takes the frame time (the console passes it
  through; GameCamera's override uses it).
* `Entity` +0xD4 is named `cullDistance`, but the camera multiplies its frame time by it;
  0x000DAB90 is probably SetTimeScale rather than SetCullDistance.
* Entity virtual slots used here but not declared on `Entity`: 8 (GetState), 9 (state
  event), 19 (GetName), 21 (SetWorldTransform), 39 (focus point), 49-51 (PlayAnim*),
  59 (anim event), 71.
* `sly/gameplay/GameplayWorld.h` calls camera modes "GameCamera" (TopCamera/NextCamera/
  CameraType/KillCamera); they are `camera::CameraMode` here and the camera entity is
  `entity::GameCamera`.
* `sly/objects/TriggerCamera*` (agent3) creates Fixed modes inline (0x001DAAF8) and can
  use `camera::FixedCameraMode` / `GameCamera::PushMode`.

## Not done
* GameCamera constructor 0x000F9854 (builds two ApeCameraDef instances, anim controller).
* Mode updates: Default stages (0x00101CC0, 0x00102BE8, 0x00104508, 0x00104F80...),
  Shoulder 0x00107890, Spline ctor/update 0x00108AF8/0x00109B38, Binocucom
  0x0010CF90/0x0010D258, Vehicle 0x0010EDD8/0x0010F990, Track 0x001139C0, Orbit
  0x00114650, Strafe 0x00115FB0 (bad instruction data), Spotlight ctor 0x00111FE0 (bad
  instruction data), 0x000FE050 (bad instruction data).
