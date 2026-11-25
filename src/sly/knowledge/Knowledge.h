// openTIT - The AI "knowledge" system (0x0038AD7C-0x00398ED0).
//
// Knowledge scripts ("KnowledgeScriptFile" XML, cooked into the level as
// 'KNOF'/'KNOH'/'KNOD' chunks) describe AI behaviour as rule lists. A
// knowledge list has variables (KVarData: int/float/bool/hash), timers
// (KTimer) and states (KnowledgeState); every state, and the "Global"
// block, holds rules (KUnit): a cause (CAlways, CIsVar, CIsTimer, ...) and a
// result (RChangeState, RSetVarDataInt, RPlaySndSFX, RFireProjectile, ...),
// or a container of results picked by weight or run in sequence.
//
// Cause and result types are registered in two static tables (7 causes at
// 0x00F6503C, 38 results at 0x00F65100). Each type owns an attribute schema
// (ApeDTD) and a definition block that maps its fields to attribute columns;
// each rule owns a data block with its attribute values. Both kinds of
// blocks come from one pool of 0x1800 blocks (the "KData" pool, 0x00F65038),
// which also keeps the string attribute values.
//
// A loaded knowledge list is a template; entities clone it (the clone gets
// the runtime timer area) and the AI evaluates the clone every frame
// (0x00254210, outside this module). The cause/result callbacks of the
// types live here.
//
// The console stores a list as one memory block: a 0x98-byte header, the
// entry array (0x84 bytes per entry), then the variable table, the timer
// table and the runtime area. The port keeps the same parts as members.
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace ape {
class ApeDTD;
class ApeStringPool;
namespace xml { class XmlElement; }
namespace iff { struct IffReader; }
}
namespace sly { namespace entity { class Entity; } }

namespace sly {
namespace knowledge {

using entity::Entity;

// ---------------------------------------------------------------------------
// KData pool

// An attribute value (4 bytes on the console: string values point into the
// pool's string buffer).
union KValue {
    int32_t i;
    uint32_t u;
    float f;
    bool b;
    const char* str;
};

constexpr int kMaxBlocks = 0x1800;   // _MAX_NUM_OF_KNOWLEDGE_DATA_BLOCK
constexpr int kBlockValues = 28;     // (0x80 - 0x10) / 4

// One 0x80-byte block. Definition blocks hold the slot map of their type
// (the attribute column of each field); data blocks hold a rule's values.
// NOTE: on the console both live in the same 0x70 bytes from +0x10.
struct KDataBlock {
    uint8_t valueCount = 0;          // +0x00 data blocks; def blocks: slot map offset (0x10)
    uint8_t field_01 = 0;
    uint8_t field_02 = 0;            // +0x02 result defs: column count; cause defs: KCauseDefFlags
    uint8_t field_03 = 0;            // +0x03 cause defs: column count
    KValue values[kBlockValues] = {};   // +0x10
    uint8_t slots[kBlockValues] = {};   // +0x10 (def blocks)
};

enum KCauseDefFlags : uint8_t {
    kCauseDef_Polled          = 0x01,   // NOTE: factory flag of the plain causes; meaning assumed
    kCauseDef_OnEnterState    = 0x02,
    kCauseDef_OnExitState     = 0x04,
    kCauseDef_BreakOnActivate = 0x20,   // a rule asked for DebugBreak="OnActivate"
    kCauseDef_BreakOnEvaluate = 0x40,   // DebugBreak="OnEvaluate"
};

// What a pool block is used for (record kind).
enum KBlockKind : uint32_t {
    kBlock_RuleResult = 5,   // standalone result data, or a result type's definition
    kBlock_CauseDef   = 6,
    kBlock_RuleData   = 7,
};

struct KDataRecord {
    KDataBlock* block = nullptr;
    void* owner = nullptr;
    uint32_t kind = 0;
    uint32_t tag = 0;        // 0x021A9E1A result data, 0x72D3CF6E cause data,
                             // 0xC2A7B944 cause def, 0x7C764E52 result def
};

// The pool (0x18028 bytes on the console).
struct KDataPool {
    KDataPool();
    ~KDataPool();
    // Takes the next block; -1 when the pool is exhausted.
    int16_t Alloc(void* owner, uint32_t kind);
    KDataBlock* Block(int16_t index) { return index == -1 ? nullptr : &blocks[index]; }

