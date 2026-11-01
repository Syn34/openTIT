// Small modules: cellAudio (null output), sys_io pads, cellRtc, cellSync,
// sceNpTrophy, cellNetCtl, sys_net. Enough for the game to run offline.
#include <atomic>
#include <chrono>
#include <ctime>
#include <thread>

#include "../hle.h"
#include "../audio_out.h"
#include "../input.h"
#include "../log.h"

using namespace opentit;
using namespace opentit::rtm;

// ---------------------------------------------------------------- cellAudio
// Ports get real buffers and the read index advances in real time (256
// samples @ 48 kHz per block). Each tick, the block at the read index of
// every started port is mixed to stereo and sent to the host device.
namespace {
struct AudioPort {
    bool open = false;
    bool started = false;
    u32 channels = 2, blocks = 8;
    u32 buffer = 0;
    u32 read_index_addr = 0;
    float level = 1.0f;
};
AudioPort g_ports[8];
std::atomic<bool> g_audio_running{false};
std::thread g_audio_thread;
u32 g_read_indices = 0;
}  // namespace

OPENTIT_HLE(cellAudio, cellAudioInit) {
    if (!g_read_indices) g_read_indices = alloc_guest(8 * 8, 0x80);
    audio_out::start();
    if (!g_audio_running.exchange(true)) {
        g_audio_thread = std::thread([] {
            auto next = std::chrono::steady_clock::now();
            while (g_audio_running) {
                next += std::chrono::microseconds(5333);
                std::this_thread::sleep_until(next);
                float mix[512] = {};
                for (auto& p : g_ports) {
                    if (!p.open || !p.started) continue;
                    const u64 idx = mem::r64(p.read_index_addr) % p.blocks;
                    audio_out::mix_block(mem::ptr(p.buffer + static_cast<u32>(idx) * p.channels * 256 * 4), p.channels,
                                         p.level, mix);
                    mem::w64(p.read_index_addr, (idx + 1) % p.blocks);
                }
                for (float& v : mix) v = v > 1.0f ? 1.0f : v < -1.0f ? -1.0f : v;
                audio_out::push(mix, 256);
            }
        });
        g_audio_thread.detach();
    }
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellAudio, cellAudioQuit) {
    g_audio_running = false;
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellAudio, cellAudioPortOpen) {
    // (CellAudioPortParam* {u64 nChannel, u64 nBlock, u64 attr, float level}, u32* portNum)
    const u32 p = arg32(ctx, 0);
    for (u32 i = 0; i < 8; ++i) {
        if (g_ports[i].open) continue;
        AudioPort& port = g_ports[i];
        port.open = true;
        port.channels = static_cast<u32>(mem::r64(p));
        port.blocks = static_cast<u32>(mem::r64(p + 8));
        if (!port.blocks) port.blocks = 8;
        if (port.channels != 2 && port.channels != 8) port.channels = 2;
        port.level = 1.0f;
        if (mem::r64(p + 16) & 0x1000) port.level = mem::rf32(p + 24);  // CELL_AUDIO_PORTATTR_INITLEVEL
        port.buffer = alloc_guest(port.channels * port.blocks * 256 * 4, 0x100);
        port.read_index_addr = g_read_indices + i * 8;
        mem::w64(port.read_index_addr, 0);
        mem::w32(arg32(ctx, 1), i);
        ret(ctx, CELL_OK);
        return;
    }
    ret(ctx, 0x80310705);  // CELL_AUDIO_ERROR_PORT_FULL
}

