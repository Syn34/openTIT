// RSX command processing: FIFO walker, method decoder and state tracker.
//
// The RSX is driven by a command buffer ("FIFO") in IO-mapped main memory.
// Each packet is a header word followed by arguments. The header names a
// method register; the low 16 bits of its offset include the subchannel, so
// 0x0000-0x1FFF is the 3D engine (NV4097, plus NV406E channel methods),
// 0x2000 the memory-to-memory copy engine (NV0039), 0x6000 the 2D surface
// (NV3062), 0xA000 image-from-CPU (NV308A), 0xC000 the scaled image blitter
// (NV3089) and 0xE000+ the driver's own commands (flip, user command).
//
// The decoder keeps every register in a flat file (State::regs) and acts on
// the methods with side effects: semaphores and labels, reports, program and
// constant uploads, draws, clears, copies, and flips. Rendering itself is a
// Backend's job; the default backend does nothing, so the decoder alone is
// the "null RSX" the game has run on so far.
#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "common/types.h"

namespace opentit::rsx {

// ---------------------------------------------------------------- registers
enum : u32 {
    NV406E_SET_REFERENCE = 0x0050,
    NV406E_SEMAPHORE_OFFSET = 0x0064,
    NV406E_SEMAPHORE_ACQUIRE = 0x0068,
    NV406E_SEMAPHORE_RELEASE = 0x006C,

    NV4097_SET_CONTEXT_DMA_COLOR_B = 0x018C,
    NV4097_SET_CONTEXT_DMA_COLOR_A = 0x0194,
    NV4097_SET_CONTEXT_DMA_ZETA = 0x0198,
    NV4097_SET_CONTEXT_DMA_REPORT = 0x01A8,
    NV4097_SET_CONTEXT_DMA_COLOR_C = 0x01B4,
    NV4097_SET_CONTEXT_DMA_COLOR_D = 0x01B8,
    NV4097_SET_SURFACE_CLIP_HORIZONTAL = 0x0200,
    NV4097_SET_SURFACE_CLIP_VERTICAL = 0x0204,
    NV4097_SET_SURFACE_FORMAT = 0x0208,
    NV4097_SET_SURFACE_PITCH_A = 0x020C,
    NV4097_SET_SURFACE_COLOR_AOFFSET = 0x0210,
    NV4097_SET_SURFACE_ZETA_OFFSET = 0x0214,
    NV4097_SET_SURFACE_COLOR_BOFFSET = 0x0218,
    NV4097_SET_SURFACE_PITCH_B = 0x021C,
    NV4097_SET_SURFACE_COLOR_TARGET = 0x0220,
    NV4097_SET_SURFACE_PITCH_Z = 0x022C,
    NV4097_SET_SURFACE_PITCH_C = 0x0280,
    NV4097_SET_SURFACE_PITCH_D = 0x0284,
    NV4097_SET_SURFACE_COLOR_COFFSET = 0x0288,
    NV4097_SET_SURFACE_COLOR_DOFFSET = 0x028C,
    NV4097_SET_SHADER_PROGRAM = 0x08E4,
    NV4097_SET_VIEWPORT_HORIZONTAL = 0x0A00,
    NV4097_SET_VIEWPORT_VERTICAL = 0x0A04,
    NV4097_SET_TRANSFORM_PROGRAM = 0x0B80,  // 32 words
    NV4097_SET_VERTEX_DATA_ARRAY_OFFSET = 0x1680,  // 16 words
    NV4097_SET_VERTEX_DATA_BASE_OFFSET = 0x1738,
    NV4097_SET_VERTEX_DATA_BASE_INDEX = 0x173C,
    NV4097_SET_VERTEX_DATA_ARRAY_FORMAT = 0x1740,  // 16 words
    NV4097_CLEAR_REPORT_VALUE = 0x17C8,
    NV4097_GET_REPORT = 0x1800,
    NV4097_SET_BEGIN_END = 0x1808,
    NV4097_ARRAY_ELEMENT16 = 0x180C,
    NV4097_ARRAY_ELEMENT32 = 0x1810,
    NV4097_DRAW_ARRAYS = 0x1814,
    NV4097_INLINE_ARRAY = 0x1818,
    NV4097_SET_INDEX_ARRAY_ADDRESS = 0x181C,
    NV4097_SET_INDEX_ARRAY_DMA = 0x1820,
    NV4097_DRAW_INDEX_ARRAY = 0x1824,
    NV4097_SET_TEXTURE_OFFSET = 0x1A00,  // 16 textures x 0x20
    NV4097_SET_SEMAPHORE_OFFSET = 0x1D6C,
    NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE = 0x1D70,
    NV4097_TEXTURE_READ_SEMAPHORE_RELEASE = 0x1D74,
    NV4097_SET_ZSTENCIL_CLEAR_VALUE = 0x1D8C,
    NV4097_SET_COLOR_CLEAR_VALUE = 0x1D90,
    NV4097_CLEAR_SURFACE = 0x1D94,
    NV4097_SET_TRANSFORM_PROGRAM_LOAD = 0x1E9C,
    NV4097_SET_TRANSFORM_PROGRAM_START = 0x1EA0,
    NV4097_SET_TRANSFORM_CONSTANT_LOAD = 0x1EFC,
    NV4097_SET_TRANSFORM_CONSTANT = 0x1F00,  // 32 words

