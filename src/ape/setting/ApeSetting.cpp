// openTIT - Setting system (0x004F32F4-0x004F7C48).
#include "ape/setting/ApeSetting.h"

#include "ape/core/ApeString.h"
#include "ape/file/ApeFile.h"
#include "ape/memory/ApeMemStack.h"
#include "ape/sys/ApeIff.h"
#include "ape/sys/ApeStringPool.h"
#include "ape/xml/ApeXml.h"
#include "platform/Platform.h"

#include <algorithm>
#include <cstring>

namespace ape::setting {

using xml::XmlDocument;
using xml::XmlElement;

SettingManager* g_settingManager = nullptr;

namespace {

// Attribute names (0x00F668DC / 0x00F668E0; the array name uses the same
// string, 0x00F668E4).
const char* const kNameAttribute = "Name";
const char* const kInheritAttribute = "Inherit";

// Root element of tag setting documents. NOTE: only the hash is known.
constexpr uint32_t kTagSettingRootHash = 0xC53F7E26;

// Name of the setting file being loaded, set by the header chunk (0x00F668D4).
const char* g_settingFileName = nullptr;

void Warn(const char* text)
{
    plat::Log(plat::LogLevel::Warning, "%s\n", text);
}

const char* ElementLocation(XmlElement* element)
{
    return element->GetLocationString();
}

// Inserts into a multimap the way the console's sorted arrays do: after
// existing equal keys.
template <class M, class V>
void MapAdd(M& map, uint32_t key, V value)
{
    map.emplace(key, value);
}

template <class M>
typename M::mapped_type MapFind(const M& map, uint32_t key)
{
    auto it = map.find(key);
    return it == map.end() ? nullptr : it->second;
}

// Number of child elements of `element`.
uint32_t CountChildElements(XmlElement* element)
{
    uint32_t count = 0;
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement())
        ++count;
    return count;
}

} // namespace

// ---------------------------------------------------------------------------
// SettingDef
// ---------------------------------------------------------------------------

// @func 0x004F6ED8 SettingDef::SettingDef
SettingDef::SettingDef()
{
    childDefs.reserve(8);
    params.reserve(8);
}

// @func 0x004F6F54 SettingDef::~SettingDef
// @func 0x004F7108 SettingDef::~SettingDef (base object)
// @func 0x004F70C0 SettingDef::~SettingDef (deleting)
SettingDef::~SettingDef()
{
    delete dtd;
    dtd = nullptr;
}

// @func 0x004F7778 SettingDef::CreateSetting
Setting* SettingDef::CreateSetting(const char* name)
{
    return new Setting(this, name);
}

// @func 0x004F7128 SettingDef::AddParam
void SettingDef::AddParam(SettingParam& param)
{
    if (param.type < kDTD_Int || param.type > kDTD_Custom)
        return;

    const bool scalar = param.type <= kDTD_String;
    // Parameters flagged kParam_HasDefault get a zero default text; the real
    // default is stored below.
    const DTDValue zero{};
    const void* textDefault = (scalar && param.usage == kParam_HasDefault) ? &zero : nullptr;
    param.attribute = dtd->AddAttribute(param.name, param.type, param.usage, -1, textDefault);
    if (scalar) {
        if (param.type == kDTD_Bool)
            param.attribute->defaultValue.u = param.defaultValue.b ? 1u : 0u;
        else
            param.attribute->defaultValue = param.defaultValue;
    }
    params.push_back(&param);
}

// ---------------------------------------------------------------------------
// Setting
// ---------------------------------------------------------------------------

// @func 0x004F75B4 Setting::Setting
// @func 0x004F77F8 Setting::Setting (base object)
Setting::Setting(SettingDef* def, const char* name)
    : def(def)
{
    const int32_t columns = def->dtd->GetColumnCount();
    if (columns) {
        values = new DTDValue[columns]();
        columnState = new uint32_t[columns]();
    }
    if (def->ChildCount())
        children = new Setting*[def->ChildCount()]();
    if (name) {
        this->name = g_settingManager->StringPool()->Add(name);
        nameHash = HashNameNoCase(name);
    }
}

// @func 0x004F7818 Setting::~Setting
// @func 0x004F79C0 Setting::~Setting (base object)
// @func 0x004F78DC Setting::~Setting (deleting)
Setting::~Setting()
{
    delete[] values;
    delete[] children;
    delete[] columnState;
}

