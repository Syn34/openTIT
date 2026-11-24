// openTIT - Upgrade shop tuning and the table of upgrade records.
#pragma once

#include "ape/tuning/ApeTuning.h"

#include <cstdint>

namespace sly {

extern ape::TuningInt g_upgradePurchaseFirstAwardNum;  // 0x010BE820
extern ape::TuningInt g_upgradePurchaseAllAwardNum;    // 0x010BE870

// Short fixed-size text plus an id and two flags, embedded in several game
// records. NOTE: name and flag meanings are guesses.
struct TextField {
    char text[0x44] = {};       // +0x00
    uint32_t id = 0;            // +0x44
    bool flagA = false;         // +0x48
    bool flagB = false;         // +0x49

    void Clear();
    void Reset(uint32_t newId);
};

// One upgrade record (0x98 bytes). Only the fields cleared at start-up are
// known.
struct UpgradeEntry {
    uint32_t field_00 = 0;
    uint32_t field_04 = 0;
    uint32_t field_08 = 0;
    uint32_t field_0C = 0;
    char name[0x40] = {};       // +0x10 NOTE: first byte cleared; assumed text
    char desc[0x40] = {};       // +0x50 NOTE: first byte cleared; assumed text
    uint32_t field_90 = 0;
    bool field_94 = false;
};

constexpr uint32_t kUpgradeCount = 0x54;

// Upgrade record by index, null when out of range.
UpgradeEntry* GetUpgradeEntry(uint32_t index);

} // namespace sly
