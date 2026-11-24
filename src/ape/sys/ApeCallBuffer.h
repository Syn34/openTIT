// openTIT - Deferred call buffer (0x00564FF8-0x00565228).
//
// A fixed memory block that records (function, argument) calls to be run
// later, typically to hand work from the game thread to the renderer. Calls
// are stored from the bottom of the block upwards; payload data copied with
// the calls is carved from the top downwards. Execute() runs everything in
// recording order and can rewind the buffer for the next frame. Blocks of
// calls can be bracketed with BeginBlock/EndBlock and replayed later.
//
// NOTE: the original class name is unknown; the code that sets the buffer up
// lives outside this module (Init() is provided for the PC build).
#pragma once

#include <cstddef>
#include <cstdint>

namespace ape {

using DeferredFunc = void (*)(void* arg);

struct DeferredCall {
    DeferredFunc func;
    void* arg;
};

// A recorded run of calls (returned by BeginBlock/EndBlock).
struct DeferredBlock {
    DeferredCall* first = nullptr;
    uint32_t count = 0;
    uint32_t frame = 0;   // buffer frame the block was recorded in
};

class ApeCallBuffer {
public:
    void Init(void* memory, size_t size);

    // Records func(arg). Dropped silently when the buffer is full.
    void Push(DeferredFunc func, void* arg);
    // Copies `size` bytes into the buffer and records func(copy).
    void PushWithData(DeferredFunc func, const void* data, size_t size);
    // Reserves 16-byte aligned payload space; null when full.
    void* AllocData(size_t size);

    void BeginBlock(DeferredBlock& block);
    void EndBlock(DeferredBlock& block);
    void ExecuteBlock(const DeferredBlock& block) const;

    // Runs every recorded call; with `reset` the buffer is emptied and the
    // frame counter advances.
    void Execute(bool reset);

    uint32_t Count() const { return m_count; }
    uint32_t Frame() const { return m_frame; }
    size_t Used() const { return m_used; }

private:
    enum : uint8_t {
        kFlag_InBlock = 0x40,
        kFlag_BlockDone = 0x80,
    };

    size_t m_capacity = 0;           // +0x00 bytes
    uint8_t* m_base = nullptr;       // +0x04
    uint8_t* m_top = nullptr;        // +0x08
    DeferredCall* m_write = nullptr; // +0x0C next call slot
    uint8_t* m_data = nullptr;       // +0x10 lowest payload byte
    size_t m_used = 0;               // +0x14 calls + payload bytes
    uint32_t m_count = 0;            // +0x18
    uint32_t m_frame = 0;            // +0x1C
    uint8_t m_flags = 0;             // +0x20
};

} // namespace ape
