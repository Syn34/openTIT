// Runtime loading of the Vulkan loader library (vulkan-1.dll / libvulkan.so.1),
// so the renderer needs only the Vulkan headers to build and works with
// whatever driver the machine has.
#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace opentit::rsx::vkl {

#define X(name) extern PFN_##name name;
#include "vk_functions.inc"
#undef X
extern PFN_vkCreateInstance vkCreateInstance;

// Load the library and the global entry points. Returns false if there is no Vulkan.
bool load_library();
// Load everything else through the instance.
void load_instance(VkInstance instance);

}  // namespace opentit::rsx::vkl

using namespace opentit::rsx::vkl;
