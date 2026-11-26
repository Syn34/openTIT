// openTIT - Per-level file lists collected from the level definitions.
//
// While the level definitions are read, the game's loaders for
// "LevelData.Level.HavokFile" and friends (registered by SlyGame::SlyGame)
// note the files of the level being started (SlyGame::pendingLevelHash) in
// these lists. The names are copied into a 32 KB string arena that is reset
// when the game is created (0x010B7834 base, 0x010B7838 cursor).
#pragma once

#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace ape::xml { class XmlElement; }

namespace sly {

struct LevelFileLists {
    static constexpr uint32_t kArenaSize = 0x8000;

    // Copies `name` into the arena; null when it does not fit.
    const char* Store(const char* name);
    void ResetArena() { cursor = 0; }

    char arena[kArenaSize] = {};                    // 0x010B7834
    uint32_t cursor = 0;                            // 0x010B7838
    ape::ApeTArray<const char*> entityFiles;        // 0x010B783C
    ape::ApeTArray<const char*> havokFiles;         // 0x010B7854
    ape::ApeTArray<const char*> skeletonFiles;      // 0x010B7868
    ape::ApeTArray<uint32_t> skeletonIds;           // 0x010B787C
    ape::ApeTArray<const char*> skelMapFiles;       // 0x010B7890
    ape::ApeTArray<uint32_t> skelMapIds;            // 0x010B78A4
    ape::ApeTArray<const char*> animFiles;          // 0x010B78B8
    ape::ApeTArray<uint32_t> animIds;               // 0x010B78CC
    const char* levelParamsFile = nullptr;          // 0x00F65C80
};

LevelFileLists& GetLevelFileLists();

// Level definition loaders (LevelDefXmlCallback).
bool LoadLevelHavokFile(ape::xml::XmlElement* element);
bool LoadLevelHavokSkeletonFile(ape::xml::XmlElement* element);
bool LoadLevelHavokSkelMapFile(ape::xml::XmlElement* element);
bool LoadLevelHavokAnimFile(ape::xml::XmlElement* element);
bool LoadLevelEntityFile(ape::xml::XmlElement* element);
bool LoadLevelGoalFile(ape::xml::XmlElement* element);
bool LoadLevelParamsFile(ape::xml::XmlElement* element);

} // namespace sly
