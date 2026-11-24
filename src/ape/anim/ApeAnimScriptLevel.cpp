// openTIT - Level side of the anim script system: the director chunks of a
// level file and the reload of the level's director scripts.
//
// A level file names its director (anim script) documents in "DIRF" chunks.
// Each one holds a "DIRH" header with the document's path and, for cooked
// levels, a "DIRD" chunk with the compiled document itself. When the data is
// not embedded the document is read from disk. Every document goes into the
// default script library (name hash 0), and the path is remembered in the
// level definition's "Director" string list so the scripts can be reloaded.
#include "ape/anim/ApeAnimScript.h"

#include "ape/file/ApeFile.h"
#include "ape/level/ApeLevelDef.h"
#include "ape/memory/ApeMemStack.h"
#include "ape/sys/ApeIff.h"
#include "ape/xml/ApeXml.h"

#include <cstring>

namespace ape {

namespace {

constexpr uint32_t kDirectorFileChunk = iff::MakeId('D', 'I', 'R', 'F');
constexpr uint32_t kDirectorHeaderChunk = iff::MakeId('D', 'I', 'R', 'H');
constexpr uint32_t kDirectorDataChunk = iff::MakeId('D', 'I', 'R', 'D');
constexpr uint32_t kDirectorList = AnimHash("Director");
constexpr uint32_t kDefaultLibrary = 0;

// Path from the last header chunk (0x010C2989); the pointer is cleared once
// the document was loaded (0x00F65FF0).
constexpr int kDirectorPathLength = 64;
char s_directorPath[kDirectorPathLength + 1];
const char* s_pendingDirector = nullptr;

// Loads a compiled document from an open file into the default library.
void LoadDirectorDocument(file::File* file)
{
    ApeAnimScriptLibrary* library = ApeAnimScriptLibrary::Get(kDefaultLibrary);
    xml::XmlDocument document;
    if (document.LoadFromStream(*file, nullptr))
        library->LoadFromXml(file->GetName(), &document);
}

// "DIRF": a director document. The nested chunks name it (and may embed it);
// a document that was not embedded is loaded from disk afterwards.
// @func 0x004BE61C DirectorFileChunkHandler
int DirectorFileChunkHandler(iff::IffReader* reader, void*)
{
    s_pendingDirector = nullptr;
    reader->ParseChunks(nullptr);
    if (!s_pendingDirector)
        return 1;

    uint32_t size = 0;
    void* data = file::LoadFile(s_pendingDirector, &size);
    if (!data)
        return 1;
    if (file::File* file = file::CreateMemoryFile(data, size, s_pendingDirector)) {
        LoadDirectorDocument(file);
        file->Close();
    }
    s_pendingDirector = nullptr;
    file::FreeFileData(data);
    return 1;
}

// "DIRH": a version byte and the 64 character path of the document.
// @func 0x004BE75C DirectorHeaderChunkHandler
int DirectorHeaderChunkHandler(iff::IffReader* reader, void*)
{
    std::memset(s_directorPath, 0, sizeof(s_directorPath));
    uint8_t version = 0;
    reader->ReadU8(&version);
    for (int i = 0; i < kDirectorPathLength; ++i)
        reader->ReadU8(reinterpret_cast<uint8_t*>(&s_directorPath[i]));
    s_pendingDirector = s_directorPath;
    if (ApeLevelDef* level = GetCurrentLevelDef())
        level->AddStringToList(kDirectorList, s_directorPath);
    return 1;
}

// "DIRD": the compiled document embedded in the level file.
// @func 0x004BE850 DirectorDataChunkHandler
int DirectorDataChunkHandler(iff::IffReader* reader, void*)
{
    const uint32_t size = reader->GetChunkSize();
    if (size == 0)
        return 0;
    ApeMemStack* stack = GetMemStack();
    void* data = stack->Alloc(size);
    reader->Read(data, size);
    if (file::File* file = file::CreateMemoryFile(data, size, s_pendingDirector)) {
        LoadDirectorDocument(file);
        file->Close();
    }
    s_pendingDirector = nullptr;   // nothing left to load from disk
    stack->Free(data);
    return 1;
}

} // namespace

// @func 0x004BCE08 RegisterDirectorChunkHandlers
void RegisterDirectorChunkHandlers()
{
    iff::RegisterChunkHandler(kDirectorFileChunk, DirectorFileChunkHandler, 0, nullptr, 0);
    iff::RegisterChunkHandler(kDirectorHeaderChunk, DirectorHeaderChunkHandler, 0, nullptr, 0);
    iff::RegisterChunkHandler(kDirectorDataChunk, DirectorDataChunkHandler, 0, nullptr, 0);
}

// Clears the default library and loads the current level's director
// documents again from disk (debug reload).
// @func 0x004BDDB8 ReloadLevelDirectors
void ReloadLevelDirectors()
{
    ApeAnimScriptLibrary* library = ApeAnimScriptLibrary::Get(kDefaultLibrary);
    if (!library)
        return;
    library->Clear();
    ApeLevelDef* level = GetCurrentLevelDef();
    if (!level)
        return;
    const uint32_t count = level->GetStringListCount(kDirectorList);
    for (uint32_t i = 0; i < count; ++i) {
        const char* path = level->GetStringFromList(kDirectorList, (int)i);
        xml::XmlDocument document(path);
        if (document.LoadFile(0))
            library->LoadFromXml(path, &document);
    }
}

} // namespace ape
