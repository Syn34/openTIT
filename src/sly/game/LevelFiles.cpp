// openTIT - Per-level file lists collected from the level definitions.
#include "sly/game/LevelFiles.h"

#include "ape/core/ApeString.h"
#include "ape/level/ApeLevelDef.h"
#include "ape/xml/ApeXml.h"
#include "sly/game/SlyGame.h"

#include <cstring>

namespace sly {

LevelFileLists& GetLevelFileLists()
{
    static LevelFileLists lists;
    return lists;
}

const char* LevelFileLists::Store(const char* name)
{
    const size_t length = std::strlen(name);
    if (cursor + length + 1 >= kArenaSize)
        return nullptr;
    char* copy = &arena[cursor];
    std::memcpy(copy, name, length + 1);
    cursor += uint32_t(length + 1);
    return copy;
}

namespace {

// The loaders only collect the files of the level being started.
bool IsPendingLevel()
{
    ape::ApeLevelDef* def = ape::GetCurrentLevelDef();
    return def && g_game && ape::HashNameNoCase(def->name) == g_game->pendingLevelHash;
}

// Notes the element's "name" in `names`; with `ids` its "id" hash (0
// without one) goes to the parallel list, whether or not the name fitted.
void CollectFile(ape::xml::XmlElement* element, ape::ApeTArray<const char*>& names,
                 ape::ApeTArray<uint32_t>* ids)
{
    if (!IsPendingLevel())
        return;
    const char* name = element->GetString("name");
    if (!name)
        return;
    if (const char* stored = GetLevelFileLists().Store(name))
        names.PushBack(stored);
    if (ids) {
        const char* id = element->GetString("id");
        ids->PushBack(id ? ape::HashNameNoCase(id) : 0u);
    }
}

} // namespace

// @func 0x00483A34 LoadLevelHavokFile
bool LoadLevelHavokFile(ape::xml::XmlElement* element)
{
    CollectFile(element, GetLevelFileLists().havokFiles, nullptr);
    return true;
}

// @func 0x00483B88 LoadLevelHavokSkeletonFile
bool LoadLevelHavokSkeletonFile(ape::xml::XmlElement* element)
{
    LevelFileLists& lists = GetLevelFileLists();
    CollectFile(element, lists.skeletonFiles, &lists.skeletonIds);
    return true;
}

// @func 0x00483D74 LoadLevelHavokSkelMapFile
bool LoadLevelHavokSkelMapFile(ape::xml::XmlElement* element)
{
    LevelFileLists& lists = GetLevelFileLists();
    CollectFile(element, lists.skelMapFiles, &lists.skelMapIds);
    return true;
}

// @func 0x00483F60 LoadLevelHavokAnimFile
bool LoadLevelHavokAnimFile(ape::xml::XmlElement* element)
{
    LevelFileLists& lists = GetLevelFileLists();
    CollectFile(element, lists.animFiles, &lists.animIds);
    return true;
}

// @func 0x0048414C LoadLevelEntityFile
bool LoadLevelEntityFile(ape::xml::XmlElement* element)
{
    CollectFile(element, GetLevelFileLists().entityFiles, nullptr);
    return true;
}

// Goal files are taken from the job manager instead; the element is accepted
// and ignored. NOTE: on the console this is the shared "return true" helper
// (0x004842A0, see IffChunkHandlers.cpp).
bool LoadLevelGoalFile(ape::xml::XmlElement*)
{
    return true;
}

// @func 0x004842A8 LoadLevelParamsFile
bool LoadLevelParamsFile(ape::xml::XmlElement* element)
{
    if (!IsPendingLevel())
        return true;
    if (const char* name = element->GetString("name")) {
        if (const char* stored = GetLevelFileLists().Store(name))
            GetLevelFileLists().levelParamsFile = stored;
    }
    return true;
}

} // namespace sly
