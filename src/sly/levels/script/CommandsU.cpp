// openTIT - Game script commands, nineteenth block (SpawnEntity to the HUD stop command).
#include "sly/levels/script/ScriptCommand.h"
#include "sly/levels/script/ScriptCommands.h"

#include "sly/levels/GameServices.h"
#include "sly/levels/ScriptServices.h"

#include <cstring>
#include <vector>

namespace sly {
namespace levels {

namespace {
constexpr uint32_t kAttrObject = AnimHash("object");
constexpr uint32_t kAttrRefObject = AnimHash("RefObject");
constexpr uint32_t kAttrSpawnPt = AnimHash("SpawnPT");
constexpr uint32_t kAttrOffsetPos = AnimHash("OffsetPos");
constexpr uint32_t kAttrSpawn30 = 0xBF28AC66u;      // NOTE: name unknown (vector)
constexpr uint32_t kAttrOffsetRot = AnimHash("OffsetRot");
constexpr uint32_t kAttrSpawnAngular = 0x29033A69u; // NOTE: name unknown (degrees, stored in radians)
constexpr uint32_t kAttrPart = AnimHash("part");
constexpr uint32_t kAttrPartVar = AnimHash("PartVar");
constexpr uint32_t kAttrEntityParams = AnimHash("EntityParams");
constexpr uint32_t kAttrEntityDefName = AnimHash("EntityDefName");
constexpr uint32_t kAttrMotionParams = AnimHash("MotionParams");
constexpr uint32_t kAttrCountMin = AnimHash("CountMin");
constexpr uint32_t kAttrCountMax = AnimHash("CountMax");
constexpr const char* kAttrEntityName = "entityName";
constexpr uint32_t kAttrImpulse = AnimHash("impulse");
constexpr uint32_t kAttrImpulseRand = AnimHash("ImpulseRand");
constexpr uint32_t kAttrImpulseOffset = 0x76D0D5A8u;  // NOTE: name unknown (vector)
constexpr uint32_t kAttrDebrisSpin = 0x425E8628u;     // NOTE: name unknown (degrees, stored in radians)
constexpr uint32_t kElementChunk = AnimHash("Chunk");
constexpr uint32_t kAttrDef = AnimHash("def");
constexpr uint32_t kAttrChunk04 = 0x63BBEE91u;        // NOTE: name unknown
constexpr uint32_t kAttrRopeDef = AnimHash("RopeDef");
constexpr uint32_t kAttrStart = AnimHash("Start");
constexpr uint32_t kAttrEnd = AnimHash("end");
constexpr uint32_t kAttrRope1C = 0x03FCC956u;  // NOTE: names unknown for the next four
constexpr uint32_t kAttrRope20 = 0xBFA06A67u;
constexpr uint32_t kAttrRope24 = 0x230121E0u;
constexpr uint32_t kAttrRope28 = 0x02F0137Bu;
constexpr uint32_t kAttrStartPart = AnimHash("StartPart");
constexpr uint32_t kAttrEndPart = AnimHash("EndPart");
constexpr uint32_t kAttrStartOffset = AnimHash("StartOffset");
constexpr uint32_t kAttrEndOffset = AnimHash("endOffset");
constexpr uint32_t kAttrRopeLength = 0xE97A4F3Au;  // NOTE: name unknown (float)
constexpr const char* kAttrMsg = "msg";
constexpr uint32_t kAttrHudObj = AnimHash("HudObj");
constexpr uint32_t kAttrAnimMode = 0xFF457B34u;     // NOTE: name unknown
constexpr uint32_t kAttrAnimRate = 0xFF478E71u;     // NOTE: name unknown
constexpr uint32_t kAttrDisplayTime = 0xE690CE6Fu;  // NOTE: name unknown
constexpr uint32_t kAttrHudText28 = 0xC79F1CEEu;    // NOTE: name unknown
constexpr uint32_t kAttrForceStart = AnimHash("ForceStart");
constexpr uint32_t kAttrSoundGroup = AnimHash("SoundGroup");
constexpr uint32_t kAttrExclusivityGroup = AnimHash("ExclusivityGroup");
constexpr uint32_t kAttrStopSound14 = 0x06FA4B5Fu;  // NOTE: name unknown
constexpr uint32_t kAttrEntity = AnimHash("Entity");
constexpr uint32_t kAttrChild = AnimHash("Child");
constexpr uint32_t kAttrLayer = AnimHash("layer");
constexpr uint32_t kAttrHudStop10 = 0x2A1E8C78u;    // NOTE: name unknown
constexpr uint32_t kAttrForceStop = AnimHash("ForceStop");

constexpr float kDegToRad = 0.017453292f;
const ape::Vec3 kZero = {0.0f, 0.0f, 0.0f, 0.0f};          // NOTE: console global 0x01164B40
const ape::Vec4 kIdentityQuat = {0.0f, 0.0f, 0.0f, 1.0f};  // NOTE: console global 0x01164C90

ape::Vec3 ScaledVec(const ape::Vec3& v, float scale)
{
    return {v.x * scale, v.y * scale, v.z * scale, v.pad};
}

void CopyName(char* dest, size_t size, const char* source)
{
    std::strncpy(dest, source, size - 1);
    dest[size - 1] = '\0';
}
} // namespace

// ---------------------------------------------------------------------------
// "SpawnEntity" (NOTE: name from a hash match)

class SpawnEntityCommand : public ApeAnimScriptCommand {
public:
    SpawnEntityCommand();
    explicit SpawnEntityCommand(XmlElement* element);
    ~SpawnEntityCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;                 // +0x0C "object"
    uint32_t m_refObject = 0;              // +0x10 "RefObject"
    uint32_t m_spawnPt = 0;                // +0x14 "SpawnPT"
    ape::Vec3 m_offsetPos = kZero;         // +0x20 "OffsetPos"
    ape::Vec3 m_vector30 = kZero;          // +0x30 attribute 0xBF28AC66
    ape::Vec4 m_offsetRot = kIdentityQuat; // +0x40 "OffsetRot" (degrees in data)
    ape::Vec3 m_angular = kZero;           // +0x50 attribute 0x29033A69 (radians)
    uint32_t m_part = 0;                   // +0x60 "part"
    uint32_t m_partVar = 0;                // +0x64 "PartVar"
    uint32_t m_entityParams = 0;           // +0x68 "EntityParams"
    uint32_t m_entityDefName = 0;          // +0x6C "EntityDefName"
    int32_t m_countMin = 1;                // +0x70 "CountMin"
    int32_t m_countMax = 1;                // +0x74 "CountMax"
    uint32_t m_motionParams = 0;           // +0x78 "MotionParams"
    char m_entityName[0x40] = {};          // +0x7D "entityName"
};

// @func 0x00093054 SpawnEntityCommand::SpawnEntityCommand
SpawnEntityCommand::SpawnEntityCommand() = default;

// @func 0x0073F770 SpawnEntityCommand::~SpawnEntityCommand
// @func 0x0073F7A0 SpawnEntityCommand::~SpawnEntityCommand (deleting)
SpawnEntityCommand::~SpawnEntityCommand() = default;

// @func 0x00093150 SpawnEntityCommand::SpawnEntityCommand
SpawnEntityCommand::SpawnEntityCommand(XmlElement* element)
{
    element->GetHash(kAttrObject, &m_object, 0);
    element->GetHash(kAttrRefObject, &m_refObject, 0);
    element->GetHash(kAttrSpawnPt, &m_spawnPt, m_spawnPt);
    element->GetVec3(kAttrOffsetPos, &m_offsetPos.x, &kZero.x);
    element->GetVec3(kAttrSpawn30, &m_vector30.x, &kZero.x);
    ape::Vec3 rotation = kZero;
    element->GetVec3(kAttrOffsetRot, &rotation.x, &kZero.x);
    m_offsetRot = QuatFromEulerDegrees(rotation);
    element->GetVec3(kAttrSpawnAngular, &m_angular.x, &kZero.x);
    m_angular = ScaledVec(m_angular, kDegToRad);
    element->GetHash(kAttrPart, &m_part, 0);
    element->GetHash(kAttrPartVar, &m_partVar, 0);
    element->GetHash(kAttrEntityParams, &m_entityParams, 0);
    element->GetHash(kAttrEntityDefName, &m_entityDefName, 0);
    element->GetHash(kAttrMotionParams, &m_motionParams, 0);
    int32_t count = 1;
    element->GetInt(kAttrCountMin, &count, 1);
    if (count >= 0)
        m_countMin = count;
    element->GetInt(kAttrCountMax, &count, 1);
    if (count >= m_countMin)
        m_countMax = count;
    if (const char* name = element->GetString(kAttrEntityName))
        CopyName(m_entityName, sizeof(m_entityName), name);
}

// @func 0x000935B4 SpawnEntityCommand::Create
ApeAnimScriptCommand* SpawnEntityCommand::Create(XmlElement* element) const
{
    return new SpawnEntityCommand(element);
}

// TODO(0x00093624 SpawnEntityCommand::Execute) builds the spawn transform from the spawn
// point / object / part and spawns CountMin..CountMax entities of EntityDefName.

// ---------------------------------------------------------------------------
// "SpawnDebris" (NOTE: name from a hash match): spawns physics chunks.

class SpawnDebrisCommand : public ApeAnimScriptCommand {
public:
    SpawnDebrisCommand();
    explicit SpawnDebrisCommand(XmlElement* element);
    ~SpawnDebrisCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    // One "Chunk" child (0x30 bytes on the console).
    struct Chunk {
        uint32_t def = 0;                    // +0x00 "def"
        uint32_t hash04 = 0;                 // +0x04 attribute 0x63BBEE91
        uint32_t part = 0;                   // +0x08 "part"
        ape::Vec3 offsetPos = kZero;         // +0x10 "OffsetPos"
        ape::Vec4 rotation = kIdentityQuat;  // +0x20 "OffsetRot" (degrees in data)
    };

