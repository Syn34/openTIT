// openTIT - Registration of the game's anim script commands.
//
// On the console the prototypes are added by the game's constructor
// (SlyGame::SlyGame, 0x0045CC18), next to the engine built-ins. These
// functions add the commands ported in this module under the same element
// name hashes; call RegisterLevelScriptCommands() once after
// ape::RegisterBuiltins().
#pragma once

namespace sly {
namespace levels {

void RegisterScriptCommandsA();
void RegisterScriptCommandsE();
void RegisterScriptCommandsEvent();
void RegisterScriptCommandsF();
void RegisterScriptCommandsG();
void RegisterScriptCommandsG2();
void RegisterScriptCommandsH();
void RegisterScriptCommandsI();
void RegisterScriptCommandsJ();

inline void RegisterLevelScriptCommands()
{
    RegisterScriptCommandsA();
    RegisterScriptCommandsEvent();
    RegisterScriptCommandsE();
    RegisterScriptCommandsF();
    RegisterScriptCommandsG();
    RegisterScriptCommandsG2();
    RegisterScriptCommandsH();
    RegisterScriptCommandsI();
    RegisterScriptCommandsJ();
}

} // namespace levels
} // namespace sly
