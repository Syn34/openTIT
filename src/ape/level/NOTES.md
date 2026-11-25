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
* GUI texture booleans: 0x051B7FEC is "ztest" (depth compare LEqual instead of
  Always); 0x2E8401A3 (enables the depth test) and font flag 0x04704F4C are
  only known by hash. Element hashes: 0x447D9F5E (bitmap font element)
  unresolved; 0x0001293C is "map" (font code point list). Font set children
  0x00255A8F match "page" (only 4-letter candidate); the per-page float
  0x7A600EB7 is "tracking" (per-font extra letter spacing, added to the
  label's tracking).
* ApeGuiObject virtual slots: 2 Clone, 35 GetLayoutSize, 36 GetCenter,
  37 Invalidate, 38 Tick(dt), 39 Draw, 40 GetScreenRect, 41 GetWorldMatrix,
  42 GetWorldScale, 43 GetWorldColor, 44 HitTest, 45/46 FindProperty, 47
  SetText (labels). The earlier "Render/Update" names were slots 38/39.
* Layout: an object is placed at the anchor point of its parent's rectangle
  plus its position (scaled by the parent's world scale with inheritscale);
  its rectangle extends from that origin by -size/2 * align. render3d
  objects are projected with the render target 0x00F66850.
* GUI file (ApeGuiFile.cpp): <gui> with <assets> (scene "name", texture,
  bitmap fonts = element 0x447D9F5E, font sets = "font"), <scripts>
  (<script> with <delay>/<set>/<interp>/message-step 0xA0CED6D9 children)
  and the object tree (element 0xA7D40E5E). The scene load/destroy
  functions 0x005904EC/0x005964B4/0x00596E3C are TODO stubs in ApeScene.cpp.
* Script message steps send GUI events (SendGuiEvent): the type is the
  index of the "msg" name (5 accept, 6 cancel, 9 start, 10 inc, 11 dec,
  12 init, others only by hash); the event argument is the player. The
  former "GuiFilter" is this event handler; the four "slots" are per pad
  (focus object per pad as far as the game code shows).
* Element type 9 is "clipregion"; type 8 wraps a world object instance
  ("objName", "noclone"; vtable 0x00B22DD0): not done, it needs the world
  instance clone 0x005A05E8 and ApeObj slot 14.
* SetGuiPadFilter / SetGuiFilter / SetGuiSlot names are guesses (callers in
  the game app only install/read them).
* ApeDTD (0x004D2BB8-0x004D3378): attribute schema used by the knowledge,
  anim-script and setting systems. Type numbers match the
  DTD_ATTRIBUTE_TYPE_* strings (VECTOR2=6, VECTOR3=7, RGBA=8, CUSTOM=9).

## Not done (still open in this range)
* 0x004D00CC-0x004D0BE4: anim-script instance pool / AVL glue (0x004C8E80
  owner) - belongs with the anim-script module.
* 0x004D0C90-0x004D28F8: Bink movie player wrapper (needs Bink).
* 0x004F0000-0x004F2FC0: level scene object creation (ApeCameraDef,
  ApeGeomInstance) - render/scene dependent.
* 0x004F32F4-0x004F7C48: setting manager / setting defs / tag settings. A
  reflection framework built on hash containers at 0x0079A1C8/0x0079B0F0/
  0x0079B250 and the string pool 0x00565CC8; none of those exist yet.

## GUI pass 2 (agent1)
* Pad input: the pad filter fills GuiPadInput (buttons, edges, cursor flags
  0x30000 and position -1..1). Direction bits 1/2/4/8 = up/down/left/right
  (table 0x010C2B40), 0x40 cancel, 0x80 accept, 0x4000 start, 0x10/0x20 send
  events 7/8. Navigation needs SetGuiNavigation (globals 0x00F66880 and
  0x00F66888; their console setters were not found).
* Object flag 30 (attribute 0xFFD1EC91) marks objects that can take the pad
  focus; GUI event 3 lets the game veto a candidate.
* Screen fade: colour 0x010C2AD4, material 0x00F66854, debug mesh 0x00F6685C,
  drawn with the matrix 0x010C29E0 at depth -20 (DrawGui 0x004D4E28).
* Tickers rebuild their glyphs in Tick (slot 38) and use the label Draw.

## GUI pass 3 (agent1)
* Tile (type 6) done: attributes textureTL..textureBR (table 0x00F668A8) and "size". The
  layout picks one image (centre only), a three piece bar (middle row, or middle column when
  only TC/BC are given) or the full nine-slice grid; missing pieces borrow a mirrored piece
  (table 0x00AEF5B4, list position = mirror bits x/y). Borders keep the texture size (or half
  the box), edges/centre repeat (wrap 1) or stretch when smaller than one texture. Console
  quirk kept: the vertical bar stores the middle height in the middle piece's width.
* Type 8 done: wraps a world instance ("objName" instance or geometry definition, cloned
  with flags 5 and renamed "%s_GUIInst" unless "noclone"); its scale follows the GUI scale and
  the children's positions are mirrored in z.
