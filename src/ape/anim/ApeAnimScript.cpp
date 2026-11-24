// openTIT - Ape engine anim scripts: blocks, states, scripts and libraries.
#include "ape/anim/ApeAnimScript.h"

#include "ape/anim/ApeAnimScriptVars.h"

#include "ape/core/ApeString.h"
#include "ape/file/ApeFile.h"
#include "ape/xml/ApeXml.h"

#include <algorithm>

namespace ape {

namespace {

constexpr uint32_t kElemState = AnimHash("state");
constexpr uint32_t kElemScripts = AnimHash("Scripts");
constexpr uint32_t kElemDirector = AnimHash("Director");
constexpr uint32_t kAttrDuration = AnimHash("Duration");
constexpr uint32_t kAttrLoop = AnimHash("Loop");
constexpr uint32_t kAttrHold = AnimHash("Hold");
constexpr uint32_t kAttrAutoRun = AnimHash("AutoRun");
constexpr uint32_t kAttrWaitForChildren = 0xDF35D0E1u;  // NOTE: name unknown
constexpr uint32_t kUnknownName = AnimHash("Unknown");

// The engine prints a warning here; only the hex formatting of the
// offending name survived in the binary.
void WarnName(uint32_t nameHash)
{
    (void)TempHexString(nameHash);
}

// The registries are function-local statics so they exist before any
// static initialiser of game code registers its prototypes.
struct Registries {
    AnimScriptVarMap globalVars;                              // 0x010C2750
    AnimScriptVarMap varPrototypes;                           // 0x010C27B8
    AVLMap<uint32_t, ApeAnimScriptCommand*> commandPrototypes; // 0x010C2820
    AVLMap<uint32_t, ApeAnimScriptLibrary*> libraries;         // 0x010C2888
    AnimScriptEventMap eventPrototypes;                        // 0x010C28F0
};

Registries& GetRegistries()
{
    static Registries* s_registries = new Registries();
    return *s_registries;
}

AnimScriptHooks g_hooks;

} // namespace

AnimScriptVarMap& GlobalVars() { return GetRegistries().globalVars; }
AnimScriptVarMap& VarPrototypes() { return GetRegistries().varPrototypes; }
AVLMap<uint32_t, ApeAnimScriptCommand*>& CommandPrototypes() { return GetRegistries().commandPrototypes; }
AnimScriptEventMap& EventPrototypes() { return GetRegistries().eventPrototypes; }
AnimScriptHooks& GetAnimScriptHooks() { return g_hooks; }

uint32_t ElementHash(const xml::XmlElement* element)
{
    return (element && element->IsValid()) ? element->GetNameHash() : 0;
}

// ---------------------------------------------------------------------------
// Base classes

ApeRefCounted::~ApeRefCounted() = default;

void ApeRefCounted::Release()
{
    if (--m_refCount < 1)
        delete this;
}

ApeAnimScriptVar::~ApeAnimScriptVar() = default;

char* ApeAnimScriptVar::ToString(char* buffer) const
{
    buffer[0] = '\0';
    return buffer;
}

// @func 0x004BB41C ApeAnimScriptCommand::~ApeAnimScriptCommand
// @func 0x004BB3C8 ApeAnimScriptCommand::~ApeAnimScriptCommand (complete)
// @func 0x004BB3D8 ApeAnimScriptCommand::~ApeAnimScriptCommand (deleting)
ApeAnimScriptCommand::~ApeAnimScriptCommand() = default;

// Two identical out-of-line copies of the base constructor exist.
// @func 0x004BB3B0 ApeAnimScriptCommand::ApeAnimScriptCommand
// @func 0x004B9CB8 ApeAnimScriptCommand::ApeAnimScriptCommand (copy 2)
ApeAnimScriptCommand::ApeAnimScriptCommand() = default;

void LoadCommandList(AnimScriptCommandList& list, xml::XmlElement* element)
{
    AVLMap<uint32_t, ApeAnimScriptCommand*>& prototypes = CommandPrototypes();
    size_t count = 0;
    for (xml::XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement())
        if (prototypes.Contains(ElementHash(child)))
            ++count;
    list.reserve(list.size() + count);
    for (xml::XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement())
        if (ApeAnimScriptCommand* command = CreateCommand(child))
            list.push_back(command);
}

void ExecuteCommandList(const AnimScriptCommandList& list, ApeAnimScriptInstance* instance,
                        ApeAnimScriptState* state)
{
    for (size_t i = 0; i < list.size(); ++i)
        list[i]->Execute(instance, state);
}

void ReleaseCommandList(AnimScriptCommandList& list)
{
    while (!list.empty()) {
        ApeAnimScriptCommand* command = list.back();
        list.pop_back();
        if (command)
            command->Release();
    }
}

// @func 0x004C0C14 ApeAnimScriptEvent::~ApeAnimScriptEvent
// @func 0x004C0CF4 ApeAnimScriptEvent::~ApeAnimScriptEvent (deleting)
ApeAnimScriptEvent::~ApeAnimScriptEvent()
{
    ReleaseCommandList(m_commands);
}

// @func 0x004B6E74 ApeAnimScriptEvent::LoadCommands
void ApeAnimScriptEvent::LoadCommands(xml::XmlElement* element)
{
    LoadCommandList(m_commands, element);
}

// ---------------------------------------------------------------------------
// Prototype registries

// @func 0x004B6894 CreateVar
ApeAnimScriptVar* CreateVar(xml::XmlElement* element)
{
    ApeAnimScriptVar* prototype = VarPrototypes().Find(ElementHash(element));
    return prototype ? prototype->Create(element) : nullptr;
}

// @func 0x004B6C88 CreateCommand
ApeAnimScriptCommand* CreateCommand(xml::XmlElement* element)
{
    ApeAnimScriptCommand* prototype = CommandPrototypes().Find(ElementHash(element));
    return prototype ? prototype->Create(element) : nullptr;
}

// @func 0x004B6DA0 IsCommandElement
bool IsCommandElement(xml::XmlElement* element)
{
    return CommandPrototypes().Contains(ElementHash(element));
}

// @func 0x004B707C CreateEvent
ApeAnimScriptEvent* CreateEvent(xml::XmlElement* element)
{
    ApeAnimScriptEvent* prototype = EventPrototypes().Find(ElementHash(element));
    if (!prototype)
        return nullptr;
    ApeAnimScriptEvent* event = prototype->Create(element);
    if (event)
        event->LoadCommands(element);
    return event;
}

// ---------------------------------------------------------------------------
// ApeAnimScriptBlock

AnimScriptVarMap& ApeAnimScriptBlock::Vars()
{
    if (!m_vars)
        m_vars.reset(new AnimScriptVarMap());
    return *m_vars;
}

AnimScriptEventMap& ApeAnimScriptBlock::Events()
{
    if (!m_events)
        m_events.reset(new AnimScriptEventMap());
    return *m_events;
}

AnimScriptTimedEventMap& ApeAnimScriptBlock::TimedEvents()
{
    if (!m_timedEvents)
        m_timedEvents.reset(new AnimScriptTimedEventMap());
    return *m_timedEvents;
}

AnimScriptStateMap& ApeAnimScriptBlock::States()
{
    if (!m_states)
        m_states.reset(new AnimScriptStateMap());
    return *m_states;
}

void ApeAnimScriptBlock::AddVar(ApeAnimScriptVar* var)
{
    const uint32_t name = var->GetNameHash();
    if (m_vars && m_vars->Contains(name)) {
        WarnName(name);   // duplicate variable
        var->Release();
        return;
    }
    Vars().Add(name, var);
}

void ApeAnimScriptBlock::AddEvent(ApeAnimScriptEvent* event)
{
    if (!event->IsTimed()) {
        // NOTE: a second untimed event of the same name is dropped by the
        // tree (and leaked) on the console; here it is released.
        AnimScriptEventMap& events = Events();
        if (events.Contains(event->GetNameHash())) {
            event->Release();
            return;
        }
        events.Add(event->GetNameHash(), event);
        return;
    }

    // Timed events firing at the same moment are merged into the first one.
    const float time = event->ApeAnimScriptEvent::GetTime();
    if (m_timedEvents && m_timedEvents->Contains(time)) {
        ApeAnimScriptEvent* existing = m_timedEvents->Find(time);
        for (ApeAnimScriptCommand* command : event->GetCommands()) {
            if (command) {
                command->AddRef();
                existing->AddCommand(command);
            }
        }
        event->Release();
        return;
    }
    TimedEvents().Add(time, event);
}

void ApeAnimScriptBlock::AddState(ApeAnimScriptState* state)
{
    const uint32_t name = state->GetNameHash();
    if (m_states && m_states->Contains(name)) {
        WarnName(name);   // duplicate state
        state->Release();
        return;
    }
    States().Add(name, state);
}

// @func 0x004B71BC ApeAnimScriptBlock::ApeAnimScriptBlock
ApeAnimScriptBlock::ApeAnimScriptBlock(const char* sourceName, xml::XmlDocument* document,
                                       xml::XmlElement* element)
{
    if (!document || !element)
        return;

    if (const char* name = element->GetString("name"))
        m_nameHash = HashNameNoCase(name);
    if (m_nameHash == 0)
        m_nameHash = kUnknownName;

    AVLMap<uint32_t, ApeAnimScriptCommand*>& commandPrototypes = CommandPrototypes();
    size_t commandCount = 0;
    for (xml::XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement())
        if (commandPrototypes.Contains(ElementHash(child)))
            ++commandCount;
    if (commandCount)
        m_commands.reserve(commandCount);

    // Each child is a variable, a command, an event or a nested state,
    // depending on which registry knows its element name.
    for (xml::XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const uint32_t kind = ElementHash(child);
        if (VarPrototypes().Contains(kind)) {
            if (ApeAnimScriptVar* var = CreateVar(child))
                AddVar(var);
        } else if (commandPrototypes.Contains(kind)) {
            if (ApeAnimScriptCommand* command = CreateCommand(child))
                m_commands.push_back(command);
        } else if (EventPrototypes().Contains(kind)) {
            if (ApeAnimScriptEvent* event = CreateEvent(child))
                AddEvent(event);
        } else if (kind == kElemState) {
            AddState(new ApeAnimScriptState(sourceName, document, child));
        } else {
            WarnName(kind);   // unknown element
        }
    }
}

// @func 0x004B7DF0 ApeAnimScriptBlock::~ApeAnimScriptBlock
// @func 0x004B8E90 ApeAnimScriptBlock::~ApeAnimScriptBlock (deleting)
ApeAnimScriptBlock::~ApeAnimScriptBlock()
{
    auto releaseAll = [](auto& map) {
        if (!map)
            return;
        map->ForEach([](auto* item) { item->Release(); });
        map->Clear();
    };
    releaseAll(m_vars);
    releaseAll(m_states);
    releaseAll(m_events);
    releaseAll(m_timedEvents);
    // NOTE: the block's own commands are not released by the console code;
    // only their array is freed.
}

// @func 0x004B8EE4 ApeAnimScriptBlock::Link
void ApeAnimScriptBlock::Link()
{
    for (ApeAnimScriptCommand* command : m_commands)
        command->Link();
    if (m_states)
        m_states->ForEach([](ApeAnimScriptState* state) { state->Link(); });
    auto linkEvent = [](ApeAnimScriptEvent* event) {
        for (ApeAnimScriptCommand* command : event->GetCommands())
            command->Link();
    };
    if (m_events)
        m_events->ForEach(linkEvent);
    if (m_timedEvents)
        m_timedEvents->ForEach(linkEvent);
}

// @func 0x004B95F0 ApeAnimScriptBlock::CountCommands
uint32_t ApeAnimScriptBlock::CountCommands() const
{
    uint32_t count = (uint32_t)m_commands.size();
    auto countEvent = [&count](ApeAnimScriptEvent* event) { count += (uint32_t)event->GetCommands().size(); };
    if (m_events)
        m_events->ForEach(countEvent);
    if (m_timedEvents)
        m_timedEvents->ForEach(countEvent);
    if (m_states)
        m_states->ForEach([&count](ApeAnimScriptState* state) { count += state->CountCommands(); });
    return count;
}

// @func 0x004B9BD8 ApeAnimScriptBlock::GetStateCount
uint32_t ApeAnimScriptBlock::GetStateCount() const
{
    return m_states ? m_states->Count() : 0;
}

// @func 0x004B9BF8 ApeAnimScriptBlock::GetVarCount
uint32_t ApeAnimScriptBlock::GetVarCount() const
{
    return m_vars ? m_vars->Count() : 0;
}

// @func 0x004B9C18 ApeAnimScriptBlock::GetEventCount
uint32_t ApeAnimScriptBlock::GetEventCount() const
{
    return m_events ? m_events->Count() : 0;
}

ApeAnimScriptState* ApeAnimScriptBlock::FindState(uint32_t nameHash)
{
    return m_states ? m_states->Find(nameHash) : nullptr;
}

// ---------------------------------------------------------------------------
// ApeAnimScriptState

ApeAnimScriptState::ApeAnimScriptState() : ApeAnimScriptBlock(nullptr, nullptr, nullptr) {}

// @func 0x004B69B8 ApeAnimScriptState::ApeAnimScriptState
ApeAnimScriptState::ApeAnimScriptState(const char* sourceName, xml::XmlDocument* document,
                                       xml::XmlElement* element)
    : ApeAnimScriptBlock(sourceName, document, element)
{
    element->GetFloat(kAttrDuration, &m_duration, 0.0f);
    element->GetBool(kAttrLoop, &m_loop, false);
    element->GetBool(kAttrHold, &m_hold, false);
    // A state lasts at least until its last timed event.
    if (m_timedEvents) {
        m_timedEvents->ForEach([this](ApeAnimScriptEvent* event) {
            m_duration = std::max(m_duration, event->ApeAnimScriptEvent::GetTime());
        });
    }
}

// @func 0x004BB430 ApeAnimScriptState::~ApeAnimScriptState
// @func 0x004BB45C ApeAnimScriptState::~ApeAnimScriptState (deleting)
ApeAnimScriptState::~ApeAnimScriptState() = default;

// @func 0x004BBABC ApeAnimScriptState::CloneForInstance
ApeAnimScriptState* ApeAnimScriptState::CloneForInstance(ApeAnimScriptInstance* instance) const
{
    auto* clone = new ApeAnimScriptState();
    clone->m_loop = m_loop;
    clone->m_isInstance = true;
    clone->m_duration = m_duration;
    clone->m_hold = m_hold;

    // Variables are copied; object references are looked up in the world.
    if (m_vars) {
        for (ApeAnimScriptVar* var = m_vars->First(); var; var = m_vars->Next()) {
            ApeAnimScriptVar* copy = var->Clone();
            if (!copy)
                continue;
            clone->Vars().Add(copy->GetNameHash(), copy);
            if (copy->GetType() == AnimScriptVarType::Object) {
                auto* objectVar = static_cast<ApeAnimScriptObjectVar*>(copy);
                if (objectVar->GetObjectName() != 0)
                    objectVar->SetObject(ApeAnimScriptInstance::FindWorldObject(objectVar->GetObjectName()));
            }
        }
    }
    // Events are shared.
    if (m_events) {
        m_events->ForEach([clone](ApeAnimScriptEvent* event) {
            clone->Events().Add(event->GetNameHash(), event);
            event->AddRef();
        });
    }
    // Timed events are shared too, but keyed by a freshly rolled time.
    if (m_timedEvents) {
        m_timedEvents->ForEach([clone](ApeAnimScriptEvent* event) {
            clone->TimedEvents().Add(event->GetTime(), event);
            event->AddRef();
        });
    }
    clone->m_nameHash = m_nameHash;
    return clone;
}

// @func 0x004BC36C ApeAnimScriptState::FireTimedEvents
void ApeAnimScriptState::FireTimedEvents(float from, float to, ApeAnimScriptInstance* instance)
{
    if (!m_timedEvents)
        return;
    for (ApeAnimScriptEvent* event = m_timedEvents->SeekLowerBound(from);
         !m_timedEvents->AtEnd() && m_timedEvents->CurrentKey() <= to; event = m_timedEvents->Next()) {
        if (event != m_lastTimedEvent)
            event->Execute(instance, this);
        m_lastTimedEvent = event;
    }
}

// @func 0x004BC7C8 ApeAnimScriptState::Update
void ApeAnimScriptState::Update(float dt, ApeAnimScriptInstance* instance)
{
    m_lastDelta = dt;
    if (m_events) {
        for (ApeAnimScriptEvent* event = m_events->First(); event; event = m_events->Next())
            if (event->Check(instance, this))
                event->Execute(instance, this);
    }

    if (!m_done)
        FireTimedEvents(m_time, m_time + dt, instance);

    if (!m_hold || m_time <= m_duration)
        m_time += dt;

    if (!m_loop) {
        if (!m_hold && m_duration <= m_time)
            m_done = true;
        return;
    }
    if (m_duration <= 0.0f) {
        m_time = 0.0f;
        return;
    }
    if (m_duration <= m_time) {
        // Wrap around and replay the events of the new lap up to now.
        while (m_duration <= m_time)
            m_time -= m_duration;
        if (m_time > 0.0f) {
            m_lastTimedEvent = nullptr;
            FireTimedEvents(0.0f, m_time, instance);
        }
    }
}

// @func 0x004BCBB0 ApeAnimScriptState::HasEvent
bool ApeAnimScriptState::HasEvent(ApeAnimScriptInstance* instance, uint32_t eventHash)
{
    return m_events && m_events->Find(eventHash) != nullptr;
}

// @func 0x004BCC90 ApeAnimScriptState::FireEvent
void ApeAnimScriptState::FireEvent(ApeAnimScriptInstance* instance, uint32_t eventHash)
{
    if (!m_events)
        return;
    if (ApeAnimScriptEvent* event = m_events->Find(eventHash))
        event->Execute(instance, this);
}

// ---------------------------------------------------------------------------
// ApeAnimScript

ApeAnimScript::ApeAnimScript(const char* sourceName, xml::XmlDocument* document, xml::XmlElement* element)
    : ApeAnimScriptBlock(sourceName, document, element)
{
    element->GetBool(kAttrAutoRun, &m_autoRun, false);
    element->GetBool(kAttrWaitForChildren, &m_waitForChildren, true);
}

// @func 0x004B9C38 ApeAnimScript::~ApeAnimScript
// @func 0x004B9C64 ApeAnimScript::~ApeAnimScript (deleting)
ApeAnimScript::~ApeAnimScript() = default;

// ---------------------------------------------------------------------------
// ApeAnimScriptLibrary

ApeAnimScriptLibrary::~ApeAnimScriptLibrary()
{
    Clear();
}

// @func 0x004BDA70 ApeAnimScriptLibrary::LoadFromXml
ApeAnimScript* ApeAnimScriptLibrary::LoadFromXml(const char* sourceName, xml::XmlDocument* document)
{
    ApeAnimScript* first = nullptr;
    xml::XmlElement* root = document->FirstChildElement();
    if (ElementHash(root) != kElemScripts)
        return nullptr;
    for (xml::XmlElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (ElementHash(child) != kElemDirector)
            continue;
        auto* script = new ApeAnimScript(sourceName, document, child);
        const uint32_t name = script->GetNameHash();
        if (m_scripts.Contains(name))
            RemoveScript(name);   // reloading replaces the old version
        m_scripts.Add(name, script);
        if (!first)
            first = script;
    }
    return first;
}

// @func 0x004BDD08 ApeAnimScriptLibrary::LoadFile
ApeAnimScript* ApeAnimScriptLibrary::LoadFile(const char* path)
{
    xml::XmlDocument document(path);
    if (!document.LoadFile(0))
        return nullptr;
    return LoadFromXml(path, &document);
}

// @func 0x004BE374 ApeAnimScriptLibrary::LoadFromStream
ApeAnimScript* ApeAnimScriptLibrary::LoadFromStream(file::File& file)
{
    xml::XmlDocument document;
    if (!document.LoadFromStream(file, nullptr))
        return nullptr;
    return LoadFromXml(file.GetName(), &document);
}

// @func 0x004BD8F4 ApeAnimScriptLibrary::RemoveScript
void ApeAnimScriptLibrary::RemoveScript(uint32_t nameHash)
{
    if (ApeAnimScript* script = m_scripts.Find(nameHash)) {
        script->Release();
        m_scripts.Remove(nameHash);
    }
}

// @func 0x004BD1CC ApeAnimScriptLibrary::Clear
void ApeAnimScriptLibrary::Clear()
{
    m_scripts.ForEach([](ApeAnimScript* script) { script->Release(); });
    m_scripts.Clear();
}

// @func 0x004BE420 ApeAnimScriptLibrary::LinkAll
void ApeAnimScriptLibrary::LinkAll()
{
    m_scripts.ForEach([](ApeAnimScript* script) { script->Link(); });
}

// @func 0x004BCE90 ApeAnimScriptLibrary::Get
ApeAnimScriptLibrary* ApeAnimScriptLibrary::Get(uint32_t nameHash)
{
    AVLMap<uint32_t, ApeAnimScriptLibrary*>& libraries = GetRegistries().libraries;
    if (!libraries.Contains(nameHash))
        libraries.Add(nameHash, new ApeAnimScriptLibrary(nameHash));
    return libraries.Find(nameHash);
}

// @func 0x004BD4B4 ApeAnimScriptLibrary::Destroy
void ApeAnimScriptLibrary::Destroy(uint32_t nameHash)
{
    AVLMap<uint32_t, ApeAnimScriptLibrary*>& libraries = GetRegistries().libraries;
    if (!libraries.Contains(nameHash))
        return;
    delete libraries.Find(nameHash);
    libraries.Remove(nameHash);
}

// @func 0x004BD7D4 ApeAnimScriptLibrary::DestroyAll
void ApeAnimScriptLibrary::DestroyAll()
{
    AVLMap<uint32_t, ApeAnimScriptLibrary*>& libraries = GetRegistries().libraries;
    while (!libraries.IsEmpty()) {
        libraries.First();
        Destroy(libraries.CurrentKey());
    }
}

// @func 0x004BDEDC ApeAnimScriptLibrary::FindScriptAnywhere
ApeAnimScript* ApeAnimScriptLibrary::FindScriptAnywhere(uint32_t scriptHash, uint32_t libraryHash,
                                                        bool searchAll)
{
    AVLMap<uint32_t, ApeAnimScriptLibrary*>& libraries = GetRegistries().libraries;
    if (ApeAnimScriptLibrary* library = libraries.Find(libraryHash))
        if (ApeAnimScript* script = library->FindScript(scriptHash))
            return script;
    if (!searchAll)
        return nullptr;
    for (ApeAnimScriptLibrary* library = libraries.First(); library; library = libraries.Next())
        if (ApeAnimScript* script = library->FindScript(scriptHash))
            return script;
    return nullptr;
}

} // namespace ape