    uint32_t m_refObject = 0;          // +0x0C "RefObject"
    float m_impulse = 0.0f;            // +0x10 "impulse"
    float m_impulseRand = 0.0f;        // +0x14 "ImpulseRand"
    ape::Vec3 m_impulseOffset = kZero; // +0x20 attribute 0x76D0D5A8
    ape::Vec3 m_spin = kZero;          // +0x30 attribute 0x425E8628 (radians)
    std::vector<Chunk> m_chunks;       // +0x40 array of 0x30-byte records
};

// @func 0x00093C80 SpawnDebrisCommand::SpawnDebrisCommand
SpawnDebrisCommand::SpawnDebrisCommand()
{
    m_chunks.reserve(8);
}

// @func 0x00093D50 SpawnDebrisCommand::SpawnDebrisCommand
SpawnDebrisCommand::SpawnDebrisCommand(XmlElement* element)
{
    m_chunks.reserve(8);
    element->GetHash(kAttrRefObject, &m_refObject, 0);
    element->GetFloat(kAttrImpulse, &m_impulse, 0.0f);
    element->GetFloat(kAttrImpulseRand, &m_impulseRand, 0.0f);
    element->GetVec3(kAttrImpulseOffset, &m_impulseOffset.x, &kZero.x);
    element->GetVec3(kAttrDebrisSpin, &m_spin.x, &kZero.x);
    m_spin = ScaledVec(m_spin, kDegToRad);

    for (XmlElement* child = element->FirstChildElement(kElementChunk); child;
         child = child->NextSiblingElement(kElementChunk)) {
        Chunk chunk;
        if (child->GetHash(kAttrDef, &chunk.def, 0) != ape::xml::kQuery_Ok)
            continue;
        child->GetHash(kAttrPart, &chunk.part, 0);
        child->GetVec3(kAttrOffsetPos, &chunk.offsetPos.x, &kZero.x);
        ape::Vec3 rotation = kZero;
        child->GetVec3(kAttrOffsetRot, &rotation.x, &kZero.x);
        child->GetHash(kAttrChunk04, &chunk.hash04, 0);
        chunk.rotation = QuatFromEulerDegrees(rotation);
        m_chunks.push_back(chunk);
    }
}

// @func 0x000941FC SpawnDebrisCommand::~SpawnDebrisCommand
// @func 0x00094264 SpawnDebrisCommand::~SpawnDebrisCommand (deleting)
SpawnDebrisCommand::~SpawnDebrisCommand() = default;

// @func 0x000942EC SpawnDebrisCommand::Create
ApeAnimScriptCommand* SpawnDebrisCommand::Create(XmlElement* element) const
{
    return new SpawnDebrisCommand(element);
}

// TODO(0x00094378 SpawnDebrisCommand::Execute) "Can't find system group for entity with RB
// '%s'...": spawns each chunk's entity and applies impulse/spin through Havok.

// ---------------------------------------------------------------------------
// "SpawnRope" (NOTE: name from a hash match)

class SpawnRopeCommand : public ApeAnimScriptCommand {
public:
    SpawnRopeCommand();
    explicit SpawnRopeCommand(XmlElement* element);
    ~SpawnRopeCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_ropeDef = 0;           // +0x0C "RopeDef"
    uint32_t m_object = 0;            // +0x10 "object"
    uint32_t m_start = 0;             // +0x14 "Start"
    uint32_t m_end = 0;               // +0x18 "end"
    uint32_t m_hash1C = 0;            // +0x1C attribute 0x03FCC956
    uint32_t m_hash20 = 0;            // +0x20 attribute 0xBFA06A67
    uint32_t m_hash24 = 0;            // +0x24 attribute 0x230121E0
    uint32_t m_hash28 = 0;            // +0x28 attribute 0x02F0137B
    uint32_t m_startPart = 0;         // +0x2C "StartPart"
    uint32_t m_endPart = 0;           // +0x30 "EndPart"
    ape::Vec3 m_startOffset = kZero;  // +0x40 "StartOffset"
    ape::Vec3 m_endOffset = kZero;    // +0x50 "endOffset"
    float m_length = 0.0f;            // +0x60 attribute 0xE97A4F3A (NOTE: not set by the constructor)
    char m_entityName[0x40] = {};     // +0x64 "entityName"
};

// @func 0x000951C0 SpawnRopeCommand::SpawnRopeCommand
SpawnRopeCommand::SpawnRopeCommand() = default;

// @func 0x0073F7F8 SpawnRopeCommand::~SpawnRopeCommand
// @func 0x0073F828 SpawnRopeCommand::~SpawnRopeCommand (deleting)
SpawnRopeCommand::~SpawnRopeCommand() = default;

// @func 0x00095270 SpawnRopeCommand::SpawnRopeCommand
SpawnRopeCommand::SpawnRopeCommand(XmlElement* element)
{
    element->GetHash(kAttrRopeDef, &m_ropeDef, 0);
    element->GetHash(kAttrObject, &m_object, m_object);
    element->GetHash(kAttrStart, &m_start, m_start);
    element->GetHash(kAttrEnd, &m_end, m_end);
    element->GetHash(kAttrRope1C, &m_hash1C, m_hash1C);
    element->GetHash(kAttrRope20, &m_hash20, m_hash20);
    element->GetHash(kAttrRope24, &m_hash24, m_hash24);
    element->GetHash(kAttrRope28, &m_hash28, m_hash28);
    element->GetHash(kAttrStartPart, &m_startPart, m_startPart);
    element->GetHash(kAttrEndPart, &m_endPart, m_endPart);
    element->GetVec3(kAttrStartOffset, &m_startOffset.x, &m_startOffset.x);
    element->GetVec3(kAttrEndOffset, &m_endOffset.x, &m_endOffset.x);
    element->GetFloat(kAttrRopeLength, &m_length, m_length);
    if (const char* name = element->GetString(kAttrEntityName))
        CopyName(m_entityName, sizeof(m_entityName), name);
}

// @func 0x00095508 SpawnRopeCommand::Create
ApeAnimScriptCommand* SpawnRopeCommand::Create(XmlElement* element) const
{
    return new SpawnRopeCommand(element);
}

// TODO(0x00095580 SpawnRopeCommand::Execute) "Rope_%s": creates the rope entity between the
// two attach points.

// ---------------------------------------------------------------------------
// Hash 0x34F5E762 (name unknown): shows a text on a HUD object.

class HudShowTextCommand : public ApeAnimScriptCommand {
public:
    HudShowTextCommand();
    ~HudShowTextCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    char m_message[0x81] = {};   // +0x0C "msg"
    uint32_t m_hudObj = 0;       // +0x90 "HudObj"
    uint32_t m_animMode = 0;     // +0x94 attribute 0xFF457B34
    float m_animRate = -1.0f;    // +0x98 attribute 0xFF478E71
    float m_displayTime = -1.0f; // +0x9C attribute 0xE690CE6F
    bool m_flagA0 = false;       // +0xA0 attribute 0xC79F1CEE
    bool m_forceStart = true;    // +0xA1 "ForceStart"
};

// @func 0x00095F10 HudShowTextCommand::HudShowTextCommand
HudShowTextCommand::HudShowTextCommand() = default;

// @func 0x0073F880 HudShowTextCommand::~HudShowTextCommand
// @func 0x0073F8B0 HudShowTextCommand::~HudShowTextCommand (deleting)
HudShowTextCommand::~HudShowTextCommand() = default;

// @func 0x00095F78 HudShowTextCommand::Create
ApeAnimScriptCommand* HudShowTextCommand::Create(XmlElement* element) const
{
    auto* command = new HudShowTextCommand();
    const char* message = element->GetString(kAttrMsg);
    if (message && *message)
        CopyName(command->m_message, 0x80, message);
    element->GetHash(kAttrHudObj, &command->m_hudObj, command->m_hudObj);
    element->GetHash(kAttrAnimMode, &command->m_animMode, command->m_animMode);
    element->GetFloat(kAttrAnimRate, &command->m_animRate, command->m_animRate);
    element->GetFloat(kAttrDisplayTime, &command->m_displayTime, command->m_displayTime);
    element->GetBool(kAttrHudText28, &command->m_flagA0, command->m_flagA0);
    element->GetBool(kAttrForceStart, &command->m_forceStart, command->m_forceStart);
    return command;
}

// @func 0x00096130 HudShowTextCommand::Execute
void HudShowTextCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    void* hud = FindHudObject(m_hudObj);
    if (!hud)
        return;
    if (m_animMode != 0)
        HudSetAnimMode(hud, m_animMode);
    HudSetAnimRate(hud, m_animRate);
    HudShowText(hud, m_message[0] ? m_message : nullptr, m_displayTime, m_forceStart, m_flagA0);
}

// ---------------------------------------------------------------------------
// "StopSound" family (hash 0xE7024843, name unknown): stops a voice, a sound
// group or every sound.

class StopSoundsCommand : public ApeAnimScriptCommand {
public:
    StopSoundsCommand();
    ~StopSoundsCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_soundGroup = 0;        // +0x0C "SoundGroup"
    uint32_t m_exclusivityGroup = 0;  // +0x10 "ExclusivityGroup"
    bool m_flag14 = false;            // +0x14 attribute 0x06FA4B5F
};

// @func 0x000961F0 StopSoundsCommand::StopSoundsCommand
StopSoundsCommand::StopSoundsCommand() = default;

// @func 0x0073F908 StopSoundsCommand::~StopSoundsCommand
// @func 0x0073F938 StopSoundsCommand::~StopSoundsCommand (deleting)
StopSoundsCommand::~StopSoundsCommand() = default;

// @func 0x00096248 StopSoundsCommand::Create
ApeAnimScriptCommand* StopSoundsCommand::Create(XmlElement* element) const
{
    auto* command = new StopSoundsCommand();
    element->GetHash(kAttrSoundGroup, &command->m_soundGroup, 0);
    element->GetHash(kAttrExclusivityGroup, &command->m_exclusivityGroup, command->m_exclusivityGroup);
    element->GetBool(kAttrStopSound14, &command->m_flag14, command->m_flag14);
    return command;
}

// @func 0x00096334 StopSoundsCommand::Execute
void StopSoundsCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    if (m_exclusivityGroup != 0)
        AudioStopTrackedVoice(m_exclusivityGroup);
    else if (m_soundGroup != 0)
        AudioStopCategory(m_soundGroup, m_flag14);
    else
        AudioStopAll();
}

