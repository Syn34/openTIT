// openTIT - Ape engine binary XML ("XMLB", *.xmb) document reader.
#include "ape/xml/ApeXml.h"

#include "ape/core/ApeString.h"
#include "ape/file/ApeFile.h"

#include <cassert>
#include <cstring>
#include <utility>

namespace ape {
namespace xml {

namespace {

constexpr uint32_t kHeaderRootCountOffset = 0x10;
constexpr uint32_t kHeaderRootsOffset = 0x14;

uint32_t ReadBE32(const void* p)
{
    const uint8_t* b = static_cast<const uint8_t*>(p);
    return (uint32_t)b[0] << 24 | (uint32_t)b[1] << 16 | (uint32_t)b[2] << 8 | b[3];
}

uint16_t ReadBE16(const void* p)
{
    const uint8_t* b = static_cast<const uint8_t*>(p);
    return (uint16_t)(b[0] << 8 | b[1]);
}

float ReadBEFloat(const void* p)
{
    const uint32_t bits = ReadBE32(p);
    float f;
    std::memcpy(&f, &bits, sizeof(f));
    return f;
}

void SwapInPlace32(void* p)
{
    const uint32_t v = ReadBE32(p);
    std::memcpy(p, &v, sizeof(v));
}

void SwapInPlace16(void* p)
{
    const uint16_t v = ReadBE16(p);
    std::memcpy(p, &v, sizeof(v));
}

AttributeType TypeOf(const XmlNodeData* node, uint32_t index)
{
    return (AttributeType)((node->typeWords[index >> 3] >> ((index & 7) * 4)) & 0xF);
}

} // namespace

// ---------------------------------------------------------------------------
// XmlNode
// ---------------------------------------------------------------------------

// @func 0x0079B94C XmlNode::~XmlNode
// @func 0x0079B95C XmlNode::~XmlNode (deleting)
XmlNode::~XmlNode() = default;

// @func 0x0079B9B0 XmlNode::ToText
void* XmlNode::ToText() { return nullptr; }

// @func 0x0079B9B8 XmlNode::ToComment
void* XmlNode::ToComment() { return nullptr; }

// @func 0x0079B9C0 XmlNode::ToUnknown
void* XmlNode::ToUnknown() { return nullptr; }

// @func 0x0079B9C8 XmlNode::ToDeclaration
void* XmlNode::ToDeclaration() { return nullptr; }

// @func 0x0079B9E0 XmlNode::ToText (const)
const void* XmlNode::ToText() const { return nullptr; }

// @func 0x0079B9E8 XmlNode::ToComment (const)
const void* XmlNode::ToComment() const { return nullptr; }

// @func 0x0079B9F0 XmlNode::ToUnknown (const)
const void* XmlNode::ToUnknown() const { return nullptr; }

// @func 0x0079B9F8 XmlNode::ToDeclaration (const)
const void* XmlNode::ToDeclaration() const { return nullptr; }

// ---------------------------------------------------------------------------
// XmlElement
// ---------------------------------------------------------------------------

// @func 0x004F88FC XmlElement::XmlElement
XmlElement::XmlElement() = default;

// @func 0x004F94E0 XmlElement::~XmlElement
// @func 0x004F94F0 XmlElement::~XmlElement (deleting)
XmlElement::~XmlElement() = default;

// @func 0x0079B9A0 XmlElement::ToDocument
XmlDocument* XmlElement::ToDocument() { return nullptr; }

// @func 0x0079BA00 XmlElement::ToElement
XmlElement* XmlElement::ToElement() { return this; }

// @func 0x0079B9D0 XmlElement::ToDocument (const)
const XmlDocument* XmlElement::ToDocument() const { return nullptr; }

// @func 0x0079BA04 XmlElement::ToElement (const)
const XmlElement* XmlElement::ToElement() const { return this; }

const XmlNodeData* XmlElement::Node() const
{
    if (!m_valid || !m_document)
        return nullptr;
    return m_parent ? m_document->Child(m_parent, m_index) : m_document->Root(m_index);
}

// @func 0x004F8944 XmlElement::GetDocumentPath
const char* XmlElement::GetDocumentPath()
{
    if (m_isElement)
        return nullptr;
    XmlDocument* doc = ToDocument();
    return doc ? doc->GetPath() : nullptr;
}

// @func 0x004F9434 XmlElement::GetDocument
XmlDocument* XmlElement::GetDocument()
{
    if (m_valid)
        return m_document;
    return ToDocument();
}

// @func 0x004FA744 XmlElement::GetText
const char* XmlElement::GetText() const
{
    const XmlNodeData* node = Node();
    if (!node)
        return nullptr;
    return m_document->At<char>(node->textOffset);
}

// @func 0x004F61C4 XmlElement::GetLocationString
const char* XmlElement::GetLocationString()
{
    XmlDocument* doc = GetDocument();
    const char* path = doc ? doc->GetDocumentPath() : nullptr;
    return TempFormat("%s %d:%d", path ? path : "", m_row + 1, m_column + 1);
}

// @func 0x004F6234 XmlElement::CountChildren
int XmlElement::CountChildren(const char* name)
{
    int count = 0;
    XmlElement* child = name ? FirstChildElement(name) : FirstChildElement();
    while (child) {
        ++count;
        child = name ? child->NextSiblingElement(name) : child->NextSiblingElement();
    }
    return count;
}

// -- navigation -------------------------------------------------------------

// @func 0x004F899C XmlElement::FirstChildNode
XmlElement* XmlElement::FirstChildNode()
{
    // The document object iterates the top level elements; any element
    // iterates its own children.
    XmlDocument* doc;
    const XmlNodeData* owner = nullptr;
    uint8_t childDepth;
    uint16_t count;
    if (!m_isElement) {
        doc = ToDocument();
        if (!doc || !doc->IsLoaded())
            return nullptr;
        childDepth = 0;
        count = doc->RootCount();
    } else {
        if (!m_valid)
            return nullptr;
        doc = m_document;
        owner = Node();
        childDepth = (uint8_t)(m_depth + 1);
        count = owner->childCount;
    }
    if (count == 0)
        return nullptr;

    XmlElement* child = doc->AllocHandle(childDepth);
    child->m_document = doc;
    child->m_parent = owner;
    child->m_index = 0;
    child->m_depth = childDepth;
    child->m_valid = true;
    child->m_nameHash = owner ? doc->Child(owner, 0)->nameHash : doc->Root(0)->nameHash;
    return child;
}

// @func 0x004F8ACC XmlElement::FirstChildNode(hash)
XmlElement* XmlElement::FirstChildNode(uint32_t nameHash)
{
    for (XmlElement* child = FirstChildNode(); child; child = child->NextSiblingNode()) {
        if (child->m_nameHash == nameHash)
            return child;
    }
    return nullptr;
}

// @func 0x004F8C30 XmlElement::FirstChildNode(name)
XmlElement* XmlElement::FirstChildNode(const char* name)
{
    return FirstChildNode(HashNameNoCase(name));
}

XmlElement* XmlElement::NextSiblingNode()
{
    if (!m_valid)
        return nullptr;
    const uint16_t count = m_parent ? m_parent->childCount : m_document->RootCount();
    const uint16_t next = (uint16_t)(m_index + 1);
    if (next >= count)
        return nullptr;

    XmlElement* sibling = m_document->AllocHandle(m_depth);
    sibling->m_document = m_document;
    sibling->m_parent = m_parent;
    sibling->m_index = next;
    sibling->m_depth = m_depth;
    sibling->m_valid = true;
    sibling->m_nameHash = m_parent ? m_document->Child(m_parent, next)->nameHash
                                   : m_document->Root(next)->nameHash;
    return sibling;
}

// @func 0x004F8C70 XmlElement::NextSiblingNode(hash)
XmlElement* XmlElement::NextSiblingNode(uint32_t nameHash)
{
    for (XmlElement* sibling = NextSiblingNode(); sibling; sibling = sibling->NextSiblingNode()) {
        if (sibling->m_nameHash == nameHash)
            return sibling;
    }
    return nullptr;
}

// @func 0x004F9140 XmlElement::FirstChildElement
XmlElement* XmlElement::FirstChildElement()
{
    XmlElement* node = FirstChildNode();
    while (node && !node->ToElement())
        node = node->NextSiblingNode();
    return node ? node->ToElement() : nullptr;
}

// @func 0x004F92BC XmlElement::FirstChildElement(hash)
XmlElement* XmlElement::FirstChildElement(uint32_t nameHash)
{
    XmlElement* node = FirstChildNode(nameHash);
    while (node && !node->ToElement())
        node = node->NextSiblingNode(nameHash);
    return node ? node->ToElement() : nullptr;
}

// @func 0x004F936C XmlElement::FirstChildElement(name)
XmlElement* XmlElement::FirstChildElement(const char* name)
{
    return FirstChildElement(HashNameNoCase(name));
}

// @func 0x004F8D74 XmlElement::NextSiblingElement
XmlElement* XmlElement::NextSiblingElement()
{
    XmlElement* node = NextSiblingNode();
    while (node && !node->ToElement())
        node = node->NextSiblingNode();
    return node ? node->ToElement() : nullptr;
}

// @func 0x004F8FC8 XmlElement::NextSiblingElement(hash)
XmlElement* XmlElement::NextSiblingElement(uint32_t nameHash)
{
    XmlElement* node = NextSiblingNode(nameHash);
    while (node && !node->ToElement())
        node = node->NextSiblingNode(nameHash);
    return node ? node->ToElement() : nullptr;
}

// @func 0x004F9078 XmlElement::NextSiblingElement(name)
XmlElement* XmlElement::NextSiblingElement(const char* name)
{
    return NextSiblingElement(HashNameNoCase(name));
}

// -- attributes -------------------------------------------------------------

const XmlAttributeData* XmlElement::FindAttribute(uint32_t nameHash, AttributeType* outType,
                                                  const void** outValue) const
{
    const XmlNodeData* node = Node();
    if (!node)
        return nullptr;
    const XmlAttributeData* attrs = m_document->At<XmlAttributeData>(node->attributesOffset);
    // Attributes are sorted by name hash.
    int lo = 0, hi = (int)node->attributeCount - 1;
    while (lo <= hi) {
        const int mid = (lo + hi) >> 1;
        const XmlAttributeData& a = attrs[mid];
        if (nameHash < a.nameHash) {
            hi = mid - 1;
        } else if (nameHash > a.nameHash) {
            lo = mid + 1;
        } else {
            const AttributeType type = TypeOf(node, (uint32_t)mid);
            // String values are prefixed by their precomputed hash.
            const uint8_t* value = m_document->Image() + a.valueOffset;
            if (type == kAttr_String)
                value += 4;
            if (outType) *outType = type;
            if (outValue) *outValue = value;
            return &a;
        }
    }
    return nullptr;
}

// @func 0x004F9534 XmlElement::GetString(hash)
const char* XmlElement::GetString(uint32_t nameHash) const
{
    AttributeType type;
    const void* value;
    if (!FindAttribute(nameHash, &type, &value) || type != kAttr_String)
        return nullptr;
    return static_cast<const char*>(value);
}

// @func 0x004F9628 XmlElement::GetString(name)
const char* XmlElement::GetString(const char* name) const
{
    return GetString(name ? HashNameNoCase(name) : 0u);
}

// @func 0x004F9768 XmlElement::GetRawAttribute
// Returns the value storage; for strings that is the hash prefix.
const void* XmlElement::GetRawAttribute(uint32_t nameHash, AttributeType* outType) const
{
    *outType = kAttr_None;
    const void* value;
    if (!FindAttribute(nameHash, outType, &value))
        return nullptr;
    if (*outType == kAttr_String)
        return static_cast<const uint8_t*>(value) - 4;
    return value;
}

// @func 0x004F986C XmlElement::GetBool
int XmlElement::GetBool(uint32_t nameHash, bool* out, bool defaultValue) const
{
    AttributeType type;
    const void* value;
    *out = defaultValue;
    if (!FindAttribute(nameHash, &type, &value))
        return kQuery_Missing;
    if (type != kAttr_Int)
        return kQuery_WrongType;
    *out = ReadBE32(value) != 0;
    return kQuery_Ok;
}

// @func 0x004F99BC XmlElement::GetInt
int XmlElement::GetInt(uint32_t nameHash, int32_t* out, int32_t defaultValue) const
{
    AttributeType type;
    const void* value;
    *out = defaultValue;
    if (!FindAttribute(nameHash, &type, &value))
        return kQuery_Missing;
    if (type == kAttr_Int) {
        *out = (int32_t)ReadBE32(value);
        return kQuery_Ok;
    }
    if (type == kAttr_Float) {
        *out = (int32_t)ReadBEFloat(value);
        return kQuery_Ok;
    }
    return kQuery_WrongType;
}

// @func 0x004F9B20 XmlElement::GetFloat
int XmlElement::GetFloat(uint32_t nameHash, float* out, float defaultValue) const
{
    AttributeType type;
    const void* value;
    *out = defaultValue;
    if (!FindAttribute(nameHash, &type, &value))
        return kQuery_Missing;
    if (type == kAttr_Int) {
        *out = (float)(int32_t)ReadBE32(value);
        return kQuery_Ok;
    }
    if (type == kAttr_Float) {
        *out = ReadBEFloat(value);
        return kQuery_Ok;
    }
    return kQuery_WrongType;
}

// @func 0x004F9C9C XmlElement::GetVec2
int XmlElement::GetVec2(uint32_t nameHash, float out[2], const float defaultValue[2]) const
{
    AttributeType type;
    const void* value;
    out[0] = defaultValue[0];
    out[1] = defaultValue[1];
    if (!FindAttribute(nameHash, &type, &value))
        return kQuery_Missing;
    if (type != kAttr_Vec2)
        return kQuery_WrongType;
    const uint8_t* v = static_cast<const uint8_t*>(value);
    out[0] = ReadBEFloat(v);
    out[1] = ReadBEFloat(v + 4);
    return kQuery_Ok;
}

// @func 0x004F9E0C XmlElement::GetVec3
int XmlElement::GetVec3(uint32_t nameHash, float out[3], const float defaultValue[3]) const
{
    AttributeType type;
    const void* value;
    out[0] = defaultValue[0];
    out[1] = defaultValue[1];
    out[2] = defaultValue[2];
    if (!FindAttribute(nameHash, &type, &value))
        return kQuery_Missing;
    if (type != kAttr_Vec3)
        return kQuery_WrongType;
    const uint8_t* v = static_cast<const uint8_t*>(value);
    out[0] = ReadBEFloat(v);
    out[1] = ReadBEFloat(v + 4);
    out[2] = ReadBEFloat(v + 8);
    return kQuery_Ok;
}

// @func 0x004F9F9C XmlElement::GetColor
int XmlElement::GetColor(uint32_t nameHash, Color8* out, const Color8* defaultValue) const
{
    AttributeType type;
    const void* value;
    *out = *defaultValue;
    if (!FindAttribute(nameHash, &type, &value))
        return kQuery_Missing;
    if (type != kAttr_Color)
        return kQuery_WrongType;
    std::memcpy(out, value, sizeof(Color8));
    return kQuery_Ok;
}

// @func 0x004FA14C XmlElement::GetHash
// Accepts name-hash attributes, plain ints, and strings (their stored hash).
int XmlElement::GetHash(uint32_t nameHash, uint32_t* out, uint32_t defaultValue) const
{
    AttributeType type;
    const void* value;
    if (!FindAttribute(nameHash, &type, &value)) {
        *out = defaultValue;
        return kQuery_Missing;
    }
    switch (type) {
    case kAttr_String:
        *out = ReadBE32(static_cast<const uint8_t*>(value) - 4);
        return kQuery_Ok;
    case kAttr_Int:
    case kAttr_Hash:
        *out = ReadBE32(value);
        return kQuery_Ok;
    default:
        *out = defaultValue;
        return kQuery_WrongType;
    }
}

// @func 0x004FA2AC XmlElement::GetStringOrNumber
// Strings are returned directly; ints and hashes are formatted with "%d".
int XmlElement::GetStringOrNumber(uint32_t nameHash, const char** out, const char* defaultValue) const
{
    AttributeType type;
    const void* value;
    if (!FindAttribute(nameHash, &type, &value)) {
        *out = defaultValue;
        return kQuery_Missing;
    }
    switch (type) {
    case kAttr_String:
        *out = static_cast<const char*>(value);
        return kQuery_Ok;
    case kAttr_Int:
    case kAttr_Hash:
        *out = TempFormat("%d", (int32_t)ReadBE32(value));
        return kQuery_Ok;
    default:
        *out = defaultValue;
        return kQuery_WrongType;
    }
}

// @func 0x004FA458 XmlElement::GetEnum
int XmlElement::GetEnum(uint32_t nameHash, uint32_t* outIndex, const uint32_t* allowedHashes,
                        uint32_t allowedCount, uint32_t defaultIndex) const
{
    uint32_t hash;
    int result = GetHash(nameHash, &hash, 0);
    if (result == kQuery_Ok) {
        for (uint32_t i = 0; i < allowedCount; ++i) {
            if (allowedHashes[i] == hash) {
                *outIndex = i;
                return kQuery_Ok;
            }
        }
        result = kQuery_NotInList;
    }
    *outIndex = defaultIndex;
    return result;
}

// @func 0x004FA504 XmlElement::GetBool(name)
int XmlElement::GetBool(const char* name, bool* out, bool defaultValue) const
{
    return GetBool(HashNameNoCase(name), out, defaultValue);
}

// @func 0x004FA564 XmlElement::GetInt(name)
int XmlElement::GetInt(const char* name, int32_t* out, int32_t defaultValue) const
{
    return GetInt(HashNameNoCase(name), out, defaultValue);
}

// @func 0x004FA5C4 XmlElement::GetFloat(name)
int XmlElement::GetFloat(const char* name, float* out, float defaultValue) const
{
    return GetFloat(HashNameNoCase(name), out, defaultValue);
}

// @func 0x004FA624 XmlElement::GetVec3(name)
int XmlElement::GetVec3(const char* name, float out[3], const float defaultValue[3]) const
{
    return GetVec3(HashNameNoCase(name), out, defaultValue);
}

// @func 0x004FA684 XmlElement::GetColor(name)
int XmlElement::GetColor(const char* name, Color8* out, const Color8* defaultValue) const
{
    return GetColor(HashNameNoCase(name), out, defaultValue);
}

// @func 0x004FA6E4 XmlElement::GetHash(name)
int XmlElement::GetHash(const char* name, uint32_t* out, uint32_t defaultValue) const
{
    return GetHash(HashNameNoCase(name), out, defaultValue);
}

// ---------------------------------------------------------------------------
// XmlDocument
// ---------------------------------------------------------------------------

// @func 0x004FA7A4 XmlDocument::XmlDocument
XmlDocument::XmlDocument()
{
    m_isElement = false;
    m_path[0] = '\0';
}

// @func 0x004FA888 XmlDocument::XmlDocument(path)
XmlDocument::XmlDocument(const char* path)
{
    m_isElement = false;
    StrCopyBounded(m_path, path ? path : "", sizeof(m_path));
}

// @func 0x004FA9C0 XmlDocument::~XmlDocument
// @func 0x004FAAAC XmlDocument::~XmlDocument (deleting)
XmlDocument::~XmlDocument() = default;

// @func 0x0079BA08 XmlDocument::ToDocument
XmlDocument* XmlDocument::ToDocument() { return this; }

// @func 0x0079B9A8 XmlDocument::ToElement
XmlElement* XmlDocument::ToElement() { return nullptr; }

// @func 0x0079BA0C XmlDocument::ToDocument (const)
const XmlDocument* XmlDocument::ToDocument() const { return this; }

// @func 0x0079B9D8 XmlDocument::ToElement (const)
const XmlElement* XmlDocument::ToElement() const { return nullptr; }

void XmlDocument::SetError(uint32_t code)
{
    // Only the first error is kept.
    if (m_hasError)
        return;
    m_hasError = true;
    m_errorCode = code;
}

uint16_t XmlDocument::RootCount() const
{
    if (m_image.size() < kHeaderRootsOffset)
        return 0;
    uint16_t count;
    std::memcpy(&count, m_image.data() + kHeaderRootCountOffset, sizeof(count));
    return count;
}

const XmlNodeData* XmlDocument::Root(uint32_t index) const
{
    uint32_t offset;
    std::memcpy(&offset, m_image.data() + kHeaderRootsOffset + index * 4, sizeof(offset));
    return At<XmlNodeData>(offset);
}

const XmlNodeData* XmlDocument::Child(const XmlNodeData* parent, uint32_t index) const
{
    const uint32_t* children = At<uint32_t>(parent->childrenOffset);
    return At<XmlNodeData>(children[index]);
}

XmlElement* XmlDocument::AllocHandle(uint8_t depth)
{
    assert(depth < kMaxDepth && "XML tree deeper than the handle pool");
    if (depth >= kMaxDepth)
        depth = kMaxDepth - 1;
    uint8_t& cursor = m_poolCursor[depth];
    XmlElement* handle = &m_pool[depth][cursor];
    cursor = (uint8_t)((cursor + 1) % kHandlesPerDepth);
    return handle;
}

// @func 0x004FABB4 XmlDocument::FixupNode
// Prepares one element (and, recursively, its children) of a freshly loaded
// image for direct access: the console relocated offsets to pointers; on PC
// the structural fields are converted from big-endian to native order and
// offsets are kept relative to the image.
void XmlDocument::FixupNode(uint32_t nodeOffset)
{
    uint8_t* base = m_image.data();
    XmlNodeData* node = reinterpret_cast<XmlNodeData*>(base + nodeOffset);
    SwapInPlace32(&node->nameHash);
    SwapInPlace32(&node->textOffset);
    SwapInPlace32(&node->nameOffset);
    SwapInPlace16(&node->childCount);
    SwapInPlace16(&node->attributeCount);
    SwapInPlace32(&node->childrenOffset);
    SwapInPlace32(&node->attributesOffset);

    const uint32_t typeWordCount = (node->attributeCount + 7u) / 8u;
    for (uint32_t i = 0; i < typeWordCount; ++i)
        SwapInPlace32(&node->typeWords[i]);

    if (node->attributesOffset) {
        XmlAttributeData* attrs = reinterpret_cast<XmlAttributeData*>(base + node->attributesOffset);
        for (uint32_t i = 0; i < node->attributeCount; ++i) {
            SwapInPlace32(&attrs[i].nameHash);
            SwapInPlace32(&attrs[i].valueOffset);
        }
    }
    if (node->childrenOffset) {
        uint32_t* children = reinterpret_cast<uint32_t*>(base + node->childrenOffset);
        for (uint32_t i = 0; i < node->childCount; ++i) {
            SwapInPlace32(&children[i]);
            FixupNode(children[i]);
        }
    }
}

bool XmlDocument::LoadFromMemory(std::vector<uint8_t> image, const char* name)
{
    if (image.size() < kHeaderRootsOffset || std::memcmp(image.data(), "XMLB", 4) != 0)
        return false;
    StrCopyBounded(m_path, name ? name : "", sizeof(m_path));
    m_image = std::move(image);

    uint8_t* base = m_image.data();
    SwapInPlace16(base + kHeaderRootCountOffset);
    const uint16_t roots = RootCount();
    for (uint32_t i = 0; i < roots; ++i) {
        uint8_t* entry = base + kHeaderRootsOffset + i * 4;
        SwapInPlace32(entry);
        uint32_t offset;
        std::memcpy(&offset, entry, sizeof(offset));
        FixupNode(offset);
    }
    m_valid = true;
    return true;
}

// @func 0x004FACCC XmlDocument::LoadFromStream
bool XmlDocument::LoadFromStream(ape::file::File& file, const char* name)
{
    const int64_t size = (int64_t)file.GetSize();
    if (size <= 0) {
        SetError(kXmlError_Empty);
        return false;
    }
    // One extra byte keeps the image NUL-terminated like the original buffer.
    std::vector<uint8_t> image((size_t)size + 1, 0);
    if (file.Read(image.data(), (size_t)size) != (size_t)size) {
        SetError(kXmlError_ReadFailed);
        return false;
    }
    image.resize((size_t)size);
    return LoadFromMemory(std::move(image), name);
}

// @func 0x004FAF10 XmlDocument::LoadFile
bool XmlDocument::LoadFile(int flags)
{
    (void)flags;
    // Swap whatever extension the path has for the compiled ".xmb".
    char path[0x80];
    StrCopyBounded(path, m_path, sizeof(path));
    if (char* dot = std::strrchr(path, '.'))
        *dot = '\0';
    std::strncat(path, ".xmb", sizeof(path) - std::strlen(path) - 1);
    StrCopyBounded(m_path, path, sizeof(m_path));

    ape::file::File* file = ape::file::OpenFile(path, "rb");
    if (!file) {
        SetError(kXmlError_ReadFailed);
        return false;
    }
    const bool ok = LoadFromStream(*file, path);
    file->Close();
    return ok;
}

} // namespace xml
} // namespace ape