// @func 0x004F7A84 Setting::ResetToDefaults
void Setting::ResetToDefaults()
{
    const int32_t columns = def->dtd->GetColumnCount();
    std::fill(values, values + columns, DTDValue{});
    for (const SettingParam* param : def->params)
        param->ApplyDefault(values);
    // NOTE: the console marks one state per *attribute*, not per column, so
    // the trailing columns of vector/colour attributes keep their state.
    const size_t attributes = def->dtd->GetAttributes().size();
    for (size_t i = 0; i < attributes; ++i)
        columnState[i] = kColumn_Default;
}

// @func 0x004F7C48 Setting::Read
bool Setting::Read(XmlElement* element, bool reload)
{
    const char* elementName = element ? element->GetString(kNameAttribute) : nullptr;
    if (!name && elementName) {
        name = g_settingManager->StringPool()->Add(elementName);
        nameHash = HashNameNoCase(elementName);
    }

    if (element) {
        for (const auto& attributePtr : def->dtd->GetAttributes()) {
            DTDAttribute* attribute = attributePtr.get();
            const int32_t column = attribute->column;
            switch (attribute->type) {
            case kDTD_Int:
            case kDTD_Int1: {
                int32_t value = 0;
                if (element->GetInt(attribute->nameHash, &value, 0) == xml::kQuery_Ok) {
                    values[column].i = value;
                    columnState[column] = kColumn_Read;
                }
                break;
            }
            case kDTD_Hash:
                if (const char* text = element->GetString(attribute->name)) {
                    values[column].u = HashNameNoCase(text);
                    columnState[column] = kColumn_Read;
                }
                break;
            case kDTD_Float: {
                float value = 0.0f;
                if (element->GetFloat(attribute->nameHash, &value, 0.0f) == xml::kQuery_Ok) {
                    values[column].f = value;
                    columnState[column] = kColumn_Read;
                }
                break;
            }
            case kDTD_Bool: {
                bool value = false;
                if (element->GetBool(attribute->nameHash, &value, false) == xml::kQuery_Ok) {
                    values[column].b = value;
                    columnState[column] = kColumn_Read;
                }
                break;
            }
            case kDTD_String:
                if (const char* text = element->GetString(attribute->name)) {
                    char* current = const_cast<char*>(values[column].str);
                    if (!reload) {
                        values[column].str = g_settingManager->StringPool()->Add(text);
                    } else if (current && StrCmp(current, text) == 0) {
                        // unchanged
                    } else if (current && StrLen(text) <= StrLen(current)) {
                        // Reload: reuse the pooled string when the new text fits.
                        StrCopy(current, text);
                    } else {
                        values[column].str = g_settingManager->StringPool()->Add(text);
                    }
                    columnState[column] = kColumn_Read;
                }
                break;
            case kDTD_Vector2: {
                const float zero[2] = {};
                float value[2];
                const int result = element->GetVec2(attribute->nameHash, value, zero);
                if (result == xml::kQuery_Ok) {
                    for (int i = 0; i < 2; ++i) {
                        values[column + i].f = value[i];
                        columnState[column + i] = kColumn_Read;
                    }
                } else if (result == xml::kQuery_WrongType) {
                    plat::Log(plat::LogLevel::Warning,
                              "Data error when read the vector2 attribute (%s) for setting (%s)\n",
                              attribute->name, name ? name : "Null");
                }
                break;
            }
            case kDTD_Vector3: {
                const float zero[3] = {};
                float value[3];
                const int result = element->GetVec3(attribute->nameHash, value, zero);
                if (result == xml::kQuery_Ok) {
                    for (int i = 0; i < 3; ++i) {
                        values[column + i].f = value[i];
                        columnState[column + i] = kColumn_Read;
                    }
                } else if (result == xml::kQuery_WrongType) {
                    plat::Log(plat::LogLevel::Warning,
                              "Data error when read the vector3 attribute (%s) for setting (%s)\n",
                              attribute->name, name ? name : "Null");
                }
                break;
            }
            case kDTD_RGBA: {
                // NOTE: the console default (0x01164CA0) is only used when the
                // attribute is rejected, in which case nothing is stored.
                const xml::Color8 fallback = {};
                xml::Color8 value;
                const int result = element->GetColor(attribute->nameHash, &value, &fallback);
                if (result == xml::kQuery_Ok) {
                    const uint8_t channels[4] = { value.r, value.g, value.b, value.a };
                    for (int i = 0; i < 4; ++i) {
                        values[column + i].u = channels[i];
                        columnState[column + i] = kColumn_Read;
                    }
                } else if (result == xml::kQuery_WrongType) {
                    plat::Log(plat::LogLevel::Warning,
                              "Data error when read the color attribute (%s) for setting (%s)\n",
                              attribute->name, name ? name : "Null");
                }
                break;
            }
            case kDTD_Custom:
                if (def->ReadCustomAttribute(element, attribute, this, values))
                    columnState[column] = kColumn_Read;
                break;
            default:
                break;
            }
            if (attribute->field_58)
                def->OnAttributeRead(attribute, this, values, reload);
        }
    }

    for (uint32_t i = 0; i < def->ChildCount(); ++i) {
        Setting* child = children[i];
        XmlElement* childElement = element ? element->FirstChildElement(child->def->GetName()) : nullptr;
        child->Read(childElement, reload);
    }
    return true;
}

