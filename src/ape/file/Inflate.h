// openTIT - DEFLATE (RFC 1951) decoder used by the zip and PSARC readers.
//
// The PS3 build statically links zlib 1.2.3 for this. We only ever need to
// decompress, so this is a small self-contained decoder written from the
// format specification. It is pull based: compressed bytes are requested
// from a source callback whenever the bit buffer runs dry, and decoded bytes
// are handed out through Read() in any portion size. That fits both users:
// zip entries are streamed out of an archive file in small pieces, PSARC
// blocks are decoded in one go with InflateBuffer().
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace ape::file {

class Inflater {
public:
    // Supplies up to `capacity` compressed bytes; returns 0 at end of input.
    using Source = std::function<size_t(uint8_t* dst, size_t capacity)>;

    Inflater();

    // Starts a new raw deflate stream (no zlib/gzip header).
    void Reset(Source source);

    // Decodes up to `bytes` bytes into dst. Returns the number produced;
    // fewer than requested means the stream ended or failed.
    size_t Read(uint8_t* dst, size_t bytes);

    bool Finished() const { return m_state == State::Done; }
    bool Failed() const { return m_state == State::Error; }
    uint64_t TotalOut() const { return m_totalOut; }

    // One-shot helper for complete zlib (RFC 1950) or raw deflate buffers.
    // Returns the number of bytes written to dst, or SIZE_MAX on error.
    static size_t InflateBuffer(const uint8_t* src, size_t srcLen, uint8_t* dst, size_t dstLen,
                                bool zlibHeader);

private:
    // Canonical Huffman decoding table. `fast` resolves codes up to
    // kFastBits long in one lookup; longer codes fall back to a canonical
    // walk using `count` / `symbols`.
    struct Huffman {
        static constexpr int kMaxBits = 15;
        static constexpr int kFastBits = 9;
        uint16_t count[kMaxBits + 1] = {};
        std::vector<uint16_t> symbols;
        uint16_t fast[1 << kFastBits] = {}; // (length << 12) | symbol, 0 = slow path
        bool Build(const uint8_t* lengths, int num);
    };

    enum class State { BlockHeader, Stored, Huffman, Done, Error };

    bool NeedBits(int n);
    uint32_t TakeBits(int n);
    int Decode(const Huffman& h);
    bool BeginBlock();
    bool ReadDynamicTables();
    void Emit(uint8_t byte);
    size_t Drain(uint8_t* dst, size_t bytes);

    Source m_source;
    State m_state = State::BlockHeader;
    bool m_lastBlock = false;

    // Input bit buffer, refilled from a small staging area.
    uint8_t m_input[4096];
    size_t m_inputPos = 0;
    size_t m_inputLen = 0;
    bool m_inputEof = false;
    uint64_t m_bitBuf = 0;
    int m_bitCount = 0;

    uint32_t m_storedLeft = 0;
    Huffman m_lit;
    Huffman m_dist;

    // 32 KiB history window. Decoded bytes land here first; Drain() copies
    // the not yet delivered tail out to the caller.
    static constexpr size_t kWindowSize = 1 << 15;
    std::vector<uint8_t> m_window;
    size_t m_windowPos = 0;   // next write position (mod window size)
    size_t m_pending = 0;     // bytes written to the window but not yet read
    uint32_t m_copyLen = 0;   // remaining length of an interrupted match
    uint32_t m_copyDist = 0;
    uint64_t m_totalOut = 0;
};

} // namespace ape::file
