// openTIT - Localised string table (hash -> text) loaded from a compiled file.
#pragma once

#include <cstdint>
#include <vector>

namespace ape {

class StringTable {
public:
    explicit StringTable(const char* path);
    virtual ~StringTable();

    // Looks up a string by id hash / id name. When the id is missing the
    // fallback (or "" if none) is returned; `quiet` suppresses the warning.
    const char* Find(uint32_t idHash, const char* fallback, bool quiet) const;
    const char* Find(const char* idName, const char* fallback, bool quiet) const;

    bool IsLoaded() const { return !m_image.empty(); }

private:
    // File layout (big-endian): 0x24-byte header, then the tables it points at.
    struct Header {
        uint8_t  unknown[0x14];
        uint32_t count;          // +0x14 number of (hash, index) pairs
        uint32_t pairsOffset;    // +0x18 pairs sorted by hash
        uint32_t offsetsOffset;  // +0x1C index -> text offset (relative to textOffset)
        uint32_t textOffset;     // +0x20 start of the text blob
    };
    const char* FindByHash(uint32_t idHash) const;

    std::vector<uint8_t> m_image;
};

} // namespace ape
