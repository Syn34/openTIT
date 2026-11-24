// openTIT - IFF chunk handlers used by the game shell's data loaders.
#pragma once

#include <cstdint>

namespace ape::iff { struct IffReader; }

namespace sly {

bool ParseAudioContainerChunk(ape::iff::IffReader* reader);
bool SkipVersionChunkA(ape::iff::IffReader* reader);
bool SkipVersionChunkB(ape::iff::IffReader* reader);
bool SkipVersionChunkC(ape::iff::IffReader* reader);
bool ParseNestedChunk(ape::iff::IffReader* reader);
bool IgnoreChunk(ape::iff::IffReader* reader);
bool IgnoreChunkCopy(ape::iff::IffReader* reader);
bool ParseGuiChunkA(ape::iff::IffReader* reader);
bool ParseGuiChunkB(ape::iff::IffReader* reader);

// Loader state cleared by the container handlers (see the .cpp).
uint32_t AudioChunkState(int index);
uint32_t GuiChunkState(int index);

} // namespace sly
