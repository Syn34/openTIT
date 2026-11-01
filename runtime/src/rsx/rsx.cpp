#include "rsx.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

#include "../log.h"
#include "opentit/memory.h"

namespace opentit::rsx {

// ---------------------------------------------------------------- names
namespace {
struct Named {
    u32 reg;
    const char* name;
};
constexpr Named kNames[] = {
#define X(off, name) {off, #name},
#include "rsx_methods.inc"
#undef X
};

// Array registers: (first, element count, element stride in bytes)
struct ArrayReg {
    u32 base, count, stride;
};
constexpr ArrayReg kArrays[] = {
    {0x0A80, 16, 4}, {0x0B00, 16, 4}, {0x0B40, 10, 4}, {0x0B80, 32, 4}, {0x1480, 32, 4}, {0x1500, 48, 4},
    {0x1680, 16, 4}, {0x1740, 16, 4}, {0x1840, 16, 4}, {0x1880, 32, 4}, {0x1900, 16, 4}, {0x1940, 16, 4},
    {0x1980, 32, 4}, {0x1A00, 16, 0x20}, {0x1C00, 64, 4}, {0x1E40, 16, 4}, {0x1F00, 32, 4}, {0x0900, 4, 0x20},
    {0xA400, 0x700, 4},
};
}  // namespace

const char* method_name(u32 reg) {
    reg &= 0xFFFC;
    const auto* it = std::lower_bound(std::begin(kNames), std::end(kNames), reg,
                                      [](const Named& n, u32 r) { return n.reg < r; });
    return it != std::end(kNames) && it->reg == reg ? it->name : nullptr;
}

std::string describe_method(u32 reg) {
    reg &= 0xFFFC;
    if (const char* n = method_name(reg)) return n;
    for (const auto& a : kArrays) {
        if (reg >= a.base && reg < a.base + a.count * a.stride) {
            const u32 idx = (reg - a.base) / a.stride, field = (reg - a.base) % a.stride;
            if (const char* n = method_name(a.base + field)) return std::string(n) + "+" + std::to_string(idx);
        }
    }
    char buf[16];
    std::snprintf(buf, sizeof buf, "0x%04X", reg);
    return buf;
}

// ---------------------------------------------------------------- state views
VertexArray State::vertex_array(int i) const {
    VertexArray a;
    const u32 fmt = reg(NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 4 * i);
    const u32 off = reg(NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 4 * i);
    a.type = fmt & 0xF;
    a.size = (fmt >> 4) & 0xF;
    a.stride = (fmt >> 8) & 0xFF;
    a.frequency = fmt >> 16;
    a.offset = off & 0x7FFFFFFF;
    a.local = (off >> 31) == 0;
    return a;
}

Surface State::surface() const {
    Surface s;
    const u32 f = reg(NV4097_SET_SURFACE_FORMAT);
    s.color_format = f & 0x1F;
    s.depth_format = (f >> 5) & 7;
    s.type = (f >> 8) & 0xF;
    s.antialias = (f >> 12) & 0xF;
    s.log2_width = (f >> 16) & 0xFF;
    s.log2_height = f >> 24;
    const u32 h = reg(NV4097_SET_SURFACE_CLIP_HORIZONTAL), v = reg(NV4097_SET_SURFACE_CLIP_VERTICAL);
    s.x = h & 0xFFFF;
    s.width = h >> 16;
    s.y = v & 0xFFFF;
    s.height = v >> 16;
    s.target = reg(NV4097_SET_SURFACE_COLOR_TARGET);
    const u32 offs[4] = {NV4097_SET_SURFACE_COLOR_AOFFSET, NV4097_SET_SURFACE_COLOR_BOFFSET,
                         NV4097_SET_SURFACE_COLOR_COFFSET, NV4097_SET_SURFACE_COLOR_DOFFSET};
    const u32 pitches[4] = {NV4097_SET_SURFACE_PITCH_A, NV4097_SET_SURFACE_PITCH_B, NV4097_SET_SURFACE_PITCH_C,
                            NV4097_SET_SURFACE_PITCH_D};
    const u32 dmas[4] = {NV4097_SET_CONTEXT_DMA_COLOR_A, NV4097_SET_CONTEXT_DMA_COLOR_B,
                         NV4097_SET_CONTEXT_DMA_COLOR_C, NV4097_SET_CONTEXT_DMA_COLOR_D};
    for (int i = 0; i < 4; ++i) {
        s.color_offset[i] = reg(offs[i]);
        s.color_pitch[i] = reg(pitches[i]);
        s.color_local[i] = reg(dmas[i]) != CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER;
    }
    s.zeta_offset = reg(NV4097_SET_SURFACE_ZETA_OFFSET);
    s.zeta_pitch = reg(NV4097_SET_SURFACE_PITCH_Z);
    s.zeta_local = reg(NV4097_SET_CONTEXT_DMA_ZETA) != CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER;
    return s;
}

Texture State::texture(int i) const {
    Texture t;
    const u32 b = NV4097_SET_TEXTURE_OFFSET + 0x20 * i;
    t.offset = reg(b);
    const u32 f = reg(b + 4);
    t.local = (f & 3) != 2;
    t.cubemap = (f >> 2) & 1;
    t.border = (f >> 3) & 1;
    t.dimension = (f >> 4) & 0xF;
    t.format = (f >> 8) & 0xFF;
    t.mipmaps = f >> 16;
    t.address = reg(b + 8);
    t.control0 = reg(b + 0xC);
    t.control1 = reg(b + 0x10);
    t.filter = reg(b + 0x14);
    const u32 rect = reg(b + 0x18);
    t.width = rect >> 16;
    t.height = rect & 0xFFFF;
    t.border_color = reg(b + 0x1C);
    return t;
}

// ---------------------------------------------------------------- decoder
Decoder::Decoder(Environment env, Backend* backend) : env_(std::move(env)), backend_(backend ? backend : &null_) {}

u32 Decoder::ea(u32 offset, bool local) const { return env_.to_ea ? env_.to_ea(offset, local) : 0; }

void Decoder::method(u32 reg, u32 v) {
    reg &= 0xFFFC;
    s_.regs[reg >> 2] = v;

    // Ranged uploads first.
    if (reg >= NV4097_SET_TRANSFORM_PROGRAM && reg < NV4097_SET_TRANSFORM_PROGRAM + 0x80) {
        s_.vp_ucode[s_.vp_write++ % s_.vp_ucode.size()] = v;
        return;
    }
    if (reg >= NV4097_SET_TRANSFORM_CONSTANT && reg < NV4097_SET_TRANSFORM_CONSTANT + 0x80) {
        s_.vp_const[s_.vp_const_write++ % s_.vp_const.size()] = v;
        return;
    }
    if (vertex_data(reg, v)) return;
    if (reg >= NV308A_COLOR && reg < NV308A_COLOR + 0x700 * 4) {
        nv308a_color((reg - NV308A_COLOR) >> 2, v);
        return;
    }

    switch (reg) {
    // ---- channel / sync
    case NV406E_SET_REFERENCE:
        if (env_.on_reference) env_.on_reference(v);
        break;
    case NV406E_SEMAPHORE_ACQUIRE: {
        const u32 addr = env_.label_base + s_.reg(NV406E_SEMAPHORE_OFFSET);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (mem::r32(addr) != v && env_.running()) {
            if (std::chrono::steady_clock::now() > deadline) {
                LOG_WARN("rsx", "semaphore acquire at 0x%08X (want 0x%08X, have 0x%08X) timed out", addr, v,
                         mem::r32(addr));
                break;
            }
            std::this_thread::yield();
        }
        break;
    }
    case NV406E_SEMAPHORE_RELEASE:
        mem::w32(env_.label_base + s_.reg(NV406E_SEMAPHORE_OFFSET), v);
        break;
    case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE:
        mem::w32(env_.label_base + s_.reg(NV4097_SET_SEMAPHORE_OFFSET),
                 (v & 0xFF00FF00) | ((v & 0xFF) << 16) | ((v >> 16) & 0xFF));
        break;
    case NV4097_TEXTURE_READ_SEMAPHORE_RELEASE:
        mem::w32(env_.label_base + s_.reg(NV4097_SET_SEMAPHORE_OFFSET), v);
        break;
    case NV4097_GET_REPORT:
        get_report(v);
        break;

    // ---- programs
    case NV4097_SET_TRANSFORM_PROGRAM_LOAD:
        s_.vp_write = (v & 511) * 4;
        break;
    case NV4097_SET_TRANSFORM_CONSTANT_LOAD:
        s_.vp_const_write = (v % 468) * 4;
        break;

    // ---- geometry
    case NV4097_SET_BEGIN_END:
        if (v) {
            draw_ = DrawCall{};
            draw_.prim = static_cast<Prim>(v);
            in_draw_ = true;
        } else {
            end_draw();
        }
        break;
    case NV4097_DRAW_ARRAYS:
        draw_.ranges.emplace_back(v & 0xFFFFFF, (v >> 24) + 1);
        break;
    case NV4097_DRAW_INDEX_ARRAY: {
        const u32 dma = s_.reg(NV4097_SET_INDEX_ARRAY_DMA);
        draw_.indexed = true;
        draw_.index16 = ((dma >> 4) & 0xF) == 1;
        draw_.index_local = (dma & 0xF) == 0;
        draw_.index_offset = s_.reg(NV4097_SET_INDEX_ARRAY_ADDRESS);
        draw_.ranges.emplace_back(v & 0xFFFFFF, (v >> 24) + 1);
        break;
    }
    case NV4097_INLINE_ARRAY:
        draw_.inline_vertices.push_back(v);
        break;
    case NV4097_ARRAY_ELEMENT16:
        draw_.immediate_indices.push_back(v & 0xFFFF);
        draw_.immediate_indices.push_back(v >> 16);
        break;
    case NV4097_ARRAY_ELEMENT32:
        draw_.immediate_indices.push_back(v);
        break;
    case NV4097_CLEAR_SURFACE:
        backend_->clear(s_, v);
        break;

    // ---- 2D engines
    case NV0039_BUFFER_NOTIFY:
        nv0039_copy();
        break;
    case NV3089_IMAGE_IN:
        nv3089_blit();
        break;

    // ---- driver commands
    case GCM_FLIP_COMMAND:
        ++flips_;
        backend_->flip(s_, v);
        if (env_.on_flip) env_.on_flip(v);
        break;
    case GCM_SET_USER_COMMAND:
        if (env_.on_user_command) env_.on_user_command(v);
        break;
    default:
        break;
    }
}

namespace {
float bits_f32(u32 v) {
    float f;
    std::memcpy(&f, &v, 4);
    return f;
}
}  // namespace

// SET_VERTEX_DATA{1F,2F,3F,4F,2S,4S,4UB,SCALED4S}_M: immediate attribute values.
bool Decoder::vertex_data(u32 reg, u32 v) {
    auto& r = s_.vertex_regs;
    if (reg >= 0x1C00 && reg < 0x1D00) {  // 4F: 16 x 4 floats
        r[(reg - 0x1C00) / 16][((reg - 0x1C00) / 4) % 4] = bits_f32(v);
    } else if (reg >= 0x1500 && reg < 0x15C0) {  // 3F
        const u32 a = (reg - 0x1500) / 12, c = ((reg - 0x1500) / 4) % 3;
        r[a][c] = bits_f32(v);
        r[a][3] = 1.0f;
    } else if (reg >= 0x1880 && reg < 0x1900) {  // 2F
        const u32 a = (reg - 0x1880) / 8;
        r[a][((reg - 0x1880) / 4) % 2] = bits_f32(v);
        r[a][2] = 0.0f;
        r[a][3] = 1.0f;
    } else if (reg >= 0x1E40 && reg < 0x1E80) {  // 1F
        const u32 a = (reg - 0x1E40) / 4;
        r[a] = {bits_f32(v), 0.0f, 0.0f, 1.0f};
    } else if (reg >= 0x1940 && reg < 0x1980) {  // 4UB (normalised; x in the top byte)
        const u32 a = (reg - 0x1940) / 4;
        for (int c = 0; c < 4; ++c) r[a][c] = static_cast<float>((v >> (24 - 8 * c)) & 0xFF) / 255.0f;
    } else if (reg >= 0x1900 && reg < 0x1940) {  // 2S
        const u32 a = (reg - 0x1900) / 4;
        r[a] = {static_cast<float>(static_cast<s16>(v >> 16)), static_cast<float>(static_cast<s16>(v)), 0.0f, 1.0f};
    } else if ((reg >= 0x1980 && reg < 0x1A00) || (reg >= 0x0A80 && reg < 0x0B00)) {  // 4S / SCALED4S
        const u32 base = reg >= 0x1980 ? 0x1980u : 0x0A80u;
        const u32 a = (reg - base) / 8, half = ((reg - base) / 4) % 2;
        r[a][half * 2] = static_cast<float>(static_cast<s16>(v >> 16));
        r[a][half * 2 + 1] = static_cast<float>(static_cast<s16>(v));
    } else {
        return false;
    }
    return true;
}

void Decoder::end_draw() {
    if (!in_draw_) return;
    in_draw_ = false;
    if (draw_.empty()) return;
    ++draws_;
    backend_->draw(s_, draw_);
}

void Decoder::get_report(u32 v) {
    const u32 type = v >> 24, offset = v & 0xFFFFFF;
    const u32 dma = s_.reg(NV4097_SET_CONTEXT_DMA_REPORT);
    const u32 addr = dma == CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_MAIN ? ea(offset, false)
                                                                      : env_.local_report_base + offset;
    if (!addr) return;
    // CellGcmReportData { u64 timer (ns); u32 value; u32 padding }
    const u64 ns = static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                        std::chrono::steady_clock::now().time_since_epoch())
                                        .count());
    mem::w64(addr, ns);
    // No real occlusion queries yet: report everything as visible so the
    // game never culls on a zero sample count.
    mem::w32(addr + 8, type == 1 /*ZPASS_PIXEL_CNT*/ ? 0xFFFF : 0);
    mem::w32(addr + 12, 0);
}

