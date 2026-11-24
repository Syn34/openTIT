// openTIT - Compiled Cg program images (.vpo / .fpo files).
#include "agl/gpu/CgProgram.h"

#include <cstring>

namespace agl::gpu {

namespace {

uint32_t Swap32(uint32_t v)
{
    return (v >> 24) | ((v >> 8) & 0xFF00u) | ((v << 8) & 0xFF0000u) | (v << 24);
}

uint16_t Swap16(uint16_t v)
{
    return static_cast<uint16_t>((v >> 8) | (v << 8));
}

void SwapWords(uint32_t* words, size_t count)
{
    for (size_t i = 0; i < count; ++i)
        words[i] = Swap32(words[i]);
}

bool HostIsBigEndian()
{
    const uint16_t probe = 1;
    uint8_t first;
    std::memcpy(&first, &probe, 1);
    return first == 0;
}

uint8_t* Bytes(CgProgram* program) { return reinterpret_cast<uint8_t*>(program); }
const uint8_t* Bytes(const CgProgram* program) { return reinterpret_cast<const uint8_t*>(program); }

// Exchanges the bytes inside each 16-bit half of a word (the fragment
// microcode is stored that way relative to plain word order).
void SwapHalfwordBytes(uint32_t* words, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        const uint32_t v = words[i];
        words[i] = ((v & 0x00FF00FFu) << 8) | ((v >> 8) & 0x00FF00FFu);
    }
}

} // namespace

bool IsVertexProfile(uint32_t profile)
{
    return profile == kCgProfile_VertexTypeC || profile == kCgProfile_VertexTypeD ||
           profile == kCgProfile_VertexRsx;
}

bool IsFragmentProfile(uint32_t profile)
{
    return profile == kCgProfile_FragmentTypeB || profile == kCgProfile_FragmentTypeC ||
           profile == kCgProfile_FragmentRsx;
}

void StoreFragmentConstant(void* ucodeSlot, const float value[4])
{
    // Console layout: each 32-bit word is stored big-endian with its two
    // 16-bit halves exchanged.
    uint8_t* out = static_cast<uint8_t*>(ucodeSlot);
    for (int i = 0; i < 4; ++i) {
        uint32_t bits;
        std::memcpy(&bits, &value[i], sizeof(bits));
        const uint32_t swapped = (bits << 16) | (bits >> 16);
        out[i * 4 + 0] = static_cast<uint8_t>(swapped >> 24);
        out[i * 4 + 1] = static_cast<uint8_t>(swapped >> 16);
        out[i * 4 + 2] = static_cast<uint8_t>(swapped >> 8);
        out[i * 4 + 3] = static_cast<uint8_t>(swapped);
    }
}

// @func 0x005DFAF4 cgbFixupEndian
void FixupEndian(CgProgram* program)
{
    // The image is native already when the profile reads correctly.
    const bool native = IsVertexProfile(program->profile) || IsFragmentProfile(program->profile);
    if (!native) {
        // Header.
        SwapWords(reinterpret_cast<uint32_t*>(program), sizeof(CgProgram) / 4);
        const uint32_t profile = program->profile;
        // The microcode is kept in console (big-endian) order: it needs
        // swapping only when the image byte order differs from the console's.
        // The image byte order is the opposite of the host order here.
        const bool imageIsBigEndian = !HostIsBigEndian();

        if (IsVertexProfile(profile)) {
            auto* vp = reinterpret_cast<uint32_t*>(Bytes(program) + program->program);
            SwapWords(vp, sizeof(CgVertexProgramHeader) / 4);
            if (!imageIsBigEndian)
                SwapWords(reinterpret_cast<uint32_t*>(Bytes(program) + program->ucode), program->ucodeSize / 4);
        }
        if (IsFragmentProfile(profile)) {
            auto* fp = reinterpret_cast<CgFragmentProgramHeader*>(Bytes(program) + program->program);
            fp->instructionCount = Swap32(fp->instructionCount);
            fp->attributeInputMask = Swap32(fp->attributeInputMask);
            fp->partialTexType = Swap32(fp->partialTexType);
            fp->texCoordsInputMask = Swap16(fp->texCoordsInputMask);
            fp->texCoords2D = Swap16(fp->texCoords2D);
            fp->texCoordsCentroid = Swap16(fp->texCoordsCentroid);
            if (!imageIsBigEndian)
                SwapHalfwordBytes(reinterpret_cast<uint32_t*>(Bytes(program) + program->ucode), program->ucodeSize / 4);
        }

        // Parameter records, their default values and embedded constant tables.
        auto* params = reinterpret_cast<CgParameter*>(Bytes(program) + program->parameterArray);
        for (uint32_t i = 0; i < program->parameterCount; ++i) {
            CgParameter& p = params[i];
            SwapWords(reinterpret_cast<uint32_t*>(&p), sizeof(CgParameter) / 4);
            if (p.defaultValue)
                SwapWords(reinterpret_cast<uint32_t*>(Bytes(program) + p.defaultValue), 4);
            if (p.embeddedConst) {
                auto* table = reinterpret_cast<uint32_t*>(Bytes(program) + p.embeddedConst);
                table[0] = Swap32(table[0]);
                SwapWords(table + 1, table[0]);
            }
        }
    }

    // Fragment constants live inside the microcode: copy each parameter's
    // default value to every place the program reads it from.
    if (program->ucode == 0)
        return;
    const CgParameter* params = GetParameters(program);
    for (uint32_t i = 0; i < program->parameterCount; ++i) {
        const CgParameter& p = params[i];
        if (!p.embeddedConst || !p.defaultValue)
            continue;
        const auto* table = reinterpret_cast<const CgEmbeddedConst*>(Bytes(program) + p.embeddedConst);
        const auto* value = reinterpret_cast<const float*>(Bytes(program) + p.defaultValue);
        for (uint32_t c = 0; c < table->count; ++c)
            StoreFragmentConstant(Bytes(program) + program->ucode + table->ucodeOffset[c], value);
    }
}

