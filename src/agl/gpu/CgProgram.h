// openTIT - Compiled Cg program images (.vpo / .fpo files).
//
// The game's shaders ship as Sony "Cg binary" images: a header, an array of
// parameter records, a profile specific program header and the GPU
// microcode, all addressed by byte offsets from the start of the image.
// The file is loaded into memory as one block and used in place.
//
// FixupEndian() makes the header, parameter and program-header fields
// native-endian. The microcode itself is kept in the console byte order;
// a backend that wants to run it decodes it from that layout.
#pragma once

#include "agl/gpu/GpuTypes.h"

#include <cstddef>
#include <cstdint>

namespace agl::gpu {

// Cg profiles found in the program header.
enum CgProfile : uint32_t {
    kCgProfile_FragmentTypeB = 0x1807,
    kCgProfile_VertexTypeC   = 0x1B59,   // older vertex profile variants
    kCgProfile_FragmentTypeC = 0x1B5E,
    kCgProfile_VertexTypeD   = 0x1B5D,
    kCgProfile_FragmentRsx   = 0x1B5C,   // sce_fp_rsx
    kCgProfile_VertexRsx     = 0x1B5B,   // sce_vp_rsx
};

// Parameter value types (Cg runtime enumeration).
enum CgType : uint32_t {
    kCgType_Float    = 0x415,
    kCgType_Float2   = 0x416,
    kCgType_Float3   = 0x417,
    kCgType_Float4   = 0x418,
    kCgType_Float1x1 = 0x419,
    kCgType_Float4x4 = 0x428,
    kCgType_Half     = 0x443,   // NOTE: the half/fixed ranges are inferred
    kCgType_Sampler2D = 0x45A,  //       from the console's type checks.
    kCgType_Sampler3D = 0x45B,
    kCgType_SamplerCube = 0x45E,
};

// Parameter variability.
enum CgVariability : uint32_t {
    kCgVar_Varying  = 0x1005,
    kCgVar_Uniform  = 0x1006,
    kCgVar_Constant = 0x1007,
};

// The image header. All offsets are relative to the header.
struct CgProgram {
    uint32_t profile;
    uint32_t binaryFormatRevision;
    uint32_t totalSize;
    uint32_t parameterCount;
    uint32_t parameterArray;   // offset of CgParameter[parameterCount]
    uint32_t program;          // offset of CgVertexProgramHeader / CgFragmentProgramHeader
    uint32_t ucodeSize;
    uint32_t ucode;            // offset of the microcode
};

struct CgParameter {
    uint32_t type;             // CgType
    uint32_t resource;         // register / sampler resource
    uint32_t variability;      // CgVariability
    uint32_t resourceIndex;    // constant register or texture unit
    uint32_t name;             // offset of the name string, 0 if none
    uint32_t defaultValue;     // offset of the default values, 0 if none
    uint32_t embeddedConst;    // offset of the embedded constant table (fragment programs)
    uint32_t semantic;         // offset of the semantic string
    uint32_t direction;
    uint32_t paramNumber;
    uint32_t isReferenced;
    uint32_t isShared;
};

struct CgVertexProgramHeader {
    uint32_t instructionCount;
    uint32_t instructionSlot;
    uint32_t registerCount;
    uint32_t attributeInputMask;
    uint32_t attributeOutputMask;
    uint32_t userClipMask;
};

struct CgFragmentProgramHeader {
    uint32_t instructionCount;
    uint32_t attributeInputMask;
    uint32_t partialTexType;
    uint16_t texCoordsInputMask;
    uint16_t texCoords2D;
    uint16_t texCoordsCentroid;
    uint8_t registerCount;
    uint8_t outputFromH0;
    uint8_t depthReplace;
    uint8_t pixelKill;
    uint8_t pad[2];
};

// Embedded constant table: the ucode offsets where one fragment constant
// is stored inline.
struct CgEmbeddedConst {
    uint32_t count;
    uint32_t ucodeOffset[1];   // [count]
};

bool IsVertexProfile(uint32_t profile);
bool IsFragmentProfile(uint32_t profile);

// Converts a freshly loaded image to native field order and writes the
// default values of embedded fragment constants into the microcode.
void FixupEndian(CgProgram* program);

// --- accessors --------------------------------------------------------------

const CgParameter* GetParameters(const CgProgram* program);
const CgParameter* FindParameter(const CgProgram* program, const char* name);
const char* GetParameterName(const CgProgram* program, const CgParameter* param);
const float* GetParameterDefaults(const CgProgram* program, const CgParameter* param);
const CgEmbeddedConst* GetEmbeddedConsts(const CgProgram* program, const CgParameter* param);
const void* GetUCode(const CgProgram* program, uint32_t* outSize);
const CgVertexProgramHeader* GetVertexHeader(const CgProgram* program);
const CgFragmentProgramHeader* GetFragmentHeader(const CgProgram* program);
// Number of temporary registers the program needs.
uint32_t GetRegisterCount(const CgProgram* program);
// Overrides the register count recorded in a fragment program image.
void SetFragmentRegisterCount(CgProgram* program, uint8_t count);

// Builds the fragment program description for the microcode copy at
// 'ucode'. Fails for images that are not valid fragment programs.
bool GetFragmentProgramDesc(const CgProgram* program, FragmentProgramDesc* desc, GpuPtr ucode);

// Builds the vertex program description (microcode is used in place).
bool GetVertexProgramDesc(const CgProgram* program, VertexProgramDesc* desc);

// Stores one constant into fragment microcode in the console layout
// (big-endian words with their 16-bit halves exchanged).
void StoreFragmentConstant(void* ucodeSlot, const float value[4]);

} // namespace agl::gpu