// @func 0x004F4CC0 Setting::InheritFrom
bool Setting::InheritFrom(const Setting* source)
{
    if (source->def->GetTypeHash() != def->GetTypeHash()) {
        plat::Log(plat::LogLevel::Warning,
                  "The source setting type (%s) is different than the dest setting type (%s)\n",
                  source->def->GetName(), def->GetName());
        return false;
    }

    const int32_t columns = def->dtd->GetColumnCount();
    for (int32_t i = 0; i < columns; ++i) {
        if (source->columnState[i] > kColumn_Default) {
            values[i] = source->values[i];
            columnState[i] = kColumn_Inherited;
        }
    }

    bool ok = true;
    for (uint32_t i = 0; i < def->ChildCount(); ++i) {
        if (!children[i]->InheritFrom(source->children[i])) {
            plat::Log(plat::LogLevel::Warning,
                      "Unsuccessfully clone the child setting type (%s) when cloning the source setting "
                      "type (%s) to dest setting type (%s)\n",
                      children[i]->def->GetName(), source->def->GetName(), def->GetName());
            ok = false;
        }
    }
    return ok;
}

// ---------------------------------------------------------------------------
// SettingArrayDef
// ---------------------------------------------------------------------------

SettingArrayDef::~SettingArrayDef()
{
    delete dtd;
}

// ---------------------------------------------------------------------------
// SettingManager
// ---------------------------------------------------------------------------

// @func 0x004F37B0 SettingManager::SettingManager
// @func 0x004F3C88 SettingManager::SettingManager (complete object)
SettingManager::SettingManager(const SettingDefFactory* defs, uint32_t defCount,
                               const SettingArrayDefFactory* arrayTypes, uint32_t arrayTypeCount,
                               int stringPoolSize, bool allocCache, bool rejectDuplicates)
    : m_strings(std::make_unique<ApeStringPool>(stringPoolSize, false, false)),
      m_rejectDuplicates(rejectDuplicates)
{
    m_unnamed.reserve(1500);
    if (allocCache)
        m_cache = std::make_unique<uint8_t[]>(32 * 0x30);

    g_settingManager = this;
    if (defCount)
        RegisterDefs(defs, defCount);
    if (arrayTypeCount)
        RegisterArrayTypes(arrayTypes, arrayTypeCount);

    iff::RegisterChunkHandler(iff::MakeId('S', 'E', 'T', 'F'), &SettingFileChunk, 0, nullptr, 0);
    iff::RegisterChunkHandler(iff::MakeId('S', 'E', 'T', 'H'), &SettingFileHeaderChunk, 0, nullptr, 0);
    iff::RegisterChunkHandler(iff::MakeId('S', 'E', 'T', 'D'), &SettingFileDataChunk, 0, nullptr, 0);
    iff::RegisterChunkHandler(iff::MakeId('G', 'M', 'T', 'F'), &TagSettingFileChunk, 0, nullptr, 0);
    iff::RegisterChunkHandler(iff::MakeId('G', 'M', 'T', 'H'), &TagSettingFileHeaderChunk, 0, nullptr, 0);
    iff::RegisterChunkHandler(iff::MakeId('G', 'M', 'T', 'D'), &TagSettingFileDataChunk, 0, nullptr, 0);
}

