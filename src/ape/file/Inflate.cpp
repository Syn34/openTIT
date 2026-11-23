// openTIT - DEFLATE (RFC 1951) decoder used by the zip and PSARC readers.
#include "ape/file/Inflate.h"

#include <algorithm>
#include <cstring>

namespace ape::file {

namespace {

// Base lengths / extra bits for length symbols 257..285 (RFC 1951 3.2.5).
constexpr uint16_t kLengthBase[29] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
constexpr uint8_t kLengthExtra[29] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
// Base distances / extra bits for distance symbols 0..29.
constexpr uint16_t kDistBase[30] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
constexpr uint8_t kDistExtra[30] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
// Order in which code length code lengths are transmitted.
constexpr uint8_t kCodeLengthOrder[19] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

uint32_t ReverseBits(uint32_t code, int length)
{
    uint32_t r = 0;
    for (int i = 0; i < length; ++i) {
        r = (r << 1) | (code & 1);
        code >>= 1;
    }
    return r;
}

} // namespace

bool Inflater::Huffman::Build(const uint8_t* lengths, int num)
{
    std::memset(count, 0, sizeof(count));
    std::memset(fast, 0, sizeof(fast));
    for (int i = 0; i < num; ++i)
        ++count[lengths[i]];
    count[0] = 0;

    // Reject over-subscribed code sets (incomplete ones are legal, e.g. a
    // distance tree with a single code).
    int left = 1;
    for (int len = 1; len <= kMaxBits; ++len) {
        left <<= 1;
        left -= count[len];
        if (left < 0)
            return false;
    }

    // Symbols sorted by code length, then by value: canonical order.
    uint16_t offsets[kMaxBits + 2] = {};
    for (int len = 1; len <= kMaxBits; ++len)
        offsets[len + 1] = (uint16_t)(offsets[len] + count[len]);
    symbols.assign(num, 0);
    for (int i = 0; i < num; ++i)
        if (lengths[i])
            symbols[offsets[lengths[i]]++] = (uint16_t)i;

    // Fill the direct lookup table for short codes.
    uint32_t code = 0;
    uint32_t nextCode[kMaxBits + 1] = {};
    for (int len = 1; len <= kMaxBits; ++len) {
        code = (code + count[len - 1]) << 1;
        nextCode[len] = code;
    }
    for (int i = 0; i < num; ++i) {
        int len = lengths[i];
        if (len == 0 || len > kFastBits)
            continue;
        uint32_t rev = ReverseBits(nextCode[len]++, len);
        for (uint32_t idx = rev; idx < (1u << kFastBits); idx += 1u << len)
            fast[idx] = (uint16_t)((len << 12) | i);
    }
    return true;
}

Inflater::Inflater() : m_window(kWindowSize) {}

void Inflater::Reset(Source source)
{
    m_source = std::move(source);
    m_state = State::BlockHeader;
    m_lastBlock = false;
    m_inputPos = m_inputLen = 0;
    m_inputEof = false;
    m_bitBuf = 0;
    m_bitCount = 0;
    m_storedLeft = 0;
    m_windowPos = 0;
    m_pending = 0;
    m_copyLen = m_copyDist = 0;
    m_totalOut = 0;
}

bool Inflater::NeedBits(int n)
{
    while (m_bitCount < n) {
        if (m_inputPos == m_inputLen) {
            if (m_inputEof || !m_source)
                return false;
            m_inputLen = m_source(m_input, sizeof(m_input));
            m_inputPos = 0;
            if (m_inputLen == 0) {
                m_inputEof = true;
                return false;
            }
        }
        m_bitBuf |= (uint64_t)m_input[m_inputPos++] << m_bitCount;
        m_bitCount += 8;
    }
    return true;
}

uint32_t Inflater::TakeBits(int n)
{
    uint32_t v = (uint32_t)(m_bitBuf & ((1ull << n) - 1));
    m_bitBuf >>= n;
    m_bitCount -= n;
    return v;
}

int Inflater::Decode(const Huffman& h)
{
    // Try to have a full fast-table index available; near the end of the
    // stream fewer bits may legitimately remain.
    NeedBits(Huffman::kFastBits);
    uint16_t entry = h.fast[m_bitBuf & ((1u << Huffman::kFastBits) - 1)];
    if (entry != 0 && (entry >> 12) <= m_bitCount) {
        TakeBits(entry >> 12);
        return entry & 0xFFF;
    }

    // Canonical decode one bit at a time.
    int code = 0, first = 0, index = 0;
    for (int len = 1; len <= Huffman::kMaxBits; ++len) {
        if (!NeedBits(1))
            return -1;
        code |= (int)TakeBits(1);
        int cnt = h.count[len];
        if (code - first < cnt)
            return h.symbols[index + (code - first)];
        index += cnt;
        first = (first + cnt) << 1;
        code <<= 1;
    }
    return -1;
}

bool Inflater::ReadDynamicTables()
{
    if (!NeedBits(14))
        return false;
    int numLit = (int)TakeBits(5) + 257;
    int numDist = (int)TakeBits(5) + 1;
    int numCodeLen = (int)TakeBits(4) + 4;
    if (numLit > 286 || numDist > 30)
        return false;

    uint8_t clLengths[19] = {};
    for (int i = 0; i < numCodeLen; ++i) {
        if (!NeedBits(3))
            return false;
        clLengths[kCodeLengthOrder[i]] = (uint8_t)TakeBits(3);
    }
    Huffman codeLen;
    if (!codeLen.Build(clLengths, 19))
        return false;

    uint8_t lengths[286 + 30] = {};
    int n = 0;
    while (n < numLit + numDist) {
        int sym = Decode(codeLen);
        if (sym < 0)
            return false;
        if (sym < 16) {
            lengths[n++] = (uint8_t)sym;
            continue;
        }
        uint8_t value = 0;
        int repeat;
        if (sym == 16) {
            if (n == 0 || !NeedBits(2))
                return false;
            value = lengths[n - 1];
            repeat = 3 + (int)TakeBits(2);
        } else if (sym == 17) {
            if (!NeedBits(3))
                return false;
            repeat = 3 + (int)TakeBits(3);
        } else {
            if (!NeedBits(7))
                return false;
            repeat = 11 + (int)TakeBits(7);
        }
        if (n + repeat > numLit + numDist)
            return false;
        while (repeat--)
            lengths[n++] = value;
    }
    if (lengths[256] == 0) // no end-of-block code
        return false;
    return m_lit.Build(lengths, numLit) && m_dist.Build(lengths + numLit, numDist);
}

bool Inflater::BeginBlock()
{
    if (!NeedBits(3))
        return false;
    m_lastBlock = TakeBits(1) != 0;
    switch (TakeBits(2)) {
    case 0: { // stored
        TakeBits(m_bitCount & 7);
        if (!NeedBits(32))
            return false;
        uint32_t len = TakeBits(16);
        uint32_t nlen = TakeBits(16);
        if ((len ^ 0xFFFF) != nlen)
            return false;
        m_storedLeft = len;
        m_state = State::Stored;
        return true;
    }
    case 1: { // fixed Huffman codes
        uint8_t lengths[288 + 30];
        std::fill(lengths, lengths + 144, (uint8_t)8);
        std::fill(lengths + 144, lengths + 256, (uint8_t)9);
        std::fill(lengths + 256, lengths + 280, (uint8_t)7);
        std::fill(lengths + 280, lengths + 288, (uint8_t)8);
        std::fill(lengths + 288, lengths + 318, (uint8_t)5);
        m_lit.Build(lengths, 288);
        m_dist.Build(lengths + 288, 30);
        m_state = State::Huffman;
        return true;
    }
    case 2:
        if (!ReadDynamicTables())
            return false;
        m_state = State::Huffman;
        return true;
    default:
        return false;
    }
}

void Inflater::Emit(uint8_t byte)
{
    m_window[m_windowPos] = byte;
    m_windowPos = (m_windowPos + 1) & (kWindowSize - 1);
    ++m_pending;
    ++m_totalOut;
}

size_t Inflater::Drain(uint8_t* dst, size_t bytes)
{
    size_t n = std::min(bytes, m_pending);
    size_t start = (m_windowPos + kWindowSize - m_pending) & (kWindowSize - 1);
    for (size_t i = 0; i < n; ++i)
        dst[i] = m_window[(start + i) & (kWindowSize - 1)];
    m_pending -= n;
    return n;
}

size_t Inflater::Read(uint8_t* dst, size_t bytes)
{
    size_t produced = 0;
    while (produced < bytes) {
        if (m_pending) {
            produced += Drain(dst + produced, bytes - produced);
            continue;
        }

        // Decode until the request can be satisfied or the window is full
        // of undelivered output.
        const size_t want = std::min(bytes - produced, kWindowSize);
        while (m_pending < want) {
            if (m_copyLen) {
                while (m_copyLen && m_pending < kWindowSize) {
                    Emit(m_window[(m_windowPos + kWindowSize - m_copyDist) & (kWindowSize - 1)]);
                    --m_copyLen;
                }
                continue;
            }
            if (m_state == State::BlockHeader) {
                if (!BeginBlock())
                    m_state = State::Error;
                continue;
            }
            if (m_state == State::Stored) {
                if (m_storedLeft == 0) {
                    m_state = m_lastBlock ? State::Done : State::BlockHeader;
                    continue;
                }
                if (!NeedBits(8)) {
                    m_state = State::Error;
                    continue;
                }
                Emit((uint8_t)TakeBits(8));
                --m_storedLeft;
                continue;
            }
            if (m_state != State::Huffman)
                break;

            int sym = Decode(m_lit);
            if (sym < 0) {
                m_state = State::Error;
            } else if (sym < 256) {
                Emit((uint8_t)sym);
            } else if (sym == 256) {
                m_state = m_lastBlock ? State::Done : State::BlockHeader;
            } else {
                sym -= 257;
                if (sym >= 29 || !NeedBits(kLengthExtra[sym])) {
                    m_state = State::Error;
                    continue;
                }
                uint32_t len = kLengthBase[sym] + TakeBits(kLengthExtra[sym]);
                int dsym = Decode(m_dist);
                if (dsym < 0 || dsym >= 30 || !NeedBits(kDistExtra[dsym])) {
                    m_state = State::Error;
                    continue;
                }
                uint32_t dist = kDistBase[dsym] + TakeBits(kDistExtra[dsym]);
                if (dist > m_totalOut) {
                    m_state = State::Error;
                    continue;
                }
                m_copyLen = len;
                m_copyDist = dist;
            }
        }
        if (m_pending == 0)
            break; // finished or failed with nothing left to hand out
    }
    return produced;
}

size_t Inflater::InflateBuffer(const uint8_t* src, size_t srcLen, uint8_t* dst, size_t dstLen,
                               bool zlibHeader)
{
    if (zlibHeader) {
        // RFC 1950: CMF/FLG pair, deflate method, no preset dictionary.
        if (srcLen < 2 || (src[0] & 0x0F) != 8 || ((src[0] << 8) | src[1]) % 31 != 0 ||
            (src[1] & 0x20))
            return SIZE_MAX;
        src += 2;
        srcLen -= 2;
    }
    Inflater inflater;
    size_t offset = 0;
    inflater.Reset([&](uint8_t* out, size_t cap) {
        size_t n = std::min(cap, srcLen - offset);
        std::memcpy(out, src + offset, n);
        offset += n;
        return n;
    });
    size_t n = inflater.Read(dst, dstLen);
    if (inflater.Failed())
        return SIZE_MAX;
    return n;
}

} // namespace ape::file