const CgParameter* GetParameters(const CgProgram* program)
{
    return reinterpret_cast<const CgParameter*>(Bytes(program) + program->parameterArray);
}

const char* GetParameterName(const CgProgram* program, const CgParameter* param)
{
    return param->name ? reinterpret_cast<const char*>(Bytes(program) + param->name) : "";
}

const CgParameter* FindParameter(const CgProgram* program, const char* name)
{
    const CgParameter* params = GetParameters(program);
    for (uint32_t i = 0; i < program->parameterCount; ++i) {
        if (params[i].name && std::strcmp(GetParameterName(program, &params[i]), name) == 0)
            return &params[i];
    }
    return nullptr;
}

const float* GetParameterDefaults(const CgProgram* program, const CgParameter* param)
{
    return param->defaultValue ? reinterpret_cast<const float*>(Bytes(program) + param->defaultValue) : nullptr;
}

const CgEmbeddedConst* GetEmbeddedConsts(const CgProgram* program, const CgParameter* param)
{
    return param->embeddedConst
        ? reinterpret_cast<const CgEmbeddedConst*>(Bytes(program) + param->embeddedConst)
        : nullptr;
}

const void* GetUCode(const CgProgram* program, uint32_t* outSize)
{
    if (outSize)
        *outSize = program->ucodeSize;
    return Bytes(program) + program->ucode;
}

const CgVertexProgramHeader* GetVertexHeader(const CgProgram* program)
{
    return IsVertexProfile(program->profile)
        ? reinterpret_cast<const CgVertexProgramHeader*>(Bytes(program) + program->program)
        : nullptr;
}

const CgFragmentProgramHeader* GetFragmentHeader(const CgProgram* program)
{
    return IsFragmentProfile(program->profile)
        ? reinterpret_cast<const CgFragmentProgramHeader*>(Bytes(program) + program->program)
        : nullptr;
}

uint32_t GetRegisterCount(const CgProgram* program)
{
    if (const CgVertexProgramHeader* vp = GetVertexHeader(program))
        return vp->registerCount;
    if (const CgFragmentProgramHeader* fp = GetFragmentHeader(program))
        return fp->registerCount;
    return 0;
}

void SetFragmentRegisterCount(CgProgram* program, uint8_t count)
{
    if (IsFragmentProfile(program->profile))
        reinterpret_cast<CgFragmentProgramHeader*>(Bytes(program) + program->program)->registerCount = count;
}

// @func 0x005DF980 cgbGetFragmentProgramConfiguration
bool GetFragmentProgramDesc(const CgProgram* program, FragmentProgramDesc* desc, GpuPtr ucode)
{
    // Only revision 6 images with a non-empty, 16-byte aligned microcode
    // block are accepted.
    if (program->binaryFormatRevision != 6 || program->ucodeSize == 0 || (program->ucodeSize & 0xF) != 0)
        return false;
    const auto* fp = reinterpret_cast<const CgFragmentProgramHeader*>(Bytes(program) + program->program);
    desc->ucode = ucode;
    desc->ucodeSize = program->ucodeSize;
    // Interpolant 5 (fog) is always routed to fragment programs.
    desc->inputMask = fp->attributeInputMask | 0x20;
    desc->texCoordsInputMask = fp->texCoordsInputMask;
    desc->texCoords2D = fp->texCoords2D;
    desc->texCoordsCentroid = fp->texCoordsCentroid;
    desc->registerCount = fp->registerCount;
    desc->outputFromH0 = fp->outputFromH0 != 0;
    desc->depthReplace = fp->depthReplace != 0;
    desc->pixelKill = fp->pixelKill != 0;
    return true;
}

bool GetVertexProgramDesc(const CgProgram* program, VertexProgramDesc* desc)
{
    const CgVertexProgramHeader* vp = GetVertexHeader(program);
    if (!vp)
        return false;
    desc->ucode = reinterpret_cast<const uint32_t*>(Bytes(program) + program->ucode);
    desc->instructionCount = vp->instructionCount;
    desc->instructionSlot = vp->instructionSlot;
    desc->registerCount = vp->registerCount;
    desc->inputMask = vp->attributeInputMask;
    desc->outputMask = vp->attributeOutputMask;
    return true;
}

} // namespace agl::gpu