void Decoder::nv0039_copy() {
    const bool in_local = s_.reg(NV0039_SET_CONTEXT_DMA_BUFFER_IN) != CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER;
    const bool out_local = s_.reg(NV0039_SET_CONTEXT_DMA_BUFFER_OUT) != CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER;
    const u32 src = ea(s_.reg(NV0039_OFFSET_IN), in_local);
    const u32 dst = ea(s_.reg(NV0039_OFFSET_OUT), out_local);
    const s32 pitch_in = static_cast<s32>(s_.reg(NV0039_PITCH_IN));
    const s32 pitch_out = static_cast<s32>(s_.reg(NV0039_PITCH_OUT));
    const u32 len = s_.reg(NV0039_LINE_LENGTH_IN);
    const u32 lines = s_.reg(NV0039_LINE_COUNT);
    const u32 fmt = s_.reg(NV0039_FORMAT);
    const u32 in_inc = std::max<u32>(fmt & 0xFF, 1), out_inc = std::max<u32>(fmt >> 8, 1);
    if (!src || !dst) {
        LOG_WARN("rsx", "NV0039 copy with unmapped address (in 0x%08X out 0x%08X)", s_.reg(NV0039_OFFSET_IN),
                 s_.reg(NV0039_OFFSET_OUT));
        return;
    }
    ++copies_;
    for (u32 y = 0; y < lines; ++y) {
        const u32 si = src + static_cast<u32>(pitch_in * static_cast<s32>(y));
        const u32 di = dst + static_cast<u32>(pitch_out * static_cast<s32>(y));
        if (in_inc == 1 && out_inc == 1) {
            mem::copy(di, si, len);
        } else {
            for (u32 x = 0; x < len; ++x) mem::w8(di + x * out_inc, mem::r8(si + x * in_inc));
        }
    }
}