// @func 0x004F3CB4 SettingManager::~SettingManager
// @func 0x004F41AC SettingManager::~SettingManager (base object)
// @func 0x004F4164 SettingManager::~SettingManager (deleting)
SettingManager::~SettingManager()
{
    g_settingManager = nullptr;
    DeleteSettingArrays();
    for (auto& entry : m_arrayTypes)
        delete entry.second;
    for (auto& entry : m_named)
        delete entry.second;
    for (Setting* setting : m_unnamed)
        delete setting;
    for (auto& entry : m_defs)
        delete entry.second;
}

void SettingManager::DeleteSettingArrays()
{
    for (auto& entry : m_arrays)
        delete entry.second;
    m_arrays.clear();
}

// @func 0x004F3554 SettingManager::RegisterDefs
void SettingManager::RegisterDefs(const SettingDefFactory* factories, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) {
        SettingDef* def = factories[i]();
        MapAdd(m_defs, def->GetTypeHash(), def);
    }
    // In type hash order.
    for (auto& entry : m_defs) {
        SettingDef* def = entry.second;
        def->dtd = new ApeDTD(def->GetName(), 0, 0);
        def->InitParams();
    }
}

// @func 0x004F32F4 SettingManager::RegisterArrayTypes
void SettingManager::RegisterArrayTypes(const SettingArrayDefFactory* factories, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) {
        SettingArrayDef* type = factories[i]();
        MapAdd(m_arrayTypes, type->GetTypeHash(), type);
    }
    for (auto& entry : m_arrayTypes) {
        SettingArrayDef* type = entry.second;
        type->dtd = new ApeDTD(type->GetName(), 0, 0);
        type->Init();
    }
}

// @func 0x004F41D8 SettingManager::Clear
void SettingManager::Clear()
{
    // The arrays only reference settings that are also registered below.
    DeleteSettingArrays();

    for (auto& entry : m_named)
        delete entry.second;
    m_named.clear();

    for (auto& entry : m_defs)
        entry.second->defaultSetting = nullptr;

    for (Setting* setting : m_unnamed)
        delete setting;
    m_unnamed.clear();

    m_strings->Reset(m_strings->Capacity());
    m_rootNamedCount = 0;
}

// @func 0x004F41CC SettingManager::ResetRootNamedCount
void SettingManager::ResetRootNamedCount()
{
    m_rootNamedCount = 0;
}

SettingDef* SettingManager::FindDef(uint32_t typeHash) const
{
    return MapFind(m_defs, typeHash);
}

Setting* SettingManager::FindSetting(uint32_t nameHash) const
{
    return MapFind(m_named, nameHash);
}

SettingArray* SettingManager::FindSettingArray(uint32_t nameHash) const
{
    return MapFind(m_arrays, nameHash);
}

// @func 0x004F480C SettingManager::Register
void SettingManager::Register(Setting* setting)
{
    if (setting->registered)
        return;
    if (setting->nameHash == 0) {
        m_unnamed.push_back(setting);
    } else {
        MapAdd(m_named, setting->nameHash, setting);
        if (!setting->isChild)
            ++m_rootNamedCount;
    }
    setting->registered = true;
    for (uint32_t i = 0; i < setting->def->ChildCount(); ++i)
        Register(setting->children[i]);
}

// @func 0x004F496C SettingManager::CreateSettingTree
Setting* SettingManager::CreateSettingTree(const char* name, SettingDef* def)
{
    Setting* setting = def->CreateSetting(name);
    setting->ResetToDefaults();
    for (SettingDef* childDef : def->childDefs) {
        Setting* child = CreateSettingTree(nullptr, childDef);
        child->isChild = true;
        setting->children[setting->childCount++] = child;
    }
    return setting;
}

// @func 0x004F57CC SettingManager::GetDefaultSetting
Setting* SettingManager::GetDefaultSetting(SettingDef* def)
{
    if (!def->defaultSetting) {
        Setting* setting = def->CreateSetting(nullptr);
        def->defaultSetting = setting;
        setting->ResetToDefaults();
        for (SettingDef* childDef : def->childDefs) {
            Setting* child = GetDefaultSetting(childDef);
            child->isChild = true;
            setting->children[setting->childCount++] = child;
        }
    }
    return def->defaultSetting;
}

