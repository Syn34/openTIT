// openTIT - Ape engine binary XML ("XMLB", *.xmb) document reader.
//
// Game data authored as XML ships pre-compiled to a compact binary tree:
// every element and attribute name is replaced by its HashNameNoCase()
// value and attribute values are stored already parsed (int, float,
// vectors, colours, name hashes or strings). The reader exposes a small
// TinyXML-like API on top of that tree.
//
// Element handles are not allocated: the document owns a pool of four
// handles per tree depth and hands them out round-robin, so a returned
// XmlElement* stays valid until four more handles at the same depth have
// been requested. That matches how the engine walks data: shallow loops
// that read a few attributes and move on.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ape { namespace file { class File; } }

namespace ape {
namespace xml {

class XmlDocument;
class XmlElement;

// Type tag of an attribute value (4 bits per attribute in the file).
enum AttributeType : uint32_t {
    kAttr_String = 0,
    kAttr_Int    = 1,
    kAttr_Float  = 2,
    kAttr_Vec2   = 3,
    kAttr_Vec3   = 4,
    kAttr_Color  = 5,
    kAttr_Hash   = 6,
    kAttr_None   = 7,   // reported when the attribute does not exist
};

// Result of the typed attribute getters.
enum QueryResult : int {
    kQuery_Ok          = 0,
    kQuery_Missing     = 1,  // no such attribute; the default was stored
    kQuery_WrongType   = 2,  // attribute exists with another type; default stored
    kQuery_NotInList   = 3,  // GetEnum: value is not one of the allowed names
};

struct Color8 { uint8_t r, g, b, a; };

// Common base of everything in a document tree.
class XmlNode {
public:
    virtual ~XmlNode();
    virtual XmlDocument* ToDocument() = 0;
    virtual XmlElement* ToElement() = 0;
    virtual void* ToText();
    virtual void* ToComment();
    // NOTE: the remaining queries follow the TinyXML layout the class is
    // modelled on (names guessed from that order); nothing here overrides
    // the text/comment/unknown/declaration ones.
    virtual void* ToUnknown();
    virtual void* ToDeclaration();
    virtual const XmlDocument* ToDocument() const = 0;
    virtual const XmlElement* ToElement() const = 0;
    virtual const void* ToText() const;
    virtual const void* ToComment() const;
    virtual const void* ToUnknown() const;
    virtual const void* ToDeclaration() const;
};

// Raw layout of one element inside a loaded image. The structural fields
// are converted to native byte order by XmlDocument::FixupNode(); attribute
// values stay big-endian and are decoded on access.
struct XmlNodeData {
    uint32_t nameHash;
    uint32_t textOffset;      // offset of the element text, 0 if none
    uint32_t nameOffset;      // offset of the original name string, 0 if stripped
    uint16_t childCount;
    uint16_t attributeCount;
    uint32_t childrenOffset;  // -> uint32_t childOffsets[childCount]
    uint32_t attributesOffset;// -> XmlAttributeData[attributeCount], sorted by hash
    uint32_t typeWords[1];    // 4-bit AttributeType per attribute, 8 per word
};

struct XmlAttributeData {
    uint32_t nameHash;
    uint32_t valueOffset;
};

class XmlElement : public XmlNode {
public:
    XmlElement();
    ~XmlElement() override;
    XmlDocument* ToDocument() override;
    XmlElement* ToElement() override;
    const XmlDocument* ToDocument() const override;
    const XmlElement* ToElement() const override;

    bool IsValid() const { return m_valid; }
    uint32_t GetNameHash() const { return m_nameHash; }

    // Path of the owning document if this node is the document itself.
    const char* GetDocumentPath();
    XmlDocument* GetDocument();
    const char* GetText() const;
    // "<document path> <row>:<column>" for diagnostics (rows/columns are 1-based;
    // compiled documents carry no positions so this prints 0:0).
    const char* GetLocationString();
    // Number of child elements, optionally only those with the given name.
    int CountChildren(const char* name);

    // -- navigation (element-only variants skip non-element nodes) --------
    XmlElement* FirstChildNode();
    XmlElement* FirstChildNode(uint32_t nameHash);
    XmlElement* FirstChildNode(const char* name);
    XmlElement* NextSiblingNode();
    XmlElement* NextSiblingNode(uint32_t nameHash);
    XmlElement* FirstChildElement();
    XmlElement* FirstChildElement(uint32_t nameHash);
    XmlElement* FirstChildElement(const char* name);
    XmlElement* NextSiblingElement();
    XmlElement* NextSiblingElement(uint32_t nameHash);
    XmlElement* NextSiblingElement(const char* name);

