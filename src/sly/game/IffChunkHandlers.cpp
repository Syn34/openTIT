// openTIT - Small IFF chunk handlers registered by the game shell's data
// loaders (audio banks and GUI data). Each receives the reader positioned at
// the start of its chunk and returns whether parsing may continue.
//
// NOTE: the chunk ids these handlers are registered for are not decoded yet;
// they are named by what they do.
#include "sly/game/IffChunkHandlers.h"

#include "ape/sys/ApeIff.h"

#include <cstdint>

namespace sly {

using ape::iff::IffReader;

namespace {

// Audio loader state cleared when its container chunk starts (0x010AC8E0,
// 0x00F65B34, 0x00F65B38).
uint32_t g_audioChunkState0 = 0;
uint32_t g_audioChunkState1 = 0;
uint32_t g_audioChunkState2 = 0;

// GUI loader state (0x00F65D30 and 0x00F65D34).
uint32_t g_guiChunkStateA = 0;
uint32_t g_guiChunkStateB = 0;

} // namespace

uint32_t AudioChunkState(int index)
{
    switch (index) {
    case 0: return g_audioChunkState0;
    case 1: return g_audioChunkState1;
    default: return g_audioChunkState2;
    }
}

uint32_t GuiChunkState(int index)
{
    return index == 0 ? g_guiChunkStateA : g_guiChunkStateB;
}

// @func 0x0044E454 ParseAudioContainerChunk
bool ParseAudioContainerChunk(IffReader* reader)
{
    g_audioChunkState0 = 0;
    g_audioChunkState1 = 0;
    g_audioChunkState2 = 0;
    reader->ParseChunks(nullptr);
    return true;
}

// The three version handlers each own a function-local static that is only
// initialised here; the byte itself is read and discarded.

// @func 0x0044ED38 SkipVersionChunkA
bool SkipVersionChunkA(IffReader* reader)
{
    static uint32_t s_state = 0;   // 0x010B1680
    (void)s_state;
    uint8_t version = 0;
    reader->ReadU8(&version);
    return true;
}

// @func 0x0044F35C SkipVersionChunkB
bool SkipVersionChunkB(IffReader* reader)
{
    static uint32_t s_state = 0;   // 0x010B16A0
    (void)s_state;
    uint8_t version = 0;
    reader->ReadU8(&version);
    return true;
}

// Format version of the audio bank being loaded (0x00F65B3C), from its
// version chunk; banks without one are version 6.
uint32_t g_audioBankVersion = 0;

uint32_t AudioBankVersion()
{
    return g_audioBankVersion;
}

// @func 0x0044F040 AudioBankVersionChunk
bool AudioBankVersionChunk(IffReader* reader)
{
    static uint8_t s_version = 0;   // 0x010B1690
    s_version = 6;
    reader->ReadU8(&s_version);
    g_audioBankVersion = s_version;
    return true;
}

// @func 0x0044F648 SkipVersionChunkC
bool SkipVersionChunkC(IffReader* reader)
{
    static uint32_t s_state = 0;   // 0x010B16B0
    (void)s_state;
    uint8_t version = 0;
    reader->ReadU8(&version);
    return true;
}

// @func 0x0048C074 ParseNestedChunk
bool ParseNestedChunk(IffReader* reader)
{
    reader->ParseChunks(nullptr);
    return true;
}

// @func 0x0048C154 IgnoreChunk
bool IgnoreChunk(IffReader* reader)
{
    (void)reader;
    return true;
}

// @func 0x004842A0 IgnoreChunk (copy 2)
bool IgnoreChunkCopy(IffReader* reader)
{
    (void)reader;
    return true;
}

// @func 0x0048C15C ParseGuiChunkA
bool ParseGuiChunkA(IffReader* reader)
{
    g_guiChunkStateA = 0;
    reader->ParseChunks(nullptr);
    return true;
}

// @func 0x0048C388 ParseGuiChunkB
bool ParseGuiChunkB(IffReader* reader)
{
    g_guiChunkStateB = 0;
    reader->ParseChunks(nullptr);
    return true;
}

} // namespace sly