    NV0039_SET_CONTEXT_DMA_BUFFER_IN = 0x2184,
    NV0039_SET_CONTEXT_DMA_BUFFER_OUT = 0x2188,
    NV0039_OFFSET_IN = 0x230C,
    NV0039_OFFSET_OUT = 0x2310,
    NV0039_PITCH_IN = 0x2314,
    NV0039_PITCH_OUT = 0x2318,
    NV0039_LINE_LENGTH_IN = 0x231C,
    NV0039_LINE_COUNT = 0x2320,
    NV0039_FORMAT = 0x2324,
    NV0039_BUFFER_NOTIFY = 0x2328,

    NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN = 0x6188,
    NV3062_SET_COLOR_FORMAT = 0x6300,
    NV3062_SET_PITCH = 0x6304,
    NV3062_SET_OFFSET_DESTIN = 0x630C,

    NV308A_POINT = 0xA304,
    NV308A_SIZE_OUT = 0xA308,
    NV308A_SIZE_IN = 0xA30C,
    NV308A_COLOR = 0xA400,  // up to 0x700 words of inline pixels

    NV3089_SET_CONTEXT_DMA_IMAGE = 0xC184,
    NV3089_SET_CONTEXT_SURFACE = 0xC198,
    NV3089_SET_COLOR_FORMAT = 0xC300,
    NV3089_CLIP_POINT = 0xC308,
    NV3089_CLIP_SIZE = 0xC30C,
    NV3089_IMAGE_OUT_POINT = 0xC310,
    NV3089_IMAGE_OUT_SIZE = 0xC314,
    NV3089_DS_DX = 0xC318,
    NV3089_DT_DY = 0xC31C,
    NV3089_IMAGE_IN_SIZE = 0xC400,
    NV3089_IMAGE_IN_FORMAT = 0xC404,
    NV3089_IMAGE_IN_OFFSET = 0xC408,
    NV3089_IMAGE_IN = 0xC40C,

    GCM_SET_USER_COMMAND = 0xEB00,
    GCM_FLIP_COMMAND = 0xFEAC,
};

// DMA object handles used by libgcm
enum : u32 {
    CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER = 0xFEED0000,  // RSX local memory
    CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER = 0xFEED0001,   // IO-mapped main memory
    CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_LOCAL = 0x66626660,
    CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_MAIN = 0xBAD68000,
};

const char* method_name(u32 reg);  // nullptr if unnamed
std::string describe_method(u32 reg);  // "NAME", "NAME+i" for array registers, or "0xXXXX"

// ---------------------------------------------------------------- decoded state
enum class Prim : u32 {
    None = 0, Points, Lines, LineLoop, LineStrip, Triangles, TriangleStrip, TriangleFan, Quads, QuadStrip, Polygon,
};

struct VertexArray {
    u32 offset = 0;
    bool local = true;
    u32 type = 0;    // CELL_GCM_VERTEX_* (1 S1, 2 F, 3 SF, 4 UB, 5 S32K, 6 CMP, 7 UB256)
    u32 size = 0;    // components, 0 = disabled
    u32 stride = 0;
    u32 frequency = 0;
    bool enabled() const { return size != 0; }
};

struct Surface {
    u32 color_format = 0, depth_format = 0, type = 0, antialias = 0;
    u32 log2_width = 0, log2_height = 0;
    u32 x = 0, y = 0, width = 0, height = 0;  // clip rectangle
    u32 target = 0;                           // CELL_GCM_SURFACE_TARGET_*
    u32 color_offset[4] = {}, color_pitch[4] = {};
    bool color_local[4] = {true, true, true, true};
    u32 zeta_offset = 0, zeta_pitch = 0;
    bool zeta_local = true;
};

struct Texture {
    u32 offset = 0;
    bool local = true;
    bool cubemap = false;
    u32 border = 0, dimension = 0, format = 0, mipmaps = 0;
    u32 width = 0, height = 0;
    u32 address = 0, control0 = 0, control1 = 0, filter = 0, border_color = 0;
    bool enabled() const { return (control0 >> 31) != 0; }
};

struct State {
    std::array<u32, 0x10000 / 4> regs{};
    std::array<u32, 512 * 4> vp_ucode{};  // vertex program: 512 x 128-bit instructions
    std::array<u32, 468 * 4> vp_const{};  // vertex constants: 468 x vec4 (float bits)
    u32 vp_write = 0;                     // word index of the next TRANSFORM_PROGRAM write
    u32 vp_const_write = 0;               // word index of the next TRANSFORM_CONSTANT write

