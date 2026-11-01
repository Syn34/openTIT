// Vulkan renderer for the RSX.
//
// Draws offscreen with dynamic rendering. RSX render targets are Vulkan
// images keyed by their address and format. They are filled from guest
// memory at first use in a frame, and written back to guest memory when
// the game switches targets, before the copy engines run, when a texture
// overlaps them, and at every flip. That keeps render-to-texture, the 2D
// engines, the CPU and the display path all coherent through guest memory,
// at the cost of a GPU->CPU readback per pass (to be optimised later).
#pragma once

#include <functional>
#include <memory>
#include <string>

#include "../rsx.h"

namespace opentit::rsx {

struct VulkanStats {
    u64 draws = 0, clears = 0, flushes = 0, pipelines = 0, shaders = 0, textures = 0, shader_failures = 0;
};

class VulkanBackend : public Backend {
public:
    ~VulkanBackend() override = default;
    virtual const VulkanStats& stats() const = 0;
    virtual std::string device_name() const = 0;
};

// Returns nullptr (and a reason) if Vulkan, a suitable device, or the shader
// compiler is unavailable.
std::unique_ptr<VulkanBackend> create_vulkan_backend(std::function<u32(u32 offset, bool local)> to_ea,
                                                     std::string* error);

bool have_vulkan();  // built with Vulkan support

}  // namespace opentit::rsx
