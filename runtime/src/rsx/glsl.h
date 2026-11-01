// RSX vertex/fragment programs -> Vulkan GLSL 450.
//
// The generated shaders share a fixed interface so that any vertex program
// pairs with any fragment program:
//   vertex inputs   location n = RSX vertex attribute n (vec4, already
//                   converted to host floats by the backend)
//   varyings        COL0 0, COL1 1, BFC0 2, BFC1 3, FOG 4, TEX0-9 5-14
//   set 0 binding 0 uniform block: vec4 vc[468] (vertex constants)
//   set 0 binding 1 + n: vertex texture n (TXL), binding 16 + n: fragment texture n
//   push constants  vertex: mat4 at offset 0 (clip-space viewport transform),
//                   uint at 64 (transform branch bits)
//                   fragment: from offset 80 (alpha test)
#pragma once

#include <string>
#include <vector>

#include "fragment_program.h"
#include "vertex_program.h"

namespace opentit::rsx::glsl {

enum : u32 {
    LOC_COL0 = 0, LOC_COL1, LOC_BFC0, LOC_BFC1, LOC_FOG, LOC_TEX0,  // TEX9 = 14
    kVaryingCount = 15,
    kVertexConstantBinding = 0,
    kVertexTextureBinding = 1,
    kFragmentTextureBinding = 16,
    kFragmentPushOffset = 80,
};

enum class TexDim : u8 { None = 0, D1, D2, D3, Cube };

struct FragmentOptions {
    TexDim textures[16] = {};      // dimension of each sampled unit (default 2D)
    bool r_exports = true;         // colour comes from R0/R2/R3/R4 (else H0/H4/H6/H8)
    bool two_sided_color = false;  // back faces read BFC0/BFC1
    bool depth_export = false;     // R1.z -> gl_FragDepth
    bool alpha_test = false;       // compare against push-constant reference
};

std::string vertex(const vp::Program& p);
std::string fragment(const fp::Program& p, const FragmentOptions& o);

// GLSL -> SPIR-V. Returns false (with the compiler log) if the shader does
// not compile or the runtime was built without glslang.
bool compile(bool vertex_stage, const std::string& source, std::vector<u32>* spirv, std::string* log);
bool have_compiler();

}  // namespace opentit::rsx::glsl
