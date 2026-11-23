// openTIT - ApeDTD: data type definitions (attribute schemas).
#include "ape/level/ApeDTD.h"

#include "ape/core/ApeString.h"

#include <cstring>

namespace ape {

int DTDColumnCount(int32_t type)
{
    switch (type) {
    case kDTD_Vector2: return 2;
    case kDTD_Vector3: return 3;
    case kDTD_RGBA:    return 4;
    default:           return 1;
    }
}

// @func 0x004D2BB8 DTDAttribute::AllocEnumNames
void DTDAttribute::AllocEnumNames(uint32_t count)
{
    enumCapacity = count;
    enumNames.clear();
    enumNames.reserve(count);
}

void DTDAttribute::AddEnumName(const char* value)
{
    if (enumNames.size() >= enumCapacity)
        return;
    char buffer[100];
    StrCopyBounded(buffer, value ? value : "", sizeof(buffer));
    enumNames.emplace_back(buffer);
}

// @func 0x004D2BFC ApeDTD::ApeDTD
ApeDTD::ApeDTD(const char* name, uint32_t attributeCapacity, uint32_t childCapacity)
    : m_name(name)
{
    m_attributes.reserve(attributeCapacity ? attributeCapacity : 2);
    if (childCapacity) {
        m_children = std::make_unique<std::vector<DTDChild>>();
        m_children->reserve(childCapacity);
        m_type3Children = std::make_unique<std::vector<DTDChild>>();
        m_type3Children->reserve(childCapacity);
    }
    m_flags &= 0x3F;
}

// @func 0x004D2D90 ApeDTD::~ApeDTD
// @func 0x004D3040 ApeDTD::~ApeDTD (deleting)
ApeDTD::~ApeDTD() = default;

// @func 0x004D3088 ApeDTD::AddAttribute
DTDAttribute* ApeDTD::AddAttribute(const char* name, int32_t type, int32_t field3C, int32_t column,
                                   const void* defaultValue)
{
    if (column == -1)
        column = m_columnCount;

    auto attribute = std::make_unique<DTDAttribute>();
    attribute->type = type;
    attribute->field_3C = field3C;
    attribute->column = column;
    StrCopyBounded(attribute->name, name ? name : "", sizeof(attribute->name));
    attribute->nameHash = HashNameNoCase(attribute->name);

    if (defaultValue) {
        const char* text = nullptr;
        switch (type) {
        case kDTD_String:
            text = *static_cast<const char* const*>(defaultValue);
            break;
        case kDTD_Bool:
            attribute->defaultValue.b = *static_cast<const bool*>(defaultValue);
            text = TempFormat("%d", attribute->defaultValue.b ? 1 : 0);
            break;
        case kDTD_Float:
            attribute->defaultValue.f = *static_cast<const float*>(defaultValue);
            text = TempFormat("%.2f", double(attribute->defaultValue.f));
            break;
        case kDTD_Int:
        case kDTD_Int1:
        case kDTD_Hash:
            attribute->defaultValue.i = *static_cast<const int32_t*>(defaultValue);
            text = TempFormat("%d", attribute->defaultValue.i);
            break;
        default:
            // NOTE: the PS3 code has no text for vector/colour/custom defaults
            // and would dereference null here; such defaults are ignored.
            break;
        }
        if (text) {
            attribute->defaultString = text;
            attribute->hasDefaultString = true;
            if (type == kDTD_String)
                attribute->defaultValue.str = attribute->defaultString.c_str();
        }
    }

    DTDAttribute* result = attribute.get();
    m_attributes.push_back(std::move(attribute));
    m_columnCount += DTDColumnCount(type);
    return result;
}

// @func 0x004D3378 ApeDTD::AddChild
bool ApeDTD::AddChild(const char* name, int32_t type)
{
    if (!m_children) {
        m_children = std::make_unique<std::vector<DTDChild>>();
        m_children->reserve(5);
    }
    if (!m_type3Children) {
        m_type3Children = std::make_unique<std::vector<DTDChild>>();
        m_type3Children->reserve(5);
    }
    DTDChild child;
    StrCopyBounded(child.name, name ? name : "", sizeof(child.name));
    child.type = type;
    if (type == 3)
        m_type3Children->push_back(child);
    else
        m_children->push_back(child);
    ++m_elementCount;
    return true;
}

} // namespace ape
