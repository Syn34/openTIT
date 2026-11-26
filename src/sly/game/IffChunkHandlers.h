// openTIT - IFF chunk handlers used by the game shell's data loaders.
#pragma once

#include <cstdint>

namespace ape::iff { struct IffReader; }

namespace sly {

bool ParseAudioContainerChunk(ape::iff::IffReader* reader);
bool SkipVersionChunkA(ape::iff::IffReader* reader);
bool SkipVersionChunkB(ape::iff::IffReader* reader);
bool SkipVersionChunkC(ape::iff::IffReader* reader);
bool AudioBankVersionChunk(ape::iff::IffReader* reader);
// Version of the audio bank being loaded (set by AudioBankVersionChunk).
uint32_t AudioBankVersion();
bool ParseNestedChunk(ape::iff::IffReader* reader);
bool IgnoreChunk(ape::iff::IffReader* reader);
bool IgnoreChunkCopy(ape::iff::IffReader* reader);
bool ParseGuiChunkA(ape::iff::IffReader* reader);
bool ParseGuiChunkB(ape::iff::IffReader* reader);

// Level file chunks: goal header (GOAH), entity header/data (ENTH/ENTD),
// level header/data (LVLH/LVLD). ParseGuiChunkA/B are the ENTF/LVLF
// containers.
bool ReadGoalHeaderChunk(ape::iff::IffReader* reader);
bool ReadEntityHeaderChunk(ape::iff::IffReader* reader);
bool ReadEntityDataChunk(ape::iff::IffReader* reader);
bool ReadLevelHeaderChunk(ape::iff::IffReader* reader);
bool ReadLevelDataChunk(ape::iff::IffReader* reader);

// Loader state cleared by the container handlers (see the .cpp).
uint32_t AudioChunkState(int index);
const char* EntityChunkFileName();
const char* LevelDataChunkName();

} // namespace sly
