// openTIT - Award (trophy) table: maps the game's award names to trophy ids.
//
// The table is read from an XML document ("Awards" root with one "Award"
// element per trophy: name, Number = trophy id, grade). Gameplay unlocks
// trophies by award name; the trophy manager does the rest.
#pragma once

#include <cstdint>

namespace sly {

// Loads the award table from `path` (replacing any previous one); false if
// the document cannot be loaded (0x000B2280).
bool LoadAwards(const char* path);
void ReleaseAwards();

void AwardTrophy(int32_t trophyId);          // -1 is ignored
void AwardTrophyByName(uint32_t awardName);  // unknown names are ignored

int CountUnlockedTrophies();
int TotalTrophyPoints();
int EarnedTrophyPoints();

// Trophy points of a grade: bronze 15, silver 30, gold 90; other grades
// (platinum) give none.
int TrophyGradePoints(uint32_t gradeHash);

} // namespace sly
