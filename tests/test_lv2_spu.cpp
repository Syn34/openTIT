// LV2 SPU thread-group syscalls driven end to end: import an SPU ELF, run a
// group, receive an SPU->PPU event, join and read the exit status.
#include <cstring>
#include <vector>

#include "../runtime/src/kernel.h"
#include "opentit/memory.h"
#include "test.h"
#include "testgen_layout.h"

using namespace opentit;

namespace {

u32 rr(u32 op11, u32 rt, u32 ra, u32 rb) { return (op11 << 21) | (rb << 14) | (ra << 7) | rt; }
u32 ri7(u32 op11, u32 rt, u32 ra, u32 i7) { return (op11 << 21) | ((i7 & 0x7F) << 14) | (ra << 7) | rt; }
u32 ri10(u32 op8, u32 rt, u32 ra, s32 i10) { return (op8 << 24) | ((u32(i10) & 0x3FF) << 14) | (ra << 7) | rt; }
u32 ri16(u32 op9, u32 rt, u32 i16) { return (op9 << 23) | ((i16 & 0xFFFF) << 7) | rt; }

// Minimal SPU ELF: one PT_LOAD at LS 0x100 holding `code`.
std::vector<u8> spu_elf(const std::vector<u32>& code) {
    std::vector<u8> e(0x100 + code.size() * 4, 0);
    std::memcpy(e.data(), "\x7f" "ELF", 4);
    e[4] = 1; e[5] = 2; e[6] = 1;
    store_be<u16>(&e[16], 2);
    store_be<u16>(&e[18], 23);
    store_be<u32>(&e[20], 1);
    store_be<u32>(&e[24], 0x100);  // entry
    store_be<u32>(&e[28], 52);     // phoff
    store_be<u16>(&e[42], 32);
    store_be<u16>(&e[44], 1);
    u8* p = &e[52];
    store_be<u32>(p, 1);
    store_be<u32>(p + 4, 0x100);
    store_be<u32>(p + 8, 0x100);
    store_be<u32>(p + 16, static_cast<u32>(code.size() * 4));
    store_be<u32>(p + 20, static_cast<u32>(code.size() * 4) + 0x100);
    store_be<u32>(p + 24, 5);
    for (std::size_t k = 0; k < code.size(); ++k) store_be<u32>(&e[0x100 + 4 * k], code[k]);
    return e;
}

u64 sys(PPUContext& c, u32 n, std::initializer_list<u64> args) {
    int k = 3;
    for (u64 a : args) c.gpr[k++] = a;
    c.gpr[11] = n;
    kernel::dispatch_syscall(c, 0);
    return c.gpr[3];
}

}  // namespace

TEST(lv2_spu_thread_group_lifecycle) {
    // SPU program: $4 = arg1_low + 10; send_event(port 7, data0 0x42, data1 = $4); exit($4)
    const u32 port = 7;
    std::vector<u32> code = {
        ri7(0x1FC, 4, 3, 4),                         // rotqbyi $4, $3, 4  (arg1 low word -> preferred slot)
        ri10(0x1C, 4, 4, 10),                        // ai $4, $4, 10
        rr(0x10D, 4, 28, 0),                         // wrch SPU_WrOutMbox, $4      (data1)
        ri16(0x082, 5, (port << 24) >> 16),          // ilhu $5, port << 8
        ri16(0x0C1, 5, 0x42),                        // iohl $5, 0x42
        rr(0x10D, 5, 30, 0),                         // wrch SPU_WrOutIntrMbox, $5  (send_event)
        rr(0x00D, 6, 29, 0),                         // rdch $6, SPU_RdInMbox       (result)
        rr(0x10D, 4, 28, 0),                         // wrch SPU_WrOutMbox, $4      (exit status)
        0x102,                                       // stop 0x102 = sys_spu_thread_exit
    };
    const auto elf = spu_elf(code);
    const u32 src = testgen::kScratch + 0x2000;
    mem::write_bytes(src, elf.data(), elf.size());
    const u32 img = testgen::kScratch + 0x1E00;
    const u32 out = testgen::kScratch + 0x1F00;
    const u32 args = testgen::kScratch + 0x1F40;
    std::memset(mem::ptr(args), 0, 32);
    mem::w64(args, 5);

    PPUContext c;
    CHECK_EQ(sys(c, 157, {img, src, elf.size(), 0}), 0u);           // _sys_spu_image_import
    CHECK_EQ(mem::r32(img + 4), 0x100u);                             // entry
    CHECK_EQ(sys(c, 128, {out, 0, 0, 16}), 0u);                      // sys_event_queue_create
    const u32 queue = mem::r32(out);
    CHECK_EQ(sys(c, 170, {out + 4, 1, 100, 0}), 0u);                 // sys_spu_thread_group_create
    const u32 group = mem::r32(out + 4);
    CHECK_EQ(sys(c, 172, {out + 8, group, 0, img, 0, args}), 0u);    // sys_spu_thread_initialize
    const u32 thread = mem::r32(out + 8);
    CHECK_EQ(sys(c, 191, {thread, queue, 1, port}), 0u);             // sys_spu_thread_connect_event
    CHECK_EQ(sys(c, 173, {group}), 0u);                              // start
    CHECK_EQ(sys(c, 130, {queue, 0, 0}), 0u);                        // sys_event_queue_receive (blocks)
    CHECK_EQ(c.gpr[4], 0xFFFFFFFF53505501ull);                       // SYS_SPU_THREAD_EVENT_USER_KEY
    CHECK_EQ(c.gpr[5], u64(thread));
    CHECK_EQ(c.gpr[6], (u64(port) << 32) | 0x42);
    CHECK_EQ(c.gpr[7], 15ull);
    CHECK_EQ(sys(c, 178, {group, out + 12, out + 16}), 0u);          // join
    CHECK_EQ(mem::r32(out + 12), 2u);                                // ALL_THREADS_EXIT
    CHECK_EQ(sys(c, 165, {thread, out + 20}), 0u);                   // exit status
    CHECK_EQ(mem::r32(out + 20), 15u);
}

TEST(lv2_timer_periodic_events) {
    PPUContext c;
    const u32 out = testgen::kScratch + 0x1F80;
    CHECK_EQ(sys(c, 128, {out, 0, 0, 16}), 0u);       // queue
    const u32 queue = mem::r32(out);
    CHECK_EQ(sys(c, 70, {out + 4}), 0u);              // sys_timer_create
    const u32 timer = mem::r32(out + 4);
    CHECK_EQ(sys(c, 73, {timer, 0, 1000}), 0x80010013u);  // start before connecting -> ENOTCONN
    CHECK_EQ(sys(c, 75, {timer, queue, 0xABCD, 11, 22}), 0u);
    CHECK_EQ(sys(c, 73, {timer, 0, 1000}), 0u);       // every 1 ms
    for (int k = 0; k < 2; ++k) {
        CHECK_EQ(sys(c, 130, {queue, 0, 1000000}), 0u);
        CHECK_EQ(c.gpr[4], 0xABCDull);
        CHECK_EQ(c.gpr[5], 11ull);
        CHECK_EQ(c.gpr[6], 22ull);
    }
    CHECK_EQ(sys(c, 74, {timer}), 0u);
    CHECK_EQ(sys(c, 71, {timer}), 0u);
}
