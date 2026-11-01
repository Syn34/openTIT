// GLSL -> SPIR-V. Uses glslang when the build has it; otherwise (or with
// OPENTIT_SHADERC=1) loads the shaderc library at run time: shaderc_shared.dll
// from the Vulkan SDK on Windows, libshaderc_shared.so.1 elsewhere.
#include "glsl.h"

#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <mutex>
#include <string>

#ifdef OPENTIT_HAS_GLSLANG
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#endif

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace opentit::rsx::glsl {

namespace {

// ---------------------------------------------------------------- shaderc (C API, loaded dynamically)
struct Shaderc {
    using Compiler = void*;
    using Options = void*;
    using Result = void*;
    Compiler (*compiler_initialize)() = nullptr;
    Options (*options_initialize)() = nullptr;
    void (*options_set_target_env)(Options, int, unsigned) = nullptr;
    Result (*compile_into_spv)(Compiler, const char*, std::size_t, int, const char*, const char*, Options) = nullptr;
    int (*result_get_compilation_status)(Result) = nullptr;
    std::size_t (*result_get_length)(Result) = nullptr;
    const char* (*result_get_bytes)(Result) = nullptr;
    const char* (*result_get_error_message)(Result) = nullptr;
    void (*result_release)(Result) = nullptr;
    Compiler compiler = nullptr;
    Options options = nullptr;
    bool ok = false;
};

void* open_shaderc() {
#if defined(_WIN32)
    if (HMODULE h = LoadLibraryA("shaderc_shared.dll")) return h;
    if (const char* sdk = std::getenv("VULKAN_SDK")) {
        const std::string p = std::string(sdk) + "\\Bin\\shaderc_shared.dll";
        if (HMODULE h = LoadLibraryA(p.c_str())) return h;
    }
    return nullptr;
#else
    for (const char* n : {"libshaderc_shared.so.1", "libshaderc_shared.so", "libshaderc.so.1"})
        if (void* h = dlopen(n, RTLD_NOW | RTLD_LOCAL)) return h;
    return nullptr;
#endif
}

void* sym(void* lib, const char* name) {
#if defined(_WIN32)
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(lib), name));
#else
    return dlsym(lib, name);
#endif
}

Shaderc& shaderc() {
    static Shaderc s;
    static std::once_flag once;
    std::call_once(once, [] {
        void* lib = open_shaderc();
        if (!lib) return;
        auto load = [&](auto& fn, const char* name) {
            fn = reinterpret_cast<std::remove_reference_t<decltype(fn)>>(sym(lib, name));
            return fn != nullptr;
        };
        if (!load(s.compiler_initialize, "shaderc_compiler_initialize") ||
            !load(s.options_initialize, "shaderc_compile_options_initialize") ||
            !load(s.options_set_target_env, "shaderc_compile_options_set_target_env") ||
            !load(s.compile_into_spv, "shaderc_compile_into_spv") ||
            !load(s.result_get_compilation_status, "shaderc_result_get_compilation_status") ||
            !load(s.result_get_length, "shaderc_result_get_length") ||
            !load(s.result_get_bytes, "shaderc_result_get_bytes") ||
            !load(s.result_get_error_message, "shaderc_result_get_error_message") ||
            !load(s.result_release, "shaderc_result_release"))
            return;
        s.compiler = s.compiler_initialize();
        s.options = s.options_initialize();
        if (!s.compiler || !s.options) return;
        s.options_set_target_env(s.options, 0 /*vulkan*/, (1u << 22) | (1u << 12) /*1.1*/);
        s.ok = true;
    });
    return s;
}

std::mutex g_shaderc_mutex;

bool compile_shaderc(bool vertex, const std::string& src, std::vector<u32>* spirv, std::string* log) {
    Shaderc& s = shaderc();
    if (!s.ok) {
        if (log) *log = "no shader compiler: install the Vulkan SDK (shaderc_shared.dll)";
        return false;
    }
    std::lock_guard<std::mutex> lk(g_shaderc_mutex);
    void* r = s.compile_into_spv(s.compiler, src.data(), src.size(), vertex ? 0 : 1, vertex ? "rsx.vert" : "rsx.frag",
                                 "main", s.options);
    if (!r) return false;
    const bool ok = s.result_get_compilation_status(r) == 0;
    if (ok && spirv) {
        const std::size_t n = s.result_get_length(r);
        spirv->resize(n / 4);
        std::memcpy(spirv->data(), s.result_get_bytes(r), n & ~std::size_t(3));
    } else if (!ok && log) {
        *log = s.result_get_error_message(r);
    }
    s.result_release(r);
    return ok;
}

bool prefer_shaderc() {
    const char* e = std::getenv("OPENTIT_SHADERC");
    return e && *e == '1';
}

}  // namespace

#ifdef OPENTIT_HAS_GLSLANG
bool have_compiler() { return true; }

bool compile(bool vertex_stage, const std::string& source, std::vector<u32>* spirv, std::string* log) {
    if (prefer_shaderc()) return compile_shaderc(vertex_stage, source, spirv, log);
    static std::once_flag once;
    std::call_once(once, [] { glslang::InitializeProcess(); });
    const EShLanguage stage = vertex_stage ? EShLangVertex : EShLangFragment;
    glslang::TShader shader(stage);
    const char* text = source.c_str();
    shader.setStrings(&text, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
    const auto messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
    if (!shader.parse(GetDefaultResources(), 450, false, messages)) {
        if (log) *log = shader.getInfoLog();
        return false;
    }
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages)) {
        if (log) *log = program.getInfoLog();
        return false;
    }
    std::vector<unsigned int> words;
    glslang::GlslangToSpv(*program.getIntermediate(stage), words);
    if (spirv) spirv->assign(words.begin(), words.end());
    return true;
}
#else
bool have_compiler() { return shaderc().ok; }
bool compile(bool vertex_stage, const std::string& source, std::vector<u32>* spirv, std::string* log) {
    return compile_shaderc(vertex_stage, source, spirv, log);
}
#endif

}  // namespace opentit::rsx::glsl
