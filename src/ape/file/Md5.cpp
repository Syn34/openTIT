// openTIT - MD5 digest (RFC 1321), used to match PSARC table-of-contents entries to names.
#include "ape/file/Md5.h"

#include <cmath>
#include <cstring>

namespace ape::file {

namespace {

// Per-step left rotation amounts, four per round.
constexpr int kShift[4][4] = { { 7, 12, 17, 22 }, { 5, 9, 14, 20 }, { 4, 11, 16, 23 }, { 6, 10, 15, 21 } };

struct Constants {
    uint32_t k[64];
    Constants()
    {
        // K[i] = floor(|sin(i + 1)| * 2^32), as defined by the RFC.
        for (int i = 0; i < 64; ++i)
            k[i] = (uint32_t)(std::fabs(std::sin((double)(i + 1))) * 4294967296.0);
    }
};

uint32_t Rotl(uint32_t v, int s) { return (v << s) | (v >> (32 - s)); }

void ProcessBlock(uint32_t state[4], const uint8_t* block, const uint32_t* k)
{
    uint32_t m[16];
    for (int i = 0; i < 16; ++i)
        m[i] = (uint32_t)block[i * 4] | ((uint32_t)block[i * 4 + 1] << 8) |
               ((uint32_t)block[i * 4 + 2] << 16) | ((uint32_t)block[i * 4 + 3] << 24);

    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    for (int i = 0; i < 64; ++i) {
        const int round = i / 16;
        uint32_t f;
        int g;
        switch (round) {
        case 0: f = (b & c) | (~b & d); g = i; break;
        case 1: f = (d & b) | (~d & c); g = (5 * i + 1) & 15; break;
        case 2: f = b ^ c ^ d; g = (3 * i + 5) & 15; break;
        default: f = c ^ (b | ~d); g = (7 * i) & 15; break;
        }
        uint32_t next = b + Rotl(a + f + k[i] + m[g], kShift[round][i & 3]);
        a = d;
        d = c;
        c = b;
        b = next;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

} // namespace

Md5Digest Md5(const void* data, size_t length)
{
    static const Constants constants;
    uint32_t state[4] = { 0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u };

    const uint8_t* p = static_cast<const uint8_t*>(data);
    size_t remaining = length;
    while (remaining >= 64) {
        ProcessBlock(state, p, constants.k);
        p += 64;
        remaining -= 64;
    }

    // Final block(s): 0x80 terminator, zero padding, 64-bit bit length.
    uint8_t tail[128] = {};
    std::memcpy(tail, p, remaining);
    tail[remaining] = 0x80;
    const size_t tailLen = remaining < 56 ? 64 : 128;
    const uint64_t bits = (uint64_t)length * 8;
    for (int i = 0; i < 8; ++i)
        tail[tailLen - 8 + i] = (uint8_t)(bits >> (8 * i));
    for (size_t off = 0; off < tailLen; off += 64)
        ProcessBlock(state, tail + off, constants.k);

    Md5Digest out;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            out.bytes[i * 4 + j] = (uint8_t)(state[i] >> (8 * j));
    return out;
}

} // namespace ape::file
