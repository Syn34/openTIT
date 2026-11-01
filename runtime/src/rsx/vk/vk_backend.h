// Vulkan renderer for the RSX.
//
// Draws offscreen with dynamic rendering. RSX render targets are Vulkan
// images keyed by their address and format, filled from guest memory at
// first use and kept on the GPU after that. A texture that matches a target
// samples the target image directly (render-to-texture without a readback).
// Targets go back to guest memory only when something there reads them:
// before the copy engines run, when a texture overlaps them without
// matching, and at each flip (once per frame for every target drawn).
#pragma once

#include <functional>
#include <memory>
#include <string>

#include "../rsx.h"

namespace opentit::rsx {

struct VulkanStats {
    u64 draws = 0, clears = 0, flushes = 0, pipelines = 0, shaders = 0, textures = 0, shader_failures = 0, rt_samples = 0;
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
