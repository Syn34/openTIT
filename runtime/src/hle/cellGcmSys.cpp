// cellGcmSys + a "null RSX".
//
// This file is the libgcm side: the GCM context and control registers, IO
// mapping, labels, display buffers, flip/VBlank handlers. Command processing
// lives in runtime/src/rsx/ (FIFO walker, method decoder, state tracker); it
// draws through an rsx::Backend, which is the null backend until a renderer
// is attached - see docs/ROADMAP.md.
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

#include "../hle.h"
#include "../log.h"
#include "../rsx/rsx.h"

using namespace opentit;
using namespace opentit::rtm;

namespace {

constexpr u32 kRsxContextBase = 0x40000000;
constexpr u32 kContextAddr = kRsxContextBase;             // CellGcmContextData copy
constexpr u32 kControlAddr = kRsxContextBase + 0x100000;  // CellGcmControl {put, get, ref}
constexpr u32 kLabelAddr = kRsxContextBase + 0x300000;    // 256 labels x 16 bytes
constexpr u32 kReportAddr = mem::kRsxLocalBase + 0x0E000000;
constexpr u32 kLocalSize = 0x0F900000;

struct Gcm {
    std::mutex m;
    u32 io_address = 0;
    u32 io_size = 0;
    u32 io_ea[4096] = {};   // io offset (1 MiB pages) -> ea page
    u32 ea_io[4096] = {};   // ea page -> io offset page
    bool io_mapped[4096] = {};
    u32 callback_opd = 0;
    u32 vblank_handler = 0;
    u32 flip_handler = 0;
    std::atomic<u32> flip_status{0};
    std::atomic<bool> running{false};
    std::atomic<u64> flips{0};
    u32 display_buffers[8][4] = {};  // offset, pitch, width, height
} g;

u64 vblank_count() {
    static const auto t0 = std::chrono::steady_clock::now();
    const auto us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t0).count();
    return static_cast<u64>(us) * 60 / 1000000;
}

void map_io(u32 ea, u32 io, u32 size) {
    for (u32 k = 0; k < (size + 0xFFFFF) >> 20; ++k) {
        const u32 iop = (io >> 20) + k, eap = (ea >> 20) + k;
        if (iop >= 4096 || eap >= 4096) break;
        g.io_ea[iop] = eap;
        g.ea_io[eap] = iop;
        g.io_mapped[iop] = true;
    }
}

// RSX offset (local or IO) -> guest EA.
u32 offset_to_ea(u32 offset, bool local) {
    if (local) return mem::kRsxLocalBase + offset;
    const u32 page = offset >> 20;
    if (page >= 4096 || !g.io_mapped[page]) return 0;
    return (g.io_ea[page] << 20) | (offset & 0xFFFFF);
}

// ---------------------------------------------------------------- FIFO consumer
// The RSX thread: walks the command buffer and feeds rsx::Decoder, which
// tracks all GPU state and services labels, reports, copies and flips. With
// no renderer attached the backend is the null one.
rsx::Decoder& decoder() {
    static rsx::Decoder d([] {
        rsx::Environment env;
        env.to_ea = [](u32 offset, bool local) { return offset_to_ea(offset, local); };
        env.label_base = kLabelAddr;
        env.local_report_base = kReportAddr;
        env.on_reference = [](u32 v) { mem::w32(kControlAddr + 8, v); };
        env.on_flip = [](u32) {
            g.flips++;
            g.flip_status = 0;  // CELL_GCM_DISPLAY_FLIP_STATUS_DONE
        };
        env.running = [] { return g.running.load(); };
        return env;
    }());
    return d;
}

