# sly/entity additions for 0x000F0000-0x00200000 (agent2, second pass)

## New files
* `CharacterMethods.cpp` + `CharacterHooks.h/.cpp`: character helpers and entity-slot
  overrides (0x0011F000-0x00141000): model variants, weapon attachment, profile costumes,
  tint/opacity, abilities, carrying, release/reset, hit reactions and damage, root motion,
  clip lookup, interaction checks, watchers, gadget swap, small state handlers.
* `RigidProp.cpp`, `RigidBodyEntities.cpp`, `PhysicsHooks.h/.cpp`: the rigid-body scripted
  entities (RigidProp, CompoundEntity, AlterEgoEntity, TargetableA/B/CBase, PushBlock).
* `ActorVariants.h/.cpp`: small actor classes (group, hit counter, watch, signal, ...).

## Findings
* Entity slots 25/26 ("render id" on Entity) hold the collision group of the physics
  body for characters and rigid-body entities: the group is written to bits 16-31 of the
  rigid body's filter word (+0x2C) and the world re-filters it (0x00930B70).
* Entity slots 27/28: the gravity factor of the physics body (an hkHalf at +0x1FE of the
  rigid body's motion); while attached, the parent's child link keeps the factor to
  restore (+0xE0 of the link, "restoreBlend" in Entity.h). Entity::Detached(blend) is
  slot 28.
* Entity slot 36 ("GetRenderObject") hands out the Havok rigid body of the physics
  component (0x00252940 is the component's body getter, not a render object).
* Entity slot 30: colour tint (amount, rgba, immediate); declared on Entity now.
* Entity slots 62-64: interaction begin(user)/cancel/end(completed); declared on Entity.
* The game object base of Entity is gameplay::StateMachine (vtable 0x00B219D0): slots
  3/6/7/8 are Stop/AdvanceState/SetState/GetState, slot 9 OnMessage; layer state data
  lives at StateMachine +0x14 (the port reaches it through hooks).
* Hit categories as characters use them: 0 physics impact, 1 melee, 2 blast,
  3 environment, 4 projectile (EntityHits.h's struct names are older guesses).
* The rigid-body entities share a 0x28-byte damage/push block in their definitions
  (`PropDamageSettings`, at +0xC4, +0xC8 for AlterEgoDef, +0xE8 for TargetableDefC);
  each class carries its own copy of the reactions on the console.
* Character profile selections live in the profile record (+0x124 weapon, +0x128 Murray,
  +0x12C Bentley, +0x130 the fourth character; costume flag bytes at +0xFC0).
* Sound actor slot names (0x00AD139C) double as state names; the slot timers (slots
  106-112) are indexed by the current state.
* Class bits: 5 disguised, 15 carrying (skin variant 4), 17 carried, 36 custom weapon,
  39 heavy carry.

## Not done
* The character constructor (0x00120A58), character swap (0x00124D58), damage
  application (0x00125A80), the message handler (0x001305C0) and the movement state
  handlers (0x00131798-0x00165198).
* Default camera mode constructor and update stages; most mode updates have AltiVec code
  the reference cannot decode.
* Definition loaders/spawns (slot 3/7 of the definitions) and placements (slot 2 of the
  secondary vtables): they need the entity constructor family from the entity module.
