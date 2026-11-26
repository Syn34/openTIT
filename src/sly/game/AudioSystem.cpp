// openTIT - Audio front end.
#include "sly/game/AudioSystem.h"
#include "ape/xml/ApeXml.h"
#include "sly/game/SoundZones.h"

#include <vector>

#include "ape/core/ApeString.h"
#include "ape/file/ApeFile.h"
#include "ape/memory/ApeMem.h"
#include "sly/game/Localization.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace sly {
namespace audio {

// ---------------------------------------------------------------------------
// Backend
// ---------------------------------------------------------------------------
namespace {

AudioBackend g_nullBackend;
AudioBackend* g_backend = &g_nullBackend;

bool g_audioEnabled = false;            // 0x010AEC60
VoicePool g_voices(64);                 // 0x010B0200

VoiceId g_musicVoice = 0;               // 0x010AFEA4
SoundName* g_musicName = nullptr;       // 0x00F65B70
SoundName* g_soundNames = nullptr;      // 0x00F65B7C

// Tracked voices, sorted by key (0x010AFEF8).
struct TrackedVoice {
    uint32_t key;
    VoiceId voice;
};
std::vector<TrackedVoice> g_trackedVoices;

constexpr int kCounterCount = 8;
int32_t g_categoryCounters[kCounterCount] = {};   // 0x010B00E0

uint32_t g_audioMemoryUsed = 0;         // 0x010AC934

TrackedVoice* FindTracked(uint32_t key)
{
    auto it = std::lower_bound(g_trackedVoices.begin(), g_trackedVoices.end(), key,
                               [](const TrackedVoice& t, uint32_t k) { return t.key < k; });
    if (it == g_trackedVoices.end() || it->key != key)
        return nullptr;
    return &*it;
}

} // namespace

AudioBackend& Backend()
{
    return *g_backend;
}

void SetBackend(AudioBackend* backend)
{
    g_backend = backend ? backend : &g_nullBackend;
}

bool AudioEnabled()
{
    return g_audioEnabled;
}

void SetAudioEnabled(bool enabled)
{
    g_audioEnabled = enabled;
}

VoicePool& Voices()
{
    return g_voices;
}

// ---------------------------------------------------------------------------
// Categories
// ---------------------------------------------------------------------------

// @func 0x0044D118 CategoryFromHash
SoundCategory CategoryFromHash(uint32_t nameHash)
{
    switch (nameHash) {
    case 0x782CF148u: return kCategory_Dialog;  // "Dialog"
    case 0x0464F605u: return kCategory_Music;   // "Music"
    case 0x00014065u: return kCategory_Sfx;     // "SFX"
    default: return kCategory_None;
    }
}

// ---------------------------------------------------------------------------
// SoundName
// ---------------------------------------------------------------------------

namespace {
std::map<uint32_t, SoundBankDefs*> g_soundBanks;              // 0x010AFEB0
SoundBankDefs* g_defaultSoundBank = nullptr;                  // 0x00F65B60
uint32_t g_defaultSoundBankHash = 0;                          // 0x010B00D8
std::vector<std::pair<uint32_t, SoundBankDefs*>> g_globalSoundBanks;   // 0x010AFEB8

SoundDef* FindInBank(const SoundBankDefs* bank, uint32_t soundHash)
{
    if (!bank)
        return nullptr;
    auto it = bank->defs.find(soundHash);
    return it == bank->defs.end() ? nullptr : it->second;
}
} // namespace

void RegisterSoundBank(uint32_t bankHash, SoundBankDefs* bank)
{
    g_soundBanks[bankHash] = bank;
}

void UnregisterSoundBank(uint32_t bankHash)
{
    g_soundBanks.erase(bankHash);
}

void SetDefaultSoundBank(uint32_t bankHash, SoundBankDefs* bank)
{
    g_defaultSoundBankHash = bankHash;
    g_defaultSoundBank = bank;
}

void AddGlobalSoundBank(uint32_t bankHash, SoundBankDefs* bank)
{
    g_globalSoundBanks.emplace_back(bankHash, bank);
}

// @func 0x00441AC0 FindSoundDef
SoundDef* FindSoundDef(uint32_t soundHash, uint32_t* bankHash)
{
    if (!AudioEnabled() || soundHash == 0)
        return nullptr;
    SoundDef* def = nullptr;
    if (*bankHash != 0) {
        SoundBankDefs* bank = *bankHash == g_defaultSoundBankHash ? g_defaultSoundBank : nullptr;
        if (!bank) {
            auto it = g_soundBanks.find(*bankHash);
            bank = it == g_soundBanks.end() ? nullptr : it->second;
        }
        def = FindInBank(bank, soundHash);
    }
    if (!def && g_defaultSoundBank) {
        def = FindInBank(g_defaultSoundBank, soundHash);
        if (def)
            *bankHash = g_defaultSoundBankHash;
    }
    if (!def) {
        for (const auto& entry : g_globalSoundBanks) {
            if (SoundDef* found = FindInBank(entry.second, soundHash)) {
                *bankHash = entry.first;
                return found;
            }
        }
    }
    return def;
}

// @func 0x00446984 CreateSoundName
SoundName* CreateSoundName(uint32_t soundHash, uint32_t bankHash)
{
    if (soundHash == 0)
        return nullptr;
    uint32_t foundBank = bankHash;
    SoundDef* def = FindSoundDef(soundHash, &foundBank);
    SoundName* name = SoundName::Create(def);
    name->refCount = 1;
    // NOTE: the console stores the requested bank, not the one the sound
    // was found in.
    name->bank = bankHash;
    name->soundHash = soundHash;
    return name;
}

// @func 0x0044BD74 ResolveSoundNames
void ResolveSoundNames()
{
    for (SoundName* name = g_soundNames; name; name = name->next)
        if (!name->def)
            name->def = FindSoundDef(name->soundHash, &name->bank);
}

SoundName* SoundName::Create(SoundDef* def)
{
    SoundName* name = new SoundName;
    name->def = def;
    name->next = g_soundNames;
    if (g_soundNames)
        g_soundNames->prev = name;
    g_soundNames = name;
    return name;
}

// @func 0x00446170 SoundNameAddRef
void SoundNameAddRef(SoundName* name)
{
    if (name)
        ++name->refCount;
}

// @func 0x004432DC SoundNameRelease
void SoundNameRelease(SoundName* name)
{
    if (!name || name->refCount <= 0)
        return;
    if (--name->refCount != 0)
        return;
    if (name->prev)
        name->prev->next = name->next;
    else
        g_soundNames = name->next;
    if (name->next)
        name->next->prev = name->prev;
    delete name;
}

// @func 0x00443150 SoundNameDef
SoundDef* SoundNameDef(const SoundName* name)
{
    return name ? name->def : nullptr;
}

// @func 0x00446960 SoundNameHasDef
bool SoundNameHasDef(const SoundName* name)
{
    return name && name->def;
}

// @func 0x00443168 SoundNameHasVariants
bool SoundNameHasVariants(SoundName* const* name)
{
    return name && *name && (*name)->def && (*name)->def->variantCount > 0;
}

// ---------------------------------------------------------------------------
// Voice pool
// ---------------------------------------------------------------------------

void VoicePool::Resize(int capacity)
{
    m_slots.assign((size_t)capacity, Slot{});
    m_order.clear();
}

VoiceId VoicePool::Add(const Voice& voice)
{
    for (size_t i = 0; i < m_slots.size(); ++i) {
        Slot& slot = m_slots[i];
        if (slot.used)
            continue;
        slot.voice = voice;
        slot.used = true;
        slot.serial = m_nextSerial;
        m_nextSerial = (uint16_t)(m_nextSerial + 1);
        if (m_nextSerial == 0)
            m_nextSerial = 1;
        m_order.push_back((uint16_t)i);
        return ((uint32_t)i << 16) | slot.serial;
    }
    return 0;
}

Voice* VoicePool::Find(VoiceId id)
{
    const uint32_t serial = id & 0xFFFFu;
    const uint32_t index = id >> 16;
    if (serial == 0 || index >= m_slots.size())
        return nullptr;
    Slot& slot = m_slots[index];
    if (!slot.used || slot.serial != serial)
        return nullptr;
    return &slot.voice;
}

void VoicePool::Remove(VoiceId id)
{
    if (!Find(id))
        return;
    const uint16_t index = (uint16_t)(id >> 16);
    m_slots[index].used = false;
    m_order.erase(std::remove(m_order.begin(), m_order.end(), index), m_order.end());
}

std::vector<VoiceId> VoicePool::Ids() const
{
    std::vector<VoiceId> ids;
    ids.reserve(m_order.size());
    for (uint16_t index : m_order)
        ids.push_back(((uint32_t)index << 16) | m_slots[index].serial);
    return ids;
}

// @func 0x004437CC IsVoiceValid
bool IsVoiceValid(VoiceId id)
{
    return g_audioEnabled && g_voices.Find(id) != nullptr;
}

// @func 0x00443368 IsVoicePlaying
bool IsVoicePlaying(VoiceId id)
{
    if (!g_audioEnabled)
        return false;
    const Voice* voice = g_voices.Find(id);
    return voice && Backend().IsPlaying(voice->handle);
}

// @func 0x0044CE10 VoiceSoundHash
uint32_t VoiceSoundHash(VoiceId id)
{
    if (!g_audioEnabled)
        return 0;
    const Voice* voice = g_voices.Find(id);
    return (voice && voice->def) ? voice->def->nameHash : 0;
}

// @func 0x00443424 StopVoice
void StopVoice(VoiceId id, bool release)
{
    if (!g_audioEnabled)
        return;
    Voice* voice = g_voices.Find(id);
    if (!voice)
        return;

    if (Backend().IsPlaying(voice->handle)) {
        if (release) {
            // The voice is cleaned up when its release phase has finished.
            Backend().Stop(voice->handle, true);
            return;
        }
        Backend().Stop(voice->handle, false);
    }

    if (id == g_musicVoice) {
        SoundNameRelease(g_musicName);
        g_musicName = nullptr;
        g_musicVoice = 0;
    }
    if (voice->trackKey != 0) {
        TrackedVoice* tracked = FindTracked(voice->trackKey);
        if (tracked && tracked->voice == id)
            g_trackedVoices.erase(g_trackedVoices.begin() + (tracked - g_trackedVoices.data()));
    }
    if (voice->countedInCategory && voice->counterIndex >= 0 && voice->counterIndex < kCounterCount)
        --g_categoryCounters[voice->counterIndex];
    g_voices.Remove(id);
}

// @func 0x0044CEE8 StopCategory
void StopCategory(SoundCategory category, bool force)
{
    if (category == kCategory_None || !g_audioEnabled)
        return;
    for (VoiceId id : g_voices.Ids()) {
        Voice* voice = g_voices.Find(id);
        if (!voice || !voice->def || voice->def->category != category)
            continue;
        if (!force && voice->def->variantCount != 0) {
            // Multi-part sounds still driven by their request are left to it.
            SoundRequest* owner = Requests().FindByVoice(id);
            if (owner && !owner->detached)
                continue;
        }
        StopVoice(id, true);
    }
}

// @func 0x0044D168 StopCategoryByHash
void StopCategoryByHash(uint32_t nameHash, bool force)
{
    if (!g_audioEnabled)
        return;
    StopCategory(CategoryFromHash(nameHash), force);
}

// @func 0x0044D1E8 StopTrackedVoice
void StopTrackedVoice(uint32_t key)
{
    const TrackedVoice* tracked = FindTracked(key);
    const VoiceId id = tracked ? tracked->voice : 0;
    if (IsVoiceValid(id))
        StopVoice(id, true);
}

void TrackVoice(uint32_t key, VoiceId id)
{
    if (TrackedVoice* tracked = FindTracked(key)) {
        tracked->voice = id;
        return;
    }
    auto it = std::lower_bound(g_trackedVoices.begin(), g_trackedVoices.end(), key,
                               [](const TrackedVoice& t, uint32_t k) { return t.key < k; });
    g_trackedVoices.insert(it, TrackedVoice{key, id});
}

void SetMusicVoice(VoiceId id, SoundName* name)
{
    SoundNameAddRef(name);
    SoundNameRelease(g_musicName);
    g_musicName = name;
    g_musicVoice = id;
}

namespace {
SoundName* g_pendingMusic = nullptr;    // 0x00F65B74
bool g_pendingMusicFlagA = false;       // 0x010AFEA8
bool g_pendingMusicFlagB = false;       // 0x010AFEA9
bool g_musicLocked = false;             // 0x010B0998 NOTE: writer not identified
} // namespace

void SetMusicLocked(bool locked)
{
    g_musicLocked = locked;
}

// @func 0x00449E48 SetPendingMusic
void SetPendingMusic(SoundName* name, bool flagA, bool flagB)
{
    if (!g_audioEnabled)
        return;
    if (g_musicLocked) {
        StopVoice(g_musicVoice, true);
        return;
    }
    if (g_pendingMusic)
        SoundNameRelease(g_pendingMusic);
    g_pendingMusic = name;
    g_pendingMusicFlagA = flagA;
    g_pendingMusicFlagB = flagB;
    if (name)
        SoundNameAddRef(name);
}

// @func 0x00449950 StopAllSounds
void StopAllSounds()
{
    if (!g_audioEnabled)
        return;
    for (VoiceId id : g_voices.Ids())
        StopVoice(id, false);
    g_trackedVoices.clear();
}

VoiceId TrackedVoiceId(uint32_t key)
{
    const TrackedVoice* tracked = FindTracked(key);
    return tracked ? tracked->voice : 0;
}

// @func 0x00449E10 StopMusic
void StopMusic(bool release)
{
    StopVoice(g_musicVoice, release);
}

int32_t CategoryVoiceCount(int index)
{
    return (index >= 0 && index < kCounterCount) ? g_categoryCounters[index] : 0;
}

// ---------------------------------------------------------------------------
// Mix states
// ---------------------------------------------------------------------------
namespace {

const MixState* g_baseMix = nullptr;               // 0x00F65B78
const MixState* g_mixStack[kMaxMixStates] = {};    // 0x010AFB84
uint32_t g_mixStackCount = 0;                      // 0x010AFE90
float g_mixCurrent[kMixParamCount] = {};           // 0x010AFB9C
float g_mixFrom[kMixParamCount] = {};              // 0x010AFD94
float g_mixTarget[kMixParamCount] = {};            // 0x010AFC98
float g_mixBlendTime = 0.0f;                       // 0x010AFE88
float g_mixBlendElapsed = 0.0f;                    // 0x010AFE8C

// TODO(0x00447860): pushes the blended mix to the sound library buses.
void ApplyMix(float dt)
{
    (void)dt;
}

} // namespace

void SetBaseMix(const MixState* base)
{
    g_baseMix = base;
}

float MixParamValue(int index)
{
    return (index >= 0 && index < kMixParamCount) ? g_mixCurrent[index] : 0.0f;
}

// @func 0x00447B70 RecomputeMix
void RecomputeMix(float blendTime)
{
    const bool instant = std::fabs(blendTime) < 0.001f;
    if (!instant) {
        std::memcpy(g_mixFrom, g_mixCurrent, sizeof(g_mixFrom));
        g_mixBlendElapsed = 0.0f;
    }

    for (int i = 0; i < kMixParamCount; ++i) {
        float value = g_baseMix ? g_baseMix->params[i].value : 0.0f;
        for (uint32_t s = 0; s < g_mixStackCount; ++s) {
            const MixParam& p = g_mixStack[s]->params[i];
            switch (p.op) {
            case kMixOp_Set: value = p.value; break;
            case kMixOp_Add: value += p.value; break;
            case kMixOp_Scale: value *= p.value; break;
            default: break;
            }
        }
        g_mixTarget[i] = value;
    }

    if (instant)
        std::memcpy(g_mixCurrent, g_mixTarget, sizeof(g_mixCurrent));
    g_mixBlendTime = blendTime;
    ApplyMix(0.0f);
}

// @func 0x0044D5BC PushMixState
void PushMixState(const MixState* state, float blendTime)
{
    if (g_mixStackCount >= (uint32_t)kMaxMixStates || !state)
        return;
    g_mixStack[g_mixStackCount++] = state;
    RecomputeMix(blendTime);
}

// @func 0x0044D614 RemoveMixState
// Removes the most recently pushed state with the same name. NOTE: the
// original keeps scanning after a removal; with duplicate entries it
// behaves differently, which never happens with the shipped mix states.
void RemoveMixState(const MixState* state, float blendTime)
{
    if (!state)
        return;
    for (int i = (int)g_mixStackCount - 1; i >= 0; --i) {
        if (g_mixStack[i]->nameHash != state->nameHash)
            continue;
        for (uint32_t j = (uint32_t)i + 1; j < g_mixStackCount; ++j)
            g_mixStack[j - 1] = g_mixStack[j];
        --g_mixStackCount;
        RecomputeMix(blendTime);
        return;
    }
}

// @func 0x0044D6D8 IsMixStateActive
bool IsMixStateActive(const MixState* state)
{
    if (!state)
        return false;
    for (int i = (int)g_mixStackCount - 1; i >= 0; --i) {
        if (g_mixStack[i]->nameHash == state->nameHash)
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Buses and request parameters
// ---------------------------------------------------------------------------

// TODO: AudioBus is an engine object (use count at +0xB2, virtual Release in
// slot 10) reconstructed elsewhere; until then references are not counted.
void BusAddRef(AudioBus* bus) { (void)bus; }
void BusRelease(AudioBus* bus) { (void)bus; }

// @func 0x00440AC4 SetRequestBusA
void SetRequestBusA(SoundParams& params, AudioBus* bus)
{
    if (params.busA)
        BusRelease(params.busA);
    if (bus)
        BusAddRef(bus);
    params.busA = bus;
}

// @func 0x00440A40 SetRequestBusB
void SetRequestBusB(SoundParams& params, AudioBus* bus)
{
    if (params.busB)
        BusRelease(params.busB);
    if (bus)
        BusAddRef(bus);
    params.busB = bus;
}

// @func 0x00442FB8 CopySoundParams
void CopySoundParams(SoundParams& dst, const SoundParams& src)
{
    dst.field_00 = src.field_00;
    dst.field_04 = src.field_04;
    dst.field_08 = src.field_08;
    dst.field_20 = src.field_20;
    dst.field_24 = src.field_24;
    dst.field_28 = src.field_28;
    dst.field_10 = src.field_10;
    dst.field_30 = src.field_30;
    dst.field_14 = src.field_14;
    dst.field_34 = src.field_34;
    dst.field_18 = src.field_18;
    std::memcpy(dst.field_2C, src.field_2C, sizeof(dst.field_2C));
    dst.field_38 = src.field_38;
    SetRequestBusA(dst, src.busA);
    SetRequestBusB(dst, src.busB);
}

// ---------------------------------------------------------------------------
// Sound requests
// ---------------------------------------------------------------------------

SoundQueue& Requests()
{
    static SoundQueue s_queue;
    return s_queue;
}

SoundQueue::SoundQueue()
{
    for (int i = kRequestCount - 1; i >= 0; --i) {
        m_records[i].next = m_free;
        m_free = &m_records[i];
    }
}

// @func 0x00445390 SoundQueue::Recycle
void SoundQueue::Recycle(SoundRequest* request)
{
    SoundNameRelease(request->name);
    SetRequestBusA(request->params, nullptr);
    SetRequestBusB(request->params, nullptr);
    request->field_48 = 0;
    request->next = m_free;
    m_free = request;
}

namespace {

// Whether 'incoming' should be placed before 'existing' in the waiting list.
// 'tie' reports two equal non-playing entries.
bool Outranks(const SoundRequest* incoming, const SoundRequest* existing, bool* tie)
{
    *tie = false;
    const SoundDef* a = SoundNameDef(existing->name);
    const SoundDef* b = SoundNameDef(incoming->name);
    if (!a || !b)
        return false;
    if (a->priority != b->priority)
        return a->priority < b->priority;
    if (existing->hasVoice)
        return false;
    if (a->variantCount == b->variantCount) {
        *tie = true;
        return false;
    }
    return a->variantCount < b->variantCount;
}

} // namespace

// @func 0x00445F08 SoundQueue::Insert
void SoundQueue::Insert(SoundRequest* request)
{
    request->queued = true;
    request->detached = false;
    request->hasVoice = false;
    request->field_70 = 0;
    request->refCount = 1;

    if (request->immediate) {
        request->next = nullptr;
        request->prev = m_playingTail;
        if (m_playingTail)
            m_playingTail->next = request;
        else
            m_playingHead = request;
        m_playingTail = request;
        return;
    }

    // Waiting list, best first. A tie with the head goes in front of it.
    bool tie = false;
    if (!m_waiting || Outranks(request, m_waiting, &tie) || tie) {
        request->prev = nullptr;
        request->next = m_waiting;
        if (m_waiting)
            m_waiting->prev = request;
        m_waiting = request;
        return;
    }
    SoundRequest* prev = m_waiting;
    for (SoundRequest* cur = prev->next;; prev = cur, cur = cur->next) {
        if (!cur || Outranks(request, cur, &tie)) {
            request->prev = prev;
            request->next = cur;
            prev->next = request;
            if (cur)
                cur->prev = request;
            return;
        }
    }
}

// TODO(0x00445A40 SoundQueue::Update): starts waiting requests when voices
// are free and retires finished ones.
void SoundQueue::Update(float dt)
{
    (void)dt;
}

// @func 0x00446198 SoundQueue::Request
SoundRequest* SoundQueue::Request(void* owner, SoundName* name, const SoundParams* params,
                                  bool immediate)
{
    if (!name || !params)
        return nullptr;
    SoundRequest* request = m_free;
    if (!request)
        return nullptr;
    m_free = request->next;

    request->name = nullptr;
    request->voice = 0;
    SetRequestBusA(request->params, nullptr);
    SetRequestBusB(request->params, nullptr);
    request->field_48 = 0;
    request->immediate = false;
    request->hasVoice = false;
    request->detached = false;
    request->queued = false;
    request->field_64 = false;
    request->field_65 = false;
    request->field_70 = 0;
    request->owner = nullptr;
    request->prev = nullptr;
    request->next = nullptr;

    SoundNameAddRef(name);
    request->name = name;
    request->immediate = immediate;
    CopySoundParams(request->params, *params);
    request->owner = owner;
    Insert(request);
    Update(0.0f);
    return request;
}

// @func 0x00446348 SoundQueue::RequestImmediate
SoundRequest* SoundQueue::RequestImmediate(SoundName* name, const SoundParams* params)
{
    return Request(nullptr, name, params, true);
}

void SoundQueue::Unlink(SoundRequest* request)
{
    if (request->prev) {
        request->prev->next = request->next;
    } else if (request->immediate) {
        m_playingHead = request->next;
    } else {
        m_waiting = request->next;
    }
    if (request->next)
        request->next->prev = request->prev;
    else if (request->immediate)
        m_playingTail = request->prev;
}

// @func 0x00446384 SoundQueue::Release
void SoundQueue::Release(SoundRequest* request)
{
    if (!request || request->refCount == 0 || --request->refCount != 0)
        return;

    if (!request->queued) {
        // Finished requests sit in the pending list until released.
        if (request->prev)
            request->prev->next = request->next;
        else
            m_pending = request->next;
        if (request->next)
            request->next->prev = request->prev;
        Recycle(request);
        return;
    }
    if (request->detached)
        return;
    if (request->hasVoice)
        StopVoice(request->voice, false);
    Unlink(request);
    request->hasVoice = false;
    request->queued = false;
    Recycle(request);
}

// @func 0x004464F0 SoundQueue::FindPlaying
SoundRequest* SoundQueue::FindPlaying(SoundName* const* name) const
{
    if (!name || !*name)
        return nullptr;
    const SoundDef* def = (*name)->def;
    for (SoundRequest* r = m_playingHead; r; r = r->next) {
        if (!r->field_65 && SoundNameDef(r->name) == def)
            return r;
    }
    return nullptr;
}

SoundRequest* SoundQueue::FindByVoice(VoiceId id) const
{
    for (SoundRequest* r = m_playingHead; r; r = r->next) {
        if (r->voice == id)
            return r;
    }
    for (SoundRequest* r = m_waiting; r; r = r->next) {
        if (r->voice == id)
            return r;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Banks and streams
// ---------------------------------------------------------------------------

// @func 0x0044DBC4 UnloadBank
void UnloadBank(LoadedBank* bank)
{
    if (!bank)
        return;
    Backend().UnloadBank(bank->bank);
    if (bank->fileData) {
        ape::file::FreeFileData(bank->fileData);
        bank->fileData = nullptr;
    }
}

// @func 0x0044DC1C StartStream
void StartStream(const uint32_t* stream, uint32_t arg)
{
    Backend().StartStream(*stream, arg);
}

// @func 0x0044DC4C IsStreamPlaying
bool IsStreamPlaying(SoundHandle h)
{
    return Backend().IsPlaying(h);
}

// @func 0x0044DC80 AudioAlloc
void* AudioAlloc(uint32_t size)
{
    g_audioMemoryUsed += size;
    return ape::MemAlloc(size, 0, 0x10);
}

uint32_t AudioMemoryUsed()
{
    return g_audioMemoryUsed;
}

// @func 0x00448680 UpdateVoiceOverPath
void UpdateVoiceOverPath()
{
    const char* path = ape::TempFormat("%sAudio/Stream/VO/%s", "",
                                       VoiceOverFolder(CurrentLanguage()));
    Backend().SetStreamRoot(2, path);
}

// TODO(0x004480D0 LoadSoundZones): parse Audio\Global\SoundZones.xml.
static void LoadSoundZones(const char* path) { (void)path; }

namespace {

// Mix states are authored as an Excel XML spreadsheet: one row per mix,
// the first cell the mix name, then 31 parameter cells.
constexpr uint32_t kXlsWorkbook = 0x30C0017Au;     // "Workbook"
constexpr uint32_t kXlsWorksheet = 0xE82C68CEu;    // "Worksheet"
constexpr uint32_t kXlsWorksheetNs = 0x058E9C34u;  // "ss:Worksheet"
constexpr uint32_t kXlsTable = 0x04BE43AEu;        // "Table"
constexpr uint32_t kXlsRow = 0x00013DBAu;          // "Row"
constexpr uint32_t kXlsCell = 0x001F8162u;         // "Cell"
constexpr uint32_t kXlsData = 0x001FE7AAu;         // "Data"
constexpr uint32_t kMixHeaderName = 0x63717A3Fu;   // NOTE: name of the header row's first cell
constexpr uint32_t kDefaultMixName = 0x86DF6221u;  // "Default"

// Mix used until a "Default" row is loaded (0x010AFF14): every parameter set to 1.
MixState g_initialMix;

void ResetMixState(MixState& mix, MixOp op)
{
    for (MixParam& param : mix.params) {
        param.op = op;
        param.value = 1.0f;
    }
}

const char* CellText(ape::xml::XmlElement* cell)
{
    ape::xml::XmlElement* data = cell->FirstChildElement(kXlsData);
    if (!data)
        return nullptr;
    const char* text = data->GetText();
    return (text && *text) ? text : nullptr;
}

} // namespace

// Parses one spreadsheet row into `mix`. A parameter cell holds "=v" (set),
// "+v" (add), "*v" (scale), "NOP" (no change) or a bare number (set).
// Returns false for the header row and rows without cells.
// @func 0x00441830 ParseMixStateRow
bool ParseMixStateRow(MixState& mix, ape::xml::XmlElement* row)
{
    ape::xml::XmlElement* cell = row->FirstChildElement(kXlsCell);
    ResetMixState(mix, kMixOp_None);
    if (!cell)
        return false;
    mix.nameHash = 0;
    if (const char* name = CellText(cell))
        mix.nameHash = ape::HashNameNoCase(name);
    if (mix.nameHash == kMixHeaderName)
        return false;

    cell = cell->NextSiblingElement(kXlsCell);
    for (int i = 0; i < kMixParamCount && cell; ++i, cell = cell->NextSiblingElement(kXlsCell)) {
        const char* text = CellText(cell);
        if (!text)
            continue;
        MixParam& param = mix.params[i];
        const char* number = nullptr;
        if (const char* p = ape::StrChr(text, '=')) {
            param.op = kMixOp_Set;
            number = p + 1;
        } else if (const char* p = ape::StrChr(text, '+')) {
            param.op = kMixOp_Add;
            number = p + 1;
        } else if (const char* p = ape::StrRChr(text, '*')) {
            param.op = kMixOp_Scale;
            number = p + 1;
        } else if (ape::StrCaseCompare(text, "NOP") == 0) {
            param.op = kMixOp_None;
            continue;
        } else {
            param.op = kMixOp_Set;
            number = text;
        }
        param.value = ape::StrToFloat(number);
    }
    return true;
}

// @func 0x00447D88 LoadMixStates
static void LoadMixStates(const char* path)
{
    ape::xml::XmlDocument document(path);
    ResetMixState(g_initialMix, kMixOp_Set);
    g_baseMix = &g_initialMix;
    RecomputeMix(0.0f);
    if (!document.LoadFile(0))
        return;
    ape::xml::XmlElement* workbook = document.FirstChildElement(kXlsWorkbook);
    if (!workbook)
        return;
    ape::xml::XmlElement* sheet = workbook->FirstChildElement(kXlsWorksheet);
    if (!sheet)
        sheet = workbook->FirstChildElement(kXlsWorksheetNs);
    ape::xml::XmlElement* table = sheet ? sheet->FirstChildElement(kXlsTable) : nullptr;
    if (!table)
        return;
    ape::xml::XmlElement* row = table->FirstChildElement(kXlsRow);
    if (row) {
        // The first row holds the column titles.
        MixState* pending = nullptr;
        for (row = row->NextSiblingElement(kXlsRow); row; row = row->NextSiblingElement(kXlsRow)) {
            if (!pending)
                pending = new MixState();
            if (!ParseMixStateRow(*pending, row) || FindMixState(pending->nameHash))
                continue;   // the record is reused for the next row
            if (pending->nameHash == kDefaultMixName)
                g_baseMix = pending;
            RegisterMixState(pending->nameHash, pending);
            pending = nullptr;
        }
        delete pending;
    }
    RecomputeMix(0.0f);
}

// @func 0x00448730 AudioInit
bool AudioInit()
{
    // The console asks the AV settings for the best of LPCM 2/6/8 channels
    // and sizes the mixer accordingly (2, 3 or 4 output port pairs).
    if (!Backend().OpenOutput(8))
        return false;

    // The bus file stays loaded for the lifetime of the sound library.
    uint32_t busSize = 0;
    static void* s_busData = nullptr;   // 0x00F65B6C
    s_busData = ape::file::LoadFile("Audio/Sly4.bus", &busSize);
    if (s_busData && !Backend().StartLibrary(s_busData, busSize))
        return false;

    Backend().SetStreamRoot(0, ape::TempFormat("%sAudio", ""));
    Backend().SetStreamRoot(1, ape::TempFormat("%sAudio/Stream/Music", ""));
    UpdateVoiceOverPath();
    Backend().SetMasterParams(0x1E);

    LoadSoundZones("Audio\\Global\\SoundZones.xml");
    LoadMixStates("Audio\\Global\\MixStates.xml");
    return true;
}

} // namespace audio
} // namespace sly