namespace {
u32 nv3089_bpp(u32 fmt) {
    switch (fmt) {
    case 1: case 2: case 7: return 2;  // A1R5G5B5, X1R5G5B5, R5G6B5
    case 8: return 1;                  // Y8
    default: return 4;
    }
}
u32 nv3062_bpp(u32 fmt) { return fmt == 4 ? 2 : fmt == 1 ? 1 : 4; }  // R5G6B5 / Y8 / 32-bit
}  // namespace

void Decoder::nv3089_blit() {
    const u32 in_fmt = s_.reg(NV3089_IMAGE_IN_FORMAT);
    const u32 in_pitch = in_fmt & 0xFFFF;
    const u32 in_size = s_.reg(NV3089_IMAGE_IN_SIZE);
    const u32 in_w = in_size & 0xFFFF, in_h = in_size >> 16;
    const u32 in_pt = s_.reg(NV3089_IMAGE_IN);
    const u32 in_x = (in_pt & 0xFFFF) >> 4, in_y = (in_pt >> 16) >> 4;  // 12.4 fixed point
    const bool in_local = s_.reg(NV3089_SET_CONTEXT_DMA_IMAGE) != CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER;
    const u32 src = ea(s_.reg(NV3089_IMAGE_IN_OFFSET), in_local);
    const u32 bpp_in = nv3089_bpp(s_.reg(NV3089_SET_COLOR_FORMAT));

    if (s_.reg(NV3089_SET_CONTEXT_SURFACE) == 0x31337A73) {  // NV309E swizzled destination
        static std::atomic<bool> once{false};
        if (!once.exchange(true)) LOG_WARN("rsx", "NV3089 blit to a swizzled surface is not implemented yet");
        return;
    }
    const bool out_local = s_.reg(NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN) != CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER;
    const u32 dst = ea(s_.reg(NV3062_SET_OFFSET_DESTIN), out_local);
    const u32 out_pitch = s_.reg(NV3062_SET_PITCH) >> 16;
    const u32 bpp_out = nv3062_bpp(s_.reg(NV3062_SET_COLOR_FORMAT));
    if (!src || !dst || !in_w || !in_h) return;

    const u32 clip_pt = s_.reg(NV3089_CLIP_POINT), clip_sz = s_.reg(NV3089_CLIP_SIZE);
    const u32 out_pt = s_.reg(NV3089_IMAGE_OUT_POINT), out_sz = s_.reg(NV3089_IMAGE_OUT_SIZE);
    const u32 cx0 = clip_pt & 0xFFFF, cy0 = clip_pt >> 16;
    const u32 cx1 = cx0 + (clip_sz & 0xFFFF), cy1 = cy0 + (clip_sz >> 16);
    const u32 ox0 = out_pt & 0xFFFF, oy0 = out_pt >> 16;
    const u32 ow = out_sz & 0xFFFF, oh = out_sz >> 16;
    const s64 ds_dx = static_cast<s32>(s_.reg(NV3089_DS_DX));
    const s64 dt_dy = static_cast<s32>(s_.reg(NV3089_DT_DY));
    if (bpp_in != bpp_out) {
        static std::atomic<bool> once{false};
        if (!once.exchange(true))
            LOG_WARN("rsx", "NV3089 blit between %u- and %u-byte formats: copying raw", bpp_in, bpp_out);
    }
    const u32 bpp = std::min(bpp_in, bpp_out);
    ++copies_;
    for (u32 y = 0; y < oh; ++y) {
        const u32 dy = oy0 + y;
        if (dy < cy0 || dy >= cy1) continue;
        const u32 sy = std::min<u32>(in_y + static_cast<u32>((static_cast<s64>(y) * dt_dy) >> 20), in_h - 1);
        for (u32 x = 0; x < ow; ++x) {
            const u32 dx = ox0 + x;
            if (dx < cx0 || dx >= cx1) continue;
            const u32 sx = std::min<u32>(in_x + static_cast<u32>((static_cast<s64>(x) * ds_dx) >> 20), in_w - 1);
            mem::copy(dst + dy * out_pitch + dx * bpp_out, src + sy * in_pitch + sx * bpp_in, bpp);
        }
    }
}

