// openTIT - Award (trophy) table (see Awards.h).
#include "sly/entity/Awards.h"

#include "ape/container/ApeTSortedMap.h"
#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"
#include "sly/trophy/TrophyManager.h"

#include <memory>
#include <vector>

namespace sly {

namespace {

constexpr uint32_t kHash_Awards = 0x73D3C5B6u;   // "Awards"
constexpr uint32_t kHash_Award  = 0x03BC823Du;   // "Award"
constexpr uint32_t kHash_Name   = 0x0024728Bu;   // "name"
constexpr uint32_t kHash_Number = 0x89EBCF49u;   // "Number" (trophy id)
constexpr uint32_t kHash_Grade  = 0xCEBBEB87u;   // NOTE: attribute name not recovered

constexpr uint32_t kGrade_Bronze = 0x7548799Au;  // "Bronze"
constexpr uint32_t kGrade_Gold   = 0x00217860u;  // "Gold"
constexpr uint32_t kGrade_Silver = 0x91C9C70Du;  // NOTE: inferred from its 30 points
constexpr uint32_t kGrade_Platinum = 0x9E63B22Cu; // NOTE: inferred (no points)

struct AwardEntry {
    uint32_t name = 0;
    int32_t points = 0;
};

// Award name -> trophy id (0x00F64624, default -1).
std::unique_ptr<ape::ApeTSortedMap<int32_t>> g_awardIds;
// Indexed by trophy id (0x00F64620; size 0x01013790).
std::vector<AwardEntry> g_awards;

// NOTE: the console bounds these loops with the trophy manager's registered
// trophy count; the award table has one slot per trophy id, which is the
// same number for a consistent data set.
int TrophyCount()
{
    return int(g_awards.size());
}

} // namespace

// @func 0x000B2814 ReleaseAwards
void ReleaseAwards()
{
    g_awardIds.reset();
    g_awards.clear();
}

int TrophyGradePoints(uint32_t gradeHash)
{
    switch (gradeHash) {
    case kGrade_Bronze: return 15;
    case kGrade_Silver: return 30;
    case kGrade_Gold:   return 90;
    case kGrade_Platinum:
    default:            return 0;
    }
}

// @func 0x000B2280 LoadAwards
bool LoadAwards(const char* path)
{
    ReleaseAwards();
    g_awardIds = std::make_unique<ape::ApeTSortedMap<int32_t>>(0, nullptr, -1);

    ape::xml::XmlDocument doc(path);
    if (!doc.LoadFile(0))
        return false;

    ape::xml::XmlElement* root = doc.FirstChildElement(kHash_Awards);
    if (root) {
        // First pass: the table is sized by the highest trophy id.
        int32_t maxId = -1;
        for (ape::xml::XmlElement* e = root->FirstChildElement(kHash_Award); e;
             e = e->NextSiblingElement(kHash_Award)) {
            int32_t id = -1;
            e->GetInt(kHash_Number, &id, -1);
            if (id > maxId)
                maxId = id;
        }
        g_awards.assign(size_t(maxId + 1), AwardEntry());

        for (ape::xml::XmlElement* e = root->FirstChildElement(kHash_Award); e;
             e = e->NextSiblingElement(kHash_Award)) {
            uint32_t name = 0;
            int32_t id = -1;
            uint32_t grade = 0;
            e->GetHash(kHash_Name, &name, 0);
            e->GetInt(kHash_Number, &id, -1);
            e->GetHash(kHash_Grade, &grade, 0);
            g_awardIds->Add(name, id);
            // NOTE: an award without a Number writes slot -1 on the console;
            // such entries are skipped here.
            if (id >= 0) {
                g_awards[size_t(id)].name = name;
                g_awards[size_t(id)].points = TrophyGradePoints(grade);
            }
        }
    }
    return true;
}

// @func 0x000B277C AwardTrophy
void AwardTrophy(int32_t trophyId)
{
    if (trophyId != -1 && TrophyManager::Instance())
        TrophyManager::Instance()->RequestUnlock(trophyId);
}

// @func 0x000B27B4 AwardTrophyByName
void AwardTrophyByName(uint32_t awardName)
{
    if (!g_awardIds)
        return;
    AwardTrophy(*g_awardIds->Find(awardName));
}

int TrophyTotal()
{
    return TrophyManager::Instance() ? TrophyCount() : 0;
}

// @func 0x000B2914 CountUnlockedTrophies
int CountUnlockedTrophies()
{
    TrophyManager* trophies = TrophyManager::Instance();
    if (!trophies)
        return 0;
    int count = 0;
    for (int id = 0; id < TrophyCount(); ++id)
        if (trophies->IsUnlocked(uint32_t(id)))
            ++count;
    return count;
}

// @func 0x000B29B0 TotalTrophyPoints
int TotalTrophyPoints()
{
    int points = 0;
    for (int id = 0; id < TrophyCount(); ++id)
        points += g_awards[size_t(id)].points;
    return points;
}

// @func 0x000B2A20 EarnedTrophyPoints
int EarnedTrophyPoints()
{
    TrophyManager* trophies = TrophyManager::Instance();
    if (!trophies)
        return 0;
    int points = 0;
    for (int id = 0; id < TrophyCount(); ++id)
        if (trophies->IsUnlocked(uint32_t(id)))
            points += g_awards[size_t(id)].points;
    return points;
}

} // namespace sly
