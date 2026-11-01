// SPU interpreter, channels and MFC.
#include <chrono>
#include <cstring>
#include <new>
#include <thread>

#include "../log.h"
#include "opentit/memory.h"
#include "opentit/spu.h"
#include "opentit/spu_ops.h"

namespace opentit {
u64 read_timebase();
}

namespace opentit::spu {

namespace {

// One lock serialises 128-byte reservation updates across all SPUs.
std::mutex g_reservation_mutex;

using OpFn = u32 (*)(SPUThread&, u32, u32);
constexpr OpFn kTable[] = {
    &ops::INVALID,
#define OP(name, bits, opc, form) &ops::name,
#include "spu/spu_ops.inc"
#undef OP
};

void update_lr(SPUThread& t) {
    if (!t.rvalid) return;
    if (std::memcmp(mem::ptr(t.raddr), t.rdata, 128) != 0) {
        t.rvalid = false;
        t.event_stat |= SPU_EVENT_LR;
    }
}

u32 dec_now(const SPUThread& t) {
    return t.dec_value - static_cast<u32>(read_timebase() - t.dec_timestamp);
}

// Event status, recomputing level-triggered sources.
u32 pending_events(SPUThread& t) {
    update_lr(t);
    if (!t.in_mbox.empty()) t.event_stat |= SPU_EVENT_MB;
    if (t.out_mbox.empty()) t.event_stat |= SPU_EVENT_LE;
    if (t.out_intr_mbox.empty()) t.event_stat |= SPU_EVENT_ME;
    if (t.snr_valid[0]) t.event_stat |= SPU_EVENT_S1;
    if (t.snr_valid[1]) t.event_stat |= SPU_EVENT_S2;
    if ((t.event_mask & SPU_EVENT_TM) && static_cast<s32>(dec_now(t)) < 0) t.event_stat |= SPU_EVENT_TM;
    return t.event_stat & t.event_mask;
}

void dma(SPUThread& t, u32 cmd, u32 lsa, u32 ea, u32 size) {
    if (!size) return;
    lsa &= kLsMask;
    if (lsa + size > kLsSize) size = kLsSize - lsa;
    const bool get = (cmd & 0x40) != 0;
    if (get) std::memcpy(t.ls + lsa, mem::ptr(ea), size);
    else std::memcpy(mem::ptr(ea), t.ls + lsa, size);
}

void mfc_command(SPUThread& t, u32 cmd) {
    const u32 tag = t.mfc_tag & 31;
    switch (cmd) {
    case MFC_PUT_CMD: case MFC_PUTB_CMD: case MFC_PUTF_CMD:
    case MFC_PUTR_CMD: case MFC_PUTRB_CMD: case MFC_PUTRF_CMD:
    case MFC_GET_CMD: case MFC_GETB_CMD: case MFC_GETF_CMD:
        std::atomic_thread_fence(std::memory_order_seq_cst);
        dma(t, cmd, t.mfc_lsa, t.mfc_eal, t.mfc_size);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        return;

    case MFC_PUTL_CMD: case MFC_PUTLB_CMD: case MFC_PUTLF_CMD:
    case MFC_GETL_CMD: case MFC_GETLB_CMD: case MFC_GETLF_CMD: {
        // List in LS at EAL: { u16 stall_notify, u16 size, u32 ea } per element.
        u32 lsa = t.mfc_lsa;
        const u32 list = t.mfc_eal & 0x3FFF8;
        for (u32 off = 0; off < t.mfc_size; off += 8) {
            const u32 w0 = t.ls32(list + off);
            const u32 ea = t.ls32(list + off + 4);
            const u32 sz = w0 & 0x7FFF;
            dma(t, cmd, lsa, ea, sz);
            lsa += (sz + 15) & ~15u;
            if (w0 & 0x80000000u) t.stall_stat |= 1u << tag;
        }
        std::atomic_thread_fence(std::memory_order_seq_cst);
        return;
    }

    case MFC_GETLLAR_CMD: {
        const u32 ea = t.mfc_eal & ~127u;
        std::lock_guard<std::mutex> lk(g_reservation_mutex);
        std::memcpy(t.rdata, mem::ptr(ea), 128);
        std::memcpy(t.ls + (t.mfc_lsa & 0x3FF80), t.rdata, 128);
        t.raddr = ea;
        t.rvalid = true;
        t.atomic_stat = MFC_GETLLAR_SUCCESS;
        return;
    }
    case MFC_PUTLLC_CMD: {
        const u32 ea = t.mfc_eal & ~127u;
        std::lock_guard<std::mutex> lk(g_reservation_mutex);
        const bool ok = t.rvalid && t.raddr == ea && std::memcmp(mem::ptr(ea), t.rdata, 128) == 0;
        if (ok) std::memcpy(mem::ptr(ea), t.ls + (t.mfc_lsa & 0x3FF80), 128);
        t.rvalid = false;
        t.atomic_stat = ok ? 0u : static_cast<u32>(MFC_PUTLLC_FAILURE);
        return;
    }
    case MFC_PUTLLUC_CMD:
    case MFC_PUTQLLUC_CMD: {
        const u32 ea = t.mfc_eal & ~127u;
        std::lock_guard<std::mutex> lk(g_reservation_mutex);
        std::memcpy(mem::ptr(ea), t.ls + (t.mfc_lsa & 0x3FF80), 128);
        if (t.rvalid && t.raddr == ea) t.rvalid = false;
        t.atomic_stat = MFC_PUTLLUC_SUCCESS;
        return;
    }

    case MFC_BARRIER_CMD:
    case MFC_EIEIO_CMD:
    case MFC_SYNC_CMD:
        std::atomic_thread_fence(std::memory_order_seq_cst);
        return;

    case MFC_SNDSIG_CMD: case MFC_SNDSIGB_CMD: case MFC_SNDSIGF_CMD:
        LOG_WARN("spu", "%s: SNDSIG to 0x%08X not supported", t.name.c_str(), t.mfc_eal);
        return;

    default:
        LOG_WARN("spu", "%s: unknown MFC command 0x%X (lsa 0x%X ea 0x%X size 0x%X)", t.name.c_str(), cmd, t.mfc_lsa,
                 t.mfc_eal, t.mfc_size);
        return;
    }
}

template <typename Pred>
void wait(SPUThread& t, std::unique_lock<std::mutex>& lk, Pred pred) {
    while (!pred()) {
        if (t.kill) return;
        t.cv.wait_for(lk, std::chrono::microseconds(50));
    }
}

}  // namespace

SPUThread::SPUThread() {
    ls = static_cast<u8*>(::operator new(kLsSize, std::align_val_t(128)));
    std::memset(ls, 0, kLsSize);
    for (auto& r : gpr) r = v128::zero();
}

SPUThread::~SPUThread() { ::operator delete(ls, std::align_val_t(128)); }

void SPUThread::push_in_mbox(u32 v) {
    std::lock_guard<std::mutex> lk(m);
    if (in_mbox.size() >= 4) in_mbox.pop_front();  // hardware overwrites the oldest
    in_mbox.push_back(v);
    cv.notify_all();
}

bool SPUThread::pop_out_mbox(u32* v) {
    std::lock_guard<std::mutex> lk(m);
    if (out_mbox.empty()) return false;
    *v = out_mbox.front();
    out_mbox.pop_front();
    cv.notify_all();
    return true;
}

void SPUThread::write_snr(int which, u32 v) {
    std::lock_guard<std::mutex> lk(m);
    if (snr_or_mode[which] && snr_valid[which]) snr[which] |= v;
    else snr[which] = v;
    snr_valid[which] = true;
    cv.notify_all();
}

u32 rdch(SPUThread& t, u32 ch) {
    switch (ch) {
    case SPU_RdEventStat: {
        std::unique_lock<std::mutex> lk(t.m);
        u32 ev = 0;
        wait(t, lk, [&] { return (ev = pending_events(t)) != 0; });
        return ev;
    }
    case SPU_RdSigNotify1:
    case SPU_RdSigNotify2: {
        const int w = ch == SPU_RdSigNotify1 ? 0 : 1;
        std::unique_lock<std::mutex> lk(t.m);
        wait(t, lk, [&] { return t.snr_valid[w]; });
        const u32 v = t.snr[w];
        t.snr[w] = 0;
        t.snr_valid[w] = false;
        t.event_stat &= ~(w ? SPU_EVENT_S2 : SPU_EVENT_S1);
        return v;
    }
    case SPU_RdDec: return dec_now(t);
    case SPU_RdEventMask: return t.event_mask;
    case MFC_RdTagMask: return t.tag_mask;
    case SPU_RdMachStat: return 0;
    case SPU_RdSRR0: return t.srr0;
    case MFC_RdTagStat: return t.tag_mask;  // every DMA completes synchronously
    case MFC_RdListStallStat: { const u32 v = t.stall_stat; t.stall_stat = 0; return v; }
    case MFC_RdAtomicStat: { const u32 v = t.atomic_stat; t.atomic_stat = 0; return v; }
    case SPU_RdInMbox: {
        std::unique_lock<std::mutex> lk(t.m);
        wait(t, lk, [&] { return !t.in_mbox.empty(); });
        if (t.in_mbox.empty()) return 0;
        const u32 v = t.in_mbox.front();
        t.in_mbox.pop_front();
        t.cv.notify_all();
        return v;
    }
    case MFC_LSA: return t.mfc_lsa;
    case MFC_EAH: return t.mfc_eah;
    case MFC_EAL: return t.mfc_eal;
    case MFC_Size: return t.mfc_size;
    case MFC_TagID: return t.mfc_tag;
    default:
        LOG_WARN("spu", "%s: rdch from unsupported channel %u", t.name.c_str(), ch);
        return 0;
    }
}

void wrch(SPUThread& t, u32 ch, u32 v) {
    switch (ch) {
    case SPU_WrEventMask: t.event_mask = v; return;
    case SPU_WrEventAck: {
        std::lock_guard<std::mutex> lk(t.m);
        t.event_stat &= ~v;
        return;
    }
    case SPU_WrDec: t.dec_value = v; t.dec_timestamp = read_timebase(); return;
    case SPU_WrSRR0: t.srr0 = v; return;
    case MFC_LSA: t.mfc_lsa = v; return;
    case MFC_EAH: t.mfc_eah = v; return;
    case MFC_EAL: t.mfc_eal = v; return;
    case MFC_Size: t.mfc_size = v & 0xFFFF; return;
    case MFC_TagID: t.mfc_tag = v & 31; return;
    case MFC_Cmd: mfc_command(t, v & 0xFF); return;
    case MFC_WrTagMask: t.tag_mask = v; return;
    case MFC_WrTagUpdate: t.tag_update = v; return;
    case MFC_WrListStallAck: return;
    case MFC_WrMSSyncReq: std::atomic_thread_fence(std::memory_order_seq_cst); return;
    case SPU_WrOutMbox: {
        std::unique_lock<std::mutex> lk(t.m);
        wait(t, lk, [&] { return t.out_mbox.empty(); });
        t.out_mbox.push_back(v);
        t.cv.notify_all();
        return;
    }
    case SPU_WrOutIntrMbox: {
        if (t.on_out_intr_mbox) {
            t.on_out_intr_mbox(t, v);
            return;
        }
        std::unique_lock<std::mutex> lk(t.m);
        wait(t, lk, [&] { return t.out_intr_mbox.empty(); });
        t.out_intr_mbox.push_back(v);
        t.cv.notify_all();
        return;
    }
    default:
        LOG_WARN("spu", "%s: wrch to unsupported channel %u (0x%X)", t.name.c_str(), ch, v);
        return;
    }
}

u32 rchcnt(SPUThread& t, u32 ch) {
    std::lock_guard<std::mutex> lk(t.m);
    switch (ch) {
    case SPU_RdEventStat: return pending_events(t) ? 1 : 0;
    case SPU_RdSigNotify1: return t.snr_valid[0] ? 1 : 0;
    case SPU_RdSigNotify2: return t.snr_valid[1] ? 1 : 0;
    case SPU_RdInMbox: return static_cast<u32>(t.in_mbox.size());
    case SPU_WrOutMbox: return t.out_mbox.empty() ? 1 : 0;
    case SPU_WrOutIntrMbox: return t.out_intr_mbox.empty() ? 1 : 0;
    case MFC_Cmd: return 16;
    case MFC_RdAtomicStat: return t.atomic_stat ? 1 : 0;
    case MFC_RdListStallStat: return t.stall_stat ? 1 : 0;
    default: return 1;
    }
}

u32 run(SPUThread& t) {
    for (;;) {
        if (t.kill.load(std::memory_order_relaxed)) return t.stop_code;
        const u32 raw = load_be<u32>(t.ls + (t.pc & 0x3FFFC));
        const u32 pc = t.pc;
        t.pc = kTable[static_cast<std::size_t>(decode(raw))](t, raw, pc) & 0x3FFFC;
        if (t.stopped) {
            t.stopped = false;
            if (t.stop_code >= 0x10000)
                LOG_WARN("spu", "%s: %s at LS 0x%05X (0x%08X)", t.name.c_str(),
                         t.stop_code == 0x10000 ? "halt" : "invalid instruction", pc, raw);
            if (t.on_stop && t.on_stop(t, t.stop_code)) continue;
            return t.stop_code;
        }
    }
}

u32 load_elf(SPUThread& t, const u8* e, std::size_t size, std::string* error) {
    auto fail = [&](const char* m) {
        if (error) *error = m;
        return 0u;
    };
    if (size < 52 || std::memcmp(e, "\x7f" "ELF", 4) != 0) return fail("not an ELF");
    if (e[4] != 1 || e[5] != 2) return fail("expected ELF32 big endian");
    if (load_be<u16>(e + 18) != 23) return fail("not an SPU ELF");
    const u32 entry = load_be<u32>(e + 24);
    const u32 phoff = load_be<u32>(e + 28);
    const u16 phnum = load_be<u16>(e + 44);
    for (u16 k = 0; k < phnum; ++k) {
        const u8* p = e + phoff + 32u * k;
        if (static_cast<std::size_t>(p - e) + 32 > size) return fail("truncated program headers");
        const u32 type = load_be<u32>(p), off = load_be<u32>(p + 4), va = load_be<u32>(p + 8);
        const u32 filesz = load_be<u32>(p + 16), memsz = load_be<u32>(p + 20);
        if (type != 1) continue;
        if (va + memsz > kLsSize || off + filesz > size) return fail("segment does not fit");
        std::memcpy(t.ls + va, e + off, filesz);
        std::memset(t.ls + va + filesz, 0, memsz - filesz);
    }
    return entry;
}

}  // namespace opentit::spu
