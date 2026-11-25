// openTIT - Streaming of a job's goal file and script files and building its
// goals (0x002D9A14-0x002DA718).
#include "sly/game/goals/JobManager.h"

#include "ape/file/ApeFile.h"
#include "ape/memory/ApeAllocator.h"
#include "ape/xml/ApeXml.h"
#include "sly/game/FileStreaming.h"
#include "sly/game/SlyGame.h"
#include "sly/game/goals/Goal.h"
#include "sly/game/goals/GoalParser.h"

#include <cstring>
#include <string>

namespace sly {
namespace game {

namespace {

// The streamed files are the compiled documents: "<name>.xmb" for "<name>.xml".
std::string CompiledPath(const char* path)
{
    std::string compiled = path ? path : "";
    const size_t dot = compiled.find('.');
    if (dot != std::string::npos && dot + 3 < compiled.size())
        compiled[dot + 3] = 'b';
    return compiled;
}

bool FileBusy(const StreamDiskFile* file)
{
    return file && file->state != StreamDiskFile::kIdle && file->state != StreamDiskFile::kDone &&
           file->state != StreamDiskFile::kFailed;
}

void ReleaseFileLoad(Episode::FileLoad& load)
{
    if (load.file) {
        load.file->Cancel();
        delete load.file;
        load.file = nullptr;
    }
    if (load.data) {
        load.allocator->Free(load.data, load.size);
        load.data = nullptr;
        load.size = 0;
    }
}

// @func 0x002DA718 OnJobFileLoaded
// The load takes the buffer over from the request.
void OnJobFileLoaded(uint32_t /*id*/, const char* /*name*/, const void* data, uint32_t size, void* userData)
{
    auto* load = static_cast<Episode::FileLoad*>(userData);
    load->data = const_cast<void*>(data);
    load->size = size;
    load->file->buffer = nullptr;
}

// @func 0x002DA158 StartJobFileLoad
void StartJobFileLoad(Episode::FileLoad* load, const char* path)
{
    auto* file = new StreamDiskFile();
    load->file = file;
    file->onLoaded = OnJobFileLoaded;
    file->userData = load;
    file->name = CompiledPath(path);
    file->Start();
    file->Update(false);
}

} // namespace

// @func 0x002D9A14 Episode::ReleaseStreams
void Episode::ReleaseStreams()
{
    ReleaseFileLoad(script);
    if (packages) {
        for (FileLoad*& load : *packages) {
            if (!load)
                continue;
            ReleaseFileLoad(*load);
            delete load;
            load = nullptr;
        }
        packages->Clear();
    }
    if (goals) {
        for (Goal*& goal : *goals) {
            delete goal;
            goal = nullptr;
        }
        goals->Clear();
    }
    loadState = kLoad_None;
}

// @func 0x002DA2A4 Episode::StartLoading
void Episode::StartLoading()
{
    if (loadState == kLoad_Loaded)
        return;
    ReleaseStreams();
    loadState = kLoad_Loading;
    if (!script.allocator)
        script.allocator = ape::GetDefaultAllocator();
    StartJobFileLoad(&script, jobScript);
    for (const ScriptPath& path : scriptPaths) {
        auto* load = new FileLoad();
        load->allocator = ape::GetDefaultAllocator();
        StartJobFileLoad(load, path.path);
        if (!packages)
            packages = new ape::ApeTArray<FileLoad*>();
        packages->PushBack(load);
    }
}

// @func 0x002DA438 Episode::UpdateLoading
// Only one busy file is advanced per call.
void Episode::UpdateLoading()
{
    if (loadState != kLoad_Loading)
        return;
    if (FileBusy(script.file)) {
        script.file->Update(false);
        return;
    }
    bool loaded = script.file && script.file->state == StreamDiskFile::kDone;
    if (packages) {
        for (FileLoad* load : *packages) {
            StreamDiskFile* file = load->file;
            if (FileBusy(file)) {
                file->Update(false);
                return;
            }
            loaded = loaded && file &&
                     (file->state == StreamDiskFile::kDone || file->state == StreamDiskFile::kFailed);
        }
    }
    if (loaded)
        loadState = kLoad_Loaded;
}

// @func 0x002DA5E0 Episode::FinishLoading
void Episode::FinishLoading()
{
    if (FileBusy(script.file)) {
        script.file->Update(true);
        return;
    }
    if (!packages)
        return;
    for (FileLoad* load : *packages) {
        if (FileBusy(load->file)) {
            load->file->Update(true);
            return;
        }
    }
}

// @func 0x002D9CC0 Episode::ParseGoals
bool Episode::ParseGoals()
{
    ape::file::File* file = nullptr;
    if (loadState == kLoad_Loading || loadState == kLoad_Loaded) {
        if (loadState == kLoad_Loading && FileBusy(script.file))
            script.file->Update(true);
        if (!script.file || script.file->state != StreamDiskFile::kDone) {
            ReleaseStreams();
            return false;
        }
        file = ape::file::CreateMemoryFile(script.data, script.size, jobScript);
    }
    if (!file)
        file = ape::file::OpenFile(CompiledPath(jobScript).c_str(), "rb");
    if (!file)
        return false;

    bool any = false;
    ape::xml::XmlDocument document(jobScript);
    if (document.LoadFromStream(*file, jobScript)) {
        ape::xml::XmlElement* root = document.FirstChildElement();
        if (root) {
            int32_t index = 0;
            for (ape::xml::XmlElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement()) {
                // NOTE: the console passes the game world's players block as the
                // goals' job; the port has no object for it yet.
                Goal* goal = sly::goals::ParseGoal(g_game, nullptr, child, index);
                if (!goal)
                    continue;
                if (!goals)
                    goals = new ape::ApeTArray<Goal*>();
                goals->PushBack(goal);
                ++index;
                any = true;
            }
        }
    }
    file->Close();
    return any;
}

// @func 0x002DA04C Episode::MoveGoalsTo
void Episode::MoveGoalsTo(ape::ApeTArray<Goal*>& goalList)
{
    if (!goals)
        return;
    for (Goal* goal : *goals)
        goalList.PushBack(goal);
    goals->Clear();
}

} // namespace game
} // namespace sly
