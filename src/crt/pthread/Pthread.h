// openTIT - POSIX threads layer of the C runtime.
//
// The console runtime ships a small pthreads implementation on top of LV2
// threads and mutexes (used by the trophy manager). The PC version keeps the
// same API surface and console error codes, built on the C++ thread library.
#pragma once

#include <cstdint>
#include <memory>

namespace crt {
namespace pthread {

// Console error codes (CELL_E*).
enum : int32_t {
    kOk       = 0,
    kEAgain   = (int32_t)0x80010001,
    kEInval   = (int32_t)0x80010002,
    kENoMem   = (int32_t)0x80010004,
    kESrch    = (int32_t)0x80010005,
};

enum MutexType : int32_t { kMutexNormal = 0, kMutexRecursive = 1, kMutexErrorCheck = 2, kMutexDefault = 3 };

struct MutexAttr {
    int32_t type = kMutexDefault;
    int32_t protocol = 0;
    int32_t kind = 0;
    int32_t shared = 0;
};

struct MutexImpl;

// A mutex can be zero-initialised (PTHREAD_MUTEX_INITIALIZER); it is then
// created on first use.
struct Mutex {
    MutexAttr attr;
    MutexImpl* impl = nullptr;
};

struct ThreadAttr {
    int32_t detached = 0;
    int32_t priority = 1000;
    uint32_t stackSize = 0x4000;
    const char* name = nullptr;
};

struct ThreadRecord;
using Thread = ThreadRecord*;
using StartRoutine = void* (*)(void*);

void LibraryInit();
void LibraryShutdown();
void MutexLibraryInit();
void MutexLibraryShutdown();

int32_t MutexInit(Mutex* mutex, const MutexAttr* attr);
int32_t MutexDestroy(Mutex* mutex);
int32_t MutexLock(Mutex* mutex);
int32_t MutexUnlock(Mutex* mutex);

int32_t AttrInit(ThreadAttr* attr);
int32_t AttrDestroy(ThreadAttr* attr);

int32_t Create(Thread* outThread, const ThreadAttr* attr, StartRoutine start, void* arg);
int32_t Join(Thread thread, void** outResult);
[[noreturn]] void Exit(void* result);
Thread Self();

// Cleanup handlers (pthread_cleanup_push/pop).
void CleanupPush(void (*routine)(void*), void* arg);
void CleanupPop(bool execute);

// Thread-specific data.
using Key = int32_t;
int32_t KeyCreate(Key* key, void (*destructor)(void*));
int32_t SetSpecific(Key key, const void* value);
void* GetSpecific(Key key);

} // namespace pthread
} // namespace crt
