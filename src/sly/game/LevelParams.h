// openTIT - Level parameters (the "LevelParams" element of a level's data
// file, 0x15D0 bytes on the console, built by 0x00481A88): the headline
// shown when the level starts, per-job headline overrides and the level's
// game settings.
#pragma once

#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace ape::xml { class XmlDocument; class XmlElement; }

namespace sly {

struct LevelParams {
    // Headline override for one job (0x28-byte records).
    struct JobHeadline {
        uint32_t jobName = 0;           // +0x00
        char text[0x24] = {};           // +0x04
    };

    int32_t field_1544 = 0;             // +0x1544 NOTE: 1 selects a special level mode
    ape::ApeTArray<JobHeadline>* jobHeadlines = nullptr; // +0x154C
    char headline[0x20] = {};           // +0x1550 "headlineString"
    void* headlineHud = nullptr;        // +0x1584 HUD element showing the headline
};

// Builds the level parameters from their element (0x00481A88).
LevelParams* CreateLevelParams(ape::xml::XmlDocument& document, ape::xml::XmlElement* element);

} // namespace sly
