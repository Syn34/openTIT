# ape/level notes (range 0x004D0000-0x004F8400)

## Requests for other modules
* `EngineConfig` (ape/core/ApeSystem.h) lacks the floats at +0x070/+0x074
  (light noRescanDist/noUpdateDist defaults; SystemInit forces 40/75 when 0,
  otherwise clamps to >= 0). ApeLevelDef.cpp keeps them as
  `g_levelNoRescanDistDefault` / `g_levelNoUpdateDistDefault` until then.
  Fields +0x0A1/+0x0A2/+0x0A3 are used by the combinedSan/SceneFile handlers.
* The engine's stricmp wrapper 0x00555D70 (-> libc 0x00723F2C) is not in
  ApeString; `level_detail::EqualsNoCase` (LevelUtil.h) stands in for it.
  Also untagged and used here: 0x00551540 (sprintf wrapper), 0x00555CF4
  (snprintf wrapper), 0x005566A8 (strcat wrapper).

## Identifications / uncertainties
* Level loader: tag-handler tree (ApeLevelDefXMLLoader) keyed by element name
  hash; all handlers of 0x004EC730 are reimplemented. ApeLevelDef keeps its
  lists in std containers instead of ApeTArray/ApeTSortedMap (the +0x230
  string-list map is a std::map).
* 0x004F32A4 is the module's static initialiser (zeroes the two AVL trees);
  covered by the C++ globals, not tagged.
* GUI resource path variable 0x243084BD ("$<var>" replaced by the language
  folder string at 0x00F668A4): the variable's spelling is unknown.
* GUI texture booleans 0x051B7FEC / 0x2E8401A3 and font flag 0x04704F4C are
  only known by hash. Element hashes: 0x447D9F5E (bitmap font element),
  0x0001293C (font glyph map child) unresolved.
* ApeGuiObject is a skeleton (tree + name + text id). The real class derives
  from a large engine base (vtable 0x00B22710, slots at 0x00761Bxx).
* ApeGuiTexture::Load/Unload (0x004D5EAC / 0x004D5F38, sampler setup
  0x004D5C64) are TODO: they need the render texture manager (0x0050C398,
  0x0051E68C, 0x00523B7C).
* SetGuiPadFilter / SetGuiFilter / SetGuiSlot names are guesses (callers in
  the game app only install/read them).
* ApeDTD (0x004D2BB8-0x004D3378): attribute schema used by the knowledge,
  anim-script and setting systems. Type numbers match the
  DTD_ATTRIBUTE_TYPE_* strings (VECTOR2=6, VECTOR3=7, RGBA=8, CUSTOM=9).

## Not done (still open in this range)
* 0x004D00CC-0x004D0BE4: anim-script instance pool / AVL glue (0x004C8E80
  owner) - belongs with the anim-script module.
* 0x004D0C90-0x004D28F8: Bink movie player wrapper (needs Bink).
* 0x004D35AC.., 0x004D3798/0x004D39B0 (GUI shutdown), 0x004D4xxx-0x004EAxxx:
  GUI object classes, GUI anims (list 0x00F6689C), anim playback list
  (0x00F668A0), GUI file parsing (0x004DE0F8, 0x004DFA60, 0x004DFD04),
  bitmap fonts (0x004D601C, 0x004D6768) - need the GUI object base class
  and the renderer.
* 0x004F0000-0x004F2FC0: level scene object creation (ApeCameraDef,
  ApeGeomInstance) - render/scene dependent.
* 0x004F32F4-0x004F7C48: setting manager / setting defs / tag settings. A
  reflection framework built on hash containers at 0x0079A1C8/0x0079B0F0/
  0x0079B250 and the string pool 0x00565CC8; none of those exist yet.
