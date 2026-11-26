# ape/anim notes (anim scripts, range 0x004B0000-0x004D0000)

## Layout
* `ApeAVLMap.h`: `AVL<K,I>` plus the traversal cursor (tree pointer, 20-entry stack,
  current node) that every registry embeds on the console. The cursor's static "found
  item" globals (0x011DE490, 0x011DE4A0, ...) are not reproduced.
* `ApeAnimScript.h/.cpp`: `ApeRefCounted` (vtable 0x00B11B20), variable/event/command
  base classes, `ApeAnimScriptBlock` (0x00B21C68), `ApeAnimScriptState` (0x00B21CC0),
  `ApeAnimScript` ("Director", 0x00B21C80), `ApeAnimScriptLibrary`, prototype registries.
* `ApeAnimScriptInstance.cpp`: running instances (0x8C-byte object, no vtable).
* `ApeAnimScriptVars/Events/Commands/ObjectCommands.cpp`, `ApeAnimScriptBuiltins.cpp`.

## Naming guesses
* `ApeAnimScriptBlock` and `ApeAnimScriptInstance` are invented names. The containers
  match the AVL instantiations in `container/ApeAVL.cpp` (`AVL<uint, ApeAnimScript*>`
  holds "Director" objects, so 0x00B21C80 is taken to be `ApeAnimScript`).
* Unresolved hashes are kept as constants with NOTE comments, for example 0xDF35D0E1
  (script flag: wait for children), 0x2A9C853E (StopChildScript: all), 0x6334AE6F,
  0xF5897C08, and the command names 0x17CD0663, 0xCE800E6C, 0x2EAE8975.
* Slot 11 of every variable class returns 0 and its purpose is unknown (`Unknown2C`).
* Slot 4 of commands and slot 6 of events build a debug `XmlElement` from a name
  string. The XmlElement port has no such constructor, so the versions that do it are
  TODOs. The versions that return null are ported.

## External systems (hooks)
These sit outside the range, so they are reached through function pointer hooks the
game layer can fill in:
* `AnimScriptHooks::findWorldObject`: entity lookup by name (0x005801DC, with a
  fallback through 0x007335C4).
* `AnimScriptVarHooks`: GUI node lookup (0x004D54FC), object-released notification
  (entity+0x104 component, vtable slot 6), object debug name, script clock (the
  singleton at 0x011DE094, field +0x10).
* `AnimScriptObjectHooks`: part lookup (0x0057F5D8 with callback 0x004D0B50), world
  transform (entity+0x50), default transform object (0x0073343C).

## Behaviour notes
* Warnings print only `TempHexString(hash)`. The format strings were stripped from the
  binary.
* Faithful quirks: the block destructor does not release its own commands. Object
  variable `Clone()` copies the object pointer without taking a reference. IfRange
  min/max default to 0 when the attribute is missing, because the XML getter stores its
  default. Call leaks its instance when there is no caller. UpdateAll only removes
  finished instances from the list and does not delete them.
* One change: a duplicate untimed event is released. The console AVL drops it and
  leaks it.

## Not done in the range
* 0x004B0124 (economy tuning static initialiser, another module's TU) and 0x004B00EC.
* (done, see below) The anim set / anim clip definitions.
* 0x004B6760, 0x004B9F2C (RegisterBuiltins: AttachObject, MoveObject, CallRandom and
  SetPostProc are still missing), 0x004BCE08/0x004BE61C/0x004BE75C/0x004BE850 (host
  live-reload handlers), 0x004BDDB8 (reload from the level manifest).
* The Execute() of the object commands (SwapDef, SpawnObject, SetVisible, SetFOV,
  Rumble, SetColor, SetScale, InterpScale, ProcAnim, PlayGuiScript, ...), plus
  AttachObject, MoveObject, CallRandom and SetPostProc ("FxType" parser 0x004CCF28).

## Anim sets and clips (`ApeAnimClip.h/.cpp`, 0x004B0850-0x004B6760)
* The manager (0x00F65F84) is embedded at +4 of a game object (vtable 0x00B0EF30, created
  by 0x000186CC with 256 slots, 12 control types at 0x00AC000C and 1 sync type at
  0x00AC0080). It has no vtable of its own; the string pool is its first member.
* The clip pool is a fixed array of 0x4C-byte tagged entries (helpers 0x00789280...);
  `AnimClipPool` is a plain C++ version. The set slot list is the engine handle list
  (`ApeTHandleList` gained Get/Remove/Clear for it).
* Attribute names known only by hash are constants with NOTE comments (0x57BAF55F,
  0x91EEB2F8 = transition clip of a blend rule, 0xA80F1E14 = channel map override of a
  set, 0xE98F593E, the event attributes, 0x044DDD31). Selection operators other than
  "LessThan"/"NotEqual" are named by their comparison.
* Faithful quirks: the blend clip's control value search is dead code (the console's test
  is "index < 0 && index >= count"), so the element at firstElement + index is used even
  for index -1. The sync type enum of the clip DTD is bounded by the control type count.
  Blend rule elements the reader rejects are still added (with an unset key on the
  console; 0 here). The "*" wildcard rule key is the target name alone (the console
  leaves its buffer uninitialised). After the first "AnimSet" element every following
  sibling is read as a set.
* 0x00789BC0/0x00789E00 (AnimSet::FindBlendInfo/FindBlendClassInfo) are tagged here
  although they sit in the helper range.

## agent2
* OcclusionPlane (0x004C4684) and Rumble (0x004C5D38) Execute done. The controller variable's
  +0x14 field is the pad (`ApeAnimScriptControllerVar::GetDevice`, never set from data); the
  default rumble pad (console global 0x00F65FF8, writer not found) is
  `AnimScriptObjectHooks::defaultRumbleDevice`.
* The other object commands still need a bridge from the anim-script object handle
  (`ApeRefCounted*`) to the world objects (`ape::world::ApeObj`); the sly/levels services use
  the same opaque handle with stubs, so that bridge is a cross-module decision.
