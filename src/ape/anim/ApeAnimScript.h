// openTIT - Ape engine anim scripts (object scripts).
//
// Anim scripts drive cut scenes, set dressing and gameplay props from data.
// A script library ("Scripts" XML document) holds named scripts
// ("Director" elements). Every script block owns:
//   * variables  - typed values (int, float, hash, object reference, ...)
//   * events     - command lists fired by name, every frame ("always"),
//                  by condition, or at a time inside a state
//   * states     - nested blocks with a duration; an instance is always in
//                  one state and advances its clock every update
//   * commands   - actions (set a variable, change state, call a script ...)
//
// Element names are resolved through prototype registries: the XML element
// name hash selects a prototype variable, event or command, whose Create()
// builds a new object from the element. Game code registers its own
// prototypes next to the engine built-ins (RegisterBuiltins()).
//
// A running script is an ApeAnimScriptInstance: it clones the script's
// variables and the current state (so per-instance clocks and variable
// values never touch the shared definition) and can own child instances
// started by the Call command.
#pragma once

#include "ape/anim/ApeAVLMap.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ape {

namespace file { class File; }
namespace xml {
class XmlDocument;
class XmlElement;
}

class ApeAnimScript;
class ApeAnimScriptCommand;
class ApeAnimScriptEvent;
class ApeAnimScriptInstance;
class ApeAnimScriptLibrary;
class ApeAnimScriptState;
class ApeAnimScriptVar;

// Compile-time HashNameNoCase() for the element and attribute names used here.
constexpr uint32_t AnimHash(const char* name)
{
    uint32_t hash = 0;
    for (; *name; ++name) {
        int c = (signed char)*name;
        if (c >= 'a' && c <= 'z')
            c -= 'a' - 'A';
        hash = hash * 31u + (uint32_t)c;
    }
    return hash;
}
static_assert(AnimHash("state") == 0x04B8CC71u, "AnimHash must match HashNameNoCase");

// ---------------------------------------------------------------------------
// Intrusively reference counted engine object. The count starts at 1 and the
// object deletes itself when Release() drops it to zero.
// NOTE: this base is shared by many engine classes (entities, components...).
class ApeRefCounted {
public:
    ApeRefCounted() = default;
    virtual ~ApeRefCounted();
    virtual void Release();

    void AddRef() { ++m_refCount; }
    int16_t GetRefCount() const { return m_refCount; }

    ApeRefCounted(const ApeRefCounted&) = delete;
    ApeRefCounted& operator=(const ApeRefCounted&) = delete;

protected:
    int16_t m_refCount = 1;
};

// Replaces a counted reference held in `slot`.
template <class T>
inline void AssignRef(T*& slot, T* value)
{
    if (value)
        value->AddRef();
    if (slot)
        slot->Release();
    slot = value;
}

// ---------------------------------------------------------------------------
// Variables

enum class AnimScriptVarType : int {
    Float = 1,
    Int = 2,
    Vec3 = 3,
    Matrix = 4,
    Object = 5,
    Controller = 6,
    Material = 7,
    Gui = 10,
    TimeFunction = 11,
    Hash = 12,
};

class ApeAnimScriptVar : public ApeRefCounted {
public:
    ApeAnimScriptVar() = default;
    ~ApeAnimScriptVar() override;

    virtual bool IsNumber() const { return false; }
    // @func 0x00793D54 ApeAnimScriptVar::IsReference
    virtual bool IsReference() const { return false; }
    // @func 0x00742AB0 ApeAnimScriptVar::GetFloat
    virtual float GetFloat() const { return 0.0f; }
    // @func 0x00742AC0 ApeAnimScriptVar::GetInt
    virtual int32_t GetInt() const { return 0; }
    // @func 0x00742AC8 ApeAnimScriptVar::GetHash
    virtual uint32_t GetHash() const { return 0; }
    virtual AnimScriptVarType GetType() const = 0;
    // Copy used when an instance or a state clone takes its own variables.
    virtual ApeAnimScriptVar* Clone() const = 0;
    // Debug text of the value; returns `buffer` (at least 64 bytes).
    virtual char* ToString(char* buffer) const;
    // NOTE: purpose unknown; every variable class returns 0.
    // @func 0x00742AE0 ApeAnimScriptVar::Unknown2C
    virtual int Unknown2C() const { return 0; }
    // The owning instance is being destroyed.
    // @func 0x00742AE8 ApeAnimScriptVar::OnOwnerDestroyed
    virtual void OnOwnerDestroyed() {}
    virtual uint32_t GetSize() const = 0;
    // Prototype factory: builds a new variable from its XML element.
    virtual ApeAnimScriptVar* Create(xml::XmlElement* element) const = 0;

