// openTIT - Game helpers around anim script instances: starting a script
// bound to an object ("this"), replacing a running one, setting variables.
#pragma once

#include <cstdint>

namespace ape {
class ApeAnimScript;
class ApeAnimScriptInstance;
class ApeRefCounted;
}

namespace sly {

// New instance of `script` with its "this" variable bound to `object`
// (field24 = `objectData`). With `autoRun` it is added to the global update
// list and removed from it once finished.
ape::ApeAnimScriptInstance* StartObjectScript(ape::ApeAnimScript* script, ape::ApeRefCounted* object,
                                              uint32_t objectData, bool autoRun);
// Same with the script looked up by name in the default library.
ape::ApeAnimScriptInstance* StartObjectScript(uint32_t scriptHash, ape::ApeRefCounted* object,
                                              uint32_t objectData, bool autoRun);

// Replaces the instance held in `slot`: the old one is sent to its "end"
// state and released once finished, then a new instance of `script` is
// started (kept alive, owned by the slot). With `stopOnly` the old instance
// is stopped even when there is no new script.
ape::ApeAnimScriptInstance* ReplaceObjectScript(ape::ApeAnimScriptInstance*& slot, ape::ApeAnimScript* script,
                                                ape::ApeRefCounted* object, uint32_t objectData,
                                                bool stopOnly);
ape::ApeAnimScriptInstance* ReplaceObjectScript(ape::ApeAnimScriptInstance*& slot, uint32_t scriptHash,
                                                uint32_t libraryHash, ape::ApeRefCounted* object,
                                                uint32_t objectData, bool stopOnly);
ape::ApeAnimScriptInstance* ReplaceObjectScript(ape::ApeAnimScriptInstance*& slot, const char* scriptName,
                                                const char* libraryName, ape::ApeRefCounted* object,
                                                uint32_t objectData, bool stopOnly);

// Binds the object variable `varHash` of the instance; false if it is
// missing or not an object variable.
bool SetScriptObjectVar(ape::ApeAnimScriptInstance* instance, uint32_t varHash, ape::ApeRefCounted* object);
// Sets the int variable `varHash`; false if missing or of another type.
bool SetScriptIntVar(ape::ApeAnimScriptInstance* instance, uint32_t varHash, int32_t value);

} // namespace sly