    std::unique_ptr<ape::ApeStringPool> strings;   // +0x00 string attribute values (0x2800 bytes)
    std::unique_ptr<KDataBlock[]> blocks;          // +0x18
    uint32_t count = 0;                            // +0x1C
    std::unique_ptr<KDataRecord[]> records;        // +0x24
    int16_t fixedCount = 0;                        // +0x18024 blocks the type definitions use
};
KDataPool* Pool();   // 0x00F65038

// ---------------------------------------------------------------------------
// Types

// Definition of a cause or result type (0x18 bytes, built by the type's
// factory at start-up).
struct KTypeDef {
    const char* name = nullptr;      // +0x00 NOTE: never set; messages print "NULL"
    uint32_t nameHash = 0;           // +0x04
    int16_t typeIndex = 0;           // +0x08
    ape::ApeDTD* dtd = nullptr;      // +0x0C
    int16_t defBlockIndex = -1;      // +0x10
    KDataBlock* defBlock = nullptr;  // +0x14
};

// Reference of a rule to its cause or result (8 bytes).
struct KRef {
    int16_t type = -1;           // index in the cause/result table
    int16_t defBlock = -1;       // the type's definition block
    int16_t dataBlock = -1;      // the rule's values
    int16_t runtime = -1;        // entry with the per-rule runtime state
};

class KnowledgeData;
using KCreateFn = KTypeDef* (*)(int16_t index);
using KAllocRuntimeFn = int16_t (*)(int16_t index, KnowledgeData* list);
using KDefaultsFn = void (*)(KDataBlock* data, const uint8_t* slots);
using KRefFn = void (*)(KRef* ref, KnowledgeData* list);
using KEvaluateFn = bool (*)(KRef* ref, KnowledgeData* list);

// Cause table row (0x1C bytes).
struct KCauseType {
    KCreateFn create;             // [0]
    KAllocRuntimeFn allocRuntime; // [1]
    KDefaultsFn setDefaults;      // [2]
    KRefFn link;                  // [3] after loading
    KRefFn clone;                 // [4] after cloning
    KEvaluateFn evaluate;         // [5]
    KTypeDef* def;                // [6] cached definition
};
// Result table row (0x2C bytes).
struct KResultType {
    KCreateFn create;             // [0]
    KAllocRuntimeFn allocRuntime; // [1]
    KRefFn release;               // [2] when the list is freed
    KDefaultsFn setDefaults;      // [3]
    KRefFn link;                  // [4] after loading
    KRefFn clone;                 // [5] after cloning
    KRefFn field_18;              // [6] NOTE: unused by every type
    KEvaluateFn execute;          // [7]
    KRefFn field_20;              // [8] NOTE: unused by every type
    KRefFn reset;                 // [9] when a state's results are reset
    KTypeDef* def;                // [10] cached definition
};
constexpr int kCauseTypeCount = 7;
constexpr int kResultTypeCount = 0x26;
extern KCauseType g_causeTypes[kCauseTypeCount];     // 0x00F6503C
extern KResultType g_resultTypes[kResultTypeCount];  // 0x00F65100

// Cause / result definitions by type name hash (0x00F65788).
struct KTypeRegistry {
    KTypeRegistry();   // 0x0038E468
    int16_t FindCause(uint32_t nameHash) const;
    int16_t FindResult(uint32_t nameHash) const;
    std::vector<std::pair<uint32_t, int16_t>> causes;    // +0x00 sorted
    std::vector<std::pair<uint32_t, int16_t>> results;   // +0x18 sorted
};
KTypeRegistry* Registry();

// Base set-up of a cause type (FireOnce, DebugBreak) (0x0038E5DC) and of a
// result type (CWeight) (0x00391694).
void InitCauseDef(KTypeDef* def, int16_t index, const char* name, uint8_t flags);
void InitResultDef(KTypeDef* def, int16_t index, const char* name);

// ---------------------------------------------------------------------------
// Knowledge lists

enum KEntryType : int32_t {
    kEntry_Unit            = 1,   // a rule
    kEntry_ContainerResult = 3,   // a result inside a rule's result container
    kEntry_StateBegin      = 4,
    kEntry_StateEnd        = 5,
    kEntry_CauseRuntime    = 6,
    kEntry_ResultRuntime   = 7,
    kEntry_Result          = 9,   // a result run on its own (anim events)
    kEntry_MaxType         = 9,
};

// How a rule's result container runs its results.
enum KContainerKind : int32_t {
    kContainer_Unknown = 0,
    kContainer_Random = 1,     // "Random"
    kContainer_Sequence = 2,   // "Sequence"
    kContainer_3 = 3,          // NOTE: the other names are only known by hash (KnowledgeCore.cpp)
    kContainer_4 = 4,
    kContainer_5 = 5,
};

// A result entry (types 3 and 9).
struct KResultEntry {
    KRef ref;
    uint32_t state = 0;
};

// A rule (entry type 1).
struct KUnit {
    const char* name = nullptr;      // +0x00 (null when the rule has none)
    const char* stateName = nullptr; // +0x04
    int16_t line = 0;                // +0x08 XML line, 1-based
    int32_t priority = 50;           // +0x0C
    bool isContainer = false;        // +0x10
    KRef cause;                      // +0x12
    KRef result;                     // +0x1C (container: result.type = result count)
    union {                          // +0x24
        uint32_t resultState = 0;
        KResultEntry* currentResult; // containers running one result at a time
    };
    int32_t containerKind = 0;       // +0x28 KContainerKind
    uint8_t containerFlags = 0;      // +0x2C bit 7: broken
    int16_t containerFirst = -1;     // +0x2E entry index of the container
};
// State markers (types 4 and 5).
struct KStateMark {
    uint32_t nameHash = 0;
    const char* name = nullptr;
    int16_t count = 0;               // end marker: entries in the state
};
// Runtime state of a rule's cause or result (types 6 and 7).
struct KRuntime {
    int16_t type = 0;
    uint16_t flags = 0;              // bit 15: the rule is broken
    uint8_t field_04[0xC] = {};
    union Payload {                  // +0x10
        float elapsed;               // CEveryXSecond
        struct {
            uint16_t index;          // variable / timer index (0xFFFF: none)
            KValue value;            // CIsVar: value to compare with
        } var;
        uint32_t hash;               // RChangeState: target state
        void* object;                // results holding an engine object
        uint8_t raw[0x6C];
    } payload = {};
    static constexpr uint16_t kBroken = 0x8000;
};

struct KEntry {
    int32_t type = 0;
    union {
        KUnit unit;
        KResultEntry result;
        KStateMark state;
        KRuntime runtime;
    };
    KEntry() : runtime() {}
};

// Variable table: names (hashes), types and values.
enum KVarType : int32_t {
    kVar_Int = 0, kVar_UInt = 1, kVar_Float = 2, kVar_Bool = 3, kVar_String = 4, kVar_Hash = 5,
};
struct KVarTable {
    std::vector<uint32_t> names;
    std::vector<int32_t> types;
    std::vector<KValue> values;
    int Find(uint32_t nameHash) const;   // -1 when absent
};
// Timer table: names and current values (the initial value comes from XML).
struct KTimerTable {
    std::vector<uint32_t> names;
    std::vector<float> values;
    int Find(uint32_t nameHash) const;
};
// Runtime area of a clone.
struct KListRuntime {
    uint32_t field_00 = 0;
    uint8_t field_04 = 0;
    uint16_t field_06 = 0;
    int16_t stateEntries[8];         // entry per state slot whose results run (-1: none)
    KListRuntime() { for (int16_t& e : stateEntries) e = -1; }
};

enum KListFlags : uint8_t {
    kList_Broken         = 0x10,
    kList_HasRuntime     = 0x20,
    kList_StateRequested = 0x40,
    kList_Template       = 0x80,
};

// One knowledge list.
class KnowledgeData {
public:
    // Entry `index`, or null when it is out of range.
    KEntry* Entry(int index);