    uint32_t GetNameHash() const { return m_nameHash; }
    void SetNameHash(uint32_t hash) { m_nameHash = hash; }

protected:
    uint32_t m_nameHash = 0;
};

// ---------------------------------------------------------------------------
// Commands

class ApeAnimScriptCommand : public ApeRefCounted {
public:
    ApeAnimScriptCommand();
    ~ApeAnimScriptCommand() override;

    // @func 0x0073B8EC ApeAnimScriptCommand::Execute
    virtual void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) {}
    // Debug-display element describing the command (null when not provided).
    // @func 0x0073B8F0 ApeAnimScriptCommand::CreateDebugElement
    virtual xml::XmlElement* CreateDebugElement() const { return nullptr; }
    // Called once after a whole library has been loaded.
    // @func 0x0073B8F8 ApeAnimScriptCommand::Link
    virtual void Link() {}
    // Prototype factory: builds a new command from its XML element.
    // @func 0x0073B8FC ApeAnimScriptCommand::Create
    virtual ApeAnimScriptCommand* Create(xml::XmlElement* element) const { return nullptr; }
};

using AnimScriptCommandList = std::vector<ApeAnimScriptCommand*>;

// Creates commands for every child of `element` that names a registered command.
void LoadCommandList(AnimScriptCommandList& list, xml::XmlElement* element);
// Executes every command of the list.
void ExecuteCommandList(const AnimScriptCommandList& list, ApeAnimScriptInstance* instance,
                        ApeAnimScriptState* state);
// Releases every command and empties the list (last first).
void ReleaseCommandList(AnimScriptCommandList& list);

// ---------------------------------------------------------------------------
// Events

class ApeAnimScriptEvent : public ApeRefCounted {
public:
    explicit ApeAnimScriptEvent(uint32_t nameHash = 0) : m_nameHash(nameHash) {}
    ~ApeAnimScriptEvent() override;

    // Time inside the state at which a timed event fires.
    // @func 0x007941B0 ApeAnimScriptEvent::GetTime
    virtual float GetTime() { return m_time; }
    // @func 0x007941B8 ApeAnimScriptEvent::IsTimed
    virtual bool IsTimed() const { return false; }
    // Polled every update for untimed events; true runs the commands.
    // @func 0x007941C0 ApeAnimScriptEvent::Check
    virtual bool Check(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) { return false; }
    // @func 0x007941C8 ApeAnimScriptEvent::CreateDebugElement
    virtual xml::XmlElement* CreateDebugElement() const { return nullptr; }
    // @func 0x007941D0 ApeAnimScriptEvent::Create
    virtual ApeAnimScriptEvent* Create(xml::XmlElement* element) const { return nullptr; }

    // Adds the commands found among the children of `element`.
    void LoadCommands(xml::XmlElement* element);
    void AddCommand(ApeAnimScriptCommand* command) { m_commands.push_back(command); }
    void Execute(ApeAnimScriptInstance* instance, ApeAnimScriptState* state) const
    {
        ExecuteCommandList(m_commands, instance, state);
    }

    const AnimScriptCommandList& GetCommands() const { return m_commands; }
    uint32_t GetNameHash() const { return m_nameHash; }

protected:
    AnimScriptCommandList m_commands;
    float m_time = 0.0f;
    uint32_t m_nameHash = 0;
};

// ---------------------------------------------------------------------------
// Script blocks

using AnimScriptVarMap = AVLMap<uint32_t, ApeAnimScriptVar*>;
using AnimScriptEventMap = AVLMap<uint32_t, ApeAnimScriptEvent*>;
using AnimScriptTimedEventMap = AVLMap<float, ApeAnimScriptEvent*>;
using AnimScriptStateMap = AVLMap<uint32_t, ApeAnimScriptState*>;

// Common part of scripts and states: named container of variables, events,
// states and commands parsed from one XML element.
// NOTE: the class name is not known; scripts and states both derive from it.
class ApeAnimScriptBlock : public ApeRefCounted {
public:
    // Builds the block from `element` (an empty block if either pointer is null).
    // `sourceName` names the data file for diagnostics.
    ApeAnimScriptBlock(const char* sourceName, xml::XmlDocument* document, xml::XmlElement* element);
    ~ApeAnimScriptBlock() override;

    // Lets every command of the block (and of nested states) resolve references.
    virtual void Link();

    uint32_t GetNameHash() const { return m_nameHash; }
    // Number of commands in the block, its events and its states (recursive).
    uint32_t CountCommands() const;
    uint32_t GetStateCount() const;
    uint32_t GetVarCount() const;
    uint32_t GetEventCount() const;

