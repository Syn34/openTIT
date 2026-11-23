// openTIT - Trophy (achievement) manager.
#include "sly/trophy/TrophyManager.h"

#include "platform/Platform.h"

#include <cstdio>
#include <cstring>

namespace sly {

TrophyManager* TrophyManager::s_instance = nullptr;

namespace {
const char* const kTrophyFile = "savedata/TROPHIES.DAT";

// Registration progress shared with RegistrationStatus().
float g_registrationProgress = 0.0f;   // 0x00F668EC
float g_registrationBase = 0.0f;       // 0x00F668F0
}

// @func 0x004FB0F0 TrophyManager::TrophyManager
TrophyManager::TrophyManager(const char* communicationId, const void* signature, UnlockCallback onUnlock,
                             uint16_t nameTableCapacity, bool buildNameTable)
    : m_communicationId(communicationId),
      m_signature(signature),
      m_onUnlock(onUnlock),
      m_nameTableCapacity(nameTableCapacity),
      m_buildNameTable(buildNameTable)
{
    s_instance = this;
    crt::pthread::MutexInit(&m_queueLock, nullptr);
    crt::pthread::MutexInit(&m_stateLock, nullptr);

    crt::pthread::ThreadAttr attr;
    crt::pthread::AttrInit(&attr);
    attr.name = "Trophy";
    if (crt::pthread::Create(&m_thread, &attr, &TrophyManager::WorkerMain, this) == crt::pthread::kOk)
        m_enabled = true;
    crt::pthread::AttrDestroy(&attr);
}

TrophyManager::~TrophyManager()
{
    Shutdown();
}

// @func 0x004FB388 TrophyManager::Shutdown
void TrophyManager::Shutdown()
{
    if (!m_thread && !m_enabled)
        return;
    m_stop = true;
    if (m_thread) {
        void* result = nullptr;
        crt::pthread::Join(m_thread, &result);
        m_thread = nullptr;
    }
    m_enabled = false;
    crt::pthread::MutexDestroy(&m_queueLock);
    crt::pthread::MutexDestroy(&m_stateLock);
    if (s_instance == this)
        s_instance = nullptr;
}

// @func 0x004FB4A0 TrophyManager::RequestUnlock
bool TrophyManager::RequestUnlock(int32_t trophyId)
{
    if (!m_enabled)
        return false;
    crt::pthread::MutexLock(&m_queueLock);
    bool queued = false;
    if ((int32_t)m_queue.size() < kQueueCapacity) {
        m_queue.push_back(trophyId);
        queued = true;
    }
    m_queuedCount = (int32_t)m_queue.size();
    crt::pthread::MutexUnlock(&m_queueLock);
    return queued;
}

// @func 0x004FB56C TrophyManager::IsUnlocked
bool TrophyManager::IsUnlocked(uint32_t trophyId) const
{
    if (!m_enabled || trophyId >= (uint32_t)kMaxTrophies)
        return false;
    return (m_unlocked[trophyId >> 5] & (1u << (trophyId & 31))) != 0;
}

// @func 0x004FB5E0 TrophyManager::RegistrationStatus
// Status 1-4 are setup phases, 5-7 the install phases (weighted 10%/80%/10%
// of the progress bar), 8-9 the final steps; anything else cancels.
int32_t TrophyManager::RegistrationStatus(void* userdata, int32_t status, int32_t completed, int32_t total)
{
    (void)userdata;
    float weight = 0.0f;
    if (status >= 5 && status <= 7)
        weight = (status == 6) ? 80.0f : 10.0f;
    else if (status <= 0 || status >= 10) {
        if (s_instance)
            s_instance->m_failed = true;
    }
    if (weight != 0.0f && total > 0) {
        const float progress = ((float)completed / (float)total) * weight + g_registrationBase;
        if (completed == total)
            g_registrationBase = progress;
        if (g_registrationProgress < progress)
            g_registrationProgress = progress;
    }
    return 0;
}

void TrophyManager::LoadUnlockState()
{
    std::unique_ptr<plat::File> f = plat::File::Open(kTrophyFile, plat::File::Mode::Read);
    uint32_t bits[kMaxTrophies / 32] = {};
    if (f && f->Read(bits, sizeof(bits)) != sizeof(bits))
        std::memset(bits, 0, sizeof(bits));
    crt::pthread::MutexLock(&m_stateLock);
    std::memcpy(m_unlocked, bits, sizeof(bits));
    crt::pthread::MutexUnlock(&m_stateLock);
}

void TrophyManager::SaveUnlockState() const
{
    std::unique_ptr<plat::File> f = plat::File::Open(kTrophyFile, plat::File::Mode::Write);
    if (f)
        f->Write(m_unlocked, sizeof(m_unlocked));
}

void* TrophyManager::WorkerMain(void* self)
{
    static_cast<TrophyManager*>(self)->WorkerLoop();
    return nullptr;
}

// @func 0x004FB704 TrophyManager::WorkerLoop
// Registration and space checks are console-only steps; on PC the loop goes
// straight to reading the stored unlock state and then serves the queue.
void TrophyManager::WorkerLoop()
{
    m_state = kState_ReadUnlocks;
    while (!m_stop) {
        switch (m_state) {
        case kState_ReadUnlocks:
            LoadUnlockState();
            m_state = kState_Process;
            break;
        case kState_Process: {
            int32_t id = -1;
            crt::pthread::MutexLock(&m_queueLock);
            if (!m_queue.empty()) {
                id = m_queue.front();
                m_queue.pop_front();
            }
            m_queuedCount = (int32_t)m_queue.size();
            crt::pthread::MutexUnlock(&m_queueLock);

            if (id >= 0 && id < kMaxTrophies && !IsUnlocked((uint32_t)id)) {
                crt::pthread::MutexLock(&m_stateLock);
                m_unlocked[id >> 5] |= 1u << (id & 31);
                SaveUnlockState();
                crt::pthread::MutexUnlock(&m_stateLock);
                if (m_onUnlock)
                    m_onUnlock(id);
            }
            break;
        }
        default:
            m_failed = true;
            m_stop = true;
            break;
        }
        plat::SleepUsec(16000);   // the console thread yields one frame per pass
    }
}

} // namespace sly
