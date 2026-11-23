// openTIT - MD5 digest (RFC 1321), used to match PSARC table-of-contents entries to names.
#pragma once

#include <cstddef>
#include <cstdint>

namespace ape::file {

struct Md5Digest {
    uint8_t bytes[16];
};

Md5Digest Md5(const void* data, size_t length);

} // namespace ape::file
