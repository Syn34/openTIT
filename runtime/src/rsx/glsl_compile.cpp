// GLSL -> SPIR-V through glslang (optional dependency).
#include "glsl.h"

#ifdef OPENTIT_HAS_GLSLANG
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <mutex>
#endif

namespace opentit::rsx::glsl {

#ifdef OPENTIT_HAS_GLSLANG
bool have_compiler() { return true; }

bool compile(bool vertex_stage, const std::string& source, std::vector<u32>* spirv, std::string* log) {
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
bool have_compiler() { return false; }
bool compile(bool, const std::string&, std::vector<u32>*, std::string* log) {
    if (log) *log = "openTIT was built without glslang";
    return false;
}
#endif

}  // namespace opentit::rsx::glsl