OPENTIT_HLE(cellAudio, cellAudioGetPortConfig) {
    // (port, CellAudioPortConfig* {u32 readIndexAddr, u32 status, u64 nChannel, u64 nBlock, u32 portSize, u32 portAddr})
    const u32 i = arg32(ctx, 0);
    if (i >= 8 || !g_ports[i].open) {
        ret(ctx, 0x80310704);  // CELL_AUDIO_ERROR_PORT_NOT_OPEN
        return;
    }
    const AudioPort& p = g_ports[i];
    const u32 c = arg32(ctx, 1);
    mem::w32(c, p.read_index_addr);
    mem::w32(c + 4, p.started ? 2 : 1);
    mem::w64(c + 8, p.channels);
    mem::w64(c + 16, p.blocks);
    mem::w32(c + 24, p.channels * p.blocks * 256 * 4);
    mem::w32(c + 28, p.buffer);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellAudio, cellAudioPortStart) {
    if (arg32(ctx, 0) < 8) g_ports[arg32(ctx, 0)].started = true;
    ret(ctx, CELL_OK);
}
OPENTIT_HLE(cellAudio, cellAudioPortStop) {
    if (arg32(ctx, 0) < 8) g_ports[arg32(ctx, 0)].started = false;
    ret(ctx, CELL_OK);
}
OPENTIT_HLE(cellAudio, cellAudioPortClose) {
    if (arg32(ctx, 0) < 8) g_ports[arg32(ctx, 0)] = AudioPort{};
    ret(ctx, CELL_OK);
}

// ---------------------------------------------------------------- sys_io (pads)
// One DualShock 3 on port 0, driven by host input (runtime/src/input.cpp).
OPENTIT_HLE_OK(sys_io, cellPadInit)
OPENTIT_HLE_OK(sys_io, cellPadSetPortSetting)
OPENTIT_HLE_OK(sys_io, cellPadSetActDirect)