void fifo_thread() {
    rsx::Fifo fifo(decoder(), kControlAddr);
    const auto io_to_ea = [](u32 io) { return offset_to_ea(io, false); };
    u64 last_report = 0;
    while (g.running) {
        if (!fifo.run_until_idle(io_to_ea)) {
            LOG_ERROR("rsx", "stopping the RSX thread");
            return;
        }
        const u64 f = decoder().flip_count();
        if (f >= last_report + 600) {
            last_report = f;
            LOG_INFO("rsx", "%llu flips, %llu draws, %llu copies", (unsigned long long)f,
                     (unsigned long long)decoder().draw_count(), (unsigned long long)decoder().copy_count());
        }
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}

// Command-buffer-full callback (context->callback). Wraps to the start.
void gcm_callback(PPUContext& ctx) {
    // (CellGcmContextData* context, u32 count)
    const u32 c = arg32(ctx, 0);
    const u32 begin = mem::r32(c), current = mem::r32(c + 8);
    u32 begin_off = 0, cur_off = 0;
    {
        std::lock_guard<std::mutex> lk(g.m);
        begin_off = (g.ea_io[begin >> 20] << 20) | (begin & 0xFFFFF);
        cur_off = (g.ea_io[current >> 20] << 20) | (current & 0xFFFFF);
    }
    mem::w32(current, begin_off | 1);           // jump back to the beginning
    mem::w32(kControlAddr, cur_off);            // flush up to the jump
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (mem::r32(kControlAddr + 4) != begin_off && std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();
    mem::w32(c + 8, begin);
    ret(ctx, CELL_OK);
}

}  // namespace

OPENTIT_HLE(cellGcmSys, _cellGcmInitBody) {
    // (CellGcmContextData** context, u32 cmdSize, u32 ioSize, u32 ioAddress)
    const u32 cmd_size = arg32(ctx, 1);
    const u32 io_size = arg32(ctx, 2);
    const u32 io_address = arg32(ctx, 3);
    mem::commit(kRsxContextBase, 0x00400000);
    mem::commit(mem::kRsxLocalBase, 0x10000000);
    {
        std::lock_guard<std::mutex> lk(g.m);
        g.io_address = io_address;
        g.io_size = io_size;
        map_io(io_address, 0, io_size);
    }
    if (!g.callback_opd) g.callback_opd = hle::make_internal_function("cellGcmCallback", &gcm_callback);

    // CellGcmContextData { u32 begin, end, current, callback }
    const u32 begin = io_address + 0x1000;  // first 4 KiB reserved
    mem::w32(kContextAddr, begin);
    mem::w32(kContextAddr + 4, io_address + std::min<u32>(cmd_size, 32 * 1024) - 4);
    mem::w32(kContextAddr + 8, begin);
    mem::w32(kContextAddr + 12, g.callback_opd);
    mem::w32(arg32(ctx, 0), kContextAddr);

    mem::w32(kControlAddr, 0x1000);
    mem::w32(kControlAddr + 4, 0x1000);
    mem::w32(kControlAddr + 8, 0xFFFFFFFF);
    for (u32 k = 0; k < 256; ++k) mem::w32(kLabelAddr + 16 * k, 0);

    if (!g.running.exchange(true)) std::thread(fifo_thread).detach();
    LOG_INFO("gcm", "init: cmd 0x%X bytes, io 0x%08X+0x%X (null RSX running)", cmd_size, io_address, io_size);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellGcmSys, cellGcmGetConfiguration) {
    // CellGcmConfig { u32 localAddress, ioAddress, localSize, ioSize, memoryFrequency, coreFrequency }
    const u32 p = arg32(ctx, 0);
    mem::w32(p, mem::kRsxLocalBase);
    mem::w32(p + 4, g.io_address);
    mem::w32(p + 8, kLocalSize);
    mem::w32(p + 12, g.io_size);
    mem::w32(p + 16, 650000000);
    mem::w32(p + 20, 500000000);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellGcmSys, cellGcmAddressToOffset) {
    const u32 ea = arg32(ctx, 0);
    u32 off;
    if (ea >= mem::kRsxLocalBase && ea < mem::kRsxLocalBase + 0x10000000) {
        off = ea - mem::kRsxLocalBase;
    } else {
        std::lock_guard<std::mutex> lk(g.m);
        const u32 page = ea >> 20;
        if (!g.ea_io[page] && !(g.io_mapped[0] && g.io_ea[0] == page)) {
            ret(ctx, 0x802100FF);  // CELL_GCM_ERROR_FAILURE
            return;
        }
        off = (g.ea_io[page] << 20) | (ea & 0xFFFFF);
    }
    mem::w32(arg32(ctx, 1), off);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellGcmSys, cellGcmMapMainMemory) {
    // (ea, size, u32* offset)
    std::lock_guard<std::mutex> lk(g.m);
    const u32 pages = (arg32(ctx, 1) + 0xFFFFF) >> 20;
    for (u32 start = 0; start + pages <= 4096; ++start) {
        bool free = true;
        for (u32 k = 0; k < pages && free; ++k) free = !g.io_mapped[start + k];
        if (!free) continue;
        map_io(arg32(ctx, 0), start << 20, arg32(ctx, 1));
        mem::w32(arg32(ctx, 2), start << 20);
        ret(ctx, CELL_OK);
        return;
    }
    ret(ctx, 0x80210002);  // CELL_GCM_ERROR_NO_IO_PAGE_TABLE
}

OPENTIT_HLE(cellGcmSys, cellGcmMapEaIoAddress) {
    std::lock_guard<std::mutex> lk(g.m);
    map_io(arg32(ctx, 0), arg32(ctx, 1), arg32(ctx, 2));
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellGcmSys, cellGcmUnmapEaIoAddress) {
    std::lock_guard<std::mutex> lk(g.m);
    const u32 page = arg32(ctx, 0) >> 20;
    if (page < 4096 && g.io_mapped[g.ea_io[page]]) g.io_mapped[g.ea_io[page]] = false;
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellGcmSys, cellGcmGetControlRegister) { ret(ctx, kControlAddr); }
OPENTIT_HLE(cellGcmSys, cellGcmGetLabelAddress) { ret(ctx, kLabelAddr + 16 * (arg32(ctx, 0) & 0xFF)); }
OPENTIT_HLE(cellGcmSys, cellGcmGetReportDataAddressLocation) { ret(ctx, kReportAddr + 16 * arg32(ctx, 0)); }
OPENTIT_HLE(cellGcmSys, cellGcmGetTimeStamp) { ret(ctx, mem::r64(kReportAddr + 16 * arg32(ctx, 0))); }
OPENTIT_HLE(cellGcmSys, cellGcmGetVBlankCount) { ret(ctx, vblank_count()); }
OPENTIT_HLE(cellGcmSys, cellGcmGetDefaultCommandWordSize) { ret(ctx, 0x400); }
OPENTIT_HLE(cellGcmSys, cellGcmGetDefaultSegmentWordSize) { ret(ctx, 0x100); }
OPENTIT_HLE_OK(cellGcmSys, cellGcmSetDefaultFifoSize)
OPENTIT_HLE_OK(cellGcmSys, cellGcmInitDefaultFifoMode)
OPENTIT_HLE_OK(cellGcmSys, cellGcmSetFlipMode)
OPENTIT_HLE_OK(cellGcmSys, cellGcmSetVBlankFrequency)
OPENTIT_HLE_OK(cellGcmSys, _cellGcmFunc15)
OPENTIT_HLE_OK(cellGcmSys, cellGcmSetTileInfo)
OPENTIT_HLE_OK(cellGcmSys, cellGcmBindTile)
OPENTIT_HLE_OK(cellGcmSys, cellGcmUnbindTile)
OPENTIT_HLE_OK(cellGcmSys, cellGcmBindZcull)
OPENTIT_HLE_OK(cellGcmSys, cellGcmUnbindZcull)

OPENTIT_HLE(cellGcmSys, cellGcmResetFlipStatus) {
    g.flip_status = 1;  // CELL_GCM_DISPLAY_FLIP_STATUS_WAITING
    ret(ctx, CELL_OK);
}

// Calls the game's VBlank handler (void handler(u32 head)) at 60 Hz from its
// own PPU context, like the LV2 interrupt thread does on hardware.
static void vblank_thread() {
    PPUContext vctx;
    vctx.stack_size = 0x10000;
    vctx.stack_addr = alloc_stack(vctx.stack_size);
    vctx.gpr[13] = alloc_tls() + 0x7030;
    set_current_ctx(&vctx);
    auto next = std::chrono::steady_clock::now();
    while (g.running) {
        next += std::chrono::microseconds(16683);
        std::this_thread::sleep_until(next);
        const u32 handler = g.vblank_handler;
        if (!handler) continue;
        vctx.gpr[1] = vctx.stack_addr + vctx.stack_size - 0x100;
        hle::call_guest(vctx, handler, {1});
    }
}

OPENTIT_HLE(cellGcmSys, cellGcmSetVBlankHandler) {
    static std::once_flag started;
    g.vblank_handler = arg32(ctx, 0);
    if (g.vblank_handler) std::call_once(started, [] { std::thread(vblank_thread).detach(); });
    LOG_INFO("gcm", "VBlank handler 0x%08X", g.vblank_handler);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellGcmSys, cellGcmSetDisplayBuffer) {
    // (id, offset, pitch, width, height)
    const u32 id = arg32(ctx, 0);
    if (id < 8) {
        for (int k = 0; k < 4; ++k) g.display_buffers[id][k] = arg32(ctx, 1 + k);
    }
    LOG_INFO("gcm", "display buffer %u: offset 0x%08X pitch %u %ux%u", id, arg32(ctx, 1), arg32(ctx, 2), arg32(ctx, 3),
             arg32(ctx, 4));
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellGcmSys, cellGcmGetTiledPitchSize) {
    static const u32 kPitches[] = {0x200, 0x300, 0x400, 0x500, 0x600, 0x700, 0x800, 0xA00, 0xC00, 0xD00,
                                   0xE00, 0x1000, 0x1400, 0x1800, 0x1A00, 0x1C00, 0x2000, 0x2800, 0x3000, 0x3400,
                                   0x3800, 0x4000, 0x5000, 0x6000, 0x6800, 0x7000, 0x8000, 0xA000, 0xC000, 0xD000,
                                   0xE000, 0x10000};
    const u32 size = arg32(ctx, 0);
    for (u32 p : kPitches)
        if (p >= size) {
            ret(ctx, p);
            return;
        }
    ret(ctx, 0);
}

// Emit a flip into the command buffer: method 0xFEAC (subchannel 7), arg = buffer id.
static void emit_flip(PPUContext& ctx, u32 context, u32 buffer_id) {
    u32 cur = mem::r32(context + 8);
    const u32 end = mem::r32(context + 4);
    if (cur + 8 > end) {
        hle::call_guest(ctx, mem::r32(context + 12), {context, 2});
        cur = mem::r32(context + 8);
    }
    mem::w32(cur, (1u << 18) | 0xFEAC);
    mem::w32(cur + 4, buffer_id);
    mem::w32(context + 8, cur + 8);
}

OPENTIT_HLE(cellGcmSys, _cellGcmSetFlipCommand) {
    emit_flip(ctx, arg32(ctx, 0), arg32(ctx, 1));
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellGcmSys, _cellGcmSetFlipCommandWithWaitLabel) {
    // (context, buffer_id, label_index, label_value)
    emit_flip(ctx, arg32(ctx, 0), arg32(ctx, 1));
    mem::w32(kLabelAddr + 16 * (arg32(ctx, 2) & 0xFF), arg32(ctx, 3));
    ret(ctx, CELL_OK);
}
