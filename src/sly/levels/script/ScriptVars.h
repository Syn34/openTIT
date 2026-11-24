// openTIT - The game's own anim script variable types.
//
// Next to the engine's variable types (ape/anim/ApeAnimScriptVars.h) the game
// registers eight more in SlyGame::SlyGame (0x0045CC18). They hold run-time
// handles that commands fill in and read back: a playing sound, a string, a
// trigger entity, a spawner or spawner group, and the three laser objects.
// Script data only gives them a "name"; their value starts out empty.
//
// Vtables 0x00B11B38..0x00B11D38 (15 slots, same layout as ApeAnimScriptVar).
// The small type/size getters and the string variable's destructors are
// shared out-of-line copies at 0x00742AB0..0x00742C6C (outside this module).
#pragma once

#include "ape/anim/ApeAnimScript.h"
#include "sly/game/AudioSystem.h"

#include <cstdint>

namespace sly {

class WorldObject;
class Spawner;
class SpawnGroup;
// Laser objects of the level's laser manager (0x00F657FC, records of 0x150,
// 0x170 and 0x70 bytes). NOTE: not ported yet; only pointers are held here.
struct Laser;
struct LaserField;
struct LaserGroup;

namespace levels {

// Variable type numbers returned by GetType().
enum SlyVarType : int {
    kVarType_Sound      = 13,
    kVarType_Trigger    = 15,
    kVarType_String     = 16,
    kVarType_Spawner    = 17,
    kVarType_SpawnGroup = 18,
    kVarType_Laser      = 19,
    kVarType_LaserField = 20,
    kVarType_LaserGroup = 21,
};

// Holds a single run-time handle of type T (all but the string variable).
// `kType` is the GetType() value; the console has one class per type with
// identical code apart from the vtable.
template <class T, int kType>
class ScriptHandleVar : public ape::ApeAnimScriptVar {
public:
    ScriptHandleVar();
    ~ScriptHandleVar() override;

    ape::AnimScriptVarType GetType() const override { return static_cast<ape::AnimScriptVarType>(kType); }
    ape::ApeAnimScriptVar* Clone() const override;
    uint32_t GetSize() const override { return 0x18; }
    ape::ApeAnimScriptVar* Create(ape::xml::XmlElement* element) const override;

    T GetValue() const { return m_value; }
    void SetValue(T value) { m_value = value; }

protected:
    T m_value{};   // +0x14
};

// A playing sound. The voice is stopped (with release) when the variable dies.
class ScriptSoundVar : public ape::ApeAnimScriptVar {
public:
    ScriptSoundVar();
    ~ScriptSoundVar() override;

    ape::AnimScriptVarType GetType() const override { return static_cast<ape::AnimScriptVarType>(kVarType_Sound); }
    ape::ApeAnimScriptVar* Clone() const override;
    uint32_t GetSize() const override { return 0x18; }
    ape::ApeAnimScriptVar* Create(ape::xml::XmlElement* element) const override;

    audio::VoiceId GetVoice() const { return m_voice; }
    void SetVoice(audio::VoiceId voice) { m_voice = voice; }

private:
    audio::VoiceId m_voice = 0;   // +0x14
};

using ScriptTriggerVar = ScriptHandleVar<WorldObject*, kVarType_Trigger>;
using ScriptSpawnerVar = ScriptHandleVar<Spawner*, kVarType_Spawner>;
using ScriptSpawnGroupVar = ScriptHandleVar<SpawnGroup*, kVarType_SpawnGroup>;
using ScriptLaserVar = ScriptHandleVar<Laser*, kVarType_Laser>;
using ScriptLaserFieldVar = ScriptHandleVar<LaserField*, kVarType_LaserField>;
using ScriptLaserGroupVar = ScriptHandleVar<LaserGroup*, kVarType_LaserGroup>;

// A string of up to 63 characters ("val").
class ScriptStringVar : public ape::ApeAnimScriptVar {
public:
    ScriptStringVar();

    ape::AnimScriptVarType GetType() const override { return static_cast<ape::AnimScriptVarType>(kVarType_String); }
    ape::ApeAnimScriptVar* Clone() const override;
    uint32_t GetSize() const override { return 0x58; }
    ape::ApeAnimScriptVar* Create(ape::xml::XmlElement* element) const override;

    // Replaces the value; null or empty strings leave it unchanged.
    void SetString(const char* text);
    const char* GetString() const { return m_text; }

private:
    char m_text[0x40] = {};   // +0x14
};

// The variable `varHash` of the instance if it exists and has the type of
// V, else null.
template <class V>
V* FindTypedVar(ape::ApeAnimScriptInstance* instance, uint32_t varHash, int type)
{
    if (varHash == 0)
        return nullptr;
    ape::ApeAnimScriptVar* var = instance->FindVar(varHash);
    if (!var || static_cast<int>(var->GetType()) != type)
        return nullptr;
    return static_cast<V*>(var);
}

// Value of a handle variable, or null when the variable is missing or of
// another type.
template <class T, int kType>
T HandleVarValue(ape::ApeAnimScriptInstance* instance, uint32_t varHash)
{
    auto* var = FindTypedVar<ScriptHandleVar<T, kType>>(instance, varHash, kType);
    return var ? var->GetValue() : T{};
}

// Adds the eight prototypes under their element names.
void RegisterScriptVars();

} // namespace levels
} // namespace sly
