# Conventions

## The rule: write new code

We do **not** translate machine code or decompiler output into C++. For each function:

1. Work out what it is for: inputs, outputs, side effects, its role in the game.
2. Write fresh C++ that does the same job, the way an engineer would write it from a spec.
3. Use the original only as a reference for behaviour.

Code that reads like a lifted listing (register-named variables, emulated stack or memory,
goto chains, instruction-by-instruction structure, raw offset arithmetic on `void*`) is wrong
even if it works.

Never: forward to original addresses, embed original addresses as pointers, emulate the CPU,
or copy code from earlier recompilation efforts. Addresses appear **only in comments**.

## Tags

```cpp
// @func 0x0056A710 HashNameNoCase
uint32_t HashNameNoCase(const char* name) { ... }
```

* `@func` marks a finished reimplementation. Exactly one tag per original function.
  When one C++ function covers several identical original copies, tag each address.
* Unfinished functions get `// TODO(0x00463000 AppInit)` plus a minimal stub
  (empty body, safe default return) so the build never breaks.
* Inline code from the original that has no address of its own (inlined helpers,
  static-init guards) needs no tag.

## Naming and types

* Keep known original names (from strings, RTTI, asserts). Otherwise use descriptive names.
* Unknown struct fields keep their offset in the name: `field_1A4`. Rename them once their
  purpose is understood.
* Flag guesses with `NOTE:` comments rather than presenting them as fact.

## Platform

Game and engine code never calls an OS API directly. Console services map to `src/platform`:

| PS3 | PC layer |
|---|---|
| `sys_lwmutex_*` | `plat::Mutex` (`std::recursive_mutex`) |
| `sys_time_get_system_time`, time base | `plat::GetSystemTimeUsec`, `plat::GetTimeBase` |
| `cellSysutilRegisterCallback/CheckCallback` | `plat::RegisterSystemCallback/DispatchSystemCallbacks` |
| `cellFs*` | `plat::File` |
| `sys_process_exit` | `plat::ExitProcess` |
| statically linked SN libc | the host C/C++ standard library |

## Regions

`docs/functions.tsv` splits the executable into the game/engine code, the Bink video
middleware and Havok 2011.3 middleware. Middleware is tracked separately in the README.