    uint32_t nameHash = 0;           // +0x00
    uint8_t stateSlotCount = 0;      // +0x04 slots used in `states`
    uint8_t currentState = 0;        // +0x05
    uint8_t requestedState = 0;      // +0x06
    uint8_t flags = 0;               // +0x07 KListFlags
    uint32_t size = 0;               // +0x08 bytes of the console block
    Entity* owner = nullptr;         // +0x0C
    struct StateSlot { uint32_t hash; uint16_t entry; } states[16] = {};   // +0x10
    uint16_t entryCount = 0;         // +0x90
    uint16_t entryCapacity = 0;      // +0x92
    std::vector<KEntry> entries;     // +0x98
    KVarTable vars;
    KTimerTable timers;
    std::unique_ptr<KListRuntime> runtime;
};

// Handle of a list in the library (8 bytes: entry array and block).
struct KnowledgeList {
    std::unique_ptr<KnowledgeData> data;
};

// How the library makes a list (KnowledgeList request, 0x1C bytes).
struct KListRequest {
    bool empty = false;                  // +0x00 make an empty list
    bool fromXml = false;                // +0x01 load `element`
    KnowledgeList* source = nullptr;     // +0x04 template to clone
    bool withRuntime = false;            // +0x08
    ape::xml::XmlElement* element = nullptr;   // +0x0C
    Entity* owner = nullptr;             // +0x10
    uint16_t entryCapacity = 0;          // +0x14 empty lists
    uint16_t varCount = 0;               // +0x16
    const char* name = nullptr;          // +0x18
};

// Every list (0x00F65798): a pool of handles plus the templates by name.
struct KnowledgeLibrary {
    explicit KnowledgeLibrary(int capacity);
    ~KnowledgeLibrary();
    // Makes a list as the request says; null when the library is full or a
    // template of the same name exists (0x00390DC8).
    KnowledgeList* Add(const KListRequest& request, uint32_t* handle);
    KnowledgeList* FindTemplate(uint32_t nameHash) const;
    // Frees every list (0x00390724) / every list that is not a template
    // (0x00390C1C).
    void Clear();
    void FreeInstances();

