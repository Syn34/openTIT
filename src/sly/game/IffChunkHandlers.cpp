// openTIT - Small IFF chunk handlers registered by the game shell's data
// loaders (audio banks and GUI data). Each receives the reader positioned at
// the start of its chunk and returns whether parsing may continue.
//
// NOTE: the chunk ids these handlers are registered for are not decoded yet;
// they are named by what they do.
#include "sly/game/IffChunkHandlers.h"

#include "ape/file/ApeFile.h"
#include "ape/level/ApeLevelDef.h"
#include "ape/memory/ApeMemStack.h"
#include "ape/sys/ApeIff.h"
#include "sly/entity/EntityDef.h"
#include "sly/game/SlyGame.h"

#include <cstdint>

namespace sly {

using ape::iff::IffReader;

namespace {

// String lists of the level definition the header names are added to.
constexpr uint32_t kLevelList_EntityFiles = 0x7A30D7E3u;
constexpr uint32_t kLevelList_LevelData = 0x1AF675EAu;

// Audio loader state cleared when its container chunk starts (0x010AC8E0,
// 0x00F65B34, 0x00F65B38).
uint32_t g_audioChunkState0 = 0;
uint32_t g_audioChunkState1 = 0;
uint32_t g_audioChunkState2 = 0;

// Names of the entity file (ENTH, 0x00F65D30) and level data file (LVLH,
// 0x00F65D34) whose data chunk follows; cleared by their container chunks.
const char* g_entityFileName = nullptr;
const char* g_levelDataName = nullptr;

// Reads the 64-character name of a header chunk (a length byte that is not
// used, then the characters) into `buffer` (0x41 bytes, zeroed once).
void ReadHeaderName(IffReader* reader, char* buffer)
{
    uint8_t length = 0;
    reader->ReadU8(&length);
    for (int i = 0; i < 0x40; ++i)
        reader->ReadU8(reinterpret_cast<uint8_t*>(&buffer[i]));
}

// Reads the current chunk into scratch memory and hands it to `use` as a
// memory file called `name`.
template <class Use>
void WithChunkFile(IffReader* reader, const char* name, Use use)
{
    const uint32_t size = reader->GetChunkSize();
    ape::ApeMemStack* stack = ape::GetMemStack();
    void* data = stack->Alloc(size);
    reader->Read(data, size);
    ape::file::File* file = ape::file::CreateMemoryFile(data, size, name);
    use(*file);
    file->Close();
    stack->Free(data);
}

} // namespace

uint32_t AudioChunkState(int index)
{
    switch (index) {
    case 0: return g_audioChunkState0;
    case 1: return g_audioChunkState1;
    default: return g_audioChunkState2;
    }
}

const char* EntityChunkFileName() { return g_entityFileName; }
const char* LevelDataChunkName() { return g_levelDataName; }

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
    g_entityFileName = nullptr;
    reader->ParseChunks(nullptr);
    return true;
}

// @func 0x0048C388 ParseGuiChunkB
bool ParseGuiChunkB(IffReader* reader)
{
    g_levelDataName = nullptr;
    reader->ParseChunks(nullptr);
    return true;
}

// Goal file header (GOAH): the name is read and kept but not used.
// @func 0x0048C0A0 ReadGoalHeaderChunk
bool ReadGoalHeaderChunk(IffReader* reader)
{
    static char s_name[0x41] = {};   // 0x010BE708
    ReadHeaderName(reader, s_name);
    return true;
}

// Entity file header (ENTH): names the entity data that follows and adds it
// to the running level's entity file list.
// @func 0x0048C194 ReadEntityHeaderChunk
bool ReadEntityHeaderChunk(IffReader* reader)
{
    static char s_name[0x41] = {};   // 0x010BE758
    ReadHeaderName(reader, s_name);
    g_entityFileName = s_name;
    if (g_game && g_game->levelDef)
        g_game->levelDef->AddStringToList(kLevelList_EntityFiles, s_name);
    return true;
}

// Entity file data (ENTD): parsed as the entity file named by the header.
// @func 0x0048C294 ReadEntityDataChunk
bool ReadEntityDataChunk(IffReader* reader)
{
    WithChunkFile(reader, g_entityFileName, [](ape::file::File& file) {
        entity::LoadEntityFileFromStream(file, 0xFE);
    });
    g_entityFileName = nullptr;
    return true;
}

// Level data header (LVLH).
// @func 0x0048C3C0 ReadLevelHeaderChunk
bool ReadLevelHeaderChunk(IffReader* reader)
{
    static char s_name[0x41] = {};   // 0x010BE7A8
    ReadHeaderName(reader, s_name);
    g_levelDataName = s_name;
    if (g_game && g_game->levelDef)
        g_game->levelDef->AddStringToList(kLevelList_LevelData, s_name);
    return true;
}

// Level data (LVLD): the level parameters document.
// @func 0x0048C4C0 ReadLevelDataChunk
bool ReadLevelDataChunk(IffReader* reader)
{
    WithChunkFile(reader, "RamDisk", [](ape::file::File& file) {
        if (g_game)
            g_game->LoadLevelParams(file);
    });
    g_levelDataName = nullptr;
    return true;
}

} // namespace sly
