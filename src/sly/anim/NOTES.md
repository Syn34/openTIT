# sly/anim notes (agent3, 0x0034A398-0x00354C18)

## Contents
* `AnimHavok.h` / `AnimHooks.cpp`: stand-ins for the Havok Animation members the game reads
  (hkaAnimation duration/type/annotation count, hkaAnimationBinding, hkaDefaultAnimationControl
  local time/speed/weights/ease state/loop counts, hkaAnimatedSkeleton controls and skeleton,
  hkaSkeleton bone names) and TODO stubs for the Havok calls (step, add/remove control, sample,
  annotations, control/pose/animated skeleton creation, removeReference).
* `AnimLayer.*`: animation layers (vtable 0x00B1F558) and blend layers (0x00B1F570): playback
  queries and setters, secondary controls, instant ease-outs, done tests, bone weight lists,
  Update (step, finish latch, pruning of eased-out controls - one per update like the console -,
  annotation events, pose sampling), binding to a skeleton.
* `AnimController.*`: the controller (vtable 0x00B1F548): constructor (global list, LOD phase),
  FindLayer, AddLayer/AddBlendLayer, layers from anim trees, distance/cull LOD decision and layer
  update, skeleton selection, Play on the first layer, event listeners, the clip / tree / blend map /
  skeleton / bone weight list registries.
* `sly/entity/EntityHooks.cpp` anim hooks now forward here. Their old names are kept but do
  not match: AnimSetWeight = playback speed (0x003541E0), AnimSetPaused = AnimLayer::Stop
  (0x00353CC0), AnimStop = AnimLayer::IsDone (0x00353EB0), AnimIsFinished = layer flag bit 27
  (which the layer code treats as "looping").

## Not done
* 0x00350958 AnimLayer::Play: the second half uses instructions the disassembler cannot decode
  (stub). Everything that plays clips (AnimController::Play, HUD state clips) reaches it.
* 0x0034BF9C controller destructor (only leaves the global list), 0x0034F690 BindSkeleton
  (per-bone objects/buffers), 0x0034D1A8 BuildPose (blending, bone overrides), 0x0034E4C8,
  0x0034A1FC/0x0034A3D4/0x0034A5E4.. (registry loading), 0x00351B60-0x003525A0 (IK, called from
  the IK component), 0x00352B08.. helpers, 0x00354C18/0x00355308.. (other layer kinds).
* 0x00341A78-0x003482F8 and 0x00355080-0x003557B8 are the Havok physics world glue (phantoms,
  listeners, wake-up), not animation; untouched.

## Uncertain
* Flag names of the layer word (+0x0C) and the controller word (+0x28) follow their use.
* Float arguments: some callers skip a GPR for a float argument, others do not; argument order
  of the ported signatures follows the field each value lands in.

## Files, streams and registration (agent3, later pass)
* `AnimStream.*`: the stream readers handed to Havok's loader: the IFF chunk stream (vtable
  0x00B1F608, Read 0x003555BC), the buffered chunk reader (0x00B1F480, ctor 0x0034640C,
  forwarding slots 0x003476A8..0x003479EC) and the game file reader (0x00B1F338, slots
  0x00347B04..0x00347EA0; built in place by its users). `HkStreamReader` is a stand-in for
  Havok's stream interface (slots 4-12). The buffered reader itself (0x007DFEF8) is a hook; the
  port reads unbuffered.
* Chunk handlers of the animation files: three resource kinds (skeletons, animations, skeleton
  maps), each with a group, a name and a data chunk (0x0034B4F4..0x0034BC7C); the animations
  group can name a separate file (LoadAnimationFile 0x0034B2C8). RegisterAnimSkeletons
  0x0034A5E4 is ported; the animation (0x00348F04) and skeleton map (0x0034AB90) registrations
  are TODO. LoadSkeletonMapFile (0x0034AF14) never closes its file (console behaviour).
* BuildHavokBoneMap 0x0034B0F8: ape skeleton bone -> Havok bone index map ("xxx:" prefixes
  are stripped from Havok bone names, as in BindSkeleton).
* AnimController: destructor 0x0034BF9C and BindSkeleton 0x0034F690 are ported. The helper
  arrays +0x60..+0x70 only have their record sizes (0xE0 / 0x20 / ? / 0x140 / ?); nothing in
  the port fills them. BuildPose 0x0034D1A8 (pose blending, IK) is still TODO.
