// openTIT - The game's audio front end: sound definitions, playing voices,
// mix states, the sound request queue and audio start-up.
//
// The console drives Sony's 989snd/SCREAM library and MultiStream. Here the
// library is reached through AudioBackend; the default backend is silent.
#pragma once

#include <cstdint>
#include <map>
#include <vector>

namespace sly {
namespace audio {

// ---------------------------------------------------------------------------
// Backend abstraction (989snd / MultiStream / cellAudio on PS3)
// ---------------------------------------------------------------------------
using SoundHandle = uint32_t;   // library voice handle, 0 = none

class AudioBackend {
public:
    virtual ~AudioBackend() = default;

    // Output setup: returns false if the audio hardware cannot be opened.
    virtual bool OpenOutput(int maxChannels) { (void)maxChannels; return true; }
    // Loads the bus/mixer description and starts the sound library.
    virtual bool StartLibrary(const void* busData, uint32_t busSize)
    {
        (void)busData; (void)busSize;
        return true;
    }
    // Directory used for one class of streamed files (0 banks, 1 music, 2 VO).
    virtual void SetStreamRoot(int index, const char* path) { (void)index; (void)path; }
    virtual bool IsPlaying(SoundHandle h) { (void)h; return false; }
    // Stops a voice; with 'release' the sound plays its release phase.
    virtual void Stop(SoundHandle h, bool release) { (void)h; (void)release; }
    virtual void UnloadBank(uint32_t bank) { (void)bank; }
    virtual void StartStream(uint32_t stream, uint32_t arg) { (void)stream; (void)arg; }
    virtual void SetMasterParams(int voices) { (void)voices; }
    // Sound script registers: per voice and global (value range -128..127).
    virtual void SetVoiceRegister(SoundHandle h, int reg, int8_t value) { (void)h; (void)reg; (void)value; }
    virtual void SetGlobalRegister(int reg, int8_t value) { (void)reg; (void)value; }
    // Pause groups: pauses the voices of the groups in 'mask' / resumes them.
    virtual void PauseGroups(uint32_t mask) { (void)mask; }
    virtual void ResumeGroups(uint32_t mask) { (void)mask; }
    // Speaker layout reported by the output (2 stereo, 3 5.1, 4 7.1).
    virtual int SpeakerMode() { return 2; }
    // The library voices a sound handle drives and the current output level
    // of one of them (used for lip sync and meters).
    virtual int SubVoiceCount(SoundHandle h) { (void)h; return 0; }
    virtual uint32_t SubVoice(SoundHandle h, int index) { (void)h; (void)index; return 0; }
    virtual float SubVoiceLevel(uint32_t subVoice) { (void)subVoice; return 0.0f; }
};

AudioBackend& Backend();
void SetBackend(AudioBackend* backend);   // null restores the silent backend

// ---------------------------------------------------------------------------
// Sound definitions and shared references to them
// ---------------------------------------------------------------------------
enum SoundCategory : int32_t {
    kCategory_None   = -1,
    kCategory_Sfx    = 0,
    kCategory_Music  = 1,
    kCategory_Dialog = 2,
};

// Maps a category name hash ("SFX", "Music", "Dialog") to its index.
SoundCategory CategoryFromHash(uint32_t nameHash);

// A sound as described by the loaded sound banks. Only the fields used by
// the front end are modelled.
struct SoundDef {
    uint32_t nameHash = 0;      // +0x00
    int32_t variantCount = 0;   // +0x58 > 0 for multi-part (queued) sounds
    int32_t priority = 0;       // +0x5C
    SoundCategory category = kCategory_Sfx; // +0x64
};

// Reference-counted handle to a SoundDef (0x18-byte bucket objects kept in a
// global list at 0x00F65B7C).
struct SoundName {
    SoundDef* def = nullptr;    // +0x00 null until a bank defines the sound
    SoundName* next = nullptr;  // +0x04
    SoundName* prev = nullptr;  // +0x08
    uint32_t bank = 0;          // +0x0C bank the sound was found in (or asked for)
    uint32_t soundHash = 0;     // +0x10
    int32_t refCount = 0;       // +0x14