// @func 0x004F5998 SettingManager::CreateSettingFromXml
Setting* SettingManager::CreateSettingFromXml(const char* name, SettingDef* def, XmlElement* element)
{
    if (!element)
        return GetDefaultSetting(def);

    Setting* setting = def->CreateSetting(name);
    setting->ResetToDefaults();
    for (uint32_t i = 0; i < def->ChildCount(); ++i) {
        SettingDef* childDef = def->childDefs[i];
        XmlElement* childElement = element->FirstChildElement(childDef->GetName());
        Setting* child = nullptr;
        // NOTE: the console tests the *parent's* Inherit attribute here.
        if (!element->GetString(kInheritAttribute)) {
            child = CreateSettingFromXml(nullptr, childDef, childElement);
        } else {
            const char* inheritName = element->GetString(kInheritAttribute);
            if (!childElement && inheritName) {
                const uint32_t hash = HashNameNoCase(inheritName);
                Setting* base = hash ? FindSetting(hash) : nullptr;
                if (!base)
                    plat::Log(plat::LogLevel::Warning,
                              "Warning! Cannot find the setting (%s) from the setting manager!\n", inheritName);
                // Without an element of its own the child shares the base's child.
                if (base)
                    child = base->children[i];
            }
            if (!child)
                child = CreateSettingTree(nullptr, childDef);
        }
        child->isChild = true;
        setting->children[setting->childCount++] = child;
    }
    return setting;
}

// @func 0x004F5428 SettingManager::ApplyInherit
bool SettingManager::ApplyInherit(XmlElement* element, Setting* setting)
{
    SettingDef* def = setting->def;
    const char* inheritName = element->GetString(kInheritAttribute);
    if (inheritName) {
        const uint32_t hash = HashNameNoCase(inheritName);
        Setting* base = hash ? FindSetting(hash) : nullptr;
        if (!base) {
            const char* name = setting->name ? setting->name : element->GetString(kNameAttribute);
            // NOTE: the console also names the parent element's "Name" in the
            // message; the parent is not reachable through this XML port.
            const char* detail;
            if (name)
                detail = TempFormat("Make sure the inherit setting is in front of this setting (%s: type %s)! (%s)",
                                    name, def->GetName(), ElementLocation(element));
            else
                detail = TempFormat("Make sure the inherit setting is in front of this setting (Unknown name: "
                                    "type %s)! (%s)",
                                    def->GetName(), ElementLocation(element));
            plat::Log(plat::LogLevel::Warning,
                      "Fail to find the inherit setting(%s), no inherit operation will be performed! %s\n",
                      inheritName, detail);
            return false;
        }
        if (!setting->InheritFrom(base)) {
            plat::Log(plat::LogLevel::Warning,
                      "Fail to fully inherit all the setting data from setting (%s) to setting (%s) (%s)\n",
                      base->name, setting->name, ElementLocation(element));
            return false;
        }
    }

    for (uint32_t i = 0; i < def->ChildCount(); ++i) {
        Setting* child = setting->children[i];
        XmlElement* childElement = element->FirstChildElement(child->def->GetName());
        if (childElement && childElement->GetString(kInheritAttribute))
            ApplyInherit(childElement, child);
    }
    return true;
}

// @func 0x004F5CEC SettingManager::ReadSettingElement
bool SettingManager::ReadSettingElement(XmlElement* element, SettingReadContext& ctx)
{
    SettingDef* def = FindDef(element->GetNameHash());
    if (!def)
        return true;   // not a setting element

    const char* name = element->GetString(kNameAttribute);
    if (m_rejectDuplicates && !ctx.reload && name && FindSetting(HashNameNoCase(name))) {
        plat::Log(plat::LogLevel::Warning,
                  "The setting (%s) exist already, the new setting will not be added to the system!\n", name);
        return true;
    }

    Setting* setting = nullptr;
    if (!ctx.reload) {
        setting = CreateSettingFromXml(name, def, element);
        ApplyInherit(element, setting);
    } else {
        // Reloading updates the existing setting in place.
        if (name)
            setting = FindSetting(HashNameNoCase(name));
        if (!setting) {
            setting = CreateSettingTree(name, def);
            ApplyInherit(element, setting);
        }
    }
    setting->Read(element, ctx.reload);
    Register(setting);
    ctx.result = setting;
    return true;
}

