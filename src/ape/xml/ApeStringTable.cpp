// openTIT - Localised string table.
#include "ape/xml/ApeStringTable.h"

#include "ape/core/ApeString.h"
#include <cstddef>
#include <memory>
#include "platform/Platform.h"

#include <cstring>

namespace ape {

namespace {
uint32_t ReadBE32(const uint8_t* p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
}

// @func 0x004F8450 StringTable::StringTable
StringTable::StringTable(const char* path)
{
    // TODO: open through the engine file system (archives) once it exists.
    std::unique_ptr<plat::File> file = plat::File::Open(path, plat::File::Mode::Read);
    if (!file)
        return;
    const int64_t size = file->Size();
    if (size < (int64_t)sizeof(Header))
        return;
    m_image.resize((size_t)size);
    if (file->Read(m_image.data(), (size_t)size) != (size_t)size)
        m_image.clear();
}

// @func 0x004F85F0 StringTable::~StringTable
// @func 0x004F8644 StringTable::~StringTable (deleting)
StringTable::~StringTable() = default;

const char* StringTable::FindByHash(uint32_t idHash) const
{
    if (m_image.empty())
        return nullptr;
    const uint8_t* base = m_image.data();
    const uint32_t count = ReadBE32(base + offsetof(Header, count));
    const uint8_t* pairs = base + ReadBE32(base + offsetof(Header, pairsOffset));
    const uint8_t* offsets = base + ReadBE32(base + offsetof(Header, offsetsOffset));
    const char* text = reinterpret_cast<const char*>(base + ReadBE32(base + offsetof(Header, textOffset)));

    int lo = 0, hi = (int)count - 1;
    while (lo <= hi) {
        const int mid = (lo + hi) >> 1;
        const uint32_t hash = ReadBE32(pairs + mid * 8);
        if (idHash < hash) {
            hi = mid - 1;
        } else if (idHash > hash) {
            lo = mid + 1;
        } else {
            const int32_t index = (int32_t)ReadBE32(pairs + mid * 8 + 4);
            if (index < 0)
                return nullptr;
            return text + ReadBE32(offsets + index * 4);
        }
    }
    return nullptr;
}

// @func 0x004F86B0 StringTable::Find(hash)
const char* StringTable::Find(uint32_t idHash, const char* fallback, bool quiet) const
{
    if (const char* s = FindByHash(idHash))
        return s;
    if (!quiet)
        plat::Log(plat::LogLevel::Warning, "String id '%s' not found\n", TempHexString(idHash));
    return fallback ? fallback : "";
}

// @func 0x004F87C4 StringTable::Find(name)
const char* StringTable::Find(const char* idName, const char* fallback, bool quiet) const
{
    if (const char* s = FindByHash(HashNameNoCase(idName)))
        return s;
    if (!quiet)
        plat::Log(plat::LogLevel::Warning, "String id '%s' not found\n", idName);
    return fallback ? fallback : "";
}

} // namespace ape