    // -- attributes ---------------------------------------------------------
    const char* GetString(uint32_t nameHash) const;
    const char* GetString(const char* name) const;
    const void* GetRawAttribute(uint32_t nameHash, AttributeType* outType) const;
    int GetBool(uint32_t nameHash, bool* out, bool defaultValue) const;
    int GetInt(uint32_t nameHash, int32_t* out, int32_t defaultValue) const;
    int GetFloat(uint32_t nameHash, float* out, float defaultValue) const;
    int GetVec2(uint32_t nameHash, float out[2], const float defaultValue[2]) const;
    int GetVec3(uint32_t nameHash, float out[3], const float defaultValue[3]) const;
    int GetColor(uint32_t nameHash, Color8* out, const Color8* defaultValue) const;
    int GetHash(uint32_t nameHash, uint32_t* out, uint32_t defaultValue) const;
    int GetStringOrNumber(uint32_t nameHash, const char** out, const char* defaultValue) const;
    int GetEnum(uint32_t nameHash, uint32_t* outIndex, const uint32_t* allowedHashes,
                uint32_t allowedCount, uint32_t defaultIndex) const;

    int GetBool(const char* name, bool* out, bool defaultValue) const;
    int GetInt(const char* name, int32_t* out, int32_t defaultValue) const;
    int GetFloat(const char* name, float* out, float defaultValue) const;
    int GetVec3(const char* name, float out[3], const float defaultValue[3]) const;
    int GetColor(const char* name, Color8* out, const Color8* defaultValue) const;
    int GetHash(const char* name, uint32_t* out, uint32_t defaultValue) const;

protected:
    friend class XmlDocument;

    // Resolves this handle to its node in the document image.
    const XmlNodeData* Node() const;
    // Node of the element whose children this handle iterates.
    const XmlAttributeData* FindAttribute(uint32_t nameHash, AttributeType* outType,
                                          const void** outValue) const;

    bool m_isElement = true;           // false for the document object itself
    int32_t m_row = -1;                // source position, unknown in compiled data
    int32_t m_column = -1;
    XmlDocument* m_document = nullptr;
    uint32_t m_nameHash = 0;
    bool m_valid = false;
    uint8_t m_depth = 0;
    uint16_t m_index = 0;              // index within the parent's child list
    const XmlNodeData* m_parent = nullptr;  // null for top level elements
};

class XmlDocument : public XmlElement {
public:
    XmlDocument();
    explicit XmlDocument(const char* path);
    ~XmlDocument() override;
    XmlDocument* ToDocument() override;
    XmlElement* ToElement() override;
    const XmlDocument* ToDocument() const override;
    const XmlElement* ToElement() const override;

    // Loads "<path without extension>.xmb".
    bool LoadFile(int flags = 0);
    // Reads a whole compiled document from an open file.
    bool LoadFromStream(ape::file::File& file, const char* name);
    // Takes ownership of an in-memory image (used by both of the above).
    bool LoadFromMemory(std::vector<uint8_t> image, const char* name);

    const char* GetPath() const { return m_path; }
    bool IsLoaded() const { return !m_image.empty(); }
    bool HasError() const { return m_hasError; }
    uint32_t GetErrorCode() const { return m_errorCode; }

    // Image access for elements.
    const uint8_t* Image() const { return m_image.data(); }
    template <class T> const T* At(uint32_t offset) const
    {
        return offset ? reinterpret_cast<const T*>(m_image.data() + offset) : nullptr;
    }
    uint16_t RootCount() const;
    const XmlNodeData* Root(uint32_t index) const;
    const XmlNodeData* Child(const XmlNodeData* parent, uint32_t index) const;

    // Hands out the next pooled handle for the given depth.
    XmlElement* AllocHandle(uint8_t depth);

    static constexpr int kMaxDepth = 16;
    static constexpr int kHandlesPerDepth = 4;

private:
    void SetError(uint32_t code);
    void FixupNode(uint32_t nodeOffset);

    std::vector<uint8_t> m_image;
    char m_path[0x80] = {};
    bool m_hasError = false;
    uint32_t m_errorCode = 0;
    uint8_t m_poolCursor[kMaxDepth] = {};
    XmlElement m_pool[kMaxDepth][kHandlesPerDepth];
};

enum XmlError : uint32_t {
    kXmlError_ReadFailed = 0x02,
    kXmlError_Empty      = 0x0D,
};

} // namespace xml
} // namespace ape