// @func 0x004F5EFC SettingManager::RemoveSettingArray
void SettingManager::RemoveSettingArray(const char* name)
{
    if (!name)
        return;
    const uint32_t hash = HashNameNoCase(name);
    auto it = hash ? m_arrays.find(hash) : m_arrays.end();
    if (it == m_arrays.end() || !it->second) {
        plat::Log(plat::LogLevel::Warning,
                  "Warning! Cannot find the setting array (%s) from the setting manager!\n", name);
        return;
    }

    SettingArray* array = it->second;
    m_arrays.erase(it);
    for (Setting* item : array->items) {
        if (item->nameHash == 0) {
            auto found = std::find(m_unnamed.rbegin(), m_unnamed.rend(), item);
            if (found != m_unnamed.rend())
                m_unnamed.erase(std::next(found).base());
        } else {
            auto range = m_named.equal_range(item->nameHash);
            if (range.first != range.second)
                m_named.erase(range.first);
        }
        delete item;
    }
    delete array;
}

// @func 0x004F62C4 SettingManager::ReadSettingArray
bool SettingManager::ReadSettingArray(XmlElement* element, SettingReadContext& ctx)
{
    SettingArrayDef* type = static_cast<SettingArrayDef*>(ctx.type);
    const char* name = element->GetString(kNameAttribute);
    if (!name) {
        plat::Log(plat::LogLevel::Warning,
                  "Fail to read the name of the setting array with setting array type (%s), this setting "
                  "array will be ignored! (%s)\n",
                  type->GetName(), ElementLocation(element));
        return false;
    }

    const uint32_t elementCount = CountChildElements(element);
    if (elementCount == 0) {
        plat::Log(plat::LogLevel::Warning,
                  "There is no child setting in the setting array (%s), the setting array will not be "
                  "created! (%s)\n",
                  name, ElementLocation(element));
        return false;
    }

    if (ctx.reload)
        RemoveSettingArray(name);

    SettingArray* array = new SettingArray;
    array->name = m_strings->Add(name);
    array->nameHash = HashNameNoCase(name);
    array->type = type;
    array->items.reserve(elementCount);

    const std::vector<DTDChild>* elementTypes = type->dtd->GetType3Children();
    for (uint32_t i = 0; i < type->elementTypeCount; ++i) {
        SettingDef* elementDef = FindDef(HashNameNoCase((*elementTypes)[i].name));
        SettingReadContext elementCtx;
        elementCtx.type = elementDef;
        elementCtx.reload = ctx.reload;
        elementCtx.inArray = true;
        const char* typeName = elementDef->GetName();
        for (XmlElement* child = element->FirstChildElement(typeName); child;
             child = child->NextSiblingElement(typeName)) {
            ReadSettingElement(child, elementCtx);
            if (elementCtx.result)
                array->items.push_back(elementCtx.result);
        }
    }

    MapAdd(m_arrays, array->nameHash, array);
    return true;
}

// @func 0x004F457C SettingManager::LoadSettingFile
void SettingManager::LoadSettingFile(XmlDocument& document, bool reload)
{
    XmlElement* rootNode = document.FirstChildNode("SettingFile");
    XmlElement* root = rootNode ? rootNode->ToElement() : nullptr;
    if (!root) {
        plat::Log(plat::LogLevel::Warning,
                  "Fail to find the setting file root element (%s)! The setting file will not be loaded!\n",
                  "SettingFile");
        return;
    }

    SettingReadContext ctx;
    ctx.reload = reload;
    bool ok = true;
    for (XmlElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (!ReadSettingElement(child, ctx))
            ok = false;
    }
    if (!ok)
        plat::Log(plat::LogLevel::Warning, "Getting error when parsing setting file setting (%s)\n",
                  document.GetDocumentPath());

    if (reload)
        return;

    for (auto& entry : m_arrayTypes) {
        SettingArrayDef* type = entry.second;
        SettingReadContext arrayCtx;
        arrayCtx.type = type;
        arrayCtx.reload = reload;
        const char* typeName = type->GetName();
        bool arraysOk = true;
        for (XmlElement* child = root->FirstChildElement(typeName); child;
             child = child->NextSiblingElement(typeName)) {
            if (!ReadSettingArray(child, arrayCtx))
                arraysOk = false;
        }
        if (!arraysOk)
            plat::Log(plat::LogLevel::Warning, "Getting error when parsing setting array type (%s)\n",
                      type->GetName());
    }
}

