#include "vk_loader.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace opentit::rsx::vkl {

#define X(name) PFN_##name name = nullptr;
#include "vk_functions.inc"
#undef X
PFN_vkCreateInstance vkCreateInstance = nullptr;

namespace {
PFN_vkGetInstanceProcAddr g_gipa = nullptr;
}

bool load_library() {
    if (g_gipa) return true;
#if defined(_WIN32)
    HMODULE lib = LoadLibraryA("vulkan-1.dll");
    if (!lib) return false;
    g_gipa = reinterpret_cast<PFN_vkGetInstanceProcAddr>(reinterpret_cast<void*>(GetProcAddress(lib, "vkGetInstanceProcAddr")));
#else
    void* lib = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!lib) lib = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!lib) return false;
    g_gipa = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(lib, "vkGetInstanceProcAddr"));
#endif
    if (!g_gipa) return false;
    vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(g_gipa(nullptr, "vkCreateInstance"));
    return vkCreateInstance != nullptr;
}

void load_instance(VkInstance instance) {
#define X(name) name = reinterpret_cast<PFN_##name>(g_gipa(instance, #name));
#include "vk_functions.inc"
#undef X
}

}  // namespace opentit::rsx::vkl
