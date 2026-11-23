// openTIT - Trophy (achievement) manager.
//
// Gameplay asks for trophies to be unlocked; a worker thread registers the
// trophy set, then drains the unlock queue. On PC there is no PSN trophy
// service, so unlocks are recorded in <data>/savedata/TROPHIES.DAT and the
// game's unlock callback is invoked exactly as on the console.
#pragma once

#include "ape/container/ApeTHandleList.h"
#include "crt/pthread/Pthread.h"

#include <cstdint>

namespace sly {

class TrophyManager {
public:
    using UnlockCallback = void (*)(int32_t trophyId);
    static constexpr int kMaxTrophies = 128;
    static constexpr int kQueueCapacity = 32;

    TrophyManager(const char* communicationId, const void* signature, UnlockCallback onUnlock,
                  uint16_t nameTableCapacity, bool buildNameTable);
    ~TrophyManager();

    // Stops the worker and releases everything.
    void Shutdown();
    // Queues an unlock; false if the manager is disabled or the queue is full.
    bool RequestUnlock(int32_t trophyId);
    bool IsUnlocked(uint32_t trophyId) const;

    // Progress callback for the (console) trophy set registration.
    static int32_t RegistrationStatus(void* userdata, int32_t status, int32_t completed, int32_t total);

    static TrophyManager* Instance() { return s_instance; }

private:
    enum State : int32_t {
        kState_Register = 0, kState_ReadGameInfo = 1, kState_CheckSpace = 2,
        kState_WaitSpace = 3, kState_ReadUnlocks = 4, kState_Process = 5, kState_Failed = 6,
    };
    static void* WorkerMain(void* self);
    void WorkerLoop();
    void LoadUnlockState();
    void SaveUnlockState() const;

    const char* m_communicationId;
    const void* m_signature;
    bool m_enabled = false;
    bool m_failed = false;
    UnlockCallback m_onUnlock;
    uint16_t m_nameTableCapacity;
    bool m_buildNameTable;

    crt::pthread::Mutex m_queueLock;     // guards m_queue
    ape::ApeTHandleList<int32_t> m_queue{ kQueueCapacity };
    int32_t m_queuedCount = 0;

    mutable crt::pthread::Mutex m_stateLock;   // guards m_unlocked
    uint32_t m_unlocked[kMaxTrophies / 32] = {};

    crt::pthread::Thread m_thread = nullptr;
    volatile bool m_stop = false;
    State m_state = kState_Register;

    static TrophyManager* s_instance;    // 0x00F668E8
};

} // namespace sly
