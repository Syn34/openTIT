// SPU thread state, channels and MFC.
//
// An SPUThread is one Synergistic Processing Unit: 128 x 128-bit registers,
// a 256 KiB local store (LS), channels, mailboxes and an MFC that moves data
// between the LS and guest main memory. SPU code is executed by the
// interpreter in runtime/src/spu/ (and, later, by statically recompiled code
// sharing the same per-instruction semantics in spu_ops.h).
#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>

#include "common/types.h"
#include "opentit/vmx.h"

namespace opentit::spu {

inline constexpr u32 kLsSize = 0x40000;
inline constexpr u32 kLsMask = 0x3FFFF;

// Channel numbers
enum : u32 {
    SPU_RdEventStat = 0,
    SPU_WrEventMask = 1,
    SPU_WrEventAck = 2,
    SPU_RdSigNotify1 = 3,
    SPU_RdSigNotify2 = 4,
    SPU_WrDec = 7,
    SPU_RdDec = 8,
    MFC_WrMSSyncReq = 9,
    MFC_RdTagMask = 12,
    SPU_RdEventMask = 11,
    SPU_RdMachStat = 13,
    SPU_WrSRR0 = 14,
    SPU_RdSRR0 = 15,
    MFC_LSA = 16,
    MFC_EAH = 17,
    MFC_EAL = 18,
    MFC_Size = 19,
    MFC_TagID = 20,
    MFC_Cmd = 21,
    MFC_WrTagMask = 22,
    MFC_WrTagUpdate = 23,
    MFC_RdTagStat = 24,
    MFC_RdListStallStat = 25,
    MFC_WrListStallAck = 26,
    MFC_RdAtomicStat = 27,
    SPU_WrOutMbox = 28,
    SPU_RdInMbox = 29,
    SPU_WrOutIntrMbox = 30,
};

// Events (SPU_RdEventStat bits)
enum : u32 {
    SPU_EVENT_MS = 0x1000,   // multisource sync
    SPU_EVENT_A = 0x800,     // privileged attention
    SPU_EVENT_LR = 0x400,    // lock line reservation lost
    SPU_EVENT_S1 = 0x200,    // signal notification 1
    SPU_EVENT_S2 = 0x100,
    SPU_EVENT_LE = 0x80,     // outbound mailbox available
    SPU_EVENT_ME = 0x40,     // outbound interrupt mailbox available
    SPU_EVENT_TM = 0x20,     // decrementer
    SPU_EVENT_MB = 0x10,     // inbound mailbox available
    SPU_EVENT_QV = 0x8,
    SPU_EVENT_SN = 0x2,      // MFC list stall-and-notify
    SPU_EVENT_TG = 0x1,      // tag group status update
};

// MFC commands
enum : u32 {
    MFC_PUT_CMD = 0x20, MFC_PUTB_CMD = 0x21, MFC_PUTF_CMD = 0x22,
    MFC_PUTL_CMD = 0x24, MFC_PUTLB_CMD = 0x25, MFC_PUTLF_CMD = 0x26,
    MFC_PUTR_CMD = 0x30, MFC_PUTRB_CMD = 0x31, MFC_PUTRF_CMD = 0x32,
    MFC_GET_CMD = 0x40, MFC_GETB_CMD = 0x41, MFC_GETF_CMD = 0x42,
    MFC_GETL_CMD = 0x44, MFC_GETLB_CMD = 0x45, MFC_GETLF_CMD = 0x46,
    MFC_SNDSIG_CMD = 0xA0, MFC_SNDSIGB_CMD = 0xA1, MFC_SNDSIGF_CMD = 0xA2,
    MFC_PUTLLUC_CMD = 0xB0, MFC_PUTLLC_CMD = 0xB4, MFC_PUTQLLUC_CMD = 0xB8,
    MFC_BARRIER_CMD = 0xC0, MFC_EIEIO_CMD = 0xC8, MFC_SYNC_CMD = 0xCC,
    MFC_GETLLAR_CMD = 0xD0,
};

// Atomic status
enum : u32 { MFC_PUTLLC_FAILURE = 1, MFC_PUTLLUC_SUCCESS = 2, MFC_GETLLAR_SUCCESS = 4 };

struct SPUThread {
    SPUThread();
    ~SPUThread();
    SPUThread(const SPUThread&) = delete;
    SPUThread& operator=(const SPUThread&) = delete;

    v128 gpr[128];
    u32 pc = 0;
    u8* ls = nullptr;  // 256 KiB, owned
    std::string name;
    u32 id = 0;
    u32 group_id = 0;

    // ---- execution status
    bool stopped = false;
    u32 stop_code = 0;
    std::atomic<bool> kill{false};

    // ---- channels
    std::mutex m;
    std::condition_variable cv;
    std::deque<u32> in_mbox;       // PPU -> SPU (depth 4)
    std::deque<u32> out_mbox;      // SPU -> PPU (depth 1)
    std::deque<u32> out_intr_mbox; // SPU -> PPU interrupt mailbox (depth 1)
    u32 snr[2] = {};
    bool snr_valid[2] = {};
    bool snr_or_mode[2] = {true, true};
    u32 event_mask = 0;
    u32 event_stat = 0;
    u32 srr0 = 0;
    u32 dec_value = 0;
    u64 dec_timestamp = 0;

    // ---- MFC
    u32 mfc_lsa = 0, mfc_eah = 0, mfc_eal = 0, mfc_size = 0, mfc_tag = 0;
    u32 tag_mask = 0;
    u32 tag_update = 0;
    u32 atomic_stat = 0;
    u32 stall_stat = 0;
    alignas(128) u8 rdata[128] = {};
    u32 raddr = 0;
    bool rvalid = false;

    // ---- hooks (installed by LV2 / SPURS emulation)
    // Return true to resume execution after the stop, false to leave the run loop.
    std::function<bool(SPUThread&, u32 code)> on_stop;
    // Interrupt mailbox write (used for sys_spu_thread_send_event & co).
    std::function<void(SPUThread&, u32 value)> on_out_intr_mbox;

    // PPU-side mailbox / signal access
    void push_in_mbox(u32 v);
    bool pop_out_mbox(u32* v);
    void write_snr(int which, u32 v);

    // Local store helpers (LS is big endian)
    u32 ls32(u32 a) const { return load_be<u32>(ls + (a & kLsMask & ~3u)); }
    void set_ls32(u32 a, u32 v) { store_be<u32>(ls + (a & kLsMask & ~3u), v); }
    v128 lsq(u32 a) const { return vmx::load(ls + (a & 0x3FFF0)); }
    void set_lsq(u32 a, const v128& v) { vmx::store(ls + (a & 0x3FFF0), v); }
};

// Channel access (blocking where the hardware blocks).
u32 rdch(SPUThread& t, u32 ch);
void wrch(SPUThread& t, u32 ch, u32 v);
u32 rchcnt(SPUThread& t, u32 ch);

// Interpreter entry: runs from t.pc until a stop the hooks don't resume, or
// until t.kill is set. Returns the final stop code.
u32 run(SPUThread& t);

// Load an SPU ELF image into the LS. Returns the entry point (0 on failure).
u32 load_elf(SPUThread& t, const u8* elf, std::size_t size, std::string* error);

}  // namespace opentit::spu