    static SoundName* Create(SoundDef* def);
};

// Sound definitions of one loaded bank, by sound name hash.
struct SoundBankDefs {
    std::map<uint32_t, SoundDef*> defs;
};
// Bank registry (0x010AFEB0), the level's default bank (0x00F65B60, name
// 0x010B00D8) and the always searched banks (0x010AFEB8).
void RegisterSoundBank(uint32_t bankHash, SoundBankDefs* bank);
void UnregisterSoundBank(uint32_t bankHash);
void SetDefaultSoundBank(uint32_t bankHash, SoundBankDefs* bank);
void AddGlobalSoundBank(uint32_t bankHash, SoundBankDefs* bank);
// Looks a sound up in bank *bankHash, then the default bank, then the global
// banks; *bankHash is updated to the bank that had it. Null when audio is
// off or the sound is unknown.
SoundDef* FindSoundDef(uint32_t soundHash, uint32_t* bankHash);
// New reference (count 1) to a sound, resolved now if possible.
SoundName* CreateSoundName(uint32_t soundHash, uint32_t bankHash);
// Retries the lookup of every unresolved sound name (after a bank load).
void ResolveSoundNames();

void SoundNameAddRef(SoundName* name);
void SoundNameRelease(SoundName* name);
SoundDef* SoundNameDef(const SoundName* name);
bool SoundNameHasDef(const SoundName* name);
bool SoundNameHasVariants(SoundName* const* name);

// ---------------------------------------------------------------------------
// Playing voices. A VoiceId packs the pool slot (high 16 bits) and a serial
// (low 16 bits); serial 0 is never used, so 0 means "no voice".
// ---------------------------------------------------------------------------
using VoiceId = uint32_t;

struct Voice {
    SoundHandle handle = 0;     // +0x10 library handle
    SoundDef* def = nullptr;    // +0x14
    bool countedInCategory = false; // +0x1C
    float volume = 1.0f;        // +0x5C effective volume (requested * def * scale)
    float requestedVolume = 1.0f; // +0x60
    float defVolume = 1.0f;     // +0x64 NOTE: set when the voice starts (0x00444610, not ported)
    float volumeScale = 1.0f;   // +0x68 NOTE: idem
    uint32_t field_90 = 0;      // NOTE: passed with the position, meaning unknown
    bool positional = false;    // +0x94 the voice follows a world position
    float position[3] = {};     // +0xC0
    bool positionDirty = false; // +0xD0 the position changed since the last update
    uint32_t trackKey = 0;      // +0xAC key in the tracked-voice map (0 = none)
    int32_t counterIndex = 0;   // +0xB4 index into the per-category counters
};

class VoicePool {
public:
    explicit VoicePool(int capacity = 0) { Resize(capacity); }
    void Resize(int capacity);
    VoiceId Add(const Voice& voice);
    void Remove(VoiceId id);
    Voice* Find(VoiceId id);
    // Iteration in insertion order.
    std::vector<VoiceId> Ids() const;

private:
    struct Slot {
        Voice voice;
        uint16_t serial = 0;
        bool used = false;
    };
    std::vector<Slot> m_slots;
    std::vector<uint16_t> m_order;
    uint16_t m_nextSerial = 1;
};

bool AudioEnabled();
void SetAudioEnabled(bool enabled);
VoicePool& Voices();

bool IsVoiceValid(VoiceId id);
bool IsVoicePlaying(VoiceId id);
uint32_t VoiceSoundHash(VoiceId id);
// Stops a voice. With 'release' the voice keeps its slot until the release
// phase ends; otherwise it is stopped and unregistered at once.
void StopVoice(VoiceId id, bool release);
// Stops every voice of a category. Without 'force', multi-part voices that
// are still owned by a pending request are left alone.
void StopCategory(SoundCategory category, bool force);
void StopCategoryByHash(uint32_t nameHash, bool force);
// Stops the voice registered under 'key' in the tracked-voice map.
void StopTrackedVoice(uint32_t key);

// Sound script registers of a voice / of the library.
void SetVoiceRegister(VoiceId id, int reg, int8_t value);
void SetGlobalRegister(int reg, int8_t value);
// Moves a positional voice and sets its volume. False when audio is off.
bool SetVoicePosition(VoiceId id, const float position[3], float volume, uint32_t field_90);
// Highest output level of the library voices of a sound (0 if none).
float VoiceLevel(VoiceId id);

// Pausing: everything (e.g. the pause menu) and individual pause groups.
void SetPauseAll(bool paused);
bool IsPauseAll();
void PauseGroups(uint32_t mask);
void ResumeGroups(uint32_t mask);
// Number of output channels of the speaker layout (2, 6 or 8).
int OutputChannelCount();
// Starts fading bus 'bus' (0..3) to 'volume' over 'seconds' (at once for 0).
void FadeBusVolume(uint32_t bus, float volume, float seconds);

// The music voice and its sound reference (0x010AFEA4 / 0x00F65B70).
void SetMusicVoice(VoiceId id, SoundName* name);
void StopMusic(bool release);

// Per-category live voice counters (0x010B00E0).
int32_t CategoryVoiceCount(int index);
void TrackVoice(uint32_t key, VoiceId id);

// ---------------------------------------------------------------------------
// Mix states: up to four modifiers applied on top of the base mix. Each mix
// has 31 parameters; a modifier sets, adds to or scales each of them.
// ---------------------------------------------------------------------------
constexpr int kMixParamCount = 31;
constexpr int kMaxMixStates = 4;

enum MixOp : int32_t {
    kMixOp_None = 0,
    kMixOp_Set = 1,
    kMixOp_Add = 2,
    kMixOp_Scale = 3,
};

struct MixParam {
    MixOp op = kMixOp_None;
    float value = 0.0f;
};

struct MixState {
    uint32_t nameHash = 0;      // +0x00
    MixParam params[kMixParamCount];
};

void SetBaseMix(const MixState* base);
void PushMixState(const MixState* state, float blendTime);
void RemoveMixState(const MixState* state, float blendTime);
bool IsMixStateActive(const MixState* state);
// Recomputes the target mix; blends towards it over 'blendTime' seconds.
void RecomputeMix(float blendTime);
float MixParamValue(int index);

// ---------------------------------------------------------------------------
// Sound requests: queued sounds waiting for a voice (priority ordered) and
// sounds currently playing. 64 request records are pooled.
// ---------------------------------------------------------------------------
class AudioBus;   // engine object referenced by requests (ref counted)

// Playback parameters copied into a request (0x44 bytes on the console).
struct SoundParams {
    uint32_t field_00 = 0;
    uint32_t field_04 = 0;
    uint32_t field_08 = 0;
    uint32_t field_10 = 0;
    uint32_t field_14 = 0;
    uint32_t field_18 = 0;
    uint32_t field_20 = 0;
    uint32_t field_24 = 0;
    uint32_t field_28 = 0;
    uint8_t field_2C[4] = {};
    uint32_t field_30 = 0;
    uint32_t field_34 = 0;
    uint32_t field_38 = 0;
    AudioBus* busA = nullptr;   // +0x3C
    AudioBus* busB = nullptr;   // +0x40
};

struct SoundRequest {
    SoundName* name = nullptr;  // +0x00
    VoiceId voice = 0;          // +0x04
    SoundParams params;         // +0x10
    int32_t field_48 = 0;
    bool immediate = false;     // +0x60 goes straight to the playing list
    bool hasVoice = false;      // +0x61
    bool detached = false;      // +0x62
    bool queued = false;        // +0x63 linked into one of the lists
    bool field_64 = false;
    bool field_65 = false;
    void* owner = nullptr;      // +0x68
    int32_t refCount = 0;       // +0x6C
    int32_t field_70 = 0;
    SoundRequest* prev = nullptr; // +0x74
    SoundRequest* next = nullptr; // +0x78
};

class SoundQueue {
public:
    static constexpr int kRequestCount = 64;
    SoundQueue();