    std::vector<std::unique_ptr<KnowledgeList>> slots;   // node values
    std::vector<uint8_t> used;
    std::vector<uint16_t> serial;
    int capacity = 0;
    int count = 0;
    bool growable = false;
    std::vector<std::pair<uint32_t, KnowledgeList*>> templates;   // +0x24
};
KnowledgeLibrary* Library();

// Module life cycle: created with the AI manager (0x00390500), destroyed
// with it (0x00390928); level start (0x00390D9C), level end (0x00390DC4),
// reset (0x00390BAC).
void CreateKnowledgeSystem(void* aiManager);
void DestroyKnowledgeSystem();
void KnowledgeLevelStart();
void KnowledgeLevelEnd();
void KnowledgeReset();
// Empty hooks of the AI manager's set-up (0x00390BA8, from 0x00018DD0) and
// tick (0x00390C18, from 0x00019AF0).
void KnowledgeInit();
void KnowledgeUpdate();
// Registers the 'KNOF' / 'KNOH' / 'KNOD' chunk handlers (part of 0x00390500).
void RegisterKnowledgeChunkHandlers();
// Loads the lists of a knowledge script document (0x0039129C).
bool LoadKnowledgeScript(void* owner, const char* data, uint32_t size);

// Loading a list (0x0038D498) / making an empty one (0x0038D91C) / cloning
// (0x0038BAE0) / freeing (0x0038DC0C). Lists are built in place in `list`.
bool BuildKnowledgeList(KnowledgeList* list, ape::xml::XmlElement* element);
void MakeEmptyKnowledgeList(KnowledgeList* list, const char* name, uint16_t entryCapacity,
                            uint16_t varCount, uint16_t timerCount);
KnowledgeData* CloneKnowledgeList(KnowledgeList* list, KnowledgeData* source, Entity* owner,
                                  bool withRuntime);
void FreeKnowledgeList(KnowledgeList* list);
// Appends a standalone result described by `element` (anim events)
// (0x0038DF4C); its entry index goes to `entryIndex`.
bool AddStandaloneResult(KnowledgeList* list, ape::xml::XmlElement* element, int16_t* entryIndex);
// Takes the next entry as a runtime entry of `entryType` for cause/result
// type `typeIndex`; -1 when the list is full.
int16_t AllocRuntimeEntry(KnowledgeData* list, int32_t entryType, int16_t typeIndex);
// Runtime state of the cause at `ref` (0x0038B118).
KRuntime* CauseRuntime(const KRef* ref, KnowledgeData* list);

// ---------------------------------------------------------------------------
// Helpers for the type callbacks

// The rule's value of field `field` (the type's slot map gives its column).
// Cause definitions keep their column count at +3, result definitions at
// +2; with no columns there is no slot map (the console then reads through
// a null pointer).
const KValue& CauseValue(const KRef* ref, int field);
const KValue& ResultValue(const KRef* ref, int field);
// The runtime entry of a rule's cause/result.
KRuntime* RuntimeOf(const KRef* ref, KnowledgeData* list);
// Reports a broken rule ("%s: (Error!) %s") and marks its runtime entry.
void ReportCauseError(const KRef* ref, KnowledgeData* list, const char* message);
void ReportResultError(const KRef* ref, KnowledgeData* list, const char* message);

} // namespace knowledge
} // namespace sly