// ---------------------------------------------------------------------------
// Hash 0xF5C11B05 (name unknown): acts on an animation layer of an entity's child.

class ChildAnimLayerCommand : public ApeAnimScriptCommand {
public:
    ChildAnimLayerCommand();
    ~ChildAnimLayerCommand() override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_object = 0;  // +0x0C "object"
    uint32_t m_entity = 0;  // +0x10 "Entity"
    uint32_t m_child = 0;   // +0x14 "Child"
    uint32_t m_layer = 0;   // +0x18 "layer"
};

// @func 0x000963A0 ChildAnimLayerCommand::ChildAnimLayerCommand
ChildAnimLayerCommand::ChildAnimLayerCommand() = default;

// @func 0x000963F0 ChildAnimLayerCommand::~ChildAnimLayerCommand
// @func 0x00096420 ChildAnimLayerCommand::~ChildAnimLayerCommand (deleting)
ChildAnimLayerCommand::~ChildAnimLayerCommand() = default;

// @func 0x00096478 ChildAnimLayerCommand::Create
ApeAnimScriptCommand* ChildAnimLayerCommand::Create(XmlElement* element) const
{
    auto* command = new ChildAnimLayerCommand();
    element->GetHash(kAttrObject, &command->m_object, 0);
    element->GetHash(kAttrEntity, &command->m_entity, command->m_entity);
    element->GetHash(kAttrChild, &command->m_child, command->m_child);
    element->GetHash(kAttrLayer, &command->m_layer, command->m_layer);
    return command;
}

