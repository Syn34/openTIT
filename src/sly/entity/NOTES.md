# sly/entity notes (range 0x000B0000-0x00180000)

## Contents
* `Entity.h/.cpp`: the game entity base class (console vtable 0x00B12550): global entity
  lists (0x01013B34 all, 0x01013B48/5C/70 secondary), lookups (by name, nearest, nearest of a
  definition, from a world object via the "pEntity" user tag), components (+0xF8), attached
  children (+0x154, 0xF0-byte records), visibility/flags, render id, cull distance, bounds,
  skeleton bones, sounds (+0x100 slots), animation queries through the controller (+0xA4),
  interaction slots 76-78, suspend/resume (slots 60/61), component template replacement
  (0x01056F04).
* `ComponentAccessors.cpp`: one-line typed component getters 0x000D935C-0x000D9614.
* `EntityDefs.cpp`: definition map (0x01013B04), spawn helpers, instance naming, deferred
  entity event queue (0x01013B84).
* `EntityHooks.h/.cpp`: every call out of this range (game object base 0x00464xxx,
  component factory 0x002635E8, anim controller 0x0034xxxx/0x0035xxxx, cull manager,
  camera position, geometry instance bytes, game world players) as TODO stubs.
* `Abilities.*`: the 107-entry character ability table (0x00AC9A18 + masks 0x00ACA980),
  FindAbility / AbilityHash / AbilityControlMask.
* `Awards.*`: award-name -> trophy table loaded from the "Awards" XML (0x000B2280) and the
  trophy count/points queries (0x000B2814-0x000B2A20).
* `SaveMessages.*`: cross-save refusal message (0x000B00FC).
* `GameCamera.*`: camera view copy, target stack queries, anim pause.
* `Players.*`, `EntityRefList.*`: small helpers (0x0016B59C, 0x000B4524, 0x000B566C).

## Changes needed outside this directory
* `sly/levels/GameServices.cpp`: `FindEntity`, `EntityFromWorldObject`, `EntityFindChild`
  (0x000DB290), `EntityDetach` (0x000D72C0), `PlayerBinocucom` (0x000D94CC) and
  `AwardTrophyByName` (0x000B27B4) can forward to `sly::entity::*` / `sly::AwardTrophyByName`.
* `sly/gameplay/GameplayWorld.cpp`: `EntityActiveSkin` (0x000D94F4), `EntityTargetingGadget`
  (0x000D949C) likewise; `CameraStopAnim` (0x000FCFDC) is `GameCamera::SetAnimPaused`.
* `sly/trophy/TrophyManager.h`: needs a registered-trophy count accessor (console +0xC8,
  valid while +0x08 is set); `Awards.cpp` uses the award table size instead.
* `sly/save`: the save-data utility callbacks 0x000B1310-0x000B1E6C (list/status/file/done
  callbacks of cellSaveData) and the TUS cross-save workers 0x000B0D4C/0x000B0FF8 have no PC
  equivalent beyond what `SaveGame.cpp` already does; not tagged. The refusal code global
  0x00F9D1BC is a parameter of `SaveRefusalMessage`.
* `src/sly/LoadingScreen.cpp` stubs (0x000AC0F0 / 0x000AC478) were left as they are: the
  loading screen thread draws through the renderer and plays MUS_Load.bnk; it needs the
  render/audio layers first.

## Uncertain identifications
* Component type names: only Character (2), Skin (0x13), Targeting (0x18), Gadget (0x1A),
  Interaction (0x1B), Carry (0x1C), Center (0x1F) come from callers; the rest keep numbers.
* Entity flag names (+0xAC/+0xB0) are from use; kEnt_Suspended (bit 2) and kEnt_Hidden
  (bit 17) are guesses from the slot 60/61/55 code.
* Slot 26/25 "render id" (+0xCC): written into the render object's +0x2C bits 16-31 and
  re-sorted there (0x00930B70) on the console; that part is not reproduced.
* `GetBounds` (slot 40) world path uses `AabbTransform` of the definition box by the
  object's world matrix (0x005772E4), local path scales the definition box.
* Ability grade hashes: Bronze/Gold match strings; 0x91C9C70D (30 points) read as Silver and
  0x9E63B22C (0 points) as Platinum. The award grade attribute name (0xCEBBEB87) is unknown.
* 0x00353748 / 0x00353780 read as animation length / current time (from the normalised-time
  code); 0x00353CC0 as "pause".
* `InteractionComponent` fields (+0x44 users, +0x58 player mask) and the player slot in the
  entity flags' top byte (bits 24-29) are inferred from the interaction slots.

## Not done (largest pieces)
* Entity slot 10 Release (0x000D74C0), UpdateChildTransforms (0x000D7A00), UpdateTransform
  (0x000D85A4), entity constructor/loader (0x000CA038, 0x000CC920), collision objects
  ("colObjName", 0x000C4020/0x000C7104), targeting flags parsers (0x000C85A8, 0x001668B4,
  0x001763F0, 0x0017CD54), rope/trigger/sound instances (0x000D06B0-0x000D5F58).
* Character AI control (0x00120A58), character loader (0x0011BD90), climbing/attach/land
  state handlers (0x00130000-0x00166000), alter egos (0x0016BF00, 0x00173390, 0x00179460),
  mine laser field tutorial (0x000B6208, 0x000B6C50), camera modes (0x00100000-0x00119C00).