    ApeAnimScriptState* FindState(uint32_t nameHash);
    AnimScriptVarMap* GetVars() const { return m_vars.get(); }
    AnimScriptEventMap* GetEvents() const { return m_events.get(); }
    AnimScriptTimedEventMap* GetTimedEvents() const { return m_timedEvents.get(); }
    AnimScriptStateMap* GetStates() const { return m_states.get(); }

protected:
    AnimScriptVarMap& Vars();
    AnimScriptEventMap& Events();
    AnimScriptTimedEventMap& TimedEvents();
    AnimScriptStateMap& States();

    void AddVar(ApeAnimScriptVar* var);
    void AddEvent(ApeAnimScriptEvent* event);
    void AddState(ApeAnimScriptState* state);

    uint32_t m_nameHash = 0;
    std::unique_ptr<AnimScriptVarMap> m_vars;
    std::unique_ptr<AnimScriptEventMap> m_events;           // untimed, by name
    std::unique_ptr<AnimScriptTimedEventMap> m_timedEvents; // by fire time
    std::unique_ptr<AnimScriptStateMap> m_states;
    AnimScriptCommandList m_commands;
};

class ApeAnimScriptState : public ApeAnimScriptBlock {
public:
    ApeAnimScriptState(const char* sourceName, xml::XmlDocument* document, xml::XmlElement* element);
    ~ApeAnimScriptState() override;

    // Per-instance copy: own variables (object references re-resolved for
    // `instance`), shared events, fresh clock.
    ApeAnimScriptState* CloneForInstance(ApeAnimScriptInstance* instance) const;

    // Polls the untimed events, fires the timed events crossed during `dt`
    // and advances (or loops) the clock.
    void Update(float dt, ApeAnimScriptInstance* instance);
    // Fires the timed events with from <= time <= to, skipping the event
    // fired last.
    void FireTimedEvents(float from, float to, ApeAnimScriptInstance* instance);
    bool HasEvent(ApeAnimScriptInstance* instance, uint32_t eventHash);
    void FireEvent(ApeAnimScriptInstance* instance, uint32_t eventHash);

    float GetTime() const { return m_time; }
    float GetDuration() const { return m_duration; }
    bool IsLooping() const { return m_loop; }
    bool IsHolding() const { return m_hold; }
    bool IsDone() const { return m_done; }

private:
    ApeAnimScriptState();

    float m_time = 0.0f;
    float m_duration = 0.0f;   // "Duration", at least the latest timed event
    float m_lastDelta = 0.0f;
    ApeAnimScriptEvent* m_lastTimedEvent = nullptr;
    bool m_loop = false;       // "Loop"
    bool m_hold = false;       // "Hold": stay at the end instead of finishing
    bool m_done = false;
    bool m_isInstance = false; // created by CloneForInstance
};

// A named script of a library ("Director" element).
class ApeAnimScript : public ApeAnimScriptBlock {
public:
    ApeAnimScript(const char* sourceName, xml::XmlDocument* document, xml::XmlElement* element);
    ~ApeAnimScript() override;

    bool IsAutoRun() const { return m_autoRun; }
    // NOTE: attribute name unresolved (hash 0xDF35D0E1, default true). When
    // set, an instance only counts as finished once its children are.
    bool WaitsForChildren() const { return m_waitForChildren; }

private:
    bool m_autoRun = false;          // "AutoRun"
    bool m_waitForChildren = true;
};

// ---------------------------------------------------------------------------
// Libraries

class ApeAnimScriptLibrary {
public:
    explicit ApeAnimScriptLibrary(uint32_t nameHash = 0) : m_nameHash(nameHash) {}
    ~ApeAnimScriptLibrary();

    // Adds every "Director" of a "Scripts" document. A script with the name
    // of an existing one replaces it. Returns the first script loaded.
    ApeAnimScript* LoadFromXml(const char* sourceName, xml::XmlDocument* document);
    ApeAnimScript* LoadFile(const char* path);
    ApeAnimScript* LoadFromStream(file::File& file);

    ApeAnimScript* FindScript(uint32_t nameHash) { return m_scripts.Find(nameHash); }
    void RemoveScript(uint32_t nameHash);
    void Clear();
    void LinkAll();

    uint32_t GetNameHash() const { return m_nameHash; }
    AVLMap<uint32_t, ApeAnimScript*>& Scripts() { return m_scripts; }

    // Returns the library with that name, creating it on first use.
    static ApeAnimScriptLibrary* Get(uint32_t nameHash);
    static void Destroy(uint32_t nameHash);
    static void DestroyAll();
    // Looks the script up in library `libraryHash`, then (if `searchAll`)
    // in every library.
    static ApeAnimScript* FindScriptAnywhere(uint32_t scriptHash, uint32_t libraryHash, bool searchAll);

private:
    uint32_t m_nameHash;
    AVLMap<uint32_t, ApeAnimScript*> m_scripts;
};

