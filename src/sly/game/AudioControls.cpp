// openTIT - Audio front end: voice parameters, pausing and bus fades
// (0x00444108, 0x0044A78C-0x0044D9C0).
#include "sly/game/AudioSystem.h"

#include "ape/memory/ApeMem.h"
#include "ape/memory/ApeRegionHeap.h"

#include <cmath>

namespace sly {
namespace audio {

namespace {

// Pause state (0x010AF1BD, 0x010AF1C0, 0x010AF1C4) and the mask last sent
// to the library (0x00F65BE0).
bool g_pauseAll = false;
uint32_t g_pausedGroups = 0;
bool g_systemPause = false;
uint32_t g_appliedPauseMask = 0;
bool g_pauseChanged = false;    // 0x00F65B88 NOTE: set by SetPauseAll, reader unknown

// Bus volume fades: current, target and change per second (0x010AF09C,
// 0x010AF0A8, 0x010AF0B4). NOTE: the start values (1.0) are assumed; the
// console arrays are set up outside the ported code.
constexpr uint32_t kBusCount = 4;
struct BusFade {
    float current = 1.0f;
    float target = 1.0f;
    float rate = 0.0f;
};
BusFade g_busFades[kBusCount];

// Sends the difference between the wanted and the applied pause mask.
void ApplyPauseMask()
{
    uint32_t mask = g_pauseAll ? 0xFFFFFFFFu : 0u;
    mask |= g_pausedGroups;
    if (g_systemPause)
        mask |= 2;
    if (AudioEnabled()) {
        const uint32_t added = mask & ~g_appliedPauseMask;
        const uint32_t removed = g_appliedPauseMask & ~mask;
        if (added)
            Backend().PauseGroups(added);
        if (removed)
            Backend().ResumeGroups(removed);
        g_appliedPauseMask = mask;
    }
}

// Voice groups of the categories (0x00F65AB0); dialog also plays on the
// second dialog group (0x00F65B28).
constexpr uint32_t kCategoryGroups[3] = { 0, 1, 2 };
constexpr uint32_t kDialogGroup2 = 0x1E;

// Audio heap and its memory (0x00F65B58 / 0x00F65B5C).
ape::ApeRegionHeap* g_audioHeap = nullptr;
void* g_audioHeapMemory = nullptr;

Voice* LiveVoice(VoiceId id)
{
    return AudioEnabled() ? Voices().Find(id) : nullptr;
}

} // namespace

// @func 0x0044A78C SetVoiceRegister
void SetVoiceRegister(VoiceId id, int reg, int8_t value)
{
    Voice* voice = LiveVoice(id);
    if (voice && Backend().IsPlaying(voice->handle))
        Backend().SetVoiceRegister(voice->handle, reg, value);
}

// @func 0x0044CEB0 SetGlobalRegister
void SetGlobalRegister(int reg, int8_t value)
{
    if (AudioEnabled())
        Backend().SetGlobalRegister(reg, value);
}

// @func 0x0044C758 SetVoicePosition
bool SetVoicePosition(VoiceId id, const float position[3], float volume, uint32_t field_90)
{
    if (!AudioEnabled())
        return false;
    if (Voice* voice = Voices().Find(id)) {
        constexpr float kEpsilon = 1e-5f;
        if (std::fabs(voice->position[0] - position[0]) >= kEpsilon ||
            std::fabs(voice->position[1] - position[1]) >= kEpsilon ||
            std::fabs(voice->position[2] - position[2]) >= kEpsilon) {
            voice->position[0] = position[0];
            voice->position[1] = position[1];
            voice->position[2] = position[2];
            voice->positionDirty = true;
        }
        voice->positional = true;
        voice->requestedVolume = volume;
        voice->field_90 = field_90;
        voice->volume = volume * voice->defVolume * voice->volumeScale;
    }
    return true;
}

// @func 0x00444108 VoiceLevel
float VoiceLevel(VoiceId id)
{
    Voice* voice = LiveVoice(id);
    if (!voice)
        return 0.0f;
    float level = 0.0f;
    const int count = Backend().SubVoiceCount(voice->handle);
    for (int i = 0; i < count; ++i)
        level = std::fmax(level, Backend().SubVoiceLevel(Backend().SubVoice(voice->handle, i)));
    return level;
}

// @func 0x0044BA6C SetPauseAll
void SetPauseAll(bool paused)
{
    g_pauseChanged = true;
    g_pauseAll = paused;
    ApplyPauseMask();
}

// @func 0x0044BB3C IsPauseAll
bool IsPauseAll()
{
    return g_pauseAll;
}

// @func 0x0044BB48 PauseGroups
void PauseGroups(uint32_t mask)
{
    g_pausedGroups |= mask;
    ApplyPauseMask();
}

// @func 0x0044BC10 ResumeGroups
void ResumeGroups(uint32_t mask)
{
    g_pausedGroups &= ~mask;
    ApplyPauseMask();
}

// @func 0x0044BCE0 OutputChannelCount
int OutputChannelCount()
{
    if (!AudioEnabled())
        return 2;
    switch (Backend().SpeakerMode()) {
    case 3: return 6;   // 5.1
    case 4: return 8;   // 7.1
    default: return 2;
    }
}

// @func 0x0044A8A8 FadeBusVolume
void FadeBusVolume(uint32_t bus, float volume, float seconds)
{
    if (bus >= kBusCount)
        return;
    BusFade& fade = g_busFades[bus];
    fade.target = volume;
    if (std::fabs(seconds) < 1e-5f || seconds <= 0.0f) {
        fade.current = volume;
        fade.rate = 0.0f;
    } else {
        fade.rate = (volume - fade.current) / seconds;
    }
}

// @func 0x0044D2E0 IsCategoryPlaying
bool IsCategoryPlaying(SoundCategory category)
{
    if (category == kCategory_None || !AudioEnabled())
        return false;
    if (category == kCategory_Dialog)
        return Backend().IsGroupActive(kCategoryGroups[kCategory_Dialog]) || Backend().IsGroupActive(kDialogGroup2);
    return Backend().IsGroupActive(kCategoryGroups[category]);
}

// @func 0x0044D3AC IsCategoryPlayingByHash
bool IsCategoryPlayingByHash(uint32_t nameHash)
{
    if (!AudioEnabled())
        return false;
    return IsCategoryPlaying(CategoryFromHash(nameHash));
}

// @func 0x0044D4C8 IsTrackedVoicePlaying
bool IsTrackedVoicePlaying(uint32_t key)
{
    return IsVoiceValid(TrackedVoiceId(key));
}

// @func 0x0044D880 SetAudioHeapSize
void SetAudioHeapSize(int kilobytes)
{
    if (g_audioHeap) {
        g_audioHeap->Destroy();
        delete g_audioHeap;
        g_audioHeap = nullptr;
    }
    if (g_audioHeapMemory) {
        ape::MemFree(g_audioHeapMemory);
        g_audioHeapMemory = nullptr;
    }
    if (kilobytes <= 0)
        return;
    const size_t bytes = static_cast<size_t>(kilobytes & 0x3FFFFF) << 10;
    g_audioHeapMemory = ape::MemAlloc(bytes, ape::kMemClass_Default, 16);
    if (!g_audioHeapMemory)
        return;
    g_audioHeap = new ape::ApeRegionHeap();
    g_audioHeap->Init(g_audioHeapMemory, bytes, false);
}

// @func 0x004431A0 IsRequestReady
bool IsRequestReady(const SoundRequest* request)
{
    if (!request->hasVoice)
        return false;
    const Voice* voice = Voices().Find(request->voice);
    const SoundHandle handle = voice ? voice->handle : 0;
    const int count = Backend().SubVoiceCount(handle);
    if (count == 0) {
        // Nothing to buffer: ready unless it is a multi-part sound.
        const SoundDef* def = request->name ? request->name->def : nullptr;
        return !def || def->variantCount <= 0;
    }
    for (int i = 0; i < count; ++i) {
        if (!Backend().SubVoiceStarted(Backend().SubVoice(handle, i)))
            return false;
    }
    return true;
}

// @func 0x004440E8 SoundNameDebugString
const char* SoundNameDebugString(const SoundName*)
{
    return "[UNKNOWN]";
}

// @func 0x004440F4 SoundRequestDebugString
const char* SoundRequestDebugString(const SoundRequest*)
{
    return "[UNKNOWN]";
}

} // namespace audio
} // namespace sly