// @func 0x004F67E4 SettingManager::ReadTagAttributes
bool SettingManager::ReadTagAttributes(Setting* setting, XmlElement* element)
{
    if (!element)
        return true;
    for (const auto& attributePtr : setting->def->dtd->GetAttributes()) {
        DTDAttribute* attribute = attributePtr.get();
        const int32_t column = attribute->column;
        switch (attribute->type) {
        case kDTD_Int:
        case kDTD_Int1:
            element->GetInt(attribute->name, &setting->values[column].i, attribute->defaultValue.i);
            setting->columnState[column] = kColumn_Read;
            break;
        case kDTD_Hash: {
            const char* text = element->GetString(attribute->name);
            setting->values[column].u = text ? HashNameNoCase(text) : attribute->defaultValue.u;
            setting->columnState[column] = kColumn_Read;
            break;
        }
        case kDTD_Float:
            element->GetFloat(attribute->name, &setting->values[column].f, attribute->defaultValue.f);
            setting->columnState[column] = kColumn_Read;
            break;
        case kDTD_Bool:
            element->GetBool(attribute->name, &setting->values[column].b, attribute->defaultValue.u == 1);
            setting->columnState[column] = kColumn_Read;
            break;
        case kDTD_String: {
            const char* text = element->GetString(attribute->name);
            if (!text)
                text = attribute->defaultValue.str;
            setting->values[column].str = text ? m_strings->Add(text) : nullptr;
            setting->columnState[column] = kColumn_Read;
            break;
        }
        case kDTD_Vector2:
            plat::Log(plat::LogLevel::Warning,
                      "Do not support data type DTD_ATTRIBUTE_TYPE_VECTOR2 for tag setting (%s)\n", setting->name);
            break;
        case kDTD_Vector3:
            plat::Log(plat::LogLevel::Warning,
                      "Do not support data type DTD_ATTRIBUTE_TYPE_VECTOR3 for tag setting (%s)\n", setting->name);
            break;
        case kDTD_RGBA:
            plat::Log(plat::LogLevel::Warning,
                      "Do not support data type DTD_ATTRIBUTE_TYPE_RGBA for tag setting (%s)\n", setting->name);
            break;
        case kDTD_Custom:
            plat::Log(plat::LogLevel::Warning,
                      "Do not support data type DTD_ATTRIBUTE_TYPE_CUSTOM for tag setting (%s)\n", setting->name);
            break;
        default:
            break;
        }
    }
    return true;
}

// @func 0x004F6B60 SettingManager::ReadTagSetting
bool SettingManager::ReadTagSetting(XmlElement* element, SettingReadContext& ctx)
{
    const char* typeName = element->GetString("type");
    if (!typeName) {
        Warn("The tag setting node with node type, the system will ignored it!");
        return false;
    }
    SettingDef* def = FindDef(HashNameNoCase(typeName));
    if (!def) {
        plat::Log(plat::LogLevel::Warning,
                  "No such setting def (%s), the tag setting definition will be ignored!\n", typeName);
        return true;
    }

    const char* name = element->GetString("name");
    if (!name)
        plat::Log(plat::LogLevel::Warning,
                  "Fail to read the name of the tag setting with setting type (%s), this setting will be "
                  "ignored! (%s)\n",
                  def->GetName(), ElementLocation(element));

    Setting* setting = nullptr;
    if (ctx.reload && name)
        setting = FindSetting(HashNameNoCase(name));
    if (!setting)
        setting = CreateSettingTree(name, def);

    ReadTagAttributes(setting, element);
    Register(setting);
    ctx.result = setting;
    return true;
}

// @func 0x004F6D94 SettingManager::ReadTagAssociations
bool SettingManager::ReadTagAssociations(XmlElement* element, SettingReadContext& ctx)
{
    const uint32_t hash = HashNameNoCase(element->GetString("name"));
    Setting* setting = hash ? FindSetting(hash) : nullptr;
    // NOTE: the console dereferences the setting without a null check.
    if (!setting)
        return true;
    SettingAssociationFn handler = setting->def->association;
    if (!handler)
        return true;
    for (XmlElement* assoc = element->FirstChildElement("association"); assoc;
         assoc = assoc->NextSiblingElement("association")) {
        if (const char* value = assoc->GetString("value"))
            handler(setting, value, ctx.reload);
    }
    return true;
}

