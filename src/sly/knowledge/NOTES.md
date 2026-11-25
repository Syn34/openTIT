# sly/knowledge notes (agent2, 0x0038AD7C-0x00398ED0)

* `Knowledge.h`: data model of the AI knowledge system. The console keeps a list as one
  block (0x98-byte header, 0x84-byte entries, variable table, timer table, 0x20-byte runtime
  area); the port keeps the same parts as members of `KnowledgeData` and the console offsets
  in comments. KData blocks keep values and slot maps in separate arrays (they share memory
  on the console).
* `KnowledgeCore.cpp`: pool, type registry, list loading (XML), cloning, freeing, library,
  module life cycle, 'KNOF'/'KNOH'/'KNOD' chunk handlers. `AIHooks.cpp` Knowledge* stubs now
  forward here. `CreateKnowledgeSystem`/`DestroyKnowledgeSystem` should be called from the
  AI manager constructor/teardown (0x000186CC / 0x00018B30, not ported).
* `KnowledgeTypes.cpp`: the two type tables, the per-type factories and runtime allocators
  (0x007755E4-0x0077A950) and the runtime-data allocator templates (0x007737A4-0x00773998
  causes, 0x007742DC-0x00774D2C results, one instantiation per type with runtime data).
  Still TODO there: 0x00773A90/0x00773E8C are the library's handle-list pool Init/Grow
  (the port keeps the library in vectors; Erase 0x00773C8C is a tagged ApeTHandleList copy) and 0x00774E1C-0x00775550 are
  RFireProjectile field getters used only by its Execute (0x00398040).
* `KnowledgeCauses.cpp`, `KnowledgeResults.cpp`: per-type callbacks. Still TODO:
  RCameraShake (0x003958C8), RSetPosition (0x003967B0), RStartMeleeAttack::Execute
  (0x0039724C), RFireProjectile (0x00397970/0x00397D8C/0x00398040).
* `KnowledgeHooks.*`: outside calls as stubs (entity/character/AI fields, director
  scripts, sound playback, pad rumble, frame time, current level definition).
* Uncertain: the rule runtime (0x00254210, other range) is what evaluates causes and runs
  results; slot names of the type tables come from how the loader calls them. Container kinds
  3/4/5 are only known by hash. The variable loader stores 0 instead of the parsed value (as on
  the console). In the container loader the console tests a not-yet-written field; the port
  tests for an unknown result type instead.
* Untouched here: 0x00398DAC, 0x00398E64, 0x00398ED0 (laser module set-up?).

* Later pass (agent3): RCameraShake, RSetPosition (puts the owner back at its world object's
  start transform), RStartMeleeAttack::Execute (attack sighting through the hook
  `AIAddMeleeAttack`, 0x0001B8C0 in sly/levels/ai), RFireProjectile Link/Clone and
  `KnowledgeLibrary::Remove` (0x00398DAC; the port also drops the list from the template map).
  RFireProjectile::Execute (0x00398040, bad instruction data) and its field getters stay TODO.