OPENTIT_HLE(sys_io, cellPadGetInfo2) {
    // CellPadInfo2 { u32 max_connect, now_connect, system_info; u32 port_status[7], port_setting[7],
    //                device_capability[7], device_type[7] }
    const u32 p = arg32(ctx, 0);
    std::memset(mem::ptr(p), 0, 4 * (3 + 4 * 7));
    mem::w32(p, 7);
    mem::w32(p + 4, 1);
    mem::w32(p + 12, 1);              // port 0: CELL_PAD_STATUS_CONNECTED
    mem::w32(p + 12 + 28 * 2, 0x7);   // capability: PS3 conformity, press, sensor
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(sys_io, cellPadGetData) {
    // (port, CellPadData* {s32 len; u16 button[64]})
    const u32 port = arg32(ctx, 0);
    if (port >= 7) {
        ret(ctx, 0x80121104);  // CELL_PAD_ERROR_INVALID_PARAMETER
        return;
    }
    input::write_cell_pad_data(arg32(ctx, 1), input::pad(static_cast<int>(port)));
    ret(ctx, CELL_OK);
}

// ---------------------------------------------------------------- cellRtc
OPENTIT_HLE(cellRtc, cellRtcGetCurrentClockLocalTime) {
    // CellRtcDateTime { u16 year, month, day, hour, minute, second; u32 microsecond }
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    const u32 p = arg32(ctx, 0);
    mem::w16(p, static_cast<u16>(tm.tm_year + 1900));
    mem::w16(p + 2, static_cast<u16>(tm.tm_mon + 1));
    mem::w16(p + 4, static_cast<u16>(tm.tm_mday));
    mem::w16(p + 6, static_cast<u16>(tm.tm_hour));
    mem::w16(p + 8, static_cast<u16>(tm.tm_min));
    mem::w16(p + 10, static_cast<u16>(tm.tm_sec));
    const auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() % 1000000;
    mem::w32(p + 12, static_cast<u32>(us));
    ret(ctx, CELL_OK);
}

// ---------------------------------------------------------------- cellSync mutex
// CellSyncMutex { be u16 rel; be u16 acq } (4-byte aligned)
OPENTIT_HLE(cellSync, cellSyncMutexInitialize) {
    mem::w32(arg32(ctx, 0), 0);
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellSync, cellSyncMutexTryLock) {
    std::atomic_ref<u32> word(*reinterpret_cast<u32*>(mem::ptr(arg32(ctx, 0))));
    u32 old = word.load();
    while (true) {
        const u16 rel = static_cast<u16>(bswap32(old) >> 16);
        const u16 acq = static_cast<u16>(bswap32(old));
        if (rel != acq) {
            ret(ctx, 0x80410101);  // CELL_SYNC_ERROR_BUSY
            return;
        }
        const u32 next = bswap32((u32(rel) << 16) | u16(acq + 1));
        if (word.compare_exchange_weak(old, next)) break;
    }
    ret(ctx, CELL_OK);
}

OPENTIT_HLE(cellSync, cellSyncMutexUnlock) {
    std::atomic_ref<u32> word(*reinterpret_cast<u32*>(mem::ptr(arg32(ctx, 0))));
    u32 old = word.load();
    while (true) {
        const u32 v = bswap32(old);
        const u32 next = bswap32(((((v >> 16) + 1) & 0xFFFF) << 16) | (v & 0xFFFF));
        if (word.compare_exchange_weak(old, next)) break;
    }
    ret(ctx, CELL_OK);
}

// ---------------------------------------------------------------- sceNpTrophy
OPENTIT_HLE_OK(sceNpTrophy, sceNpTrophyInit)
OPENTIT_HLE_OK(sceNpTrophy, sceNpTrophyTerm)
OPENTIT_HLE_OK(sceNpTrophy, sceNpTrophyDestroyContext)
OPENTIT_HLE_OK(sceNpTrophy, sceNpTrophyDestroyHandle)

OPENTIT_HLE(sceNpTrophy, sceNpTrophyCreateContext) {
    mem::w32(arg32(ctx, 0), 1);
    ret(ctx, CELL_OK);
}
OPENTIT_HLE(sceNpTrophy, sceNpTrophyCreateHandle) {
    mem::w32(arg32(ctx, 0), 1);
    ret(ctx, CELL_OK);
}
OPENTIT_HLE_OK(sceNpTrophy, sceNpTrophyRegisterContext)
OPENTIT_HLE(sceNpTrophy, sceNpTrophyGetRequiredDiskSpace) {
    mem::w64(arg32(ctx, 2), 0);
    ret(ctx, CELL_OK);
}
OPENTIT_HLE(sceNpTrophy, sceNpTrophyUnlockTrophy) {
    LOG_INFO("trophy", "unlocked trophy %d", static_cast<s32>(arg32(ctx, 2)));
    if (arg32(ctx, 3)) mem::w32(arg32(ctx, 3), static_cast<u32>(-1));  // no platinum
    ret(ctx, CELL_OK);
}
OPENTIT_HLE(sceNpTrophy, sceNpTrophyGetTrophyUnlockState) {
    // (context, handle, SceNpTrophyFlagArray* (128 bytes), u32* count)
    std::memset(mem::ptr(arg32(ctx, 2)), 0, 128);
    mem::w32(arg32(ctx, 3), 0);
    ret(ctx, CELL_OK);
}

// ---------------------------------------------------------------- network (offline)
OPENTIT_HLE_OK(cellNetCtl, cellNetCtlInit)
OPENTIT_HLE_OK(cellNetCtl, cellNetCtlTerm)
OPENTIT_HLE(cellNetCtl, cellNetCtlGetState) {
    mem::w32(arg32(ctx, 0), 0);  // CELL_NET_CTL_STATE_Disconnected
    ret(ctx, CELL_OK);
}
OPENTIT_HLE(cellNetCtl, cellNetCtlGetInfo) { ret(ctx, 0x80130106); }  // CELL_NET_CTL_ERROR_NOT_CONNECTED

OPENTIT_HLE_OK(sys_net, sys_net_initialize_network_ex)
OPENTIT_HLE_OK(sys_net, sys_net_finalize_network)
OPENTIT_HLE(sys_net, _sys_net_errno_loc) {
    static u32 loc = 0;
    if (!loc) loc = alloc_guest(4, 4);
    ret(ctx, loc);
}
OPENTIT_HLE(sys_net, socket) { ret(ctx, static_cast<u64>(-1)); }
