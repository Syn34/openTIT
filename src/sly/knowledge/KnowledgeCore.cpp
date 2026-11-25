// openTIT - Knowledge system: pool, type registry, list loading/cloning,
// library and chunk handlers (0x0038AD7C-0x003915FC). See Knowledge.h.
#include "sly/knowledge/Knowledge.h"
#include "sly/knowledge/KnowledgeHooks.h"

#include "ape/core/ApeString.h"
#include "ape/file/ApeFile.h"
#include "ape/level/ApeDTD.h"
#include "ape/level/ApeLevelDef.h"
#include "ape/memory/ApeMemStack.h"
#include "ape/sys/ApeIff.h"
#include "ape/sys/ApeStringPool.h"
#include "ape/xml/ApeXml.h"
#include "platform/Platform.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>

namespace sly {
namespace knowledge {

using ape::xml::XmlElement;
using ape::xml::XmlDocument;

namespace {

constexpr uint32_t kTag_ResultData = 0x021A9E1Au;
constexpr uint32_t kTag_CauseData  = 0x72D3CF6Eu;
constexpr uint32_t kTag_CauseDef   = 0xC2A7B944u;
constexpr uint32_t kTag_ResultDef  = 0x7C764E52u;

constexpr uint32_t kHash_Global      = 0x7D7BF7A3u;   // "Global"
constexpr uint32_t kHash_RContainer  = 0xCD1F984Fu;   // "RContainer"
constexpr uint32_t kHash_Type        = 0x0027873Au;   // "Type"
constexpr uint32_t kHash_Priority    = 0xE92E69A4u;   // "priority"
constexpr uint32_t kHash_Size        = 0x0026D801u;   // "Size"
constexpr uint32_t kHash_Value       = 0x04DA9951u;   // "value"
constexpr uint32_t kHash_Knowledge   = 0x0279785Eu;   // "Knowledge" (level string list)
constexpr uint32_t kHash_OnEvaluate  = 0xC7CE5778u;
constexpr uint32_t kHash_OnActivate  = 0x5B757FD2u;

constexpr uint16_t kDefaultListSize = 0x40;
constexpr uint16_t kRuntimeAreaSize = 0x20;            // 0x00F8F0EC

void Warn(const char* fmt, ...)
{
    char text[0x200];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(text, sizeof text, fmt, args);
    va_end(args);
    plat::Log(plat::LogLevel::Warning, "%s\n", text);
}

// Console sizes of the parts of a list block.
uint32_t VarTableSize(uint32_t count) { return (count * 0xC + 0x17) & ~0xFu; }
uint32_t TimerTableSize(uint32_t count) { return (count * 8 + 0x15) & ~0xFu; }

int Row(XmlElement* element)
{
    (void)element;
    return 0;   // compiled documents carry no positions (the console prints row + 1)
}

const char* DocPath(XmlElement* element)
{
    XmlDocument* document = element->GetDocument();
    return document ? document->GetDocumentPath() : "";
}

KDataPool* s_pool = nullptr;               // 0x00F65038
KTypeRegistry* s_registry = nullptr;       // 0x00F65788
KnowledgeLibrary* s_library = nullptr;     // 0x00F65798
void* s_aiManager = nullptr;               // 0x00F6578C
const char* s_scriptFileName = "Knowledge";// 0x00F65790 name from the 'KNOH' chunk
char s_scriptFileNameBuffer[0x41];         // 0x0106CFA0

int16_t FindSorted(const std::vector<std::pair<uint32_t, int16_t>>& table, uint32_t hash)
{
    auto it = std::lower_bound(table.begin(), table.end(), std::make_pair(hash, (int16_t)INT16_MIN));
    return (it != table.end() && it->first == hash) ? it->second : (int16_t)-1;
}

} // namespace

KDataPool* Pool() { return s_pool; }
KTypeRegistry* Registry() { return s_registry; }
KnowledgeLibrary* Library() { return s_library; }

// ---------------------------------------------------------------------------
// Pool

KDataPool::KDataPool()
    : strings(new ape::ApeStringPool(0x2800, false, false)),
      blocks(new KDataBlock[kMaxBlocks]),
      records(new KDataRecord[kMaxBlocks])
{
}

KDataPool::~KDataPool() = default;

int16_t KDataPool::Alloc(void* owner, uint32_t kind)
{
    if (count >= (uint32_t)kMaxBlocks) {
        Warn("Run out of KData Block (%d), please increase the block list "
             "(_MAX_NUM_OF_KNOWLEDGE_DATA_BLOCK)", kMaxBlocks);
        return -1;
    }
    KDataRecord& record = records[count];
    record.block = &blocks[count];
    record.owner = owner;
    record.kind = kind;
    return (int16_t)count++;
}

// ---------------------------------------------------------------------------
// Types

// @func 0x0038E5DC InitCauseDef
void InitCauseDef(KTypeDef* def, int16_t index, const char* name, uint8_t flags)
{
    def->typeIndex = index;
    def->name = nullptr;
    def->dtd = new ape::ApeDTD(name, 0, 0);
    def->nameHash = ape::HashNameNoCase(name);
    def->defBlockIndex = s_pool->Alloc(def, kBlock_CauseDef);
    def->defBlock = s_pool->Block(def->defBlockIndex);
    if (def->defBlock) {
        def->defBlock->valueCount = 0;
        def->defBlock->field_02 = 0;
        def->defBlock->field_03 = 0;
        s_pool->records[def->defBlockIndex].tag = kTag_CauseDef;
        def->defBlock->field_02 |= flags;
    }
    ape::DTDAttribute* fireOnce = def->dtd->AddAttribute("FireOnce", ape::kDTD_Bool, 1);
    fireOnce->defaultValue.u = 0;
    ape::DTDAttribute* debugBreak = def->dtd->AddAttribute("DebugBreak", ape::kDTD_Hash, 1);
    debugBreak->defaultValue.u = 0;
    debugBreak->AllocEnumNames(2);
    debugBreak->AddEnumName("OnEvaluate");
    debugBreak->AddEnumName("OnActivate");
}

// @func 0x00391694 InitResultDef
void InitResultDef(KTypeDef* def, int16_t index, const char* name)
{
    def->typeIndex = index;
    def->name = nullptr;
    def->dtd = new ape::ApeDTD(name, 0, 0);
    def->nameHash = ape::HashNameNoCase(name);
    def->defBlockIndex = s_pool->Alloc(def, kBlock_RuleResult);
    def->defBlock = s_pool->Block(def->defBlockIndex);
    if (def->defBlock) {
        def->defBlock->valueCount = 0;
        def->defBlock->field_02 = 0;
        s_pool->records[def->defBlockIndex].tag = kTag_ResultDef;
    }
    ape::DTDAttribute* weight = def->dtd->AddAttribute("CWeight", ape::kDTD_Float, 1);
    weight->defaultValue.f = 100.0f;
}

// Builds every type definition and indexes them by name.
// @func 0x0038E1D4 KTypeRegistry::Populate
static void PopulateRegistry(KTypeRegistry* registry)
{
    for (int i = 0; i < kCauseTypeCount; ++i) {
        KCauseType& type = g_causeTypes[i];
        if (!type.def)
            type.def = type.create((int16_t)i);
        registry->causes.emplace_back(type.def->nameHash, (int16_t)i);
    }
    std::sort(registry->causes.begin(), registry->causes.end());
    for (int i = 0; i < kResultTypeCount; ++i) {
        KResultType& type = g_resultTypes[i];
        if (!type.def)
            type.def = type.create((int16_t)i);
        registry->results.emplace_back(type.def->nameHash, (int16_t)i);
    }
    std::sort(registry->results.begin(), registry->results.end());
    // Blocks taken so far belong to the definitions and survive resets.
    s_pool->fixedCount = (int16_t)s_pool->count;
}

// @func 0x0038E468 KTypeRegistry::KTypeRegistry
KTypeRegistry::KTypeRegistry()
{
    causes.reserve(kCauseTypeCount);
    results.reserve(kResultTypeCount);
    s_registry = this;
    PopulateRegistry(this);
}

int16_t KTypeRegistry::FindCause(uint32_t nameHash) const { return FindSorted(causes, nameHash); }
int16_t KTypeRegistry::FindResult(uint32_t nameHash) const { return FindSorted(results, nameHash); }

// ---------------------------------------------------------------------------
// Helpers for the callbacks

const KValue& CauseValue(const KRef* ref, int field)
{
    const KDataBlock* data = s_pool->Block(ref->dataBlock);
    const KDataBlock* def = s_pool->Block(ref->defBlock);
    return data->values[def->slots[field]];
}

const KValue& ResultValue(const KRef* ref, int field)
{
    const KDataBlock* data = s_pool->Block(ref->dataBlock);
    const KDataBlock* def = s_pool->Block(ref->defBlock);
    return data->values[def->slots[field]];
}

KRuntime* RuntimeOf(const KRef* ref, KnowledgeData* list)
{
    KEntry* entry = list->Entry(ref->runtime);
    return entry ? &entry->runtime : nullptr;
}

void ReportCauseError(const KRef* ref, KnowledgeData* list, const char* message)
{
    KRuntime* runtime = RuntimeOf(ref, list);
    const char* typeName = g_causeTypes[runtime->type].def->name;
    Warn("%s: (Error!) %s", typeName ? typeName : "NULL", message);
    runtime->flags |= KRuntime::kBroken;
}

void ReportResultError(const KRef* ref, KnowledgeData* list, const char* message)
{
    KRuntime* runtime = RuntimeOf(ref, list);
    const char* typeName = g_resultTypes[runtime->type].def->name;
    Warn("%s: (Error!) %s", typeName ? typeName : "NULL", message);
    runtime->flags |= KRuntime::kBroken;
}

KEntry* KnowledgeData::Entry(int index)
{
    if (index < 0 || index >= entryCount || (size_t)index >= entries.size())
        return nullptr;
    KEntry* entry = &entries[(size_t)index];
    return entry->type > kEntry_MaxType ? nullptr : entry;
}

int KVarTable::Find(uint32_t nameHash) const
{
    for (size_t i = 0; i < names.size(); ++i)
        if (names[i] == nameHash)
            return (int)i;
    return -1;
}

int KTimerTable::Find(uint32_t nameHash) const
{
    for (size_t i = 0; i < names.size(); ++i)
        if (names[i] == nameHash)
            return (int)i;
    return -1;
}

// @func 0x0038B118 CauseRuntime
KRuntime* CauseRuntime(const KRef* ref, KnowledgeData* list)
{
    KEntry* entry = list->Entry(ref->runtime);
    return entry ? &entry->runtime : nullptr;
}

// ---------------------------------------------------------------------------
// Attribute values

// Reads the attributes of a rule's cause or result into its data block, in
// the order of the type's schema.
// @func 0x0038AD7C ReadRuleAttributes
static void ReadRuleAttributes(KDataBlock* data, XmlElement* element, const ape::ApeDTD* dtd)
{
    const auto& attributes = dtd->GetAttributes();
    for (size_t i = 0; i < attributes.size() && i < (size_t)kBlockValues; ++i) {
        const ape::DTDAttribute& attr = *attributes[i];
        KValue& value = data->values[i];
        switch (attr.type) {
        case ape::kDTD_String: {
            const char* text = element->GetString(attr.name);
            if (!text) {
                text = attr.defaultValue.str;
                if (!text) {
                    if (attr.field_3C != 1)
                        Warn("Fail to find the required string attribute (%s) in script file (%s %d:%d)!",
                             attr.name, DocPath(element), Row(element) + 1, 1);
                    break;
                }
            }
            if (!*text) {
                value.str = nullptr;
            } else {
                value.str = s_pool->strings->Add(text);
                if (!value.str)
                    Warn("Cannot add string (%s) to the knowledge data, run out of string buffer!", text);
            }
            break;
        }
        case ape::kDTD_Bool: {
            int32_t raw = 0;
            element->GetInt(attr.nameHash, &raw, attr.defaultValue.i);
            value.b = raw != 0;
            break;
        }
        case ape::kDTD_Float:
            element->GetFloat(attr.nameHash, &value.f, attr.defaultValue.f);
            break;
        case ape::kDTD_Hash: {
            const char* text = element->GetString(attr.name);
            if (text)
                value.u = ape::HashNameNoCase(text);
            else if (attr.defaultValue.u)
                value.u = attr.defaultValue.u;
            break;
        }
        case ape::kDTD_Int1:
        case ape::kDTD_Int:
            element->GetInt(attr.nameHash, &value.i, attr.defaultValue.i);
            break;
        default:
            break;
        }
    }
}

// Takes a data block for a rule's cause and reads its values; also records
// a DebugBreak request on the type's definition.
// @func 0x0038C4E0 AllocCauseData
static void AllocCauseData(KTypeDef* def, KnowledgeList* owner, XmlElement* element, int16_t* out)
{
    const int columns = def->dtd->GetColumnCount();
    if (!columns)
        return;
    KDataBlock* defBlock = def->defBlock;
    defBlock->field_03 = (uint8_t)columns;
    *out = s_pool->Alloc(owner, kBlock_RuleData);
    KDataBlock* data = s_pool->Block(*out);
    if (!data)
        return;
    s_pool->records[*out].tag = kTag_CauseData;
    data->valueCount = (uint8_t)columns;
    for (int i = 0; i < columns && i < kBlockValues; ++i)
        data->values[i].u = 0;
    if (g_causeTypes[def->typeIndex].setDefaults && defBlock->field_03)
        g_causeTypes[def->typeIndex].setDefaults(data, defBlock->slots);
    ReadRuleAttributes(data, element, def->dtd);
    const uint32_t debugBreak = data->values[1].u;
    if (debugBreak == kHash_OnEvaluate)
        defBlock->field_02 |= kCauseDef_BreakOnEvaluate;
    else if (debugBreak == kHash_OnActivate)
        defBlock->field_02 |= kCauseDef_BreakOnActivate;
}

// Same for a result.
// @func 0x0038BE18 AllocResultData
static void AllocResultData(KTypeDef* def, KnowledgeList* owner, uint32_t kind, XmlElement* element,
                            int16_t* out)
{
    const int columns = def->dtd->GetColumnCount();
    if (!columns)
        return;
    KDataBlock* defBlock = def->defBlock;
    defBlock->field_02 = (uint8_t)columns;
    *out = s_pool->Alloc(owner, kind);
    KDataBlock* data = s_pool->Block(*out);
    if (!data)
        return;
    s_pool->records[*out].tag = kTag_ResultData;
    data->valueCount = (uint8_t)columns;
    for (int i = 0; i < columns && i < kBlockValues; ++i)
        data->values[i].u = 0;
    if (g_resultTypes[def->typeIndex].setDefaults && defBlock->field_02)
        g_resultTypes[def->typeIndex].setDefaults(data, defBlock->slots);
    ReadRuleAttributes(data, element, def->dtd);
}

// ---------------------------------------------------------------------------
// Building lists

// The entry after the used ones. NOTE: the console writes past the block
// when a script needs more entries than its Size; the port hands out a
// scratch entry instead.
static KEntry& NextEntry(KnowledgeData* data)
{
    if (data->entryCount < data->entries.size())
        return data->entries[data->entryCount];
    static KEntry s_scratch;
    s_scratch = KEntry();
    return s_scratch;
}

// Gives a rule's cause and results their runtime entries.
// @func 0x0038B3A8 AllocUnitRuntime
static void AllocUnitRuntime(KUnit* unit, KnowledgeData* data)
{
    unit->cause.runtime = g_causeTypes[unit->cause.type].allocRuntime(unit->cause.type, data);
    if (!unit->isContainer) {
        unit->result.runtime = g_resultTypes[unit->result.type].allocRuntime(unit->result.type, data);
        return;
    }
    for (int i = 0; i < unit->result.type; ++i) {
        KEntry* entry = data->Entry(unit->containerFirst + i + 1);
        entry->result.ref.runtime =
            g_resultTypes[entry->result.ref.type].allocRuntime(entry->result.ref.type, data);
    }
}

// Lets the cause and the results resolve their names after loading.
// @func 0x0038B588 LinkUnit
static void LinkUnit(KUnit* unit, KnowledgeData* data)
{
    if (KRefFn link = g_causeTypes[unit->cause.type].link)
        link(&unit->cause, data);
    if (!unit->isContainer) {
        if (KRefFn link = g_resultTypes[unit->result.type].link)
            link(&unit->result, data);
        return;
    }
    for (int i = 0; i < unit->result.type; ++i) {
        KEntry* entry = data->Entry(unit->containerFirst + i + 1);
        if (KRefFn link = g_resultTypes[entry->result.ref.type].link)
            link(&entry->result.ref, data);
        entry->result.state = 0;
    }
}

// The same after cloning.
// @func 0x0038B78C CloneUnit
static void CloneUnit(KUnit* unit, KnowledgeData* data)
{
    if (KRefFn clone = g_causeTypes[unit->cause.type].clone)
        clone(&unit->cause, data);
    if (!unit->isContainer) {
        if (KRefFn clone = g_resultTypes[unit->result.type].clone)
            clone(&unit->result, data);
        return;
    }
    for (int i = 0; i < unit->result.type; ++i) {
        KEntry* entry = data->Entry(unit->containerFirst + i + 1);
        if (KRefFn clone = g_resultTypes[entry->result.ref.type].clone)
            clone(&entry->result.ref, data);
        entry->result.state = 0;
    }
}

// Resets the running result(s) of a rule's container.
// @func 0x0038B174 ResetContainer
static void ResetContainer(KUnit* unit, KnowledgeData* data)
{
    switch (unit->containerKind) {
    case kContainer_Random:
    case kContainer_Sequence:
    case kContainer_4:
    case kContainer_5:
        // These run one result at a time; the unit points at it.
        if (KResultEntry* current = unit->currentResult) {
            if (KRefFn reset = g_resultTypes[current->ref.type].reset)
                reset(&current->ref, data);
            current->state = 0;
        }
        break;
    case kContainer_3:
        for (int i = 0; i < unit->result.type; ++i) {
            KEntry* entry = data->Entry(unit->containerFirst + i + 1);
            if (KRefFn reset = g_resultTypes[entry->result.ref.type].reset)
                reset(&entry->result.ref, data);
            entry->result.state = 0;
        }
        break;
    default:
        break;
    }
}

// Reads a rule's result container ("RContainer" with a Type and results).
// @func 0x0038C0C8 BuildResultContainer
static bool BuildResultContainer(KnowledgeList* list, XmlElement* element)
{
    KnowledgeData* data = list->data.get();
    KUnit& unit = NextEntry(data).unit;
    unit.isContainer = true;
    unit.result.type = 0;   // result count
    unit.result.defBlock = 0;
    unit.resultState = 0;
    unit.containerKind = kContainer_Unknown;
    unit.result.dataBlock = -1;
    unit.containerFlags &= 0x3F;
    unit.containerFirst = (int16_t)data->entryCount;
    ++data->entryCount;

    uint32_t kind = 0;
    if (element->GetHash(kHash_Type, &kind, 0) != 0) {
        Warn("Fail to find the type hash for this result container (%s)", "No Name");
        unit.containerFlags |= 0x80;
        return false;
    }
    switch (kind) {
    case 0x7F1667C1u: unit.containerKind = kContainer_Sequence; break;   // "Sequence"
    case 0x3686BC00u: unit.containerKind = kContainer_5; break;
    case 0xF185AD4Bu: unit.containerKind = kContainer_4; break;
    case 0x90BFA336u: unit.containerKind = kContainer_3; break;
    case 0x8FA5D8C3u: unit.containerKind = kContainer_Random; break;     // "Random"
    default: unit.containerKind = kContainer_Unknown; break;
    }

    XmlElement* child = element->FirstChildElement();
    if (!child) {
        Warn("Cannot find the result element in result container (%s) (%s)", "No Name", "");
        unit.containerFlags |= 0x80;
        return false;
    }
    for (; child; child = child->NextSiblingElement()) {
        KEntry& entry = NextEntry(data);
        entry.type = kEntry_ContainerResult;
        KRef& ref = entry.result.ref;
        ref.type = s_registry->FindResult(child->GetNameHash());
        int16_t dataBlock = -1;
        if (ref.type == -1) {
            Warn("Cannot find a result type (%s) in the system in (%s %d:%d)",
                 ape::TempHexString(child->GetNameHash()), DocPath(child), Row(child) + 1, 1);
        } else {
            KTypeDef* def = g_resultTypes[ref.type].def;
            AllocResultData(def, list, kBlock_RuleData, child, &dataBlock);
        }
        ref.dataBlock = dataBlock;
        // NOTE: the console tests the (not yet written) definition block
        // here; an unknown type is what it guards against.
        if (ref.type == -1) {
            Warn("Fail to find a result def (%s) in result container (%s)",
                 ape::TempHexString(child->GetNameHash()), "No Name");
            unit.containerFlags |= 0x80;
            return false;
        }
        ref.defBlock = g_resultTypes[ref.type].def->defBlockIndex;
        ref.runtime = -1;
        entry.result.state = 0;
        ++data->entryCount;
        ++unit.result.type;
    }
    return true;
}

// Reads a rule (KUnit: a cause element followed by a result element or a
// result container).
// @func 0x0038C7F0 BuildUnit
static bool BuildUnit(KnowledgeList* list, XmlElement* element, const char* stateName)
{
    KnowledgeData* data = list->data.get();
    KEntry& entry = NextEntry(data);
    entry.type = kEntry_Unit;
    KUnit& unit = entry.unit;
    unit.stateName = stateName;
    unit.line = (int16_t)(Row(element) + 1);
    unit.name = nullptr;   // NOTE: the console keeps no rule names
    int32_t priority = 50;
    element->GetInt(kHash_Priority, &priority, 50);
    unit.priority = priority;

    XmlElement* causeElement = element->FirstChildElement();
    if (!causeElement) {
        Warn("Cannot find the cause element in knowledge (%s) (%s %d:%d)", "", DocPath(element),
             Row(element) + 1, 1);
        data->flags |= kList_Broken;
        return false;
    }
    unit.cause.type = s_registry->FindCause(causeElement->GetNameHash());
    int16_t causeData = -1;
    if (unit.cause.type == -1) {
        Warn("Cannot find a cause type (hash 0x%08x) in the system in (%s %d:%d)",
             causeElement->GetNameHash(), DocPath(causeElement), Row(causeElement) + 1, 1);
    } else {
        AllocCauseData(g_causeTypes[unit.cause.type].def, list, causeElement, &causeData);
    }
    unit.cause.dataBlock = causeData;
    if (unit.cause.type == -1)
        return false;
    unit.cause.defBlock = g_causeTypes[unit.cause.type].def->defBlockIndex;
    unit.cause.runtime = -1;

    XmlElement* resultElement = causeElement->NextSiblingElement();
    if (!resultElement) {
        Warn("Cannot find the result element in knowledge (%s) (%s %d:%d)", "", DocPath(element),
             Row(element) + 1, 1);
        data->flags |= kList_Broken;
        return false;
    }
    if (resultElement->GetNameHash() == kHash_RContainer) {
        BuildResultContainer(list, resultElement);
    } else {
        unit.isContainer = false;
        unit.result.type = s_registry->FindResult(resultElement->GetNameHash());
        int16_t resultData = -1;
        if (unit.result.type == -1) {
            Warn("Cannot find a result type (%s) in the system in (%s %d:%d)",
                 ape::TempHexString(resultElement->GetNameHash()), DocPath(resultElement),
                 Row(resultElement) + 1, 1);
        } else {
            AllocResultData(g_resultTypes[unit.result.type].def, list, kBlock_RuleData, resultElement,
                            &resultData);
        }
        unit.result.dataBlock = resultData;
        if (unit.result.type == -1)
            return false;
        unit.result.defBlock = g_resultTypes[unit.result.type].def->defBlockIndex;
        unit.result.runtime = -1;
        unit.resultState = 0;
        ++data->entryCount;
    }
    AllocUnitRuntime(&unit, data);
    LinkUnit(&unit, data);
    return true;
}

// Begin/end markers around a state's rules; the begin marker gets the
// number of entries in between. Every marker uses a state slot (only the
// begin marker records its name and entry there).
static int16_t BeginState(KnowledgeData* data, const char* name, uint32_t hash)
{
    KEntry& begin = NextEntry(data);
    begin.type = kEntry_StateBegin;
    begin.state.name = name;
    begin.state.nameHash = hash;
    begin.state.count = 0;
    data->states[data->stateSlotCount].hash = hash;
    data->states[data->stateSlotCount].entry = data->entryCount;
    ++data->stateSlotCount;
    ++data->entryCount;
    return (int16_t)data->entryCount;
}

static void EndState(KnowledgeData* data, const char* name, uint32_t hash, int16_t first)
{
    const int16_t last = (int16_t)data->entryCount;
    KEntry& end = NextEntry(data);
    end.type = kEntry_StateEnd;
    end.state.name = name;
    end.state.nameHash = hash;
    ++data->entryCount;
    if (KEntry* begin = data->Entry(first - 1))
        begin->state.count = (int16_t)(last - first);
    ++data->stateSlotCount;
}

// @func 0x0038CCF4 BuildState
static bool BuildState(KnowledgeList* list, XmlElement* element)
{
    KnowledgeData* data = list->data.get();
    const char* name = element->GetString("Name");
    if (!name || !*name) {
        Warn("A knowledge state contains no name! (%s %d:%d)", DocPath(element), Row(element) + 1, 1);
        data->flags |= kList_Broken;
        return false;
    }
    const uint32_t hash = ape::HashNameNoCase(name);
    const int16_t first = BeginState(data, name, hash);
    for (XmlElement* unit = element->FirstChildElement("KUnit"); unit;
         unit = unit->NextSiblingElement("KUnit"))
        BuildUnit(list, unit, name);
    EndState(data, name, hash, first);
    return true;
}

// @func 0x0038CF98 ReadTimers
static bool ReadTimers(KnowledgeData* data, XmlElement* element, uint16_t count)
{
    data->timers.names.assign(count, 0);
    data->timers.values.assign(count, 0.0f);
    int i = 0;
    for (XmlElement* timer = element->FirstChildElement("KTimer"); timer;
         timer = timer->NextSiblingElement("KTimer"), ++i) {
        const char* name = timer->GetString("TimerName");
        if (!name)
            return false;
        data->timers.names[i] = ape::HashNameNoCase(name);
        float value = 0.0f;
        timer->GetFloat(kHash_Value, &value, 0.0f);
        data->timers.values[i] = value;
    }
    return true;
}

// NOTE: the console reads each variable's "value" (warning when it does
// not parse) but then stores 0; kept.
// @func 0x0038D140 ReadVariables
static bool ReadVariables(KnowledgeData* data, XmlElement* element, uint16_t count)
{
    data->vars.names.assign(count, 0);
    data->vars.types.assign(count, kVar_Int);
    data->vars.values.assign(count, KValue{});
    int i = 0;
    for (XmlElement* var = element->FirstChildElement("KVarData"); var;
         var = var->NextSiblingElement("KVarData"), ++i) {
        const char* typeName = var->GetString("Type");
        if (!typeName)
            return false;
        int32_t type;
        switch (ape::HashNameNoCase(typeName)) {
        case 0x03FE2A3Cu: type = kVar_Float; break;   // "float"
        case 0x0021B90Eu: type = kVar_Hash; break;    // "hash"
        case 0x001F32EAu: type = kVar_Bool; break;    // "bool"
        default: type = kVar_Int; break;
        }
        data->vars.types[i] = type;
        const char* name = var->GetString("VarName");
        if (!name)
            return false;
        data->vars.names[i] = ape::HashNameNoCase(name);
        KValue parsed{};
        switch (type) {
        case kVar_Float:
            if (var->GetFloat(kHash_Value, &parsed.f, 0.0f) != 0)
                Warn("Error to read float value KVarData (%s)", name);
            break;
        case kVar_Bool:
            if (var->GetBool(kHash_Value, &parsed.b, false) != 0)
                Warn("Error to read bool value KVarData (%s)", name);
            break;
        case kVar_Hash:
            if (var->GetHash(kHash_Value, &parsed.u, 0) != 0)
                Warn("Error to read hash value KVarData (%s)", name);
            break;
        default:
            if (var->GetInt(kHash_Value, &parsed.i, 0) != 0)
                Warn("Error to read int value KVarData (%s)", name);
            break;
        }
        data->vars.values[i].u = 0;
    }
    return true;
}

static KnowledgeData* NewListData(uint16_t capacity, uint16_t varCount, uint16_t timerCount)
{
    KnowledgeData* data = new KnowledgeData();
    data->entryCapacity = capacity;
    data->entries.resize(capacity + 1u);   // one spare so NextEntry() stays valid when full
    data->size = (capacity * 0x84u + VarTableSize(varCount) + TimerTableSize(timerCount) + 0xA7u) & ~0xFu;
    data->flags = kList_Template;
    return data;
}

// @func 0x0038D498 BuildKnowledgeList
bool BuildKnowledgeList(KnowledgeList* list, XmlElement* element)
{
    const char* name = element->GetString("Name") ? element->GetString("Name") : nullptr;
    int32_t capacity = kDefaultListSize;
    element->GetInt(kHash_Size, &capacity, kDefaultListSize);
    uint16_t varCount = 0;
    for (XmlElement* e = element->FirstChildElement("KVarData"); e; e = e->NextSiblingElement("KVarData"))
        ++varCount;
    uint16_t timerCount = 0;
    for (XmlElement* e = element->FirstChildElement("KTimer"); e; e = e->NextSiblingElement("KTimer"))
        ++timerCount;

    list->data.reset(NewListData((uint16_t)capacity, varCount, timerCount));
    KnowledgeData* data = list->data.get();
    data->nameHash = ape::HashNameNoCase(name);
    ReadVariables(data, element, varCount);
    ReadTimers(data, element, timerCount);

    const int16_t first = BeginState(data, "Global", kHash_Global);
    for (XmlElement* unit = element->FirstChildElement("KUnit"); unit;
         unit = unit->NextSiblingElement("KUnit"))
        BuildUnit(list, unit, "Global");
    EndState(data, "Global", kHash_Global, first);

    for (XmlElement* state = element->FirstChildElement("KnowledgeState"); state;
         state = state->NextSiblingElement("KnowledgeState"))
        BuildState(list, state);
    return true;
}

// @func 0x0038D91C MakeEmptyKnowledgeList
void MakeEmptyKnowledgeList(KnowledgeList* list, const char* name, uint16_t entryCapacity,
                            uint16_t varCount, uint16_t timerCount)
{
    list->data.reset(NewListData(entryCapacity, varCount, timerCount));
    KnowledgeData* data = list->data.get();
    data->vars.names.assign(varCount, 0);
    data->vars.types.assign(varCount, kVar_Int);
    data->vars.values.assign(varCount, KValue{});
    data->nameHash = ape::HashNameNoCase(name);
    data->timers.names.assign(timerCount, 0);
    data->timers.values.assign(timerCount, 0.0f);
}

// Console sizes of the block parts (tables 0x0038BA10, block 0x0038BA6C,
// block with the runtime area 0x0038B990).
// @func 0x0038BA10 KnowledgeTablesSize
static uint32_t TablesSize(const KnowledgeData* data)
{
    return VarTableSize((uint32_t)data->vars.names.size()) +
           TimerTableSize((uint32_t)data->timers.names.size());
}
// @func 0x0038BA6C KnowledgeBlockSize
static uint32_t BlockSize(const KnowledgeData* data)
{
    return data->entryCount * 0x84u + TablesSize(data) + 0x98u;
}
// @func 0x0038B990 KnowledgeBlockSizeWithRuntime
static uint32_t BlockSizeWithRuntime(const KnowledgeData* data)
{
    return BlockSize(data) + kRuntimeAreaSize;
}

// A clone holds only the used entries; it is not a template.
// @func 0x0038BAE0 CloneKnowledgeList
KnowledgeData* CloneKnowledgeList(KnowledgeList* list, KnowledgeData* source, Entity* owner,
                                  bool withRuntime)
{
    const uint32_t size = ((withRuntime ? BlockSizeWithRuntime(source) : BlockSize(source)) + 0xF) & ~0xFu;
    KnowledgeData* data = new KnowledgeData();
    list->data.reset(data);
    data->nameHash = source->nameHash;
    data->stateSlotCount = source->stateSlotCount;
    data->currentState = source->currentState;
    data->requestedState = source->requestedState;
    data->flags = (uint8_t)(source->flags & ~kList_Template);
    std::copy(std::begin(source->states), std::end(source->states), std::begin(data->states));
    data->entryCount = source->entryCount;
    data->entryCapacity = source->entryCount;
    data->entries.assign(source->entries.begin(), source->entries.begin() + source->entryCount);
    data->entries.emplace_back();
    data->vars = source->vars;
    data->timers = source->timers;
    data->size = size;
    data->owner = owner;

    for (int i = 0; i < data->entryCount; ++i) {
        KEntry* entry = data->Entry(i);
        if (entry->type == kEntry_Unit) {
            CloneUnit(&entry->unit, data);
        } else if (entry->type == kEntry_Result) {
            if (KRefFn clone = g_resultTypes[entry->result.ref.type].clone)
                clone(&entry->result.ref, data);
        }
    }
    if (withRuntime) {
        data->flags |= kList_HasRuntime;
        data->runtime.reset(new KListRuntime());
    }
    return data;
}

// Resets the results of running states and lets standalone results free
// their objects.
// @func 0x0038DC0C FreeKnowledgeList
void FreeKnowledgeList(KnowledgeList* list)
{
    KnowledgeData* data = list->data.get();
    if (!data)
        return;
    if ((data->flags & kList_HasRuntime) && data->runtime) {
        for (int16_t index : data->runtime->stateEntries) {
            if (index == -1)
                continue;
            KEntry* entry = data->Entry(index);
            KUnit& unit = entry->unit;
            if (!unit.isContainer) {
                if (KRefFn reset = g_resultTypes[unit.result.type].reset)
                    reset(&unit.result, data);
                unit.resultState = 0;
            } else {
                ResetContainer(&unit, data);
            }
        }
    }
    for (int i = 0; i < data->entryCount; ++i) {
        KEntry* entry = data->Entry(i);
        if (entry && entry->type == kEntry_Result) {
            if (KRefFn release = g_resultTypes[entry->result.ref.type].release)
                release(&entry->result.ref, data);
        }
    }
    list->data.reset();
}

// @func 0x0038DF4C AddStandaloneResult
bool AddStandaloneResult(KnowledgeList* list, XmlElement* element, int16_t* entryIndex)
{
    KnowledgeData* data = list->data.get();
    *entryIndex = (int16_t)data->entryCount;
    KEntry& entry = NextEntry(data);
    entry.type = kEntry_Result;
    KRef& ref = entry.result.ref;
    ref.type = s_registry->FindResult(element->GetNameHash());
    int16_t dataBlock = -1;
    if (ref.type == -1) {
        Warn("Cannot find a result type (%s) in the system in (%s %d:%d)",
             ape::TempHexString(element->GetNameHash()), DocPath(element), Row(element) + 1, 1);
    } else {
        AllocResultData(g_resultTypes[ref.type].def, list, kBlock_RuleResult, element, &dataBlock);
    }
    ref.dataBlock = dataBlock;
    if (ref.type == -1)
        return false;
    ref.defBlock = g_resultTypes[ref.type].def->defBlockIndex;
    ref.runtime = -1;
    entry.result.state = 0;
    ++data->entryCount;
    ref.runtime = g_resultTypes[ref.type].allocRuntime(ref.type, data);
    if (KRefFn link = g_resultTypes[ref.type].link)
        link(&ref, data);
    return true;
}

// Makes the list a request asks for.
// @func 0x0038DB04 MakeKnowledgeList
static void MakeKnowledgeList(KnowledgeList* list, const KListRequest& request)
{
    if (request.empty) {
        MakeEmptyKnowledgeList(list, request.name, request.entryCapacity, request.varCount, 0);
    } else if (request.fromXml) {
        if (!BuildKnowledgeList(list, request.element))
            Warn("Fail to load knowledge list (%s) from file (%s)", DocPath(request.element),
                 request.element->GetString("Name"));
    } else {
        KnowledgeData* source = request.source->data.get();
        if (source->flags & kList_Broken)
            Warn("Ignore cloning operation of the knowledge list (%s) since it contains error!", "");
        else
            CloneKnowledgeList(list, source, request.owner, request.withRuntime);
    }
}

// ---------------------------------------------------------------------------
// Library

KnowledgeLibrary::KnowledgeLibrary(int capacity_)
    : capacity(capacity_)
{
    slots.resize((size_t)capacity);
    for (auto& slot : slots)
        slot.reset(new KnowledgeList());
    used.assign((size_t)capacity, 0);
    serial.assign((size_t)capacity, 0);
}

KnowledgeLibrary::~KnowledgeLibrary() = default;

KnowledgeList* KnowledgeLibrary::FindTemplate(uint32_t nameHash) const
{
    for (const auto& entry : templates)
        if (entry.first == nameHash)
            return entry.second;
    return nullptr;
}

// @func 0x00390DC8 KnowledgeLibrary::Add
KnowledgeList* KnowledgeLibrary::Add(const KListRequest& request, uint32_t* handle)
{
    if (count >= capacity)
        return nullptr;
    if (request.element) {
        const char* name = request.element->GetString("Name");
        if (FindTemplate(ape::HashNameNoCase(name))) {
            Warn("Fail to add KList (%s) to the KList Def Lib, duplicate KList def found!", name);
            return nullptr;
        }
    }
    int index = 0;
    while (index < capacity && used[(size_t)index])
        ++index;
    static uint16_t s_serial = 0;
    s_serial = (uint16_t)(s_serial == 0xFFFF ? 1 : s_serial + 1);
    used[(size_t)index] = 1;
    serial[(size_t)index] = s_serial;
    ++count;
    *handle = (uint32_t)index << 16 | s_serial;

    KnowledgeList* list = slots[(size_t)index].get();
    MakeKnowledgeList(list, request);
    if (list->data && (list->data->flags & kList_Template))
        templates.emplace_back(list->data->nameHash, list);
    return list;
}

// @func 0x00390724 KnowledgeLibrary::Clear
void KnowledgeLibrary::Clear()
{
    for (int i = 0; i < capacity; ++i) {
        if (used[(size_t)i])
            FreeKnowledgeList(slots[(size_t)i].get());
        used[(size_t)i] = 0;
    }
    count = 0;
    templates.clear();
}

// @func 0x00398DAC KnowledgeLibrary::Remove
void KnowledgeLibrary::Remove(uint32_t handle)
{
    const uint16_t handleSerial = (uint16_t)(handle & 0xFFFF);
    const int index = (int)(handle >> 16);
    if (handleSerial == 0 || index >= capacity || serial[(size_t)index] != handleSerial || !used[(size_t)index])
        return;
    KnowledgeList* list = slots[(size_t)index].get();
    if (list->data && (list->data->flags & kList_Template)) {
        for (auto it = templates.begin(); it != templates.end(); ++it) {
            if (it->second == list) {
                templates.erase(it);
                break;
            }
        }
    }
    FreeKnowledgeList(list);
    used[(size_t)index] = 0;
    --count;
}

// @func 0x00390C1C KnowledgeLibrary::FreeInstances
void KnowledgeLibrary::FreeInstances()
{
    for (int i = 0; i < capacity; ++i) {
        if (!used[(size_t)i])
            continue;
        KnowledgeList* list = slots[(size_t)i].get();
        if (list->data && (list->data->flags & kList_Template))
            continue;
        FreeKnowledgeList(list);
        used[(size_t)i] = 0;
        --count;
    }
}

// ---------------------------------------------------------------------------
// Module

// @func 0x00390500 CreateKnowledgeSystem
void CreateKnowledgeSystem(void* aiManager)
{
    s_aiManager = aiManager;
    s_pool = new KDataPool();
    new KTypeRegistry();   // registers itself
    s_library = new KnowledgeLibrary(0x100);
    s_library->growable = false;
    RegisterKnowledgeChunkHandlers();
}

// @func 0x00390928 DestroyKnowledgeSystem
void DestroyKnowledgeSystem()
{
    s_aiManager = nullptr;
    if (s_library) {
        s_library->Clear();
        delete s_library;
        s_library = nullptr;
    }
    delete s_registry;
    s_registry = nullptr;
    delete s_pool;
    s_pool = nullptr;
}

// @func 0x00390BA8 KnowledgeInit
void KnowledgeInit() {}

// @func 0x00390C18 KnowledgeUpdate
void KnowledgeUpdate() {}

// @func 0x00390BAC KnowledgeReset
void KnowledgeReset()
{
    s_library->Clear();
    s_pool->strings->Reset(s_pool->strings->Capacity());
    s_pool->count = (uint32_t)s_pool->fixedCount;
}

// @func 0x00390D9C KnowledgeLevelStart
void KnowledgeLevelStart()
{
    s_library->FreeInstances();
}

// @func 0x00390DC4 KnowledgeLevelEnd
void KnowledgeLevelEnd() {}

// Loads every "KnowledgeScript" of a script document into the library.
// @func 0x0039129C LoadKnowledgeScriptFile
static bool LoadKnowledgeScriptFile(void* owner, ape::file::File* file)
{
    (void)owner;
    XmlDocument document;
    if (!file || !document.LoadFromStream(*file, nullptr)) {
        Warn("Fail to use the script file ID (%s) to load up the setting file!", s_scriptFileName);
        return false;
    }
    XmlElement* root = document.FirstChildElement("KnowledgeScriptFile");
    if (!root) {
        Warn("Cannot find the script root tag (%s) of the script xml file (%s)!", "KnowledgeScriptFile",
             document.GetDocumentPath());
        return false;
    }
    XmlElement* script = root->FirstChildElement("KnowledgeScript");
    if (!script) {
        Warn("Cannot find the script tag (%s) under the root tag (%s) in the script file (%s)!",
             "KnowledgeScript", "KnowledgeScriptFile", document.GetDocumentPath());
        return false;
    }
    for (; script; script = script->NextSiblingElement("KnowledgeScript")) {
        KListRequest request;
        request.fromXml = true;
        request.withRuntime = true;
        request.element = script;
        request.entryCapacity = kDefaultListSize;
        uint32_t handle = 0;
        s_library->Add(request, &handle);
    }
    return true;
}

bool LoadKnowledgeScript(void* owner, const char* data, uint32_t size)
{
    ape::file::File* file = ape::file::CreateMemoryFile(const_cast<char*>(data), size, s_scriptFileName);
    const bool ok = LoadKnowledgeScriptFile(owner, file);
    if (file)
        file->Close();
    return ok;
}

// 'KNOF': form around a knowledge script.
// @func 0x00391460 ChunkKnowledgeForm
static int ChunkKnowledgeForm(ape::iff::IffReader* reader, void*)
{
    s_scriptFileName = nullptr;
    reader->ParseChunks(nullptr);
    return 1;
}

// 'KNOH': the script's file name (a length byte and 64 characters); also
// listed in the level definition under "Knowledge".
// @func 0x00391498 ChunkKnowledgeHeader
static int ChunkKnowledgeHeader(ape::iff::IffReader* reader, void*)
{
    uint8_t length = 0;
    reader->ReadU8(&length);
    for (int i = 0; i < 0x40; ++i)
        reader->ReadU8(reinterpret_cast<uint8_t*>(&s_scriptFileNameBuffer[1 + i]));
    s_scriptFileName = &s_scriptFileNameBuffer[1];
    if (ape::ApeLevelDef* level = CurrentLevelDef())
        level->AddStringToList(kHash_Knowledge, s_scriptFileName);
    return 1;
}

// 'KNOD': the compiled script document.
// @func 0x00391598 ChunkKnowledgeData
static int ChunkKnowledgeData(ape::iff::IffReader* reader, void*)
{
    const uint32_t size = reader->GetChunkSize();
    void* buffer = ape::GetMemStack()->Alloc(size);
    reader->Read(buffer, size);
    ape::file::File* file = ape::file::CreateMemoryFile(buffer, size, s_scriptFileName);
    LoadKnowledgeScriptFile(s_aiManager, file);
    if (file)
        file->Close();
    ape::GetMemStack()->Free(buffer);
    s_scriptFileName = nullptr;
    return 1;
}

void RegisterKnowledgeChunkHandlers()
{
    using ape::iff::MakeId;
    ape::iff::RegisterChunkHandler(MakeId('K', 'N', 'O', 'F'), ChunkKnowledgeForm, 0, nullptr, 0);
    ape::iff::RegisterChunkHandler(MakeId('K', 'N', 'O', 'H'), ChunkKnowledgeHeader, 0, nullptr, 0);
    ape::iff::RegisterChunkHandler(MakeId('K', 'N', 'O', 'D'), ChunkKnowledgeData, 0, nullptr, 0);
}

} // namespace knowledge
} // namespace sly