void Decoder::nv308a_color(u32 index, u32 v) {
    const u32 pt = s_.reg(NV308A_POINT);
    const u32 x0 = pt & 0xFFFF, y0 = pt >> 16;
    const u32 w = std::max<u32>(s_.reg(NV308A_SIZE_IN) & 0xFFFF, 1);
    const bool out_local = s_.reg(NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN) != CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER;
    const u32 dst = ea(s_.reg(NV3062_SET_OFFSET_DESTIN), out_local);
    const u32 pitch = s_.reg(NV3062_SET_PITCH) >> 16;
    const u32 bpp = nv3062_bpp(s_.reg(NV3062_SET_COLOR_FORMAT));
    if (!dst) return;
    if (bpp == 4) {
        const u32 x = x0 + index % w, y = y0 + index / w;
        mem::w32(dst + y * pitch + x * 4, v);
    } else {
        // 16-bit destinations pack two pixels per word, high half first.
        for (u32 k = 0; k < 2; ++k) {
            const u32 p = index * 2 + k;
            const u32 x = x0 + p % w, y = y0 + p / w;
            mem::w16(dst + y * pitch + x * 2, static_cast<u16>(k == 0 ? v >> 16 : v));
        }
    }
}

// ---------------------------------------------------------------- FIFO
bool Fifo::run_until_idle(const std::function<u32(u32 io)>& io_to_ea) {
    for (;;) {
        const u32 put = mem::r32(control_);
        const u32 get = mem::r32(control_ + 4);
        if (get == put) return true;
        const u32 ea = io_to_ea(get);
        if (!ea) {
            LOG_ERROR("rsx", "FIFO get 0x%08X is not mapped", get);
            return false;
        }
        const u32 cmd = mem::r32(ea);
        ++packets_;
        if ((cmd & 0xE0000003) == 0x20000000) {  // old-style jump
            mem::w32(control_ + 4, cmd & 0x1FFFFFFC);
            continue;
        }
        if ((cmd & 3) == 1) {  // jump
            mem::w32(control_ + 4, cmd & 0xFFFFFFFC);
            continue;
        }
        if ((cmd & 3) == 2) {  // call
            if (sp_ < 16) call_stack_[sp_++] = get + 4;
            else LOG_ERROR("rsx", "FIFO call stack overflow at 0x%08X", get);
            mem::w32(control_ + 4, cmd & 0xFFFFFFFC);
            continue;
        }
        if (cmd == 0x00020000) {  // return
            mem::w32(control_ + 4, sp_ ? call_stack_[--sp_] : get + 4);
            continue;
        }
        const u32 count = (cmd >> 18) & 0x7FF;
        const bool non_inc = (cmd & 0x40000000) != 0;
        const u32 reg = cmd & 0xFFFC;
        for (u32 k = 0; k < count; ++k) d_.method(non_inc ? reg : reg + 4 * k, mem::r32(ea + 4 + 4 * k));
        mem::w32(control_ + 4, get + 4 + 4 * count);
    }
}

}  // namespace opentit::rsx
