// LV2 kernel emulation: objects, threads, memory, syscalls.
#pragma once

#include <condition_variable>
#include <string>
#include <thread>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "opentit/ppu_context.h"
#include "runtime.h"

namespace opentit::kernel {

// ---------------------------------------------------------------- allocators
class RangeAllocator {
public:
    RangeAllocator(u32 base, u32 size) : base_(base), size_(size) { free_[base] = size; }
    u32 alloc(u32 size, u32 align);  // returns 0 on failure
    bool free(u32 addr);
    u32 used() const { return used_; }
    u32 base() const { return base_; }
    u32 size() const { return size_; }

private:
    std::mutex m_;
    u32 base_, size_, used_ = 0;
    std::map<u32, u32> free_;       // addr -> size
    std::map<u32, u32> allocated_;  // addr -> size
};

RangeAllocator& user_memory();     // sys_memory_allocate
RangeAllocator& mmapper_space();   // sys_mmapper_allocate_address
RangeAllocator& kernel_memory();   // stacks, TLS, internal structures

// ---------------------------------------------------------------- IDs
struct Object {
    virtual ~Object() = default;
};

u32 add_object(std::shared_ptr<Object> obj);
std::shared_ptr<Object> get_object(u32 id);
bool remove_object(u32 id);

template <typename T>
std::shared_ptr<T> get(u32 id) {
    return std::dynamic_pointer_cast<T>(get_object(id));
}

// ---------------------------------------------------------------- sync primitives
// Wait helper: timeout_us == 0 means "infinite" (LV2 convention).
template <typename Lock, typename Pred>
bool wait_for(std::condition_variable& cv, Lock& lk, u64 timeout_us, Pred pred) {
    if (timeout_us == 0) {
        cv.wait(lk, pred);
        return true;
    }
    return cv.wait_for(lk, std::chrono::microseconds(timeout_us), pred);
}

struct Mutex : Object {
    std::mutex m;
    std::condition_variable cv;
    u32 owner = 0;
    u32 count = 0;
    bool recursive = false;

    u32 lock(u32 tid, u64 timeout_us);
    u32 trylock(u32 tid);
    u32 unlock(u32 tid);
};

struct Cond : Object {
    std::shared_ptr<Mutex> mutex;
    std::condition_variable_any cv;
    u64 signals = 0;
};

struct EventQueueEntry {
    u64 source, data1, data2, data3;
};

struct EventQueue : Object {
    std::mutex m;
    std::condition_variable cv;
    std::deque<EventQueueEntry> events;
    u32 size = 127;
    u64 key = 0;
    bool destroyed = false;
};

struct EventFlag : Object {
    std::mutex m;
    std::condition_variable cv;
    u64 pattern = 0;
};

struct EventPort : Object {
    u64 name = 0;
    std::weak_ptr<EventQueue> queue;
};

// ---------------------------------------------------------------- threads
struct Thread : Object {
    u32 id = 0;
    std::string name;
    std::unique_ptr<PPUContext> ctx;
    std::thread host;
    std::mutex m;
    std::condition_variable cv;
    bool finished = false;
    bool joinable = false;
    u64 exit_status = 0;
    s32 priority = 1000;
};

u32 create_thread(u32 entry_opd, u64 arg, s32 prio, u32 stack_size, bool joinable, const std::string& name,
                  bool start_now);
u32 current_thread_id();
std::shared_ptr<Thread> thread_by_id(u32 id);
void register_main_thread(PPUContext& ctx, u32 id);

// ---------------------------------------------------------------- syscalls
void dispatch_syscall(PPUContext& ctx, u32 pc);
// SPU syscalls (lv2_spu.cpp). Returns false if `n` is not an SPU syscall.
bool spu_syscall(PPUContext& ctx, u32 n, u64* result);
// Post an event to a queue (false if the queue is full).
bool send_event(EventQueue& q, u64 source, u64 d1, u64 d2, u64 d3);
const char* syscall_name(u32 n);

}  // namespace opentit::kernel
