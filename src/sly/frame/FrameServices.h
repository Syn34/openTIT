// openTIT - Small per-frame services the game flips at the end of every frame.
#pragma once

#include <cstdint>

namespace sly {

// A linear scratch buffer that is emptied once per frame.
struct FrameBuffer {
    uint32_t field_00 = 0;      // +0x00 unknown
    uint8_t* begin = nullptr;   // +0x04
    uint8_t* end = nullptr;     // +0x08
    uint8_t* cursor = nullptr;  // +0x0C next free byte
    uint8_t* limit = nullptr;   // +0x10
    uint32_t usedA = 0;         // +0x14 NOTE: statistics; exact meaning unknown
    uint32_t usedB = 0;         // +0x18
    uint32_t frame = 0;         // +0x1C number of times the buffer was reset
    uint8_t flags = 0;          // +0x20 top two bits are per-frame state

    void Reset();
};

// Two FrameBuffers used alternately: data written last frame stays readable
// for one more frame while the other buffer is refilled.
struct FrameBufferPair {
    FrameBuffer* first = nullptr;    // +0x00
    FrameBuffer* second = nullptr;   // +0x04
    FrameBuffer* current = nullptr;  // +0x08
    FrameBuffer* previous = nullptr; // +0x0C

    // Makes the other buffer current and empties it.
    void Swap();
};

// Callbacks queued during the frame and run once at its end.
using EndOfFrameFn = void (*)(void* arg);
void QueueEndOfFrameCallback(EndOfFrameFn fn, void* arg);
void RunEndOfFrameCallbacks();

// The game's global frame buffers (pointer at 0x00F65C90 on the console).
FrameBufferPair* GetGameFrameBuffers();
void SetGameFrameBuffers(FrameBufferPair* pair);

// Double-buffered counter pair (0x01195090) flipped every frame.
void FlipFrameCounters();
// Zeroes both counters (world shutdown).
void ResetFrameCounters();
uint32_t& CurrentFrameCounter();

// Debug-draw style buffers (pointer at 0x010AA614) that are only swapped on
// frames where something requested it.
void RequestDebugBufferSwap();
void SetDebugFrameBuffers(FrameBufferPair* pair);
void SwapDebugBuffersIfRequested();

void EndFrameHook();

// Blend state used by the debug draw buffers (0x010AA208), laid out like a
// material's blend words: bit 31 enables blending, bits 15-30 hold the
// source factor; then the equation and the destination factor (GL enums).
struct DebugDrawBlend {
    uint32_t word = 0;          // +0x00
    uint16_t equation = 0;      // +0x04
    uint16_t dstFactor = 0;     // +0x06

    bool Enabled() const { return (word >> 31) != 0; }
    uint16_t SrcFactor() const { return uint16_t((word >> 15) & 0xFFFF); }
};
const DebugDrawBlend& GetDebugDrawBlend();
void SetDebugDrawBlend(bool enable, uint16_t srcFactor, uint16_t dstFactor, uint16_t equation);
// Back to standard alpha blending factors with blending switched off.
void ResetDebugDrawBlend();

} // namespace sly