    // Takes a free record and queues 'name' with 'params'.
    SoundRequest* Request(void* owner, SoundName* name, const SoundParams* params, bool immediate);
    SoundRequest* RequestImmediate(SoundName* name, const SoundParams* params);
    void Release(SoundRequest* request);
    SoundRequest* FindPlaying(SoundName* const* name) const;
    // Owner of a voice, searching the playing then the waiting list.
    SoundRequest* FindByVoice(VoiceId id) const;
    // Advances the queue. TODO(0x00445A40).
    void Update(float dt);

private:
    void Recycle(SoundRequest* request);
    void Insert(SoundRequest* request);
    void Unlink(SoundRequest* request);

    SoundRequest m_records[kRequestCount];
    SoundRequest* m_free = nullptr;        // +0x2000
    SoundRequest* m_pending = nullptr;     // +0x2004
    SoundRequest* m_playingHead = nullptr; // +0x2008
    SoundRequest* m_playingTail = nullptr; // +0x200C
    SoundRequest* m_waiting = nullptr;     // +0x2010
};

SoundQueue& Requests();   // 0x010A8C40

// Reference to an engine audio bus object; AddRef bumps its use count
// (bus + 0xB2), Release goes through its virtual slot 10.
void BusAddRef(AudioBus* bus);
void BusRelease(AudioBus* bus);
void SetRequestBusA(SoundParams& params, AudioBus* bus);
void SetRequestBusB(SoundParams& params, AudioBus* bus);
void CopySoundParams(SoundParams& dst, const SoundParams& src);

// ---------------------------------------------------------------------------
// Banks, streams and start-up
// ---------------------------------------------------------------------------
struct LoadedBank {
    uint32_t bank = 0;          // library bank id
    void* fileData = nullptr;   // file image kept while the bank is loaded
};
void UnloadBank(LoadedBank* bank);
void StartStream(const uint32_t* stream, uint32_t arg);
bool IsStreamPlaying(SoundHandle h);
// Allocation hook used by the bank loader; tracks the audio memory total.
void* AudioAlloc(uint32_t size);
uint32_t AudioMemoryUsed();

// Points the VO stream root at the current voice language.
void UpdateVoiceOverPath();
// Opens the audio output and starts the sound library.
bool AudioInit();

} // namespace audio
} // namespace sly