    u32 reg(u32 method) const { return regs[(method & 0xFFFC) >> 2]; }

    VertexArray vertex_array(int i) const;
    Surface surface() const;
    Texture texture(int i) const;
    u32 vp_start() const { return reg(NV4097_SET_TRANSFORM_PROGRAM_START); }
    u32 fp_offset() const { return reg(NV4097_SET_SHADER_PROGRAM) & ~3u; }
    bool fp_local() const { return (reg(NV4097_SET_SHADER_PROGRAM) & 3) == 1; }
};

struct DrawCall {
    Prim prim = Prim::None;
    bool indexed = false;
    bool index16 = false;     // else 32-bit indices
    u32 index_offset = 0;
    bool index_local = true;
    std::vector<std::pair<u32, u32>> ranges;  // (first, count)
    std::vector<u32> inline_vertices;          // INLINE_ARRAY words
    std::vector<u32> immediate_indices;        // ARRAY_ELEMENT16/32
    bool empty() const { return ranges.empty() && inline_vertices.empty() && immediate_indices.empty(); }
};

// What a renderer implements. Defaults do nothing (null RSX).
class Backend {
public:
    virtual ~Backend() = default;
    virtual void draw(const State&, const DrawCall&) {}
    virtual void clear(const State&, u32 /*mask*/) {}
    virtual void flip(const State&, u32 /*buffer*/) {}
};

// Where the decoder reads and writes guest memory.
struct Environment {
    // RSX offset -> guest EA (local: RSX local memory, else IO space). 0 if unmapped.
    std::function<u32(u32 offset, bool local)> to_ea;
    u32 label_base = 0;          // 256 labels x 16 bytes
    u32 local_report_base = 0;   // CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_LOCAL area
    std::function<void(u32)> on_reference;   // NV406E_SET_REFERENCE
    std::function<void(u32)> on_flip;        // GCM_FLIP_COMMAND (buffer id)
    std::function<void(u32)> on_user_command;
    std::function<bool()> running = [] { return true; };
};

class Decoder {
public:
    Decoder(Environment env, Backend* backend = nullptr);

    // Execute one method write. `reg` is the byte offset including subchannel.
    void method(u32 reg, u32 value);

    State& state() { return s_; }
    const State& state() const { return s_; }
    void set_backend(Backend* b) { backend_ = b ? b : &null_; }

    u64 draw_count() const { return draws_; }
    u64 flip_count() const { return flips_; }
    u64 copy_count() const { return copies_; }

private:
    void end_draw();
    void nv0039_copy();
    void nv3089_blit();
    void nv308a_color(u32 index, u32 value);
    void get_report(u32 value);
    u32 ea(u32 offset, bool local) const;

    Environment env_;
    Backend null_;
    Backend* backend_;
    State s_;
    DrawCall draw_;
    bool in_draw_ = false;
    u64 draws_ = 0, flips_ = 0, copies_ = 0;
};

// Walks a command buffer through the CellGcmControl registers {put, get, ref}
// at `control` (guest EA), following jumps, calls and returns. Offsets in
// the FIFO are IO offsets.
class Fifo {
public:
    Fifo(Decoder& d, u32 control) : d_(d), control_(control) {}
    // Process packets until get == put. Returns false if the FIFO pointed at
    // unmapped memory (the walker cannot continue).
    bool run_until_idle(const std::function<u32(u32 io)>& io_to_ea);
    u64 packets() const { return packets_; }

private:
    Decoder& d_;
    u32 control_;
    u32 call_stack_[16] = {};
    int sp_ = 0;
    u64 packets_ = 0;
};

}  // namespace opentit::rsx
