// openTIT - Agl shader programs: loading, caching and fragment constant patching.
//
// Shaders are pairs of compiled Cg programs loaded from the Shaders archive:
// "System\Shaders\<name>_V.vpo" (vertex) and "System\Shaders\<name>.fpo"
// (fragment). Programs are cached by name hash and shared.
//
// Fragment program constants are embedded in the fragment microcode on this
// hardware generation. Agl keeps a table of up to 34 named constants per
// program; constants that change are patched into the microcode, and the
// patched program is copied into a per-frame "patch buffer" so that draws
// recorded earlier in the frame keep their own values.
#pragma once

#include "agl/AglTypes.h"
#include "agl/gpu/CgProgram.h"
#include "agl/gpu/CommandList.h"

#include <cstdint>
#include <vector>

namespace agl {

constexpr int kMaxShaderParams = 34;

// A named constant every fragment program may use, mapped to a fixed slot.
struct AglGlobalShaderParam {
    const char* name;
    uint32_t slot;
};

struct AglShaderParam {
    const gpu::CgParameter* param = nullptr;   // null when the program does not use the slot
    int32_t resourceIndex = 0;
    const AglGlobalShaderParam* global = nullptr;
    int32_t constant = -1;                     // index into AglFragmentProgram::constants
};

struct AglFragmentProgram {
    AglMemBlock ucode;                 // GPU copy of the microcode (patched in place)
    gpu::CgProgram* program = nullptr;
    uint16_t ucodeSize = 0;
    int32_t refCount = 1;
    uint64_t dirtyMask = 0;            // constants changed since the last upload
    uint32_t uploadFrame = 0;          // frame of the last upload
    uint32_t constantCount = 0;
    // Patch table: for each used constant, its embedded copy count followed
    // by that many microcode offsets; terminated by 0.
    std::vector<uint32_t> patchTable;
    float constants[kMaxShaderParams][4] = {};
    AglShaderParam params[kMaxShaderParams];
    gpu::FragmentProgramDesc desc;
};

struct AglVertexProgram {
    const void* ucode = nullptr;
    gpu::CgProgram* program = nullptr;
    uint16_t ucodeSize = 0;
    uint16_t registerCount = 0;
    int32_t refCount = 1;
    // Pre-recorded "load this program" block; binding calls it.
    gpu::CommandList commands{"AglVertexProgram"};
};

// A vertex/fragment program pair as used by materials and passes.
struct AglShader {
    uint32_t field_0 = 0;
    AglVertexProgram* vertexProgram = nullptr;
    AglFragmentProgram* fragmentProgram = nullptr;
    char name[32] = {};
};

// Program creation from loaded Cg images (the image must stay alive).
AglVertexProgram* CreateVertexProgram(gpu::CgProgram* program);
AglFragmentProgram* CreateFragmentProgram(gpu::CgProgram* program);

// Resolves the global parameter table (plus an optional null-terminated
// extra table) against the program and builds its patch table.
void BindShaderParams(AglFragmentProgram* fp, const AglGlobalShaderParam* extra);
void BuildPatchTable(AglFragmentProgram* fp);

// Cached loads by shader name.
AglVertexProgram* LoadVertexProgram(const char* name);
AglFragmentProgram* LoadFragmentProgram(const char* name, const AglGlobalShaderParam* extra);
void LoadShaderPrograms(AglVertexProgram** vp, AglFragmentProgram** fp, const char* name,
                        const AglGlobalShaderParam* extra);
// Loads a shader pair; 'fragmentName' may be null to use 'name' for both.
void ShaderLoad(AglShader* shader, const char* name, const char* fragmentName, const AglGlobalShaderParam* extra);

// Sets constant 'slot' of the current fragment program.
void SetShaderConstant(uint32_t slot, const float value[4]);
// Patches changed constants into the program's microcode and binds a copy
// of it from the patch buffer.
void UploadFragmentConstants(AglFragmentProgram* fp);

// Per-frame patch buffer management.
void InitShaderPatchBuffer(uint32_t size);
void ResetShaderPatchBuffer();

const AglGlobalShaderParam* GetGlobalShaderParams();

} // namespace agl
