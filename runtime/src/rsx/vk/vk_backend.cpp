#include "vk_backend.h"

#ifndef OPENTIT_HAS_VULKAN

namespace opentit::rsx {
bool have_vulkan() { return false; }
std::unique_ptr<VulkanBackend> create_vulkan_backend(std::function<u32(u32, bool)>, std::string* error) {
    if (error) *error = "openTIT was built without Vulkan";
    return nullptr;
}
}  // namespace opentit::rsx

#else

#include <vulkan/vulkan.h>

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <unordered_map>
#include <vector>

#include "../../log.h"
#include "../fragment_program.h"
#include "../geometry.h"
#include "../glsl.h"
#include "../shader_dump.h"
#include "../texture.h"
#include "../vertex_program.h"
#include "opentit/memory.h"

namespace opentit::rsx {

bool have_vulkan() { return true; }

namespace {

// ---------------------------------------------------------------- RSX register offsets used here
enum : u32 {
    R_ALPHA_TEST_ENABLE = 0x0304, R_ALPHA_FUNC = 0x0308, R_ALPHA_REF = 0x030C,
    R_BLEND_ENABLE = 0x0310, R_BLEND_SFACTOR = 0x0314, R_BLEND_DFACTOR = 0x0318, R_BLEND_COLOR = 0x031C,
    R_BLEND_EQUATION = 0x0320, R_COLOR_MASK = 0x0324,
    R_STENCIL_ENABLE = 0x0328, R_STENCIL_MASK = 0x032C, R_STENCIL_FUNC = 0x0330, R_STENCIL_REF = 0x0334,
    R_STENCIL_FUNC_MASK = 0x0338, R_STENCIL_FAIL = 0x033C, R_STENCIL_ZFAIL = 0x0340, R_STENCIL_ZPASS = 0x0344,
    R_TWO_SIDED_STENCIL = 0x0348, R_BACK_STENCIL_MASK = 0x034C, R_BACK_STENCIL_FUNC = 0x0350,
    R_BACK_STENCIL_REF = 0x0354, R_BACK_STENCIL_FUNC_MASK = 0x0358, R_BACK_STENCIL_FAIL = 0x035C,
    R_BACK_STENCIL_ZFAIL = 0x0360, R_BACK_STENCIL_ZPASS = 0x0364, R_BLEND_ENABLE_MRT = 0x036C,
    R_CLIP_MIN = 0x0394, R_CLIP_MAX = 0x0398,
    R_SCISSOR_H = 0x08C0, R_SCISSOR_V = 0x08C4,
    R_VIEWPORT_OFFSET = 0x0A20, R_VIEWPORT_SCALE = 0x0A30,
    R_DEPTH_FUNC = 0x0A6C, R_DEPTH_MASK = 0x0A70, R_DEPTH_TEST = 0x0A74,
    R_CULL_FACE = 0x1830, R_FRONT_FACE = 0x1834, R_CULL_ENABLE = 0x183C,
    R_TEXTURE_CONTROL3 = 0x1840,
    R_SHADER_CONTROL = 0x1D60,
    R_SHADER_WINDOW = 0x1D88,
    R_TWO_SIDE_LIGHT = 0x142C,
    R_TRANSFORM_BRANCH_BITS = 0x1FF8,
};

float f32_of(u32 v) {
    float f;
    std::memcpy(&f, &v, 4);
    return f;
}

VkCompareOp compare_op(u32 f) {
    switch (f & 0xF) {
    case 0: return VK_COMPARE_OP_NEVER;
    case 1: return VK_COMPARE_OP_LESS;
    case 2: return VK_COMPARE_OP_EQUAL;
    case 3: return VK_COMPARE_OP_LESS_OR_EQUAL;
    case 4: return VK_COMPARE_OP_GREATER;
    case 5: return VK_COMPARE_OP_NOT_EQUAL;
    case 6: return VK_COMPARE_OP_GREATER_OR_EQUAL;
    default: return VK_COMPARE_OP_ALWAYS;
    }
}

VkBlendFactor blend_factor(u32 f) {
    switch (f) {
    case 0: return VK_BLEND_FACTOR_ZERO;
    case 1: return VK_BLEND_FACTOR_ONE;
    case 0x300: return VK_BLEND_FACTOR_SRC_COLOR;
    case 0x301: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case 0x302: return VK_BLEND_FACTOR_SRC_ALPHA;
    case 0x303: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case 0x304: return VK_BLEND_FACTOR_DST_ALPHA;
    case 0x305: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case 0x306: return VK_BLEND_FACTOR_DST_COLOR;
    case 0x307: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case 0x308: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
    case 0x8001: return VK_BLEND_FACTOR_CONSTANT_COLOR;
    case 0x8002: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
    case 0x8003: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
    case 0x8004: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
    default: return VK_BLEND_FACTOR_ONE;
    }
}

VkBlendOp blend_op(u32 e) {
    switch (e) {
    case 0x8007: return VK_BLEND_OP_MIN;
    case 0x8008: return VK_BLEND_OP_MAX;
    case 0x800A: case 0xF005: return VK_BLEND_OP_SUBTRACT;
    case 0x800B: case 0xF006: return VK_BLEND_OP_REVERSE_SUBTRACT;
    default: return VK_BLEND_OP_ADD;
    }
}

VkStencilOp stencil_op(u32 o) {
    switch (o) {
    case 0: return VK_STENCIL_OP_ZERO;
    case 0x1E01: return VK_STENCIL_OP_REPLACE;
    case 0x1E02: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    case 0x1E03: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    case 0x150A: return VK_STENCIL_OP_INVERT;
    case 0x8507: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
    case 0x8508: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    default: return VK_STENCIL_OP_KEEP;
    }
}

VkSamplerAddressMode address_mode(u32 m) {
    switch (m) {
    case 1: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case 2: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case 4: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case 6: case 7: case 8: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    default: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }
}

VkPrimitiveTopology vk_topology(Topology t) {
    switch (t) {
    case Topology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case Topology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case Topology::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case Topology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case Topology::TriangleFan: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

// ---------------------------------------------------------------- surface formats
struct ColorFormat {
    VkFormat vk;
    u32 bytes;  // guest bytes per pixel
};

ColorFormat color_format(u32 rsx) {
    switch (rsx) {
    case 1: case 2: case 3: return {VK_FORMAT_R8G8B8A8_UNORM, 2};
    case 9: return {VK_FORMAT_R8_UNORM, 1};
    case 10: return {VK_FORMAT_R8G8_UNORM, 2};
    case 11: return {VK_FORMAT_R16G16B16A16_SFLOAT, 8};
    case 12: return {VK_FORMAT_R32G32B32A32_SFLOAT, 16};
    case 13: return {VK_FORMAT_R32_SFLOAT, 4};
    default: return {VK_FORMAT_R8G8B8A8_UNORM, 4};
    }
}

u32 vk_texel_bytes(VkFormat f) {
    switch (f) {
    case VK_FORMAT_R8_UNORM: return 1;
    case VK_FORMAT_R8G8_UNORM: return 2;
    case VK_FORMAT_R16G16B16A16_SFLOAT: return 8;
    case VK_FORMAT_R32G32B32A32_SFLOAT: return 16;
    default: return 4;
    }
}

// One pixel: Vulkan image bytes (host order) <-> guest bytes (big endian, RSX layout).
void pixel_to_guest(u32 rsx, const u8* v, u8* g) {
    switch (rsx) {
    case 1: case 2: {  // X1R5G5B5
        const u16 p = static_cast<u16>((rsx == 2 ? 0x8000 : 0) | ((v[0] >> 3) << 10) | ((v[1] >> 3) << 5) | (v[2] >> 3));
        store_be<u16>(g, p);
        return;
    }
    case 3: store_be<u16>(g, static_cast<u16>(((v[0] >> 3) << 11) | ((v[1] >> 2) << 5) | (v[2] >> 3))); return;
    case 9: g[0] = v[0]; return;
    case 10: g[0] = v[1]; g[1] = v[0]; return;
    case 11: for (int c = 0; c < 4; ++c) { u16 h; std::memcpy(&h, v + 2 * c, 2); store_be<u16>(g + 2 * c, h); } return;
    case 12: for (int c = 0; c < 4; ++c) { u32 w; std::memcpy(&w, v + 4 * c, 4); store_be<u32>(g + 4 * c, w); } return;
    case 13: { u32 w; std::memcpy(&w, v, 4); store_be<u32>(g, w); return; }
    case 14: case 15: case 16:  // xBGR
        g[0] = rsx == 14 ? 0 : rsx == 15 ? 255 : v[3]; g[1] = v[2]; g[2] = v[1]; g[3] = v[0];
        return;
    default:  // 4, 5, 8: xRGB
        g[0] = rsx == 4 ? 0 : rsx == 5 ? 255 : v[3]; g[1] = v[0]; g[2] = v[1]; g[3] = v[2];
        return;
    }
}

void pixel_from_guest(u32 rsx, const u8* g, u8* v) {
    switch (rsx) {
    case 1: case 2: {
        const u16 p = load_be<u16>(g);
        v[0] = static_cast<u8>(((p >> 10) & 31) * 255 / 31); v[1] = static_cast<u8>(((p >> 5) & 31) * 255 / 31);
        v[2] = static_cast<u8>((p & 31) * 255 / 31); v[3] = 255;
        return;
    }
    case 3: {
        const u16 p = load_be<u16>(g);
        v[0] = static_cast<u8>((p >> 11) * 255 / 31); v[1] = static_cast<u8>(((p >> 5) & 63) * 255 / 63);
        v[2] = static_cast<u8>((p & 31) * 255 / 31); v[3] = 255;
        return;
    }
    case 9: v[0] = g[0]; return;
    case 10: v[0] = g[1]; v[1] = g[0]; return;
    case 11: for (int c = 0; c < 4; ++c) { const u16 h = load_be<u16>(g + 2 * c); std::memcpy(v + 2 * c, &h, 2); } return;
    case 12: for (int c = 0; c < 4; ++c) { const u32 w = load_be<u32>(g + 4 * c); std::memcpy(v + 4 * c, &w, 4); } return;
    case 13: { const u32 w = load_be<u32>(g); std::memcpy(v, &w, 4); return; }
    case 14: case 15: case 16: v[0] = g[3]; v[1] = g[2]; v[2] = g[1]; v[3] = rsx == 16 ? g[0] : 255; return;
    default: v[0] = g[1]; v[1] = g[2]; v[2] = g[3]; v[3] = rsx == 8 ? g[0] : 255; return;
    }
}

// 64-bit multiply/xorshift hash over 8-byte words: ~10x faster than byte FNV.
u64 fast_hash(const u8* p, std::size_t n) {
    u64 h = 0x9E3779B97F4A7C15ull ^ n;
    std::size_t k = 0;
    for (; k + 8 <= n; k += 8) {
        u64 w;
        std::memcpy(&w, p + k, 8);
        h = (h ^ w) * 0xFF51AFD7ED558CCDull;
        h ^= h >> 32;
    }
    for (; k < n; ++k) h = (h ^ p[k]) * 0x100000001B3ull;
    return h;
}

u32 log2u(u32 v) {
    u32 r = 0;
    while ((1u << r) < v) ++r;
    return r;
}

#define VKCHECK(expr)                                                              \
    do {                                                                           \
        const VkResult r_ = (expr);                                                \
        if (r_ != VK_SUCCESS) LOG_ERROR("vk", "%s failed: %d", #expr, (int)r_);    \
    } while (0)

// ================================================================ backend
class Impl final : public VulkanBackend {
public:
    explicit Impl(std::function<u32(u32, bool)> to_ea) : to_ea_(std::move(to_ea)) {}
    ~Impl() override;

    bool init(std::string* error);

    void draw(const State& s, const DrawCall& c) override;
    void clear(const State& s, u32 mask) override;
    void flip(const State& s, u32 buffer) override;
    void sync() override { flush(); }
    void invalidate(u32 ea, u32 size) override;
    const VulkanStats& stats() const override { return stats_; }
    std::string device_name() const override { return device_name_; }

private:
    struct Buffer {
        VkBuffer buf = VK_NULL_HANDLE;
        VkDeviceMemory mem = VK_NULL_HANDLE;
        u8* map = nullptr;
        VkDeviceSize size = 0;
    };
    struct Image {
        VkImage img = VK_NULL_HANDLE;
        VkDeviceMemory mem = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        u32 width = 0, height = 0;
    };
    struct Target {
        Image image;
        u32 ea = 0, pitch = 0, rsx_format = 0;
        bool depth = false, swizzled = false;
        bool loaded = false;  // GPU contents valid
        std::map<u32, VkImageView> sample_views;  // by TEXTURE_CONTROL1 remap (depth: 0x10000 | kind)
        bool dirty = false;   // GPU contents newer than guest memory
        u64 last_used = 0;
        u32 bytes() const { return pitch * image.height; }
    };
    struct Texture {
        Image image;
        u64 hash = 0;
        u64 checked_epoch = ~0ull;
        u64 last_used = 0;
        VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;
    };

    // ---- helpers
    u32 find_memory(u32 bits, VkMemoryPropertyFlags flags) const;
    bool make_buffer(Buffer& b, VkDeviceSize size, VkBufferUsageFlags usage);
    void destroy(Buffer& b);
    bool make_image(Image& im, VkFormat f, u32 w, u32 h, u32 depth, u32 mips, u32 layers, VkImageUsageFlags usage,
                    VkImageType type, VkImageViewType view_type, VkImageAspectFlags aspect);
    void destroy(Image& im);
    void transition(Image& im, VkImageLayout to, VkImageAspectFlags aspect, u32 mips = VK_REMAINING_MIP_LEVELS,
                    u32 layers = VK_REMAINING_ARRAY_LAYERS);
    VkImageAspectFlags aspect_of(VkFormat f) const;
    u8* ring_alloc(VkDeviceSize size, VkDeviceSize align, VkDeviceSize* offset);

    void begin_commands();
    void submit_and_wait();
    void end_rendering();
    void flush();  // write back dirty targets
    void write_back(Target& t);
    void load_target(Target& t);

    bool bind_targets(const State& s, bool need_depth);
    Target* get_target(u32 offset, bool local, u32 rsx_format, u32 w, u32 h, u32 pitch, bool depth, bool swizzled);

    VkShaderModule vertex_shader(const State& s, const vp::Program& p);
    VkShaderModule fragment_shader(const State& s, const fp::Program& p, const glsl::FragmentOptions& o, u64* hash);
    VkPipeline pipeline(const State& s, VkShaderModule vs, VkShaderModule fs, Topology topo, u32 attr_mask,
                        u32 const_mask, bool restart);
    enum TexKind { TEX_COLOR = 0, TEX_SHADOW, TEX_DEPTH_RGBA, TEX_DEPTH_VALUE };
    VkImageView texture_view(const State& s, int unit, glsl::TexDim* dim, TexKind* kind);
    VkImageView target_as_texture(const TextureDesc& d, u32 ea, glsl::TexDim dim);
    VkImageView depth_as_texture(const State& s, int unit, const TextureDesc& d, u32 ea, TexKind* kind);
    VkSampler sampler(const State& s, int unit, bool compare = false);
    VkShaderModule module(bool vertex, const char* glsl);
    VkPipeline clear_pipeline(u32 channel_mask);

    std::function<u32(u32, bool)> to_ea_;
    VulkanStats stats_;
    std::string device_name_;

    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice phys_ = VK_NULL_HANDLE;
    VkDevice dev_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;
    u32 queue_family_ = 0;
    VkPhysicalDeviceMemoryProperties memprops_{};
    VkDeviceSize ubo_align_ = 256;
    VkFormat depth24_ = VK_FORMAT_D24_UNORM_S8_UINT;
    bool independent_blend_ = false;
    VkCommandPool pool_ = VK_NULL_HANDLE;
    VkCommandBuffer cmd_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;
    bool recording_ = false;
    bool rendering_ = false;
    u64 epoch_ = 1;  // bumps at every flip
    u64 tick_ = 0;

    VkDescriptorSetLayout set_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipe_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool desc_pool_ = VK_NULL_HANDLE;

    Buffer ring_;
    VkDeviceSize ring_head_ = 0;
    Buffer staging_;

    std::map<std::tuple<u32, u32, u32, u32, bool>, Target> targets_;  // (ea, fmt, w, h, depth)
    std::vector<Target*> bound_color_;
    Target* bound_depth_ = nullptr;
    std::unordered_map<u64, Texture> textures_;
    std::unordered_map<u64, VkShaderModule> vs_cache_, fs_cache_;
    std::unordered_map<u64, VkPipeline> pipe_cache_;
    std::unordered_map<u64, VkSampler> sampler_cache_;
    Image dummy_[4];  // 1D, 2D, 3D, cube
    VkShaderModule clear_vs_ = VK_NULL_HANDLE, clear_fs_ = VK_NULL_HANDLE;
    VkSampler dummy_sampler_ = VK_NULL_HANDLE;
};

Impl::~Impl() {
    if (!dev_) {
        if (instance_) vkDestroyInstance(instance_, nullptr);
        return;
    }
    vkDeviceWaitIdle(dev_);
    for (auto& [k, t] : targets_) {
        for (auto& [r, v] : t.sample_views) vkDestroyImageView(dev_, v, nullptr);
        destroy(t.image);
    }
    for (auto& [k, t] : textures_) destroy(t.image);
    for (auto& d : dummy_) destroy(d);
    if (clear_vs_) vkDestroyShaderModule(dev_, clear_vs_, nullptr);
    if (clear_fs_) vkDestroyShaderModule(dev_, clear_fs_, nullptr);
    for (auto& [k, m] : vs_cache_) if (m) vkDestroyShaderModule(dev_, m, nullptr);
    for (auto& [k, m] : fs_cache_) if (m) vkDestroyShaderModule(dev_, m, nullptr);
    for (auto& [k, p] : pipe_cache_) if (p) vkDestroyPipeline(dev_, p, nullptr);
    for (auto& [k, s] : sampler_cache_) vkDestroySampler(dev_, s, nullptr);
    if (dummy_sampler_) vkDestroySampler(dev_, dummy_sampler_, nullptr);
    destroy(ring_);
    destroy(staging_);
    vkDestroyDescriptorPool(dev_, desc_pool_, nullptr);
    vkDestroyPipelineLayout(dev_, pipe_layout_, nullptr);
    vkDestroyDescriptorSetLayout(dev_, set_layout_, nullptr);
    vkDestroyFence(dev_, fence_, nullptr);
    vkDestroyCommandPool(dev_, pool_, nullptr);
    vkDestroyDevice(dev_, nullptr);
    vkDestroyInstance(instance_, nullptr);
}

bool Impl::init(std::string* error) {
    auto fail = [&](const char* why) {
        if (error) *error = why;
        return false;
    };
    if (!glsl::have_compiler()) return fail("no shader compiler (build with glslang)");

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "openTIT";
    app.apiVersion = VK_API_VERSION_1_3;
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &app;
    if (vkCreateInstance(&ici, nullptr, &instance_) != VK_SUCCESS) return fail("vkCreateInstance failed");

    u32 n = 0;
    vkEnumeratePhysicalDevices(instance_, &n, nullptr);
    std::vector<VkPhysicalDevice> devs(n);
    vkEnumeratePhysicalDevices(instance_, &n, devs.data());
    int best = -1;
    for (u32 k = 0; k < n; ++k) {
        VkPhysicalDeviceProperties p;
        vkGetPhysicalDeviceProperties(devs[k], &p);
        if (p.apiVersion < VK_API_VERSION_1_3) continue;
        const int score = p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 3
                          : p.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? 2 : 1;
        if (best < 0 || score > best) {
            best = score;
            phys_ = devs[k];
        }
    }
    if (!phys_) return fail("no Vulkan 1.3 device");
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(phys_, &props);
    device_name_ = props.deviceName;
    ubo_align_ = std::max<VkDeviceSize>(props.limits.minUniformBufferOffsetAlignment, 16);
    vkGetPhysicalDeviceMemoryProperties(phys_, &memprops_);

    vkGetPhysicalDeviceQueueFamilyProperties(phys_, &n, nullptr);
    std::vector<VkQueueFamilyProperties> fams(n);
    vkGetPhysicalDeviceQueueFamilyProperties(phys_, &n, fams.data());
    bool found = false;
    for (u32 k = 0; k < n; ++k)
        if (fams[k].queueFlags & VK_QUEUE_GRAPHICS_BIT) { queue_family_ = k; found = true; break; }
    if (!found) return fail("no graphics queue");

    VkPhysicalDeviceFeatures have{};
    vkGetPhysicalDeviceFeatures(phys_, &have);
    VkPhysicalDeviceFeatures want{};
    want.independentBlend = have.independentBlend;
    independent_blend_ = have.independentBlend;
    want.samplerAnisotropy = have.samplerAnisotropy;
    want.depthClamp = have.depthClamp;
    VkPhysicalDeviceVulkan13Features f13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    f13.dynamicRendering = VK_TRUE;

    const float prio = 1.0f;
    VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = queue_family_;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;
    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.pNext = &f13;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.pEnabledFeatures = &want;
    if (vkCreateDevice(phys_, &dci, nullptr, &dev_) != VK_SUCCESS) return fail("vkCreateDevice failed");
    vkGetDeviceQueue(dev_, queue_family_, 0, &queue_);

    VkFormatProperties fp;
    vkGetPhysicalDeviceFormatProperties(phys_, VK_FORMAT_D24_UNORM_S8_UINT, &fp);
    if (!(fp.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
        depth24_ = VK_FORMAT_D32_SFLOAT_S8_UINT;

    VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pci.queueFamilyIndex = queue_family_;
    VKCHECK(vkCreateCommandPool(dev_, &pci, nullptr, &pool_));
    VkCommandBufferAllocateInfo cai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cai.commandPool = pool_;
    cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cai.commandBufferCount = 1;
    VKCHECK(vkAllocateCommandBuffers(dev_, &cai, &cmd_));
    VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VKCHECK(vkCreateFence(dev_, &fci, nullptr, &fence_));

    // Descriptor layout: UBO, 4 vertex textures, 16 fragment textures.
    std::vector<VkDescriptorSetLayoutBinding> b;
    b.push_back({glsl::kVertexConstantBinding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr});
    for (u32 t = 0; t < 4; ++t)
        b.push_back({glsl::kVertexTextureBinding + t, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                     VK_SHADER_STAGE_VERTEX_BIT, nullptr});
    for (u32 t = 0; t < 16; ++t)
        b.push_back({glsl::kFragmentTextureBinding + t, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                     VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
    VkDescriptorSetLayoutCreateInfo dl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    dl.bindingCount = static_cast<u32>(b.size());
    dl.pBindings = b.data();
    VKCHECK(vkCreateDescriptorSetLayout(dev_, &dl, nullptr, &set_layout_));
    const VkPushConstantRange ranges[2] = {{VK_SHADER_STAGE_VERTEX_BIT, 0, 80},
                                           {VK_SHADER_STAGE_FRAGMENT_BIT, glsl::kFragmentPushOffset, 16}};
    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pl.setLayoutCount = 1;
    pl.pSetLayouts = &set_layout_;
    pl.pushConstantRangeCount = 2;
    pl.pPushConstantRanges = ranges;
    VKCHECK(vkCreatePipelineLayout(dev_, &pl, nullptr, &pipe_layout_));
    const VkDescriptorPoolSize sizes[2] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8192},
                                           {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8192 * 20}};
    VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dp.maxSets = 8192;
    dp.poolSizeCount = 2;
    dp.pPoolSizes = sizes;
    VKCHECK(vkCreateDescriptorPool(dev_, &dp, nullptr, &desc_pool_));

    if (!make_buffer(ring_, 64u << 20, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
        return fail("could not allocate the upload ring");
    if (!make_buffer(staging_, 64u << 20, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT))
        return fail("could not allocate the staging buffer");

    // Dummy textures for unbound units (black, alpha 0... RSX returns 0 for disabled units).
    begin_commands();
    const struct { VkImageType t; VkImageViewType v; u32 layers; u32 d; } dims[4] = {
        {VK_IMAGE_TYPE_1D, VK_IMAGE_VIEW_TYPE_1D, 1, 1}, {VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, 1, 1},
        {VK_IMAGE_TYPE_3D, VK_IMAGE_VIEW_TYPE_3D, 1, 1}, {VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_CUBE, 6, 1}};
    for (int k = 0; k < 4; ++k) {
        make_image(dummy_[k], VK_FORMAT_R8G8B8A8_UNORM, 1, 1, dims[k].d, 1, dims[k].layers,
                   VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, dims[k].t, dims[k].v,
                   VK_IMAGE_ASPECT_COLOR_BIT);
        transition(dummy_[k], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        VkClearColorValue zero{};
        VkImageSubresourceRange r{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, dims[k].layers};
        vkCmdClearColorImage(cmd_, dummy_[k].img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &zero, 1, &r);
        transition(dummy_[k], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    submit_and_wait();
    VkSamplerCreateInfo sci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sci.magFilter = sci.minFilter = VK_FILTER_NEAREST;
    VKCHECK(vkCreateSampler(dev_, &sci, nullptr, &dummy_sampler_));
    LOG_INFO("vk", "Vulkan renderer on %s", device_name_.c_str());
    return true;
}

// ---------------------------------------------------------------- resources
u32 Impl::find_memory(u32 bits, VkMemoryPropertyFlags flags) const {
    for (u32 k = 0; k < memprops_.memoryTypeCount; ++k)
        if ((bits & (1u << k)) && (memprops_.memoryTypes[k].propertyFlags & flags) == flags) return k;
    return ~0u;
}

bool Impl::make_buffer(Buffer& b, VkDeviceSize size, VkBufferUsageFlags usage) {
    VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bci.size = size;
    bci.usage = usage;
    if (vkCreateBuffer(dev_, &bci, nullptr, &b.buf) != VK_SUCCESS) return false;
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(dev_, b.buf, &req);
    VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    mai.allocationSize = req.size;
    mai.memoryTypeIndex = find_memory(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (mai.memoryTypeIndex == ~0u || vkAllocateMemory(dev_, &mai, nullptr, &b.mem) != VK_SUCCESS) return false;
    vkBindBufferMemory(dev_, b.buf, b.mem, 0);
    void* p = nullptr;
    vkMapMemory(dev_, b.mem, 0, size, 0, &p);
    b.map = static_cast<u8*>(p);
    b.size = size;
    return true;
}

void Impl::destroy(Buffer& b) {
    if (b.buf) vkDestroyBuffer(dev_, b.buf, nullptr);
    if (b.mem) vkFreeMemory(dev_, b.mem, nullptr);
    b = Buffer{};
}

bool Impl::make_image(Image& im, VkFormat f, u32 w, u32 h, u32 depth, u32 mips, u32 layers, VkImageUsageFlags usage,
                      VkImageType type, VkImageViewType view_type, VkImageAspectFlags aspect) {
    VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ici.flags = view_type == VK_IMAGE_VIEW_TYPE_CUBE ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
    ici.imageType = type;
    ici.format = f;
    ici.extent = {std::max<u32>(w, 1), std::max<u32>(h, 1), std::max<u32>(depth, 1)};
    ici.mipLevels = std::max<u32>(mips, 1);
    ici.arrayLayers = layers;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.usage = usage;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(dev_, &ici, nullptr, &im.img) != VK_SUCCESS) return false;
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(dev_, im.img, &req);
    VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    mai.allocationSize = req.size;
    mai.memoryTypeIndex = find_memory(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mai.memoryTypeIndex == ~0u) mai.memoryTypeIndex = find_memory(req.memoryTypeBits, 0);
    if (vkAllocateMemory(dev_, &mai, nullptr, &im.mem) != VK_SUCCESS) return false;
    vkBindImageMemory(dev_, im.img, im.mem, 0);
    VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vci.image = im.img;
    vci.viewType = view_type;
    vci.format = f;
    vci.subresourceRange = {aspect & ~VK_IMAGE_ASPECT_STENCIL_BIT ? aspect & ~VK_IMAGE_ASPECT_STENCIL_BIT : aspect,
                            0, ici.mipLevels, 0, layers};
    if (vkCreateImageView(dev_, &vci, nullptr, &im.view) != VK_SUCCESS) return false;
    im.format = f;
    im.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    im.width = ici.extent.width;
    im.height = ici.extent.height;
    return true;
}

void Impl::destroy(Image& im) {
    if (im.view) vkDestroyImageView(dev_, im.view, nullptr);
    if (im.img) vkDestroyImage(dev_, im.img, nullptr);
    if (im.mem) vkFreeMemory(dev_, im.mem, nullptr);
    im = Image{};
}

VkImageAspectFlags Impl::aspect_of(VkFormat f) const {
    switch (f) {
    case VK_FORMAT_D16_UNORM: case VK_FORMAT_D32_SFLOAT: return VK_IMAGE_ASPECT_DEPTH_BIT;
    case VK_FORMAT_D24_UNORM_S8_UINT: case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    default: return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

void Impl::transition(Image& im, VkImageLayout to, VkImageAspectFlags aspect, u32 mips, u32 layers) {
    if (im.layout == to) return;
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    b.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    b.oldLayout = im.layout;
    b.newLayout = to;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = im.img;
    b.subresourceRange = {aspect, 0, mips, 0, layers};
    vkCmdPipelineBarrier(cmd_, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &b);
    im.layout = to;
}

u8* Impl::ring_alloc(VkDeviceSize size, VkDeviceSize align, VkDeviceSize* offset) {
    VkDeviceSize at = (ring_head_ + align - 1) & ~(align - 1);
    if (at + size > ring_.size) {
        // Out of space: finish the work in flight and start over.
        flush();
        submit_and_wait();
        begin_commands();
        at = 0;
        if (size > ring_.size) return nullptr;
    }
    ring_head_ = at + size;
    *offset = at;
    return ring_.map + at;
}

// ---------------------------------------------------------------- command flow
void Impl::begin_commands() {
    if (recording_) return;
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VKCHECK(vkBeginCommandBuffer(cmd_, &bi));
    recording_ = true;
}

void Impl::submit_and_wait() {
    if (!recording_) return;
    end_rendering();
    VKCHECK(vkEndCommandBuffer(cmd_));
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd_;
    VKCHECK(vkQueueSubmit(queue_, 1, &si, fence_));
    VKCHECK(vkWaitForFences(dev_, 1, &fence_, VK_TRUE, ~0ull));
    vkResetFences(dev_, 1, &fence_);
    vkResetCommandBuffer(cmd_, 0);
    vkResetDescriptorPool(dev_, desc_pool_, 0);
    recording_ = false;
    ring_head_ = 0;
}

void Impl::end_rendering() {
    if (!rendering_) return;
    vkCmdEndRendering(cmd_);
    rendering_ = false;
}

void Impl::invalidate(u32 ea, u32 size) {
    for (auto& [k, t] : targets_) {
        if (ea < t.ea + t.bytes() && t.ea < ea + size) {
            if (t.dirty) write_back(t);  // keep the GPU side's other pixels
            t.loaded = false;
        }
    }
    for (auto& [k, t] : textures_) t.checked_epoch = ~0ull;
}

void Impl::flush() {
    bool any = false;
    for (auto& [k, t] : targets_) any |= t.dirty;
    if (!any) return;
    ++stats_.flushes;
    begin_commands();
    end_rendering();
    // Copy every dirty target into the staging buffer, then convert to guest memory.
    struct Pending { Target* t; VkDeviceSize offset; };
    std::vector<Pending> pending;
    VkDeviceSize at = 0;
    for (auto& [k, t] : targets_) {
        if (!t.dirty) continue;
        const VkImageAspectFlags full = aspect_of(t.image.format);
        const VkImageAspectFlags aspect = t.depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        const u32 texel = t.depth ? (t.image.format == VK_FORMAT_D16_UNORM ? 2 : 4) : vk_texel_bytes(t.image.format);
        const VkDeviceSize need = static_cast<VkDeviceSize>(t.image.width) * t.image.height * texel;
        const VkDeviceSize stencil_need = t.depth && (full & VK_IMAGE_ASPECT_STENCIL_BIT)
                                              ? static_cast<VkDeviceSize>(t.image.width) * t.image.height : 0;
        if (at + need + stencil_need > staging_.size) break;  // rest go next time
        transition(t.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, full);
        VkBufferImageCopy r{};
        r.bufferOffset = at;
        r.imageSubresource = {aspect, 0, 0, 1};
        r.imageExtent = {t.image.width, t.image.height, 1};
        vkCmdCopyImageToBuffer(cmd_, t.image.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staging_.buf, 1, &r);
        if (stencil_need) {
            r.bufferOffset = at + need;
            r.imageSubresource.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
            vkCmdCopyImageToBuffer(cmd_, t.image.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staging_.buf, 1, &r);
        }
        pending.push_back({&t, at});
        at += (need + stencil_need + 15) & ~VkDeviceSize(15);
    }
    submit_and_wait();
    begin_commands();
    for (const auto& p : pending) {
        Target& t = *p.t;
        const u8* src = staging_.map + p.offset;
        const u32 w = t.image.width, h = t.image.height;
        if (t.depth) {
            const u32 dt = t.image.format == VK_FORMAT_D16_UNORM ? 2 : 4;
            const u8* sten = src + static_cast<std::size_t>(w) * h * dt;
            const bool has_stencil = aspect_of(t.image.format) & VK_IMAGE_ASPECT_STENCIL_BIT;
            for (u32 y = 0; y < h; ++y)
                for (u32 x = 0; x < w; ++x) {
                    u32 raw = 0;
                    std::memcpy(&raw, src + (static_cast<std::size_t>(y) * w + x) * dt, dt);
                    u32 d24;
                    if (dt == 2) d24 = raw << 8;
                    else if (t.image.format == VK_FORMAT_D24_UNORM_S8_UINT) d24 = raw & 0xFFFFFF;
                    else d24 = static_cast<u32>(std::clamp(f32_of(raw), 0.0f, 1.0f) * 16777215.0f + 0.5f);
                    const u32 ea = t.ea + y * t.pitch;
                    if (t.rsx_format == 1) {
                        mem::w16(ea + x * 2, static_cast<u16>(d24 >> 8));
                    } else {
                        const u8 s8 = has_stencil ? sten[static_cast<std::size_t>(y) * w + x] : 0;
                        mem::w32(ea + x * 4, (d24 << 8) | s8);
                    }
                }
        } else {
            const u32 texel = vk_texel_bytes(t.image.format);
            const u32 bpp = color_format(t.rsx_format).bytes;
            const u32 lw = log2u(w), lh = log2u(h);
            if (!t.swizzled && (t.rsx_format == 8 || t.rsx_format == 4 || t.rsx_format == 5)) {
                const u8 fixed_a = t.rsx_format == 4 ? 0 : 255;
                for (u32 y = 0; y < h; ++y) {
                    const u8* in = src + static_cast<std::size_t>(y) * w * 4;
                    u8* out = mem::ptr(t.ea + y * t.pitch);
                    if (t.rsx_format == 8) {
                        for (u32 x = 0; x < w; ++x) {
                            out[4 * x] = in[4 * x + 3]; out[4 * x + 1] = in[4 * x]; out[4 * x + 2] = in[4 * x + 1]; out[4 * x + 3] = in[4 * x + 2];
                        }
                    } else {
                        for (u32 x = 0; x < w; ++x) {
                            out[4 * x] = fixed_a; out[4 * x + 1] = in[4 * x]; out[4 * x + 2] = in[4 * x + 1]; out[4 * x + 3] = in[4 * x + 2];
                        }
                    }
                }
            } else {
                for (u32 y = 0; y < h; ++y)
                    for (u32 x = 0; x < w; ++x) {
                        const u32 off = t.swizzled ? swizzle_offset(x, y, 0, lw, lh, 0) * bpp : y * t.pitch + x * bpp;
                        pixel_to_guest(t.rsx_format, src + (static_cast<std::size_t>(y) * w + x) * texel,
                                       mem::ptr(t.ea + off));
                    }
            }
        }
        t.dirty = false;
    }
    for (auto& [k, t] : textures_) t.checked_epoch = ~0ull;  // guest memory changed under them
}

void Impl::write_back(Target& t) {
    // flush() handles every dirty target; one-off write-backs reuse it.
    (void)t;
    flush();
}

void Impl::load_target(Target& t) {
    if (t.loaded) return;
    t.loaded = true;
    begin_commands();
    end_rendering();
    const VkImageAspectFlags full = aspect_of(t.image.format);
    if (t.depth) {
        // Depth starts cleared; games clear it before use.
        transition(t.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, full);
        VkClearDepthStencilValue v{1.0f, 0};
        VkImageSubresourceRange r{full, 0, 1, 0, 1};
        vkCmdClearDepthStencilImage(cmd_, t.image.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &v, 1, &r);
        return;
    }
    const u32 w = t.image.width, h = t.image.height;
    const u32 texel = vk_texel_bytes(t.image.format);
    const u32 bpp = color_format(t.rsx_format).bytes;
    if (static_cast<VkDeviceSize>(w) * h * texel > staging_.size) return;
    u8* dst = staging_.map;
    const u32 lw = log2u(w), lh = log2u(h);
    if (!t.swizzled && (t.rsx_format == 8 || t.rsx_format == 4 || t.rsx_format == 5)) {
        const bool alpha = t.rsx_format == 8;
        for (u32 y = 0; y < h; ++y) {
            const u8* in = mem::ptr(t.ea + y * t.pitch);
            u8* out = dst + static_cast<std::size_t>(y) * w * 4;
            for (u32 x = 0; x < w; ++x) {
                out[4 * x] = in[4 * x + 1]; out[4 * x + 1] = in[4 * x + 2]; out[4 * x + 2] = in[4 * x + 3];
                out[4 * x + 3] = alpha ? in[4 * x] : 255;
            }
        }
    } else {
        for (u32 y = 0; y < h; ++y)
            for (u32 x = 0; x < w; ++x) {
                const u32 g = t.swizzled ? swizzle_offset(x, y, 0, lw, lh, 0) * bpp : y * t.pitch + x * bpp;
                pixel_from_guest(t.rsx_format, mem::ptr(t.ea + g), dst + (static_cast<std::size_t>(y) * w + x) * texel);
            }
    }
    transition(t.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, full);
    VkBufferImageCopy r{};
    r.bufferOffset = 0;
    r.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    r.imageExtent = {w, h, 1};
    vkCmdCopyBufferToImage(cmd_, staging_.buf, t.image.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &r);
    // staging is reused by the next upload, so finish this one now.
    submit_and_wait();
    begin_commands();
}

Impl::Target* Impl::get_target(u32 offset, bool local, u32 rsx_format, u32 w, u32 h, u32 pitch, bool depth,
                               bool swizzled) {
    const u32 ea = to_ea_(offset, local);
    if (!ea || !w || !h) return nullptr;
    const auto key = std::make_tuple(ea, rsx_format, w, h, depth);
    auto it = targets_.find(key);
    if (it == targets_.end()) {
        Target t;
        t.ea = ea;
        t.rsx_format = rsx_format;
        t.depth = depth;
        t.swizzled = swizzled;
        t.pitch = pitch ? pitch : w * (depth ? (rsx_format == 1 ? 2 : 4) : color_format(rsx_format).bytes);
        const VkFormat f = depth ? (rsx_format == 1 ? VK_FORMAT_D16_UNORM : depth24_) : color_format(rsx_format).vk;
        const VkImageUsageFlags usage = (depth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                                               : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) |
                                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                        VK_IMAGE_USAGE_SAMPLED_BIT;
        if (!make_image(t.image, f, w, h, 1, 1, 1, usage, VK_IMAGE_TYPE_2D, VK_IMAGE_VIEW_TYPE_2D, aspect_of(f))) {
            LOG_ERROR("vk", "could not create a %ux%u render target", w, h);
            return nullptr;
        }
        it = targets_.emplace(key, t).first;
        LOG_DEBUG("vk", "new %s target 0x%08X %ux%u format %u", depth ? "depth" : "color", ea, w, h, rsx_format);
    }
    Target& t = it->second;
    t.pitch = pitch ? pitch : t.pitch;
    t.last_used = ++tick_;
    return &t;
}

bool Impl::bind_targets(const State& s, bool need_depth) {
    const Surface sf = s.surface();
    const u32 w = sf.width ? sf.width : 1u << sf.log2_width, h = sf.height ? sf.height : 1u << sf.log2_height;
    const bool swz = sf.type == 2;
    std::vector<Target*> colors;
    u32 count = 0, first = 0;
    switch (sf.target) {
    case 1: count = 1; first = 0; break;  // A
    case 2: count = 1; first = 1; break;  // B
    case 0x13: count = 2; break;
    case 0x17: count = 3; break;
    case 0x1F: count = 4; break;
    default: count = 0; break;
    }
    for (u32 k = 0; k < count; ++k) {
        const u32 i = first + k;
        Target* t = get_target(sf.color_offset[i], sf.color_local[i], sf.color_format, w, h, sf.color_pitch[i], false, swz);
        if (t) colors.push_back(t);
    }
    Target* depth = nullptr;
    if (need_depth && sf.depth_format)
        depth = get_target(sf.zeta_offset, sf.zeta_local, sf.depth_format, w, h, sf.zeta_pitch, true, false);
    if (colors.empty() && !depth) return false;

    const bool same = rendering_ && colors == bound_color_ && depth == bound_depth_;
    if (same) return true;
    // A different set of targets. The previous pass's results stay on the GPU;
    // they reach guest memory only when something there reads them.
    end_rendering();
    for (Target* t : colors) load_target(*t);
    if (depth) load_target(*depth);
    begin_commands();
    std::vector<VkRenderingAttachmentInfo> ca;
    for (Target* t : colors) {
        transition(t->image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        VkRenderingAttachmentInfo a{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        a.imageView = t->image.view;
        a.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        a.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        a.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        ca.push_back(a);
    }
    VkRenderingAttachmentInfo da{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    if (depth) {
        transition(depth->image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, aspect_of(depth->image.format));
        da.imageView = depth->image.view;
        da.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        da.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        da.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    }
    VkRenderingInfo ri{VK_STRUCTURE_TYPE_RENDERING_INFO};
    const u32 rw = colors.empty() ? depth->image.width : colors[0]->image.width;
    const u32 rh = colors.empty() ? depth->image.height : colors[0]->image.height;
    ri.renderArea = {{0, 0}, {rw, rh}};
    ri.layerCount = 1;
    ri.colorAttachmentCount = static_cast<u32>(ca.size());
    ri.pColorAttachments = ca.data();
    const bool stencil = depth && (aspect_of(depth->image.format) & VK_IMAGE_ASPECT_STENCIL_BIT);
    ri.pDepthAttachment = depth ? &da : nullptr;
    ri.pStencilAttachment = stencil ? &da : nullptr;
    vkCmdBeginRendering(cmd_, &ri);
    rendering_ = true;
    bound_color_ = colors;
    bound_depth_ = depth;
    return true;
}

// ---------------------------------------------------------------- shaders and pipelines
VkShaderModule Impl::vertex_shader(const State&, const vp::Program& p) {
    u64 h = 0xCBF29CE484222325ull;
    for (const auto& i : p.insns) h = fnv1a(i.d, sizeof i.d, h);
    h = fnv1a(&p.start, 4, h);
    auto it = vs_cache_.find(h);
    if (it != vs_cache_.end()) return it->second;
    const std::string src = glsl::vertex(p);
    std::vector<u32> spv;
    std::string log;
    VkShaderModule m = VK_NULL_HANDLE;
    if (glsl::compile(true, src, &spv, &log)) {
        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        ci.codeSize = spv.size() * 4;
        ci.pCode = spv.data();
        VKCHECK(vkCreateShaderModule(dev_, &ci, nullptr, &m));
        ++stats_.shaders;
    } else {
        ++stats_.shader_failures;
        LOG_ERROR("vk", "vertex program %016llX failed to compile:\n%s", (unsigned long long)h, log.c_str());
    }
    vs_cache_[h] = m;
    return m;
}

VkShaderModule Impl::fragment_shader(const State&, const fp::Program& p, const glsl::FragmentOptions& o, u64* hash) {
    u64 h = 0xCBF29CE484222325ull;
    for (const auto& i : p.insns) {
        h = fnv1a(i.d, sizeof i.d, h);
        if (i.has_constant) h = fnv1a(i.constant, sizeof i.constant, h);
    }
    h = fnv1a(&o, sizeof o, h);
    *hash = h;
    auto it = fs_cache_.find(h);
    if (it != fs_cache_.end()) return it->second;
    const std::string src = glsl::fragment(p, o);
    std::vector<u32> spv;
    std::string log;
    VkShaderModule m = VK_NULL_HANDLE;
    if (glsl::compile(false, src, &spv, &log)) {
        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        ci.codeSize = spv.size() * 4;
        ci.pCode = spv.data();
        VKCHECK(vkCreateShaderModule(dev_, &ci, nullptr, &m));
        ++stats_.shaders;
    } else {
        ++stats_.shader_failures;
        LOG_ERROR("vk", "fragment program %016llX failed to compile:\n%s", (unsigned long long)h, log.c_str());
    }
    fs_cache_[h] = m;
    return m;
}

VkPipeline Impl::pipeline(const State& s, VkShaderModule vs, VkShaderModule fs, Topology topo, u32 attr_mask,
                          u32 const_mask, bool restart) {
    // Everything that goes into the pipeline, hashed.
    struct Key {
        VkShaderModule vs, fs;
        u32 topo, attr_mask, const_mask, restart;
        u32 blend_enable, blend_mrt, sfactor, dfactor, equation, color_mask;
        u32 depth_test, depth_func, depth_mask;
        u32 stencil[16];
        u32 cull_enable, cull_face, front_face, origin_bottom;
        VkFormat colors[4];
        VkFormat depth;
        u32 color_count;
    } k;
    std::memset(&k, 0, sizeof k);  // hashed as bytes: padding must be zero
    k.vs = vs;
    k.fs = fs;
    k.topo = static_cast<u32>(topo);
    k.attr_mask = attr_mask;
    k.const_mask = const_mask;
    k.restart = restart;
    k.blend_enable = s.reg(R_BLEND_ENABLE);
    k.blend_mrt = s.reg(R_BLEND_ENABLE_MRT);
    k.sfactor = s.reg(R_BLEND_SFACTOR);
    k.dfactor = s.reg(R_BLEND_DFACTOR);
    k.equation = s.reg(R_BLEND_EQUATION);
    k.color_mask = s.reg(R_COLOR_MASK);
    k.depth_test = s.reg(R_DEPTH_TEST);
    k.depth_func = s.reg(R_DEPTH_FUNC);
    k.depth_mask = s.reg(R_DEPTH_MASK);
    for (u32 i = 0; i < 16; ++i) k.stencil[i] = s.reg(R_STENCIL_ENABLE + 4 * i);
    k.cull_enable = s.reg(R_CULL_ENABLE);
    k.cull_face = s.reg(R_CULL_FACE);
    k.front_face = s.reg(R_FRONT_FACE);
    k.origin_bottom = ((s.reg(R_SHADER_WINDOW) >> 12) & 0xF) == 1;
    for (std::size_t i = 0; i < bound_color_.size(); ++i) k.colors[i] = bound_color_[i]->image.format;
    k.color_count = static_cast<u32>(bound_color_.size());
    k.depth = bound_depth_ ? bound_depth_->image.format : VK_FORMAT_UNDEFINED;
    const u64 h = fnv1a(&k, sizeof k);
    auto it = pipe_cache_.find(h);
    if (it != pipe_cache_.end()) return it->second;

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vs, "main", nullptr};
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fs, "main", nullptr};

    std::vector<VkVertexInputBindingDescription> binds;
    std::vector<VkVertexInputAttributeDescription> attrs;
    for (u32 a = 0; a < 16; ++a) {
        if (!((attr_mask | const_mask) & (1u << a))) continue;
        binds.push_back({a, (attr_mask & (1u << a)) ? 16u : 0u, VK_VERTEX_INPUT_RATE_VERTEX});
        attrs.push_back({a, a, VK_FORMAT_R32G32B32A32_SFLOAT, 0});
    }
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vi.vertexBindingDescriptionCount = static_cast<u32>(binds.size());
    vi.pVertexBindingDescriptions = binds.data();
    vi.vertexAttributeDescriptionCount = static_cast<u32>(attrs.size());
    vi.pVertexAttributeDescriptions = attrs.data();
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = vk_topology(topo);
    ia.primitiveRestartEnable = restart ? VK_TRUE : VK_FALSE;
    VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.lineWidth = 1.0f;
    rs.cullMode = VK_CULL_MODE_NONE;
    if (k.cull_enable) {
        rs.cullMode = k.cull_face == 0x404 ? VK_CULL_MODE_FRONT_BIT
                      : k.cull_face == 0x408 ? VK_CULL_MODE_FRONT_AND_BACK : VK_CULL_MODE_BACK_BIT;
    }
    // RSX decides facing GL-style in window space. With a top-left window
    // origin that space is Vulkan's framebuffer space, whose facing formula
    // has the opposite sign; a bottom-left origin flips y and cancels that.
    const bool ccw = k.front_face != 0x900;
    rs.frontFace = (ccw != (k.origin_bottom != 0)) ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    if (bound_depth_) {
        ds.depthTestEnable = k.depth_test ? VK_TRUE : VK_FALSE;
        ds.depthWriteEnable = k.depth_test && k.depth_mask ? VK_TRUE : VK_FALSE;
        ds.depthCompareOp = compare_op(k.depth_func);
        if (s.reg(R_STENCIL_ENABLE) && (aspect_of(bound_depth_->image.format) & VK_IMAGE_ASPECT_STENCIL_BIT)) {
            ds.stencilTestEnable = VK_TRUE;
            auto face = [&](u32 base) {
                VkStencilOpState st{};
                st.writeMask = s.reg(base);
                st.compareOp = compare_op(s.reg(base + 4));
                st.reference = s.reg(base + 8) & 0xFF;
                st.compareMask = s.reg(base + 12);
                st.failOp = stencil_op(s.reg(base + 16));
                st.depthFailOp = stencil_op(s.reg(base + 20));
                st.passOp = stencil_op(s.reg(base + 24));
                return st;
            };
            ds.front = face(R_STENCIL_MASK);
            ds.back = s.reg(R_TWO_SIDED_STENCIL) ? face(R_BACK_STENCIL_MASK) : ds.front;
        }
    }
    std::array<VkPipelineColorBlendAttachmentState, 4> att{};
    const u32 cm = k.color_mask;
    for (u32 i = 0; i < 4; ++i) {
        auto& a = att[i];
        const bool en = i == 0 ? k.blend_enable != 0 : independent_blend_ ? ((k.blend_mrt >> i) & 1) != 0 : k.blend_enable != 0;
        a.blendEnable = en ? VK_TRUE : VK_FALSE;
        a.srcColorBlendFactor = blend_factor(k.sfactor & 0xFFFF);
        a.srcAlphaBlendFactor = blend_factor(k.sfactor >> 16);
        a.dstColorBlendFactor = blend_factor(k.dfactor & 0xFFFF);
        a.dstAlphaBlendFactor = blend_factor(k.dfactor >> 16);
        a.colorBlendOp = blend_op(k.equation & 0xFFFF);
        a.alphaBlendOp = blend_op(k.equation >> 16);
        a.colorWriteMask = ((cm >> 16) & 1 ? VK_COLOR_COMPONENT_R_BIT : 0) | ((cm >> 8) & 1 ? VK_COLOR_COMPONENT_G_BIT : 0) |
                           (cm & 1 ? VK_COLOR_COMPONENT_B_BIT : 0) | ((cm >> 24) & 1 ? VK_COLOR_COMPONENT_A_BIT : 0);
    }
    VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = k.color_count;
    cb.pAttachments = att.data();
    const VkDynamicState dyn[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_BLEND_CONSTANTS};
    VkPipelineDynamicStateCreateInfo dy{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dy.dynamicStateCount = 3;
    dy.pDynamicStates = dyn;
    VkPipelineRenderingCreateInfo pr{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    pr.colorAttachmentCount = k.color_count;
    pr.pColorAttachmentFormats = k.colors;
    pr.depthAttachmentFormat = k.depth;
    pr.stencilAttachmentFormat =
        bound_depth_ && (aspect_of(k.depth) & VK_IMAGE_ASPECT_STENCIL_BIT) ? k.depth : VK_FORMAT_UNDEFINED;
    VkGraphicsPipelineCreateInfo gp{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gp.pNext = &pr;
    gp.stageCount = 2;
    gp.pStages = stages;
    gp.pVertexInputState = &vi;
    gp.pInputAssemblyState = &ia;
    gp.pViewportState = &vp;
    gp.pRasterizationState = &rs;
    gp.pMultisampleState = &ms;
    gp.pDepthStencilState = &ds;
    gp.pColorBlendState = &cb;
    gp.pDynamicState = &dy;
    gp.layout = pipe_layout_;
    VkPipeline p = VK_NULL_HANDLE;
    VKCHECK(vkCreateGraphicsPipelines(dev_, VK_NULL_HANDLE, 1, &gp, nullptr, &p));
    pipe_cache_[h] = p;
    ++stats_.pipelines;
    return p;
}

// ---------------------------------------------------------------- textures
VkImageView Impl::texture_view(const State& s, int unit, glsl::TexDim* dim, TexKind* kind) {
    *kind = TEX_COLOR;
    const rsx::Texture t = s.texture(unit);
    *dim = t.cubemap ? glsl::TexDim::Cube : t.dimension == 1 ? glsl::TexDim::D1 : t.dimension == 3 ? glsl::TexDim::D3 : glsl::TexDim::D2;
    const int dummy = *dim == glsl::TexDim::D1 ? 0 : *dim == glsl::TexDim::D3 ? 2 : *dim == glsl::TexDim::Cube ? 3 : 1;
    if (!t.enabled() || !t.width) return dummy_[dummy].view;
    TextureDesc d;
    d.format = t.format;
    d.width = t.width;
    d.height = std::max<u32>(t.height, 1);
    const u32 c3 = s.reg(R_TEXTURE_CONTROL3 + 4 * unit);
    d.depth = t.dimension == 3 ? std::max<u32>(c3 >> 20, 1) : 1;
    d.pitch = c3 & 0xFFFFF;
    d.mipmaps = std::max<u32>(t.mipmaps, 1);
    d.cubemap = t.cubemap;
    d.remap = t.control1 & 0xFFFF;
    const u32 ea = to_ea_(t.offset, t.local);
    if (!ea) return dummy_[dummy].view;
    const u32 size = texture_size(d);
    // Render-to-texture: sample the target image itself when it matches.
    if (VkImageView rv = target_as_texture(d, ea, *dim)) return rv;
    if (*dim == glsl::TexDim::D2)
        if (VkImageView dv = depth_as_texture(s, unit, d, ea, kind)) return dv;
    // Otherwise a render target the texture overlaps must reach guest memory first.
    for (auto& [k, rt] : targets_)
        if (rt.dirty && ea < rt.ea + rt.bytes() && rt.ea < ea + size) { flush(); break; }

    // Hash the fields, not the struct (its padding is indeterminate).
    const u32 fields[] = {d.format, d.width, d.height, d.depth, d.mipmaps, d.cubemap ? 1u : 0u, d.pitch, d.remap, ea};
    const u64 key = fnv1a(fields, sizeof fields);
    Texture& tex = textures_[key];
    tex.last_used = ++tick_;
    if (tex.image.img && tex.checked_epoch == epoch_) return tex.image.view;
    const u64 content = fast_hash(mem::ptr(ea), size);
    tex.checked_epoch = epoch_;
    if (tex.image.img && tex.hash == content) return tex.image.view;
    tex.hash = content;

    // (Re)decode and upload.
    const auto levels = decode_texture(d, mem::ptr(ea));
    if (levels.empty()) return dummy_[dummy].view;
    const u32 layers = d.cubemap ? 6 : 1;
    if (!tex.image.img) {
        const VkImageType type = *dim == glsl::TexDim::D1 ? VK_IMAGE_TYPE_1D : *dim == glsl::TexDim::D3 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
        const VkImageViewType vt = *dim == glsl::TexDim::D1 ? VK_IMAGE_VIEW_TYPE_1D : *dim == glsl::TexDim::D3 ? VK_IMAGE_VIEW_TYPE_3D
                                   : *dim == glsl::TexDim::Cube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
        if (!make_image(tex.image, VK_FORMAT_R8G8B8A8_UNORM, d.width, d.height, d.depth, d.mipmaps, layers,
                        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, type, vt, VK_IMAGE_ASPECT_COLOR_BIT)) {
            textures_.erase(key);
            return dummy_[dummy].view;
        }
        tex.view_type = vt;
        ++stats_.textures;
    }
    std::size_t total = 0;
    for (const auto& l : levels) total += l.rgba.size();
    if (total > staging_.size) return dummy_[dummy].view;
    const bool was_rendering = rendering_;
    end_rendering();
    std::vector<VkBufferImageCopy> copies;
    std::size_t at = 0;
    for (std::size_t k = 0; k < levels.size(); ++k) {
        std::memcpy(staging_.map + at, levels[k].rgba.data(), levels[k].rgba.size());
        VkBufferImageCopy r{};
        r.bufferOffset = at;
        r.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, static_cast<u32>(k % d.mipmaps), static_cast<u32>(k / d.mipmaps), 1};
        r.imageExtent = {levels[k].width, levels[k].height, levels[k].depth};
        copies.push_back(r);
        at += (levels[k].rgba.size() + 15) & ~std::size_t(15);
    }
    transition(tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdCopyBufferToImage(cmd_, staging_.buf, tex.image.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           static_cast<u32>(copies.size()), copies.data());
    transition(tex.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    // The staging buffer is reused, so this upload has to land before the next.
    submit_and_wait();
    begin_commands();
    if (was_rendering) {
        // Re-open the pass on the same targets.
        bound_color_.clear();
        bound_depth_ = nullptr;
    }
    return tex.image.view;
}

// Depth buffers read as textures: shadow compares, depth values, or (for an
// xRGB8 texture over a depth buffer) the 24 depth bits spread over channels.
VkImageView Impl::depth_as_texture(const State& s, int unit, const TextureDesc& d, u32 ea, TexKind* kind) {
    const u32 base = d.base_format();
    const bool depth_fmt = base == TEX_DEPTH24_D8 || base == TEX_DEPTH24_D8_FLOAT || base == TEX_DEPTH16 || base == TEX_DEPTH16_FLOAT;
    const bool argb = base == TEX_A8R8G8B8 || base == TEX_D8R8G8B8;
    if ((!depth_fmt && !argb) || d.mipmaps > 1 || d.cubemap) return VK_NULL_HANDLE;
    const u32 zfunc = (s.reg(NV4097_SET_TEXTURE_OFFSET + 0x20 * unit + 8) >> 28) & 0xF;
    for (auto& [k, t] : targets_) {
        if (t.ea != ea || !t.depth || !t.loaded) continue;
        if (t.image.width != d.width || t.image.height != d.height) continue;
        if (argb && t.rsx_format != 2) continue;  // only Z24S8 has bits to spread
        const TexKind want = depth_fmt ? (zfunc ? TEX_SHADOW : TEX_DEPTH_VALUE) : TEX_DEPTH_RGBA;
        const u32 key = 0x10000u | static_cast<u32>(want);
        auto vit = t.sample_views.find(key);
        if (vit == t.sample_views.end()) {
            VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            vci.image = t.image.img;
            vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            vci.format = t.image.format;
            if (want == TEX_DEPTH_VALUE)
                vci.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R};
            vci.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
            VkImageView v = VK_NULL_HANDLE;
            if (vkCreateImageView(dev_, &vci, nullptr, &v) != VK_SUCCESS) return VK_NULL_HANDLE;
            vit = t.sample_views.emplace(key, v).first;
        }
        if (t.image.layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            begin_commands();
            end_rendering();
            bound_color_.clear();
            bound_depth_ = nullptr;
            transition(t.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, aspect_of(t.image.format));
        }
        *kind = want;
        ++stats_.rt_samples;
        return vit->second;
    }
    return VK_NULL_HANDLE;
}

VkImageView Impl::target_as_texture(const TextureDesc& d, u32 ea, glsl::TexDim dim) {
    if (dim != glsl::TexDim::D2 || d.mipmaps > 1 || d.cubemap) return VK_NULL_HANDLE;
    const u32 base = d.base_format();
    for (auto& [k, t] : targets_) {
        if (t.ea != ea || t.depth || !t.loaded) continue;
        if (t.image.width != d.width || t.image.height != d.height) continue;
        if (t.swizzled == d.linear()) continue;
        if (d.linear() && d.pitch && d.pitch != t.pitch) continue;
        // How texture input channels (A, R, G, B) are found in the image.
        VkComponentSwizzle in[4];
        switch (base) {
        case TEX_A8R8G8B8: case TEX_D8R8G8B8:
            if (t.rsx_format != 8 && t.rsx_format != 4 && t.rsx_format != 5) continue;
            in[0] = VK_COMPONENT_SWIZZLE_A; in[1] = VK_COMPONENT_SWIZZLE_R; in[2] = VK_COMPONENT_SWIZZLE_G; in[3] = VK_COMPONENT_SWIZZLE_B;
            if (base == TEX_D8R8G8B8 || t.rsx_format != 8) in[0] = VK_COMPONENT_SWIZZLE_ONE;
            break;
        case TEX_W16_Z16_Y16_X16_FLOAT: case TEX_W32_Z32_Y32_X32_FLOAT:
            if (t.rsx_format != (base == TEX_W16_Z16_Y16_X16_FLOAT ? 11u : 12u)) continue;
            in[0] = VK_COMPONENT_SWIZZLE_A; in[1] = VK_COMPONENT_SWIZZLE_R; in[2] = VK_COMPONENT_SWIZZLE_G; in[3] = VK_COMPONENT_SWIZZLE_B;
            break;
        case TEX_X32_FLOAT: case TEX_B8:
            if (t.rsx_format != (base == TEX_X32_FLOAT ? 13u : 9u)) continue;
            in[0] = in[1] = in[2] = in[3] = VK_COMPONENT_SWIZZLE_R;
            break;
        default:
            continue;
        }
        auto vit = t.sample_views.find(d.remap);
        if (vit == t.sample_views.end()) {
            VkComponentSwizzle out[4];  // A, R, G, B
            for (int o = 0; o < 4; ++o) {
                const u32 sel = (d.remap >> (2 * o)) & 3, ctl = (d.remap >> (8 + 2 * o)) & 3;
                out[o] = ctl == 0 ? VK_COMPONENT_SWIZZLE_ZERO : ctl == 1 ? VK_COMPONENT_SWIZZLE_ONE : in[sel];
            }
            VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            vci.image = t.image.img;
            vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            vci.format = t.image.format;
            vci.components = {out[1], out[2], out[3], out[0]};
            vci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            VkImageView v = VK_NULL_HANDLE;
            if (vkCreateImageView(dev_, &vci, nullptr, &v) != VK_SUCCESS) return VK_NULL_HANDLE;
            vit = t.sample_views.emplace(d.remap, v).first;
        }
        if (t.image.layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            // Layout changes can't happen inside a rendering scope: close it.
            begin_commands();
            end_rendering();
            bound_color_.clear();
            bound_depth_ = nullptr;
            transition(t.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        }
        ++stats_.rt_samples;
        return vit->second;
    }
    return VK_NULL_HANDLE;
}

VkSampler Impl::sampler(const State& s, int unit, bool compare) {
    const u32 b = NV4097_SET_TEXTURE_OFFSET + 0x20 * unit;
    const u32 addr = s.reg(b + 8), filter = s.reg(b + 0x14), c0 = s.reg(b + 0xC);
    const u32 zfunc = compare ? (addr >> 28) & 0xF : 0;
    const u64 key = (static_cast<u64>(addr & 0xFFFFF) << 32) | (static_cast<u64>(zfunc) << 52) | (compare ? 1ull << 56 : 0) |
                    (filter & 0xFF0F0000) | ((c0 >> 7) & 0xFFFF);
    auto it = sampler_cache_.find(key);
    if (it != sampler_cache_.end()) return it->second;
    VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    const u32 mag = (filter >> 24) & 0xF, min = (filter >> 16) & 0xFF;
    ci.magFilter = mag == 1 ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    // min: 1 nearest, 2 linear, 3 nearest-mip-nearest, 4 linear-mip-nearest, 5 nearest-mip-linear, 6 linear-mip-linear
    ci.minFilter = (min == 1 || min == 3 || min == 5) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    ci.mipmapMode = (min == 5 || min == 6) ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    ci.addressModeU = address_mode(addr & 0xF);
    ci.addressModeV = address_mode((addr >> 8) & 0xF);
    ci.addressModeW = address_mode((addr >> 16) & 0xF);
    ci.minLod = static_cast<float>((c0 >> 19) & 0xFFF) / 256.0f;
    ci.maxLod = (min == 1 || min == 2) ? ci.minLod + 0.25f : static_cast<float>((c0 >> 7) & 0xFFF) / 256.0f;
    if (ci.maxLod < ci.minLod) ci.maxLod = ci.minLod;
    ci.borderColor = compare ? VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE : VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    if (compare) {
        ci.compareEnable = VK_TRUE;
        ci.compareOp = compare_op(zfunc);  // CELL_GCM_TEXTURE_ZFUNC_* uses the same order
    }
    VkSampler smp = VK_NULL_HANDLE;
    VKCHECK(vkCreateSampler(dev_, &ci, nullptr, &smp));
    sampler_cache_[key] = smp;
    return smp;
}

// ---------------------------------------------------------------- draws
void Impl::draw(const State& s, const DrawCall& c) {
    const vp::Program v = vp::analyze(s.vp_ucode.data(), s.vp_start());
    const u32 fp_ea = to_ea_(s.fp_offset(), s.fp_local());
    if (!fp_ea || v.insns.empty()) return;
    const fp::Program f = fp::parse(mem::ptr(fp_ea), 0x10000);
    if (f.truncated) return;

    glsl::FragmentOptions fo;
    std::memset(&fo, 0, sizeof fo);  // hashed as bytes: no stray padding
    const u32 ctrl = s.reg(R_SHADER_CONTROL);
    fo.r_exports = (ctrl & 0x40) != 0;
    fo.depth_export = (ctrl & 0xE) != 0 && f.writes_depth;
    fo.alpha_test = s.reg(R_ALPHA_TEST_ENABLE) != 0;
    fo.two_sided_color = s.reg(R_TWO_SIDE_LIGHT) != 0;
    VkDescriptorImageInfo images[20]{};
    for (int u = 0; u < 16; ++u) {
        images[4 + u] = {dummy_sampler_, dummy_[1].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        if (!(f.textures & (1u << u))) continue;
        glsl::TexDim dim;
        TexKind kind;
        const VkImageView view = texture_view(s, u, &dim, &kind);
        fo.textures[u] = dim;
        if (kind == TEX_SHADOW) fo.shadow_mask |= static_cast<u16>(1u << u);
        if (kind == TEX_DEPTH_RGBA) fo.depth_rgba_mask |= static_cast<u16>(1u << u);
        images[4 + u] = {sampler(s, u, kind == TEX_SHADOW), view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }
    for (int u = 0; u < 4; ++u) images[u] = {dummy_sampler_, dummy_[1].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

    const VkShaderModule vs = vertex_shader(s, v);
    u64 fh = 0;
    const VkShaderModule fs = fragment_shader(s, f, fo, &fh);
    if (!vs || !fs) return;

    const Geometry g = assemble(s, c, to_ea_, v.inputs_read);
    if (g.indices.empty()) return;

    begin_commands();
    if (!bind_targets(s, true)) return;
    const u32 const_mask = v.inputs_read & ~g.attribute_mask;
    const bool restart = g.topology == Topology::LineStrip || g.topology == Topology::TriangleStrip ||
                         g.topology == Topology::TriangleFan;
    const VkPipeline p = pipeline(s, vs, fs, g.topology, g.attribute_mask, const_mask, restart);
    if (!p) return;

    // ---- uploads (vertex attributes, constants, indices, uniform block)
    VkDeviceSize offs[16] = {};
    VkBuffer bufs[16] = {};
    u32 first_binding = 16, last_binding = 0;
    for (u32 a = 0; a < 16; ++a) {
        if (!((g.attribute_mask | const_mask) & (1u << a))) continue;
        const bool array = g.attribute_mask & (1u << a);
        const std::size_t bytes = array ? g.attributes[a].size() * 4 : 16;
        u8* dst = ring_alloc(bytes, 16, &offs[a]);
        if (!dst) return;
        std::memcpy(dst, array ? static_cast<const void*>(g.attributes[a].data()) : g.constant_attributes[a].data(), bytes);
        bufs[a] = ring_.buf;
        first_binding = std::min(first_binding, a);
        last_binding = std::max(last_binding, a);
    }
    VkDeviceSize ioff = 0;
    u8* idst = ring_alloc(g.indices.size() * 4, 4, &ioff);
    if (!idst) return;
    std::memcpy(idst, g.indices.data(), g.indices.size() * 4);
    VkDeviceSize uoff = 0;
    u8* udst = ring_alloc(sizeof(u32) * s.vp_const.size(), ubo_align_, &uoff);
    if (!udst) return;
    std::memcpy(udst, s.vp_const.data(), sizeof(u32) * s.vp_const.size());
    // bind_targets may have been reset by a texture upload's resubmit
    if (!rendering_ && !bind_targets(s, true)) return;

    VkDescriptorSetAllocateInfo dai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    dai.descriptorPool = desc_pool_;
    dai.descriptorSetCount = 1;
    dai.pSetLayouts = &set_layout_;
    VkDescriptorSet set = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(dev_, &dai, &set) != VK_SUCCESS) {
        flush();
        submit_and_wait();
        begin_commands();
        if (!bind_targets(s, true) || vkAllocateDescriptorSets(dev_, &dai, &set) != VK_SUCCESS) return;
    }
    VkDescriptorBufferInfo ubo{ring_.buf, uoff, sizeof(u32) * s.vp_const.size()};
    std::vector<VkWriteDescriptorSet> writes;
    VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    w.dstSet = set;
    w.dstBinding = glsl::kVertexConstantBinding;
    w.descriptorCount = 1;
    w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w.pBufferInfo = &ubo;
    writes.push_back(w);
    for (u32 k = 0; k < 20; ++k) {
        VkWriteDescriptorSet t{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        t.dstSet = set;
        t.dstBinding = k < 4 ? glsl::kVertexTextureBinding + k : glsl::kFragmentTextureBinding + (k - 4);
        t.descriptorCount = 1;
        t.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        t.pImageInfo = &images[k];
        writes.push_back(t);
    }
    vkUpdateDescriptorSets(dev_, static_cast<u32>(writes.size()), writes.data(), 0, nullptr);

    // ---- viewport transform: clip -> RSX window (scale/offset) -> Vulkan NDC
    const VkExtent2D ext{bound_color_.empty() ? bound_depth_->image.width : bound_color_[0]->image.width,
                         bound_color_.empty() ? bound_depth_->image.height : bound_color_[0]->image.height};
    float sc[4], of[4];
    for (int k = 0; k < 4; ++k) {
        sc[k] = f32_of(s.reg(R_VIEWPORT_SCALE + 4 * k));
        of[k] = f32_of(s.reg(R_VIEWPORT_OFFSET + 4 * k));
    }
    const float W = static_cast<float>(ext.width), H = static_cast<float>(ext.height);
    // Column-major mat4 for GLSL: m[col][row]
    float m[16] = {};
    m[0] = 2.0f * sc[0] / W;
    const u32 win = s.reg(R_SHADER_WINDOW);
    const bool bottom = ((win >> 12) & 0xF) == 1;
    const float wh = static_cast<float>(win & 0xFFF ? win & 0xFFF : ext.height);
    m[5] = (bottom ? -2.0f : 2.0f) * sc[1] / H;
    m[10] = sc[2];
    m[12] = 2.0f * of[0] / W - 1.0f;
    m[13] = 2.0f * (bottom ? wh - of[1] : of[1]) / H - 1.0f;
    m[14] = of[2];
    m[15] = 1.0f;
    struct { float m[16]; u32 branch_bits; u32 pad[3]; } vpush{};
    std::memcpy(vpush.m, m, sizeof m);
    vpush.branch_bits = s.reg(R_TRANSFORM_BRANCH_BITS);
    struct { float alpha_ref; u32 alpha_func; u32 pad[2]; } fpush{};
    const bool fp32 = bound_color_.empty() ? false : bound_color_[0]->image.format != VK_FORMAT_R8G8B8A8_UNORM;
    fpush.alpha_ref = fp32 ? f32_of(s.reg(R_ALPHA_REF)) : static_cast<float>(s.reg(R_ALPHA_REF) & 0xFF) / 255.0f;
    fpush.alpha_func = s.reg(R_ALPHA_FUNC);

    vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, p);
    vkCmdBindDescriptorSets(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe_layout_, 0, 1, &set, 0, nullptr);
    vkCmdPushConstants(cmd_, pipe_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, 80, &vpush);
    vkCmdPushConstants(cmd_, pipe_layout_, VK_SHADER_STAGE_FRAGMENT_BIT, glsl::kFragmentPushOffset, 16, &fpush);
    VkViewport vpt{0.0f, 0.0f, W, H, 0.0f, 1.0f};
    vkCmdSetViewport(cmd_, 0, 1, &vpt);
    const u32 sh = s.reg(R_SCISSOR_H), sv = s.reg(R_SCISSOR_V);
    s32 x0 = static_cast<s32>(sh & 0xFFFF), y0 = static_cast<s32>(sv & 0xFFFF);
    s32 x1 = sh >> 16 ? x0 + static_cast<s32>(sh >> 16) : static_cast<s32>(ext.width);
    s32 y1 = sv >> 16 ? y0 + static_cast<s32>(sv >> 16) : static_cast<s32>(ext.height);
    if (bottom && (sv >> 16)) {
        const s32 h0 = static_cast<s32>(wh);
        const s32 ny0 = h0 - y1, ny1 = h0 - y0;
        y0 = ny0;
        y1 = ny1;
    }
    x0 = std::clamp<s32>(x0, 0, static_cast<s32>(ext.width));
    y0 = std::clamp<s32>(y0, 0, static_cast<s32>(ext.height));
    x1 = std::clamp<s32>(x1, x0, static_cast<s32>(ext.width));
    y1 = std::clamp<s32>(y1, y0, static_cast<s32>(ext.height));
    VkRect2D sci{{x0, y0}, {static_cast<u32>(x1 - x0), static_cast<u32>(y1 - y0)}};
    vkCmdSetScissor(cmd_, 0, 1, &sci);
    const u32 bc = s.reg(R_BLEND_COLOR);
    const float blend[4] = {((bc >> 16) & 0xFF) / 255.0f, ((bc >> 8) & 0xFF) / 255.0f, (bc & 0xFF) / 255.0f, (bc >> 24) / 255.0f};
    vkCmdSetBlendConstants(cmd_, blend);
    if (last_binding >= first_binding)
        vkCmdBindVertexBuffers(cmd_, first_binding, last_binding - first_binding + 1, bufs + first_binding, offs + first_binding);
    vkCmdBindIndexBuffer(cmd_, ring_.buf, ioff, VK_INDEX_TYPE_UINT32);
    for (const auto& b : g.batches)
        if (b.second) vkCmdDrawIndexed(cmd_, b.second, 1, b.first, 0, 0);
    for (Target* t : bound_color_) t->dirty = true;
    if (bound_depth_) bound_depth_->dirty = true;
    ++stats_.draws;
}

VkShaderModule Impl::module(bool vertex, const char* src) {
    std::vector<u32> spv;
    std::string log;
    if (!glsl::compile(vertex, src, &spv, &log)) {
        LOG_ERROR("vk", "internal shader failed: %s", log.c_str());
        return VK_NULL_HANDLE;
    }
    VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ci.codeSize = spv.size() * 4;
    ci.pCode = spv.data();
    VkShaderModule m = VK_NULL_HANDLE;
    VKCHECK(vkCreateShaderModule(dev_, &ci, nullptr, &m));
    return m;
}

// Full-screen triangle writing a push-constant colour through a channel mask,
// for clears that only touch some colour channels.
VkPipeline Impl::clear_pipeline(u32 channel_mask) {
    if (!clear_vs_) {
        clear_vs_ = module(true,
                           "#version 450\n"
                           "void main() {\n"
                           "    vec2 p = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);\n"
                           "    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);\n"
                           "}\n");
        clear_fs_ = module(false,
                           "#version 450\n"
                           "layout(push_constant) uniform P { layout(offset = 80) vec4 c; } pc;\n"
                           "layout(location = 0) out vec4 o0;\n"
                           "layout(location = 1) out vec4 o1;\n"
                           "layout(location = 2) out vec4 o2;\n"
                           "layout(location = 3) out vec4 o3;\n"
                           "void main() { o0 = pc.c; o1 = pc.c; o2 = pc.c; o3 = pc.c; }\n");
    }
    if (!clear_vs_ || !clear_fs_) return VK_NULL_HANDLE;
    struct Key { u32 tag, mask, count; VkFormat colors[4]; VkFormat depth; } k;
    std::memset(&k, 0, sizeof k);
    k.tag = 0xC1EA4u;
    k.mask = channel_mask;
    k.count = static_cast<u32>(bound_color_.size());
    for (u32 i = 0; i < k.count; ++i) k.colors[i] = bound_color_[i]->image.format;
    k.depth = bound_depth_ ? bound_depth_->image.format : VK_FORMAT_UNDEFINED;
    const u64 h = fnv1a(&k, sizeof k);
    if (auto it = pipe_cache_.find(h); it != pipe_cache_.end()) return it->second;

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, clear_vs_, "main", nullptr};
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, clear_fs_, "main", nullptr};
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    std::array<VkPipelineColorBlendAttachmentState, 4> att{};
    for (auto& a : att)
        a.colorWriteMask = ((channel_mask & 0x10) ? VK_COLOR_COMPONENT_R_BIT : 0) | ((channel_mask & 0x20) ? VK_COLOR_COMPONENT_G_BIT : 0) |
                           ((channel_mask & 0x40) ? VK_COLOR_COMPONENT_B_BIT : 0) | ((channel_mask & 0x80) ? VK_COLOR_COMPONENT_A_BIT : 0);
    VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = k.count;
    cb.pAttachments = att.data();
    const VkDynamicState dyn[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dy{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dy.dynamicStateCount = 2;
    dy.pDynamicStates = dyn;
    VkPipelineRenderingCreateInfo pr{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    pr.colorAttachmentCount = k.count;
    pr.pColorAttachmentFormats = k.colors;
    pr.depthAttachmentFormat = k.depth;
    pr.stencilAttachmentFormat = bound_depth_ && (aspect_of(k.depth) & VK_IMAGE_ASPECT_STENCIL_BIT) ? k.depth : VK_FORMAT_UNDEFINED;
    VkGraphicsPipelineCreateInfo gp{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gp.pNext = &pr;
    gp.stageCount = 2;
    gp.pStages = stages;
    gp.pVertexInputState = &vi;
    gp.pInputAssemblyState = &ia;
    gp.pViewportState = &vp;
    gp.pRasterizationState = &rs;
    gp.pMultisampleState = &ms;
    gp.pDepthStencilState = &ds;
    gp.pColorBlendState = &cb;
    gp.pDynamicState = &dy;
    gp.layout = pipe_layout_;
    VkPipeline p = VK_NULL_HANDLE;
    VKCHECK(vkCreateGraphicsPipelines(dev_, VK_NULL_HANDLE, 1, &gp, nullptr, &p));
    pipe_cache_[h] = p;
    return p;
}

void Impl::clear(const State& s, u32 mask) {
    begin_commands();
    const bool want_depth = (mask & 3) != 0;
    if (!bind_targets(s, want_depth || s.surface().depth_format != 0)) return;
    std::vector<VkClearAttachment> atts;
    const bool partial_color = (mask & 0xF0) && (mask & 0xF0) != 0xF0 && !bound_color_.empty();
    if ((mask & 0xF0) && !partial_color) {
        const u32 v = s.reg(NV4097_SET_COLOR_CLEAR_VALUE);
        for (u32 k = 0; k < bound_color_.size(); ++k) {
            VkClearAttachment a{};
            a.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            a.colorAttachment = k;
            const VkFormat f = bound_color_[k]->image.format;
            const float r = ((v >> 16) & 0xFF) / 255.0f, g = ((v >> 8) & 0xFF) / 255.0f, b = (v & 0xFF) / 255.0f,
                        al = (v >> 24) / 255.0f;
            if (f == VK_FORMAT_R8_UNORM) a.clearValue.color = {{b, 0, 0, 0}};
            else a.clearValue.color = {{r, g, b, al}};
            atts.push_back(a);
        }
    }
    if (want_depth && bound_depth_) {
        const u32 v = s.reg(NV4097_SET_ZSTENCIL_CLEAR_VALUE);
        VkClearAttachment a{};
        const bool z16 = bound_depth_->rsx_format == 1;
        const bool has_stencil = aspect_of(bound_depth_->image.format) & VK_IMAGE_ASPECT_STENCIL_BIT;
        a.aspectMask = ((mask & 1) ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) | ((mask & 2) && has_stencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
        a.clearValue.depthStencil.depth = z16 ? (v & 0xFFFF) / 65535.0f : (v >> 8) / 16777215.0f;
        a.clearValue.depthStencil.stencil = v & 0xFF;
        if (a.aspectMask) atts.push_back(a);
    }
    if (atts.empty() && !partial_color) return;
    const Surface sf = s.surface();
    const u32 w = bound_color_.empty() ? bound_depth_->image.width : bound_color_[0]->image.width;
    const u32 h = bound_color_.empty() ? bound_depth_->image.height : bound_color_[0]->image.height;
    const u32 sh = s.reg(R_SCISSOR_H), sv = s.reg(R_SCISSOR_V);
    u32 x0 = std::max(sf.x, sh & 0xFFFF), y0 = std::max(sf.y, sv & 0xFFFF);
    u32 x1 = std::min(w, sh >> 16 ? (sh & 0xFFFF) + (sh >> 16) : w), y1 = std::min(h, sv >> 16 ? (sv & 0xFFFF) + (sv >> 16) : h);
    if (x0 >= x1 || y0 >= y1) { x0 = 0; y0 = 0; x1 = w; y1 = h; }
    VkClearRect r{{{static_cast<s32>(x0), static_cast<s32>(y0)}, {x1 - x0, y1 - y0}}, 0, 1};
    if (!atts.empty()) vkCmdClearAttachments(cmd_, static_cast<u32>(atts.size()), atts.data(), 1, &r);
    if (partial_color) {
        if (VkPipeline p = clear_pipeline(mask & 0xF0)) {
            const u32 v = s.reg(NV4097_SET_COLOR_CLEAR_VALUE);
            const float c[4] = {((v >> 16) & 0xFF) / 255.0f, ((v >> 8) & 0xFF) / 255.0f, (v & 0xFF) / 255.0f, (v >> 24) / 255.0f};
            vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, p);
            vkCmdPushConstants(cmd_, pipe_layout_, VK_SHADER_STAGE_FRAGMENT_BIT, glsl::kFragmentPushOffset, 16, c);
            VkViewport vpt{0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h), 0.0f, 1.0f};
            vkCmdSetViewport(cmd_, 0, 1, &vpt);
            vkCmdSetScissor(cmd_, 0, 1, &r.rect);
            vkCmdDraw(cmd_, 3, 1, 0, 0);
        }
    }
    for (Target* t : bound_color_)
        if (mask & 0xF0) t->dirty = true;
    if (bound_depth_ && want_depth) bound_depth_->dirty = true;
    ++stats_.clears;
}

void Impl::flip(const State&, u32) {
    flush();
    submit_and_wait();
    bound_color_.clear();
    bound_depth_ = nullptr;
    // Targets keep their GPU contents across frames; writes by the copy
    // engines invalidate them (invalidate()), anything else is not tracked.
    ++epoch_;
    // Drop resources not used for a while.
    if (textures_.size() > 2048) {
        for (auto it = textures_.begin(); it != textures_.end();) {
            if (tick_ - it->second.last_used > 100000) { destroy(it->second.image); it = textures_.erase(it); }
            else ++it;
        }
    }
}

}  // namespace

std::unique_ptr<VulkanBackend> create_vulkan_backend(std::function<u32(u32, bool)> to_ea, std::string* error) {
    auto b = std::make_unique<Impl>(std::move(to_ea));
    if (!b->init(error)) return nullptr;
    return b;
}

}  // namespace opentit::rsx

#endif  // OPENTIT_HAS_VULKAN
