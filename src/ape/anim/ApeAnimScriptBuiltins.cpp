// openTIT - Registration of the engine's built-in anim script prototypes.
#include "ape/anim/ApeAnimScript.h"

#include "ape/anim/ApeAnimScriptCommands.h"
#include "ape/anim/ApeAnimScriptEvents.h"
#include "ape/anim/ApeAnimScriptObjectCommands.h"
#include "ape/anim/ApeAnimScriptVars.h"

namespace ape {

// TODO(0x004B9F2C RegisterBuiltins) the console version also registers the
// SetPostProc command, which is
// not ported yet.
void RegisterBuiltins()
{
    AnimScriptVarMap& vars = VarPrototypes();
    vars.Add(AnimHash("hash"), new ApeAnimScriptHashVar());
    vars.Add(AnimHash("int"), new ApeAnimScriptIntVar());
    vars.Add(AnimHash("float"), new ApeAnimScriptFloatVar());
    vars.Add(AnimHash("object"), new ApeAnimScriptObjectVar());
    vars.Add(AnimHash("Matrix"), new ApeAnimScriptMatrixVar());
    vars.Add(AnimHash("vec3"), new ApeAnimScriptVec3Var());
    vars.Add(AnimHash("controller"), new ApeAnimScriptControllerVar());
    vars.Add(AnimHash("GUI"), new ApeAnimScriptGuiVar());
    vars.Add(AnimHash("TimeFunction"), new ApeAnimScriptTimeFunctionVar());
    vars.Add(AnimHash("Material"), new ApeAnimScriptMaterialVar());

    AnimScriptEventMap& events = EventPrototypes();
    events.Add(AnimHash("TimeEvent"), new ApeAnimScriptTimeEvent());
    events.Add(AnimHash("AlwaysEvent"), new ApeAnimScriptAlwaysEvent());
    events.Add(AnimHash("event"), new ApeAnimScriptGenericEvent());
    events.Add(AnimHash("RefcountEvent"), new ApeAnimScriptRefcountEvent());

    AVLMap<uint32_t, ApeAnimScriptCommand*>& commands = CommandPrototypes();
    commands.Add(AnimHash("IfEquals"), new ApeAnimScriptIfEqualsCommand());
    commands.Add(AnimHash("IfRange"), new ApeAnimScriptIfRangeCommand());
    commands.Add(AnimHash("call"), new ApeAnimScriptCallCommand());
    commands.Add(AnimHash("CallRandom"), new ApeAnimScriptCallRandomCommand());
    commands.Add(AnimHash("StopChildScript"), new ApeAnimScriptStopChildScriptCommand());
    commands.Add(AnimHash("SetFloat"), new ApeAnimScriptSetFloatCommand());
    commands.Add(AnimHash("SetInt"), new ApeAnimScriptSetIntCommand());
    commands.Add(AnimHash("ChangeState"), new ApeAnimScriptChangeStateCommand());
    commands.Add(AnimHash("SwapDef"), new ApeAnimScriptSwapDefCommand());
    commands.Add(AnimHash("SetObject"), new ApeAnimScriptSetObjectCommand());
    commands.Add(AnimHash("SpawnObject"), new ApeAnimScriptSpawnObjectCommand());
    commands.Add(AnimHash("SetVisible"), new ApeAnimScriptSetVisibleCommand());
    commands.Add(AnimHash("SetFOV"), new ApeAnimScriptSetFovCommand());
    commands.Add(0x17CD0663u, new ApeAnimScriptOcclusionPlaneCommand());
    commands.Add(0xCE800E6Cu, new ApeAnimScriptEnableObjectCommand());
    commands.Add(0x2EAE8975u, new ApeAnimScriptObjectRateCommand());
    commands.Add(AnimHash("SetDeflector"), new ApeAnimScriptSetDeflectorCommand());
    commands.Add(AnimHash("Rumble"), new ApeAnimScriptRumbleCommand());
    commands.Add(AnimHash("SetColor"), new ApeAnimScriptSetColorCommand());
    commands.Add(AnimHash("EnableProcAnim"), new ApeAnimScriptEnableProcAnimCommand());
    commands.Add(AnimHash("SetScale"), new ApeAnimScriptSetScaleCommand());
    commands.Add(AnimHash("SetMatrixVar"), new ApeAnimScriptSetMatrixVarCommand());
    commands.Add(AnimHash("InterpScale"), new ApeAnimScriptInterpScaleCommand());
    commands.Add(AnimHash("CreateProcAnim"), new ApeAnimScriptCreateProcAnimCommand());
    commands.Add(AnimHash("PlayGuiScript"), new ApeAnimScriptPlayGuiScriptCommand());
    commands.Add(AnimHash("AttachObject"), new ApeAnimScriptAttachObjectCommand());
    commands.Add(AnimHash("MoveObject"), new ApeAnimScriptMoveObjectCommand());
}

} // namespace ape
