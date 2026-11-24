// openTIT - Backend-neutral GPU vocabulary used by the Agl graphics layer.
//
// On the console Agl drove the RSX through libgcm. The enumerations below
// keep the numeric values the console renderer used (they are the standard
// OpenGL token values for comparison/blend/stencil state and the RSX values
// for formats and primitive types), so data tables and serialized material
// state can be passed through unchanged. Backends translate them to their own
// API (see Device.h).
#pragma once

#include <cstddef>
#include <cstdint>

namespace agl::gpu {

// Where a block of GPU-visible memory lives. The console had video memory
// ("local") and IO-mapped system memory ("main"). On PC both are plain host
// memory; the value is kept so backends can choose upload strategies.
enum class MemoryLocation : uint8_t {
    Local = 0,
    Main  = 1,
};

// A reference to GPU-visible memory. On the console this was an offset into
// local or main memory; here it is the CPU address of the block.
struct GpuPtr {
    const void* data = nullptr;
    MemoryLocation location = MemoryLocation::Local;

    bool IsNull() const { return data == nullptr; }
};

enum class CompareFunc : uint16_t {
    Never    = 0x0200,
    Less     = 0x0201,
    Equal    = 0x0202,
    LEqual   = 0x0203,
    Greater  = 0x0204,
    NotEqual = 0x0205,
    GEqual   = 0x0206,
    Always   = 0x0207,
};

enum class BlendFactor : uint16_t {
    Zero                  = 0x0000,
    One                   = 0x0001,
    SrcColor              = 0x0300,
    OneMinusSrcColor      = 0x0301,
    SrcAlpha              = 0x0302,
    OneMinusSrcAlpha      = 0x0303,
    DstAlpha              = 0x0304,
    OneMinusDstAlpha      = 0x0305,
    DstColor              = 0x0306,
    OneMinusDstColor      = 0x0307,
    SrcAlphaSaturate      = 0x0308,
    ConstantColor         = 0x8001,
    OneMinusConstantColor = 0x8002,
    ConstantAlpha         = 0x8003,
    OneMinusConstantAlpha = 0x8004,
};

enum class BlendEquation : uint16_t {
    Add             = 0x8006,
    Min             = 0x8007,
    Max             = 0x8008,
    Subtract        = 0x800A,
    ReverseSubtract = 0x800B,
};

enum class StencilOp : uint16_t {
    Zero     = 0x0000,
    Keep     = 0x1E00,
    Replace  = 0x1E01,
    Incr     = 0x1E02,
    Decr     = 0x1E03,
    Invert   = 0x150A,
    IncrWrap = 0x8507,
    DecrWrap = 0x8508,
};

enum class CullFace : uint16_t {
    Front        = 0x0404,
    Back         = 0x0405,
    FrontAndBack = 0x0408,
};

enum class FrontFace : uint16_t {
    CW  = 0x0900,
    CCW = 0x0901,
};

enum class PrimitiveType : uint8_t {
    Points        = 1,
    Lines         = 2,
    LineLoop      = 3,
    LineStrip     = 4,
    Triangles     = 5,
    TriangleStrip = 6,
    TriangleFan   = 7,
    Quads         = 8,
    QuadStrip     = 9,
    Polygon       = 10,
};

enum class IndexType : uint8_t {
    U32 = 0,
    U16 = 1,
};

// Component type of a vertex attribute stream.
enum class VertexAttribType : uint8_t {
    ShortNorm  = 1,   // signed 16-bit, normalized
    Float      = 2,
    Half       = 3,
    UByteNorm  = 4,   // unsigned 8-bit, normalized
    Short      = 5,   // signed 16-bit, not normalized
    Packed111110 = 6, // 11:11:10 signed normalized
    UByte      = 7,   // unsigned 8-bit, not normalized
};

constexpr int kMaxVertexAttributes = 16;
constexpr int kMaxTextureUnits = 16;
constexpr int kMaxColorTargets = 4;
constexpr int kMaxVertexConstants = 468;   // 4-float registers
constexpr int kMaxReportSlots = 2048;
constexpr int kMaxLabels = 256;
constexpr int kMaxUserClipPlanes = 6;

// Surface clear mask.
enum ClearMask : uint32_t {
    kClear_Depth   = 0x01,
    kClear_Stencil = 0x02,
    kClear_R       = 0x10,
    kClear_G       = 0x20,
    kClear_B       = 0x40,
    kClear_A       = 0x80,
    kClear_Color   = kClear_R | kClear_G | kClear_B | kClear_A,
    kClear_All     = kClear_Color | kClear_Depth | kClear_Stencil,
};

// Color write mask (SetColorMask), same bit layout as the console:
// A in bits 24..31, R in 16..23, G in 8..15, B in 0..7.
enum ColorMask : uint32_t {
    kColorMask_B   = 0x00000001,
    kColorMask_G   = 0x00000100,
    kColorMask_R   = 0x00010000,
    kColorMask_A   = 0x01000000,
    kColorMask_RGB = kColorMask_R | kColorMask_G | kColorMask_B,
    kColorMask_All = kColorMask_RGB | kColorMask_A,
};

// --- textures ------------------------------------------------------------

// Texel formats (RSX values, without the linear/normalized flag bits).
enum class TextureFormat : uint8_t {
    B8         = 0x81,
    A1R5G5B5   = 0x82,
    A4R4G4B4   = 0x83,
    R5G6B5     = 0x84,
    A8R8G8B8   = 0x85,
    DXT1       = 0x86,
    DXT3       = 0x87,
    DXT5       = 0x88,
    G8B8       = 0x8B,
    Depth24S8  = 0x90,   // DEPTH24_D8
    Depth24S8F = 0x91,   // DEPTH24_D8_FLOAT
    Depth16    = 0x92,
    Depth16F   = 0x93,
    X16        = 0x94,
    Y16X16     = 0x95,
    R5G5B5A1   = 0x97,
    W16Z16Y16X16F = 0x9A,
    W32Z32Y32X32F = 0x9B,
    X32F       = 0x9C,
    D1R5G5B5   = 0x9D,
    D8R8G8B8   = 0x9E,
    Y16X16F    = 0x9F,
};

// Flag bits that may be OR-ed into the stored format byte.
enum TextureFormatFlags : uint8_t {
    kTexFmt_Linear     = 0x20,   // pitch-linear instead of swizzled
    kTexFmt_Unnormalized = 0x40, // texel (not 0..1) coordinates
    kTexFmt_FormatMask = 0x9F,
};

enum class TextureDimension : uint8_t {
    Tex1D = 1,
    Tex2D = 2,
    Tex3D = 3,
};

enum class TextureWrap : uint8_t {
    Wrap                  = 1,
    Mirror                = 2,
    ClampToEdge           = 3,
    Border                = 4,
    Clamp                 = 5,
    MirrorOnceClampToEdge = 6,
    MirrorOnceBorder      = 7,
    MirrorOnceClamp       = 8,
};

enum class TextureMinFilter : uint8_t {
    Nearest              = 1,
    Linear               = 2,
    NearestMipmapNearest = 3,
    LinearMipmapNearest  = 4,
    NearestMipmapLinear  = 5,
    LinearMipmapLinear   = 6,
    Convolution          = 7,
};

enum class TextureMagFilter : uint8_t {
    Nearest     = 1,
    Linear      = 2,
    Convolution = 4,
};

enum class TextureCacheKind : uint8_t {
    Fragment = 1,
    Vertex   = 2,
};

// Everything that describes one texture image (format, layout, memory).
struct TextureDesc {
    uint8_t format = 0;         // TextureFormat | TextureFormatFlags
    uint8_t mipmaps = 1;
    TextureDimension dimension = TextureDimension::Tex2D;
    bool cubemap = false;
    uint32_t remap = 0;         // component swizzle (console encoding)
    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t depth = 1;
    uint32_t pitch = 0;         // bytes per row for linear formats, 0 for swizzled
    GpuPtr memory;
};

// Per-unit sampling state.
struct SamplerControl {
    bool enable = false;
    uint16_t minLod = 0;        // 4.8 fixed point
    uint16_t maxLod = 0;        // 4.8 fixed point
    uint8_t maxAniso = 0;       // 0 = 1x ... 7 = 16x
};

struct SamplerAddress {
    TextureWrap wrapS = TextureWrap::Wrap;
    TextureWrap wrapT = TextureWrap::Wrap;
    TextureWrap wrapR = TextureWrap::Wrap;
    uint8_t unsignedRemap = 0;
    uint8_t depthCompare = 0;   // depth texture compare function (0..7)
    uint8_t gamma = 0;          // per-component sRGB decode mask
};

struct SamplerFilter {
    uint16_t lodBias = 0;       // signed 1.4.8 fixed point (13 bits)
    TextureMinFilter min = TextureMinFilter::Linear;
    TextureMagFilter mag = TextureMagFilter::Linear;
    uint8_t convolution = 1;    // convolution kernel (quincunx etc.)
};

// --- render targets ------------------------------------------------------

enum class SurfaceColorFormat : uint8_t {
    X1R5G5B5_Z1R5G5B5 = 1,
    X1R5G5B5_O1R5G5B5 = 2,
    R5G6B5            = 3,
    X8R8G8B8_Z8R8G8B8 = 4,
    X8R8G8B8_O8R8G8B8 = 5,
    A8R8G8B8          = 8,
    B8                = 9,
    G8B8              = 10,
    F_W16Z16Y16X16    = 11,
    F_W32Z32Y32X32    = 12,
    F_X32             = 13,
    X8B8G8R8_Z8B8G8R8 = 14,
    X8B8G8R8_O8B8G8R8 = 15,
    A8B8G8R8          = 16,
};

enum class SurfaceDepthFormat : uint8_t {
    Z16   = 1,
    Z24S8 = 2,
};

enum class SurfaceAntialias : uint8_t {
    Center1      = 0,
    Diagonal2    = 3,
    SquareCentered4 = 4,
    SquareRotated4  = 5,
};

// Which color targets are written (bit mask over kMaxColorTargets).
enum SurfaceTargetMask : uint8_t {
    kTarget_None = 0,
    kTarget_0 = 1,
    kTarget_1 = 2,
    kTarget_2 = 4,
    kTarget_3 = 8,
};

struct SurfaceDesc {
    SurfaceColorFormat colorFormat = SurfaceColorFormat::A8R8G8B8;
    uint8_t colorTargets = kTarget_0;      // SurfaceTargetMask
    GpuPtr color[kMaxColorTargets];
    uint32_t colorPitch[kMaxColorTargets] = {};
    SurfaceDepthFormat depthFormat = SurfaceDepthFormat::Z24S8;
    GpuPtr depth;
    uint32_t depthPitch = 0;
    bool swizzled = false;
    uint8_t widthLog2 = 0;                 // swizzled surfaces only
    uint8_t heightLog2 = 0;
    SurfaceAntialias antialias = SurfaceAntialias::Center1;
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t width = 0;
    uint16_t height = 0;
};

// --- programs ------------------------------------------------------------

// A vertex program ready for the GPU. 'ucode' points at instructionCount
// 16-byte instructions in the console microcode format.
struct VertexProgramDesc {
    const uint32_t* ucode = nullptr;
    uint32_t instructionCount = 0;
    uint32_t instructionSlot = 0;
    uint32_t registerCount = 0;
    uint32_t inputMask = 0;     // vertex attributes read
    uint32_t outputMask = 0;    // interpolants written
};

// A fragment program image in GPU-visible memory.
struct FragmentProgramDesc {
    GpuPtr ucode;
    uint32_t ucodeSize = 0;
    uint32_t inputMask = 0;          // interpolants read
    uint16_t texCoordsInputMask = 0;
    uint16_t texCoords2D = 0;
    uint16_t texCoordsCentroid = 0;
    uint8_t registerCount = 0;
    bool outputFromH0 = false;       // half precision output
    bool depthReplace = false;
    bool pixelKill = false;
};

// --- miscellaneous -------------------------------------------------------

enum class UserClipMode : uint8_t {
    Disable     = 0,
    LessThan    = 1,
    GreaterEqual = 2,
};

enum class ReportType : uint8_t {
    ZPassPixelCount = 1,
    ZCullStats      = 2,
    ZCullStats1     = 3,
    ZCullStats2     = 4,
    ZCullStats3     = 5,
};

struct Viewport {
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    float zMin = 0.0f;
    float zMax = 1.0f;
    float scale[4] = {};
    float offset[4] = {};
};

} // namespace agl::gpu
