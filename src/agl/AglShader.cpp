// openTIT - Agl shader programs: loading, caching and fragment constant patching.
#include "agl/AglShader.h"

#include "agl/AglState.h"
#include "ape/core/ApeString.h"
#include "ape/file/ApeFile.h"

#include <cstdio>
#include <cstring>
#include <map>

namespace agl {

namespace {

// 0x00F6748C: constants shared by all fragment programs.
const AglGlobalShaderParam kGlobalParams[] = {
    { "gvLightData[0]", 16 },  { "gvLightData[1]", 17 },  { "gvLightData[2]", 18 },
    { "gvLightData[3]", 19 },  { "gvLightData[4]", 20 },  { "gvLightData[5]", 21 },
    { "gvLightData[6]", 22 },  { "gvLightData[7]", 23 },  { "gvLightData[8]", 24 },
    { "gvLightData[9]", 25 },  { "gvLightData[10]", 26 }, { "gvLightData[11]", 27 },
    { "gVecFogColor", 32 },
    { nullptr, 0 },
};

std::map<uint32_t, AglVertexProgram*> g_vertexPrograms;     // 0x010F1350
std::map<uint32_t, AglFragmentProgram*> g_fragmentPrograms; // 0x010F1368

// Double-buffered per-frame copy area for patched fragment programs
// (0x010F1338).
struct PatchBuffer {
    uint32_t size = 0;
    uint32_t index = 0;
    AglMemBlock buffers[2];
    int32_t freeOffset = 0;       // allocation goes downwards from the end
    int32_t lowestFree = 0;
};
PatchBuffer g_patch;

gpu::CgProgram* LoadProgramImage(const char* format, const char* name)
{
    char path[256];
    std::snprintf(path, sizeof(path), format, name);
    uint32_t size = 0;
    return static_cast<gpu::CgProgram*>(ape::file::LoadFile(path, &size));
}

// Parameter types whose values can be patched into the microcode: float
// vectors and the other embedded constant types the shaders use.
bool IsPatchableType(uint32_t type)
{
    return (type >= 0x415 && type <= 0x418) || type == 0x443 || (type >= 0x45A && type <= 0x45E);
}

void ResolveParam(AglFragmentProgram* fp, const AglGlobalShaderParam* entry)
{
    AglShaderParam& p = fp->params[entry->slot];
    p.global = entry;
    p.param = gpu::FindParameter(fp->program, entry->name);
    if (!p.param)
        return;
    p.resourceIndex = static_cast<int32_t>(p.param->resourceIndex);
    // Only parameters that are actually embedded in the microcode count.
    const gpu::CgEmbeddedConst* embedded = gpu::GetEmbeddedConsts(fp->program, p.param);
    if (p.resourceIndex == -1 || !embedded || embedded->count == 0)
        p.param = nullptr;
}

} // namespace

const AglGlobalShaderParam* GetGlobalShaderParams()
{
    return kGlobalParams;
}

// @func 0x00533EE4 AglVertexProgram::Create
AglVertexProgram* CreateVertexProgram(gpu::CgProgram* program)
{
    AglVertexProgram* vp = new AglVertexProgram();
    gpu::FixupEndian(program);
    vp->program = program;
    uint32_t ucodeSize = 0;
    vp->ucode = gpu::GetUCode(program, &ucodeSize);
    vp->ucodeSize = static_cast<uint16_t>(ucodeSize);
    vp->refCount = 1;
    // Record the load once; binding the program is a call to this block.
    vp->commands.Reset();
    vp->commands.SetVertexProgram(*program, vp->ucode);
    vp->commands.Return();
    return vp;
}

// @func 0x00533BC0 AglFragmentProgram::Create
AglFragmentProgram* CreateFragmentProgram(gpu::CgProgram* program)
{
    AglFragmentProgram* fp = new AglFragmentProgram();
    gpu::FixupEndian(program);
    fp->program = program;
    uint32_t ucodeSize = 0;
    const void* ucode = gpu::GetUCode(program, &ucodeSize);
    fp->ucodeSize = static_cast<uint16_t>(ucodeSize);
    // The GPU reads fragment programs from GPU memory: keep a copy there.
    MemBlockAlloc(&fp->ucode, fp->ucodeSize, 0x80, gpu::MemoryLocation::Main, kAglMem_Shader);
    std::memcpy(fp->ucode.data, ucode, fp->ucodeSize);
    fp->refCount = 1;
    gpu::GetFragmentProgramDesc(program, &fp->desc, fp->ucode.Ptr());
    return fp;
}

// @func 0x00533978 AglFragmentProgram::BindParams
void BindShaderParams(AglFragmentProgram* fp, const AglGlobalShaderParam* extra)
{
    for (const AglGlobalShaderParam* entry = kGlobalParams; entry->name; ++entry)
        ResolveParam(fp, entry);
    if (extra) {
        for (const AglGlobalShaderParam* entry = extra; entry->name; ++entry)
            ResolveParam(fp, entry);
    }
    // Number the used slots consecutively; constants are stored densely.
    fp->constantCount = 0;
    for (AglShaderParam& p : fp->params) {
        if (p.param)
            p.constant = static_cast<int32_t>(fp->constantCount++);
    }
    BuildPatchTable(fp);
}

// @func 0x0053378C AglFragmentProgram::BuildPatchTable
void BuildPatchTable(AglFragmentProgram* fp)
{
    std::vector<uint32_t> table;
    for (AglShaderParam& p : fp->params) {
        if (!p.param)
            continue;
        const gpu::CgEmbeddedConst* embedded = gpu::GetEmbeddedConsts(fp->program, p.param);
        if (!embedded || embedded->count == 0) {
            p.param = nullptr;
            continue;
        }
        // The console gave up on the whole table at the first parameter of
        // a type it cannot patch.
        if (!IsPatchableType(p.param->type))
            return;
        table.push_back(embedded->count);
        for (uint32_t i = 0; i < embedded->count; ++i)
            table.push_back(embedded->ucodeOffset[i]);
    }
    table.push_back(0);
    fp->patchTable = std::move(table);
}

// @func 0x00534074 AglShader::LoadVertexProgram
AglVertexProgram* LoadVertexProgram(const char* name)
{
    const uint32_t hash = ape::HashNameNoCase(name);
    auto it = g_vertexPrograms.find(hash);
    if (it != g_vertexPrograms.end())
        return it->second;
    gpu::CgProgram* image = LoadProgramImage("System\\Shaders\\%s_V.vpo", name);
    if (!image)
        return nullptr;
    AglVertexProgram* vp = CreateVertexProgram(image);
    g_vertexPrograms[hash] = vp;
    vp->attributeInputMask = gpu::GetVertexAttribInputMask(vp->program);
    return vp;
}

// @func 0x00533DF8 AglShader::LoadFragmentProgram
AglFragmentProgram* LoadFragmentProgram(const char* name, const AglGlobalShaderParam* extra)
{
    const uint32_t hash = ape::HashNameNoCase(name);
    auto it = g_fragmentPrograms.find(hash);
    if (it != g_fragmentPrograms.end())
        return it->second;
    gpu::CgProgram* image = LoadProgramImage("System\\Shaders\\%s.fpo", name);
    if (!image)
        return nullptr;
    AglFragmentProgram* fp = CreateFragmentProgram(image);
    g_fragmentPrograms[hash] = fp;
    BindShaderParams(fp, extra);
    return fp;
}

// @func 0x00534160 AglShader::LoadPrograms
void LoadShaderPrograms(AglVertexProgram** vp, AglFragmentProgram** fp, const char* name,
                        const AglGlobalShaderParam* extra)
{
    *vp = LoadVertexProgram(name);
    *fp = LoadFragmentProgram(name, extra);
}

// @func 0x0053A9D8 AglShader::Load
void ShaderLoad(AglShader* shader, const char* name, const char* fragmentName, const AglGlobalShaderParam* extra)
{
    ape::StrCopyBounded(shader->name, name, sizeof(shader->name));
    shader->vertexProgram = LoadVertexProgram(name);
    shader->fragmentProgram = LoadFragmentProgram(fragmentName ? fragmentName : name, extra);
}

// @func 0x0053ABB8 AglShader::LinkPrograms
void LinkShaderPrograms(AglShader* shader, uint32_t texCoordMask)
{
    constexpr uint32_t kTexCoordBits = 0xFC0;
    const uint32_t written = gpu::GetAttribOutputMask(shader->vertexProgram->program);
    AglFragmentProgram* fp = shader->fragmentProgram;
    gpu::SetAttribOutputMask(fp->program, (texCoordMask & kTexCoordBits) | (written & ~kTexCoordBits));
    gpu::GetFragmentProgramDesc(fp->program, &fp->desc, fp->ucode.Ptr());
}

// @func 0x0052F270 AglShader::SetConstant
void SetShaderConstant(uint32_t slot, const float value[4])
{
    AglRenderState& state = GetRenderState();
    AglFragmentProgram* fp = state.fragmentProgram;
    if (!fp || slot >= uint32_t(kMaxShaderParams) || !fp->params[slot].param)
        return;
    const int32_t c = fp->params[slot].constant;
    const bool changed = std::memcmp(fp->constants[c], value, sizeof(fp->constants[c])) != 0;
    std::memcpy(fp->constants[c], value, sizeof(fp->constants[c]));
    // A program last uploaded in another frame must be uploaded again.
    if (changed || fp->uploadFrame != state.frame)
        fp->dirtyMask |= uint64_t(1) << c;
}

// @func 0x0052F4F0 AglShader::UploadFragmentConstants
void UploadFragmentConstants(AglFragmentProgram* fp)
{
    AglRenderState& state = GetRenderState();

    // Carve the copy out of this frame's patch buffer (128-byte aligned).
    const int32_t size = static_cast<int32_t>((fp->ucodeSize + 0x7F) & ~0x7F);
    g_patch.freeOffset = (g_patch.freeOffset - size) & ~0x7F;
    AglMemBlock& buffer = g_patch.buffers[g_patch.index];

    // Patch each changed constant into every place the microcode reads it.
    uint8_t* ucode = static_cast<uint8_t*>(fp->ucode.data);
    const uint32_t* entry = fp->patchTable.data();
    uint64_t dirty = fp->dirtyMask;
    for (uint32_t c = 0; dirty != 0 && entry && *entry != 0; ++c, dirty >>= 1) {
        const uint32_t count = *entry++;
        if (dirty & 1) {
            for (uint32_t i = 0; i < count; ++i)
                gpu::StoreFragmentConstant(ucode + entry[i], fp->constants[c]);
        }
        entry += count;
    }
    if (state.fragmentProgram)
        state.fragmentProgram->dirtyMask = 0;
    fp->uploadFrame = state.frame;

    if (!buffer.data || g_patch.freeOffset < 0) {
        // Out of patch space (or no buffer): bind the master copy.
        Cmd().SetFragmentProgramAddress(fp->ucode.Ptr());
        return;
    }
    uint8_t* copy = static_cast<uint8_t*>(buffer.data) + g_patch.freeOffset;
    std::memcpy(copy, ucode, fp->ucodeSize);
    Cmd().SetFragmentProgramAddress(buffer.Ptr(static_cast<size_t>(g_patch.freeOffset)));
}

void InitShaderPatchBuffer(uint32_t size)
{
    g_patch.size = size;
    for (AglMemBlock& buffer : g_patch.buffers)
        MemBlockAlloc(&buffer, size, 0x80, gpu::MemoryLocation::Main, kAglMem_Shader);
    g_patch.freeOffset = static_cast<int32_t>(size) - 0x200;
    g_patch.lowestFree = g_patch.freeOffset;
    g_patch.index = 0;
}

void ResetShaderPatchBuffer()
{
    // Track the low-water mark and report it when it drops by a kilobyte step.
    const int32_t lowest = g_patch.freeOffset < g_patch.lowestFree ? g_patch.freeOffset : g_patch.lowestFree;
    if ((lowest >> 12) < (g_patch.lowestFree >> 12))
        std::printf("INFO: Shader patch buffer: %3dk free\n", lowest >> 10);
    g_patch.lowestFree = lowest;
    g_patch.freeOffset = static_cast<int32_t>(g_patch.size) - 0x200;
    g_patch.index ^= 1;
    ++GetRenderState().frame;
}

} // namespace agl