// TODO(0x0009658C ChildAnimLayerCommand::Execute) entity/child lookup then 0x00254814.

// ---------------------------------------------------------------------------
// Hash 0xF05E4B22 (name unknown): stops a HUD object's animation.

class HudStopCommand : public ApeAnimScriptCommand {
public:
    HudStopCommand();
    ~HudStopCommand() override;
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) override;
    ApeAnimScriptCommand* Create(XmlElement* element) const override;

private:
    uint32_t m_hudObj = 0;    // +0x0C "HudObj"
    bool m_flag10 = false;    // +0x10 attribute 0x2A1E8C78
    bool m_forceStop = true;  // +0x11 "ForceStop"
};

// @func 0x000966D8 HudStopCommand::HudStopCommand
HudStopCommand::HudStopCommand() = default;

// @func 0x0073F990 HudStopCommand::~HudStopCommand
// @func 0x0073F9C0 HudStopCommand::~HudStopCommand (deleting)
HudStopCommand::~HudStopCommand() = default;

// @func 0x00096730 HudStopCommand::Create
ApeAnimScriptCommand* HudStopCommand::Create(XmlElement* element) const
{
    auto* command = new HudStopCommand();
    element->GetHash(kAttrHudObj, &command->m_hudObj, 0);
    element->GetBool(kAttrHudStop10, &command->m_flag10, command->m_flag10);
    element->GetBool(kAttrForceStop, &command->m_forceStop, command->m_forceStop);
    return command;
}

// @func 0x00096820 HudStopCommand::Execute
void HudStopCommand::Execute(ApeAnimScriptInstance*, ApeAnimScriptState*)
{
    HudStopByName(m_hudObj, m_flag10, m_forceStop);
}

// ---------------------------------------------------------------------------

void RegisterScriptCommandsU()
{
    auto& prototypes = ape::CommandPrototypes();
    prototypes.Add(AnimHash("SpawnEntity"), new SpawnEntityCommand());
    prototypes.Add(AnimHash("SpawnDebris"), new SpawnDebrisCommand());
    prototypes.Add(AnimHash("SpawnRope"), new SpawnRopeCommand());
    prototypes.Add(0x34F5E762u, new HudShowTextCommand());
    prototypes.Add(0xE7024843u, new StopSoundsCommand());
    prototypes.Add(0xF5C11B05u, new ChildAnimLayerCommand());
    prototypes.Add(0xF05E4B22u, new HudStopCommand());
}

} // namespace levels
} // namespace sly
