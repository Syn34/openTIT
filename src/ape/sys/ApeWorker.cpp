// openTIT - Ape engine worker threads.
#include "ape/sys/ApeWorker.h"

#include <condition_variable>
#include <mutex>
#include <thread>

namespace ape {

struct EngineThread {
    const char* name;
    std::thread thread;
};

namespace {

// A one-bit event flag (sys_event_flag with a single bit, waited in AND mode
// and cleared by the waiter).
class EventFlag {
public:
    void Set()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_set = true;
        m_cond.notify_all();
    }
    void WaitAndClear()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [this] { return m_set; });
        m_set = false;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cond;
    bool m_set = false;
};

struct Worker {
    const char* name = nullptr;
    EngineThread* thread = nullptr;
    EventFlag start;        // "WaitStart"
    EventFlag complete;     // "WaitComplete"
    WorkerFunc func = nullptr;
    void* arg = nullptr;
    bool busy = false;
    bool quit = false;      // PC only: lets DestroyWorkerThreads() join the thread
};

const char* const kWorkerNames[kNumWorkerThreads] = { "ApeWorkerThread_1" };
Worker* s_workers[kNumWorkerThreads] = {};   // 0x01164DF8

} // namespace

// @func 0x0056F5BC StartEngineThread
EngineThread* StartEngineThread(const char* name, void (*entry)(void* arg), void* arg, const ThreadParams* params)
{
    static const ThreadParams kDefaultParams;
    if (!params)
        params = &kDefaultParams;
    // NOTE: priority and the 64 KiB stack size have no portable equivalent in
    // std::thread and are not applied.
    (void)params->priority;
    EngineThread* thread = new EngineThread{ name, std::thread(entry, arg) };
    return thread;
}

void JoinEngineThread(EngineThread* thread)
{
    if (!thread)
        return;
    if (thread->thread.joinable())
        thread->thread.join();
    delete thread;
}

// Body of every worker thread: wait for a job, run it, report completion.
// @func 0x00565378 WorkerThreadMain
static void WorkerThreadMain(void* param)
{
    Worker* worker = static_cast<Worker*>(param);
    for (;;) {
        worker->start.WaitAndClear();
        if (worker->quit)
            return;
        if (worker->arg && worker->func)
            worker->func(worker->arg);
        worker->complete.Set();
    }
}

// @func 0x0055DA00 CreateWorkerThreads
bool CreateWorkerThreads()
{
    for (int i = 0; i < kNumWorkerThreads; ++i) {
        Worker* worker = new Worker();
        worker->name = kWorkerNames[i];
        worker->thread = StartEngineThread(worker->name, &WorkerThreadMain, worker);
        s_workers[i] = worker;
    }
    return true;
}

// Part of the core system shutdown (0x0055E7C0).
void DestroyWorkerThreads()
{
    for (int i = 0; i < kNumWorkerThreads; ++i) {
        WaitWorker((uint32_t)i);
        Worker* worker = s_workers[i];
        if (!worker)
            continue;
        worker->quit = true;
        worker->start.Set();
        JoinEngineThread(worker->thread);
        delete worker;
        s_workers[i] = nullptr;
    }
}

// @func 0x00565304 KickWorker
void KickWorker(uint32_t index, WorkerFunc func, void* arg)
{
    Worker* worker = s_workers[index & 0xFFFF];
    worker->func = func;
    worker->arg = arg;
    worker->start.Set();
    worker->busy = true;
}

// @func 0x0055E2AC WaitWorker
void WaitWorker(uint32_t index)
{
    Worker* worker = s_workers[index & 0xFFFF];
    if (!worker || !worker->busy)
        return;
    worker->complete.WaitAndClear();
    worker->busy = false;
}

} // namespace ape
