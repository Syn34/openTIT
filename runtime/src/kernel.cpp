#include "kernel.h"

#include <atomic>

#include "log.h"
#include "opentit/memory.h"

namespace opentit::kernel {

// ---------------------------------------------------------------- allocators

u32 RangeAllocator::alloc(u32 size, u32 align) {
    if (!size) size = 1;
    if (align < 16) align = 16;
    std::lock_guard<std::mutex> lk(m_);
    for (auto it = free_.begin(); it != free_.end(); ++it) {
        const u32 start = it->first;
        const u32 avail = it->second;
        const u32 aligned = (start + align - 1) & ~(align - 1);
        const u32 pad = aligned - start;
        if (pad > avail || avail - pad < size) continue;
        const u32 tail = avail - pad - size;
        free_.erase(it);
        if (pad) free_[start] = pad;
        if (tail) free_[aligned + size] = tail;
        allocated_[aligned] = size;
        used_ += size;
        return aligned;
    }
    return 0;
}

bool RangeAllocator::free(u32 addr) {
    std::lock_guard<std::mutex> lk(m_);
    auto it = allocated_.find(addr);
    if (it == allocated_.end()) return false;
    u32 start = addr, size = it->second;
    used_ -= size;
    allocated_.erase(it);
    // Coalesce with neighbours.
    auto next = free_.lower_bound(start);
    if (next != free_.end() && start + size == next->first) {
        size += next->second;
        next = free_.erase(next);
    }
    if (next != free_.begin()) {
        auto prev = std::prev(next);
        if (prev->first + prev->second == start) {
            start = prev->first;
            size += prev->second;
            free_.erase(prev);
        }
    }
    free_[start] = size;
    return true;
}

RangeAllocator& user_memory() {
    static RangeAllocator a(mem::kHeapBase, mem::kHeapSize);
    return a;
}
RangeAllocator& mmapper_space() {
    static RangeAllocator a(mem::kMmapperBase, 0x0E000000);
    return a;
}
RangeAllocator& kernel_memory() {
    static RangeAllocator a(0x3E000000, 0x02000000);
    return a;
}

// ---------------------------------------------------------------- IDs

namespace {
std::mutex g_obj_mutex;
std::unordered_map<u32, std::shared_ptr<Object>> g_objects;
std::atomic<u32> g_next_id{0x40000100};
}  // namespace

u32 add_object(std::shared_ptr<Object> obj) {
    const u32 id = g_next_id.fetch_add(0x100);
    std::lock_guard<std::mutex> lk(g_obj_mutex);
    g_objects[id] = std::move(obj);
    return id;
}

std::shared_ptr<Object> get_object(u32 id) {
    std::lock_guard<std::mutex> lk(g_obj_mutex);
    auto it = g_objects.find(id);
    return it == g_objects.end() ? nullptr : it->second;
}

bool remove_object(u32 id) {
    std::lock_guard<std::mutex> lk(g_obj_mutex);
    return g_objects.erase(id) != 0;
}

// ---------------------------------------------------------------- mutex

u32 Mutex::lock(u32 tid, u64 timeout_us) {
    std::unique_lock<std::mutex> lk(m);
    if (owner == tid) {
        if (!recursive) return rtm::CELL_EDEADLK;
        ++count;
        return rtm::CELL_OK;
    }
    if (!wait_for(cv, lk, timeout_us, [&] { return owner == 0; })) return rtm::CELL_ETIMEDOUT;
    owner = tid;
    count = 1;
    return rtm::CELL_OK;
}

u32 Mutex::trylock(u32 tid) {
    std::lock_guard<std::mutex> lk(m);
    if (owner == tid) {
        if (!recursive) return rtm::CELL_EDEADLK;
        ++count;
        return rtm::CELL_OK;
    }
    if (owner) return rtm::CELL_EBUSY;
    owner = tid;
    count = 1;
    return rtm::CELL_OK;
}

u32 Mutex::unlock(u32 tid) {
    std::lock_guard<std::mutex> lk(m);
    if (owner != tid) return rtm::CELL_EPERM;
    if (--count == 0) {
        owner = 0;
        cv.notify_one();
    }
    return rtm::CELL_OK;
}

}  // namespace opentit::kernel

namespace opentit::rtm {

void kernel_init() {
    using namespace kernel;
    mem::commit(user_memory().base(), user_memory().size());
    mem::commit(kernel_memory().base(), kernel_memory().size());
    mem::commit(mem::kStackBase, mem::kStackRegionSize);
}

u32 alloc_guest(u32 size, u32 align) {
    const u32 a = kernel::kernel_memory().alloc(size, align);
    if (a) std::memset(mem::ptr(a), 0, size);
    return a;
}

void free_guest(u32 addr) { kernel::kernel_memory().free(addr); }

u32 alloc_stack(u32 size) {
    static kernel::RangeAllocator stacks(mem::kStackBase, mem::kStackRegionSize);
    size = (size + 0xFFFF) & ~0xFFFFu;
    const u32 a = stacks.alloc(size, 0x10000);
    if (a) std::memset(mem::ptr(a), 0, size);
    return a;
}

u32 alloc_tls() {
    const auto tls = elf().tls();
    const u32 memsz = tls ? static_cast<u32>(tls->memsz) : 0;
    const u32 filesz = tls ? static_cast<u32>(tls->filesz) : 0;
    const u32 base = alloc_guest(0x30 + memsz + 0x10, 0x80);
    if (tls && filesz) std::memcpy(mem::ptr(base + 0x30), mem::ptr(static_cast<u32>(tls->vaddr)), filesz);
    return base;
}

}  // namespace opentit::rtm