// ---------------------------------------------------------------------------
// Instances

// NOTE: the class name is not known ("%s_inst" names spawned objects).
class ApeAnimScriptInstance {
public:
    explicit ApeAnimScriptInstance(ApeAnimScript* script);
    ~ApeAnimScriptInstance();

    ApeAnimScriptInstance(const ApeAnimScriptInstance&) = delete;
    ApeAnimScriptInstance& operator=(const ApeAnimScriptInstance&) = delete;

    // Variable lookup: current state, then globals, then the instance's own.
    ApeAnimScriptVar* FindVar(uint32_t nameHash);
    // Object referenced by the object variable `nameHash`. With `fromWorld`
    // the name is looked up among the world's objects instead.
    ApeRefCounted* ResolveObject(uint32_t nameHash, bool fromWorld);
    // World object with that name (through AnimScriptHooks), not counted.
    static ApeRefCounted* FindWorldObject(uint32_t nameHash);
    // field_24 of the object variable `nameHash`, 0 if there is none.
    uint32_t GetObjectVarField24(uint32_t nameHash);

    bool SetState(ApeAnimScriptState* state);
    bool SetState(uint32_t stateHash);
    bool SetStateAndUpdate(ApeAnimScriptState* state);
    bool SetStateAndUpdate(uint32_t stateHash);
    // Requests a state change at the end of the next update.
    void RequestState(uint32_t stateHash) { m_pendingState = stateHash; }

    void Update(float dt);
    void SendEvent(uint32_t eventHash);
    bool IsFinished(bool ignoreChildren) const;

    void AddChild(ApeAnimScriptInstance* child) { m_children.push_back(child); }
    // Destroys the children running the script `scriptHash`.
    void StopChildren(uint32_t scriptHash);
    void StopAllChildren();
    // First child running the script `scriptHash`.
    ApeAnimScriptInstance* FindChild(uint32_t scriptHash) const;

    ApeAnimScript* GetScript() const { return m_script; }
    ApeAnimScriptState* GetState() const { return m_state; }
    AnimScriptVarMap& Vars() { return m_vars; }
    const std::vector<ApeAnimScriptInstance*>& Children() const { return m_children; }

    bool IsActive() const { return m_active; }
    void SetActive(bool active) { m_active = active; }
    void SetRemoveWhenFinished(bool remove) { m_removeWhenFinished = remove; }
    void SetKeepAlive(bool keep) { m_keepAlive = keep; }

    // Creates an instance of a script of the default library and adds it to
    // the global update list.
    static ApeAnimScriptInstance* Create(uint32_t scriptHash);
    static void Register(ApeAnimScriptInstance* instance);
    static void UpdateAll(float dt);
    static std::vector<ApeAnimScriptInstance*>& All();
    static bool s_paused;

private:
    void Init();

    ApeAnimScript* m_script = nullptr;
    ApeAnimScriptState* m_state = nullptr;
    AnimScriptVarMap m_vars;
    std::vector<ApeAnimScriptInstance*> m_children;
    uint32_t m_pendingState = 0;
    bool m_finished = false;            // state ran out
    bool m_active = true;
    bool m_removeWhenFinished = false;  // drop from the global list once finished
    bool m_keepAlive = false;           // never reports finished
};

// ---------------------------------------------------------------------------
// Registries

// Global variables visible to every instance.
AnimScriptVarMap& GlobalVars();
// Prototypes keyed by XML element name hash.
AnimScriptVarMap& VarPrototypes();
AVLMap<uint32_t, ApeAnimScriptCommand*>& CommandPrototypes();
AnimScriptEventMap& EventPrototypes();

ApeAnimScriptVar* CreateVar(xml::XmlElement* element);
ApeAnimScriptCommand* CreateCommand(xml::XmlElement* element);
bool IsCommandElement(xml::XmlElement* element);
// Creates the event and loads its commands.
ApeAnimScriptEvent* CreateEvent(xml::XmlElement* element);

// Registers the engine's built-in variable, event and command prototypes.
void RegisterBuiltins();

// Level files: handlers of the director chunks (DIRF/DIRH/DIRD), and a reload
// of the current level's director documents into the default library.
void RegisterDirectorChunkHandlers();
void ReloadLevelDirectors();

// Hooks into systems outside the script module.
struct AnimScriptHooks {
    // World object with the given name hash (borrowed reference) or null.
    ApeRefCounted* (*findWorldObject)(uint32_t nameHash) = nullptr;
};
AnimScriptHooks& GetAnimScriptHooks();

// Name hash of an element (0 for an invalid handle).
uint32_t ElementHash(const xml::XmlElement* element);

} // namespace ape
