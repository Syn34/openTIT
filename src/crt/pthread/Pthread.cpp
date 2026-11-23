// openTIT - POSIX threads layer of the C runtime.
#include "crt/pthread/Pthread.h"

#include <algorithm>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

namespace crt {
namespace pthread {

struct MutexImpl {
    MutexType type;
    std::recursive_mutex lock;
};

struct CleanupHandler {
    void (*routine)(void*);
    void* arg;
};

struct ThreadRecord {
    ThreadAttr attr;
    StartRoutine start = nullptr;
    void* arg = nullptr;
    void* result = nullptr;
    std::thread thread;
    std::mutex stateLock;
    bool exited = false;
    bool joined = false;
    std::vector<CleanupHandler> cleanup;
    std::vector<const void*> specific;
};

namespace {

// Thrown by Exit() to unwind back to the thread trampoline.
struct ThreadExit {
    void* result;
};

constexpr int kMaxKeys = 64;

std::mutex& MutexRegistryLock()
{
    static std::mutex s_lock;
    return s_lock;
}
std::list<MutexImpl*>& MutexRegistry()
{
    static std::list<MutexImpl*> s_list;
    return s_list;
}

std::mutex& ThreadRegistryLock()
{
    static std::mutex s_lock;
    return s_lock;
}
std::list<ThreadRecord*>& ThreadRegistry()
{
    static std::list<ThreadRecord*> s_list;
    return s_list;
}

struct KeySlot {
    bool used = false;
    void (*destructor)(void*) = nullptr;
};
std::mutex g_keyLock;
KeySlot g_keys[kMaxKeys];

thread_local ThreadRecord* t_self = nullptr;
// Record for threads not created through Create() (e.g. the main thread).
thread_local ThreadRecord t_foreignRecord;

ThreadRecord* CurrentRecord()
{
    return t_self ? t_self : &t_foreignRecord;
}

// @func 0x005E6A34 MutexCreateLazy
// Creates the backing lock of a statically initialised mutex on first use.
int32_t EnsureMutex(Mutex* mutex)
{
    std::lock_guard<std::mutex> guard(MutexRegistryLock());
    if (mutex->impl)
        return kOk;
    MutexImpl* impl = new (std::nothrow) MutexImpl();
    if (!impl)
        return kENoMem;
    impl->type = (MutexType)mutex->attr.type;
    MutexRegistry().push_back(impl);
    mutex->impl = impl;
    return kOk;
}

// @func 0x005E6FB0 UnregisterThread
// Removes a record from the thread list (all records when `record` is null).
bool UnregisterThread(ThreadRecord* record)
{
    std::lock_guard<std::mutex> guard(ThreadRegistryLock());
    auto& list = ThreadRegistry();
    if (!record) {
        list.clear();
        return true;
    }
    auto it = std::find(list.begin(), list.end(), record);
    if (it == list.end())
        return true;
    list.erase(it);
    return false;
}

// @func 0x005E7220 FindThread
ThreadRecord* FindThread(ThreadRecord* record)
{
    std::lock_guard<std::mutex> guard(ThreadRegistryLock());
    auto& list = ThreadRegistry();
    return std::find(list.begin(), list.end(), record) != list.end() ? record : nullptr;
}

// @func 0x005E7B8C RunCleanupHandlers
void RunCleanupHandlers(ThreadRecord* record)
{
    while (!record->cleanup.empty()) {
        CleanupHandler h = record->cleanup.back();
        record->cleanup.pop_back();
        h.routine(h.arg);
    }
}

// @func 0x005E7C0C RunSpecificDestructors
void RunSpecificDestructors(ThreadRecord* record)
{
    for (int key = 0; key < (int)record->specific.size() && key < kMaxKeys; ++key) {
        void (*destructor)(void*) = nullptr;
        {
            std::lock_guard<std::mutex> guard(g_keyLock);
            if (g_keys[key].used)
                destructor = g_keys[key].destructor;
        }
        void* value = const_cast<void*>(record->specific[key]);
        if (destructor && value)
            destructor(value);
    }
    record->specific.clear();
}

// @func 0x005E7460 FinishThread
// Common tail of a thread: cleanup handlers, thread-specific destructors,
// then the result is published for Join(). Detached records are freed.
void FinishThread(ThreadRecord* record, void* result)
{
    RunCleanupHandlers(record);
    RunSpecificDestructors(record);
    bool detached;
    {
        std::lock_guard<std::mutex> guard(record->stateLock);
        record->result = result;
        record->exited = true;
        detached = record->attr.detached != 0;
    }
    if (detached) {
        UnregisterThread(record);
        record->thread.detach();
        delete record;
    }
}

// @func 0x005E7554 ThreadTrampoline
void ThreadTrampoline(ThreadRecord* record)
{
    t_self = record;
    void* result = nullptr;
    try {
        result = record->start(record->arg);
    } catch (const ThreadExit& e) {
        result = e.result;
    }
    FinishThread(record, result);
}

} // namespace

// @func 0x005E7784 LibraryInit
void LibraryInit()
{
    t_self = nullptr;   // the calling (main) thread uses the foreign record
}

// @func 0x005E7744 LibraryShutdown
void LibraryShutdown()
{
    UnregisterThread(nullptr);
}

// @func 0x005E69D0 MutexLibraryInit
void MutexLibraryInit()
{
    (void)MutexRegistry();
}

// @func 0x005E6ED8 MutexLibraryShutdown
void MutexLibraryShutdown()
{
    std::lock_guard<std::mutex> guard(MutexRegistryLock());
    for (MutexImpl* impl : MutexRegistry())
        delete impl;
    MutexRegistry().clear();
}

// @func 0x005E6D1C MutexInit
int32_t MutexInit(Mutex* mutex, const MutexAttr* attr)
{
    if (!mutex)
        return kEInval;
    mutex->attr = attr ? *attr : MutexAttr();
    mutex->impl = nullptr;
    return EnsureMutex(mutex);
}

// @func 0x005E6DB8 MutexDestroy
int32_t MutexDestroy(Mutex* mutex)
{
    if (!mutex)
        return kEInval;
    std::lock_guard<std::mutex> guard(MutexRegistryLock());
    if (!mutex->impl)
        return kOk;
    MutexRegistry().remove(mutex->impl);
    delete mutex->impl;
    mutex->impl = nullptr;
    return kOk;
}

// @func 0x005E6CC0 MutexLock
int32_t MutexLock(Mutex* mutex)
{
    if (const int32_t err = EnsureMutex(mutex))
        return err;
    mutex->impl->lock.lock();
    return kOk;
}

// @func 0x005E6C68 MutexUnlock
int32_t MutexUnlock(Mutex* mutex)
{
    if (const int32_t err = EnsureMutex(mutex))
        return err;
    mutex->impl->lock.unlock();
    return kOk;
}

// @func 0x005E6944 AttrInit
int32_t AttrInit(ThreadAttr* attr)
{
    if (!attr)
        return kEInval;
    *attr = ThreadAttr();
    return kOk;
}

// @func 0x005E69A4 AttrDestroy
int32_t AttrDestroy(ThreadAttr* attr)
{
    return attr ? kOk : kEInval;
}

// @func 0x005E7824 Create
int32_t Create(Thread* outThread, const ThreadAttr* attr, StartRoutine start, void* arg)
{
    if (!outThread)
        return kEInval;
    ThreadRecord* record = new (std::nothrow) ThreadRecord();
    if (!record)
        return kENoMem;
    record->attr = attr ? *attr : ThreadAttr();
    record->start = start;
    record->arg = arg;
    {
        std::lock_guard<std::mutex> guard(ThreadRegistryLock());
        ThreadRegistry().push_back(record);
    }
    try {
        record->thread = std::thread(ThreadTrampoline, record);
    } catch (...) {
        UnregisterThread(record);
        delete record;
        *outThread = nullptr;
        return kEAgain;
    }
    *outThread = record;
    return kOk;
}

// @func 0x005E760C Join
int32_t Join(Thread thread, void** outResult)
{
    ThreadRecord* record = FindThread(thread);
    if (!record)
        return kESrch;
    if (record->attr.detached || record->joined || record->thread.get_id() == std::this_thread::get_id())
        return kEInval;
    record->joined = true;
    record->thread.join();
    if (outResult)
        *outResult = record->result;
    UnregisterThread(record);
    delete record;
    return kOk;
}

// @func 0x005E7320 Self
Thread Self()
{
    return CurrentRecord();
}

void Exit(void* result)
{
    if (t_self)
        throw ThreadExit{ result };
    // The main thread exiting through pthread_exit ends the process.
    RunCleanupHandlers(CurrentRecord());
    std::exit(0);
}

// @func 0x005E70DC CancelAllThreads
// Shutdown helper: forgets every thread record (the console cancels them).
void CancelAllThreads()
{
    UnregisterThread(nullptr);
}

void CleanupPush(void (*routine)(void*), void* arg)
{
    CurrentRecord()->cleanup.push_back({ routine, arg });
}

void CleanupPop(bool execute)
{
    ThreadRecord* record = CurrentRecord();
    if (record->cleanup.empty())
        return;
    CleanupHandler h = record->cleanup.back();
    record->cleanup.pop_back();
    if (execute)
        h.routine(h.arg);
}

int32_t KeyCreate(Key* key, void (*destructor)(void*))
{
    std::lock_guard<std::mutex> guard(g_keyLock);
    for (int i = 0; i < kMaxKeys; ++i) {
        if (!g_keys[i].used) {
            g_keys[i] = { true, destructor };
            *key = i;
            return kOk;
        }
    }
    return kEAgain;
}

int32_t SetSpecific(Key key, const void* value)
{
    if (key < 0 || key >= kMaxKeys)
        return kEInval;
    ThreadRecord* record = CurrentRecord();
    if ((int)record->specific.size() <= key)
        record->specific.resize(key + 1, nullptr);
    record->specific[key] = value;
    return kOk;
}

void* GetSpecific(Key key)
{
    ThreadRecord* record = CurrentRecord();
    if (key < 0 || key >= (int)record->specific.size())
        return nullptr;
    return const_cast<void*>(record->specific[key]);
}

} // namespace pthread
} // namespace crt
