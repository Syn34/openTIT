// openTIT - Ape engine worker threads ("ApeWorkerThread_N").
//
// The engine keeps a small fixed set of worker threads (one in this game)
// that each run one job at a time: KickWorker() hands a function and its
// argument to the worker, WaitWorker() blocks until it has finished. On the
// console each worker is a PPU thread driven by two event flags,
// "WaitStart" and "WaitComplete".
#pragma once

#include <cstdint>

namespace ape {

constexpr int kNumWorkerThreads = 1;

using WorkerFunc = void (*)(void* arg);

// Creates the worker threads (core system init).
bool CreateWorkerThreads();
// Stops and destroys them, waiting for a running job first.
void DestroyWorkerThreads();

// Starts `func(arg)` on worker `index`. The job only runs when both are
// non-null. The previous job must have been waited for.
void KickWorker(uint32_t index, WorkerFunc func, void* arg);
// Waits for the job started on worker `index`, if any.
void WaitWorker(uint32_t index);

// Thread creation parameters (the console passes priority 0x600 by default).
struct ThreadParams {
    int priority = 0x600;
    uint32_t field_04 = 0;   // NOTE: never read by the creation helper
    uint32_t field_08 = 1;   // NOTE: never read by the creation helper
};
// Starts a joinable engine thread running entry(arg) with a 64 KiB stack.
// `name` names the thread for debugging. Returns an opaque handle for
// JoinEngineThread, or null on failure.
struct EngineThread;
EngineThread* StartEngineThread(const char* name, void (*entry)(void* arg), void* arg,
                                const ThreadParams* params = nullptr);
void JoinEngineThread(EngineThread* thread);

} // namespace ape