// @func 0x004F4B28 SettingManager::LoadTagSettingFile
void SettingManager::LoadTagSettingFile(XmlDocument& document, bool reload)
{
    XmlElement* root = document.FirstChildElement();
    if (!root || root->GetNameHash() != kTagSettingRootHash)
        return;

    SettingReadContext ctx;
    ctx.reload = reload;

    bool ok = true;
    for (XmlElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (!ReadTagSetting(child, ctx))
            ok = false;
    }
    if (!ok)
        plat::Log(plat::LogLevel::Warning,
                  "Getting error when parsing tag setting file in setting reading phase(%s)\n",
                  document.GetDocumentPath());

    // Associations may refer to any tag setting of the file, so they are
    // read in a second pass.
    ok = true;
    for (XmlElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement()) {
        if (!ReadTagAssociations(child, ctx))
            ok = false;
    }
    if (!ok)
        plat::Log(plat::LogLevel::Warning,
                  "Getting error when parsing tag setting file in association reading phase(%s)\n",
                  document.GetDocumentPath());
}

// ---------------------------------------------------------------------------
// Chunk handlers
// ---------------------------------------------------------------------------

namespace {

// Header chunk payload: one byte (ignored) and the 64-character file name.
const char* ReadFileNameHeader(iff::IffReader* reader, char (&buffer)[0x41])
{
    uint8_t ignored = 0;
    reader->ReadU8(&ignored);
    for (int i = 1; i <= 0x40; ++i)
        reader->ReadU8(reinterpret_cast<uint8_t*>(&buffer[i]));
    // NOTE: the console relies on the zeroed buffer for termination.
    buffer[0x40] = '\0';
    return &buffer[1];
}

using DocumentLoader = void (*)(SettingManager* manager, XmlDocument& document);

// Data chunk payload: a compiled XML document.
void LoadDocumentChunk(iff::IffReader* reader, DocumentLoader load, const char* failure)
{
    const uint32_t size = reader->GetChunkSize();
    if (!size)
        return;
    void* data = GetMemStack()->Alloc(size);
    reader->Read(data, size);
    file::File* stream = file::CreateMemoryFile(data, size, g_settingFileName);
    SettingManager* manager = g_settingManager;
    {
        XmlDocument document;
        if (stream && document.LoadFromStream(*stream, nullptr))
            load(manager, document);
        else
            Warn(failure);
    }
    if (stream)
        stream->Close();
    GetMemStack()->Free(data);
    g_settingFileName = nullptr;
}

char g_settingFileNameBuffer[0x41];      // 0x010C2D20
char g_tagSettingFileNameBuffer[0x41];   // 0x010C2D70

} // namespace

// @func 0x004F4F58 SettingFileChunk
int SettingFileChunk(iff::IffReader* reader, void* arg)
{
    g_settingFileName = nullptr;
    reader->ParseChunks(nullptr);
    return 1;
}

// @func 0x004F4F90 SettingFileHeaderChunk
int SettingFileHeaderChunk(iff::IffReader* reader, void* arg)
{
    g_settingFileName = ReadFileNameHeader(reader, g_settingFileNameBuffer);
    return 1;
}

// @func 0x004F505C SettingFileDataChunk
int SettingFileDataChunk(iff::IffReader* reader, void* arg)
{
    LoadDocumentChunk(
        reader, [](SettingManager* manager, XmlDocument& document) { manager->LoadSettingFile(document, false); },
        "Fail to use the setting file ID to load up the setting file!");
    return 0;
}

// @func 0x004F51C0 TagSettingFileChunk
int TagSettingFileChunk(iff::IffReader* reader, void* arg)
{
    g_settingFileName = nullptr;
    reader->ParseChunks(nullptr);
    return 1;
}

// @func 0x004F51F8 TagSettingFileHeaderChunk
int TagSettingFileHeaderChunk(iff::IffReader* reader, void* arg)
{
    g_settingFileName = ReadFileNameHeader(reader, g_tagSettingFileNameBuffer);
    return 1;
}

// @func 0x004F52C4 TagSettingFileDataChunk
int TagSettingFileDataChunk(iff::IffReader* reader, void* arg)
{
    LoadDocumentChunk(
        reader,
        [](SettingManager* manager, XmlDocument& document) { manager->LoadTagSettingFile(document, false); },
        "Fail to use the tag setting file ID to load up the tag setting file!");
    return 0;
}

} // namespace ape::setting
