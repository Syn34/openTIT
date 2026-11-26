// openTIT - Ape engine bring-up and engine-wide configuration.
#pragma once

#include <cstdint>

namespace ape {

// Subsystem groups accepted by SystemInit().
enum SystemInitFlags : uint32_t {
    kInit_Core     = 0x1,  // thread ids, memory, file system
    kInit_Graphics = 0x2,  // NOTE: the world object module (0x00588828); Agl is flag 4
    kInit_Audio    = 0x4,  // NOTE: inferred from call order, unverified
    kInit_Input    = 0x8,  // NOTE: inferred from call order, unverified
    kInit_AllButCore = ~kInit_Core,
};

// Engine-wide tunables, filled in by the game's main() before SystemInit().
// The PS3 global lives at 0x010C5A20. Most fields are pool sizes and
// capacities whose consumers have not been reconstructed yet; they keep
// their byte offsets in the name until their meaning is known.
struct EngineConfig {
    const char* gameName = nullptr;   // +0x000 "Sly4"
    float    field_014 = 0.0f;        // +0x014 copied into the runtime context by SystemInit
    bool     field_022 = false;       // +0x022 changes render/tick ordering in the main loop
    bool     field_024 = false;       // +0x024
    bool     field_025 = false;       // +0x025
    float    field_038 = 0.0f;        // +0x038
    uint32_t field_05C = 0;           // +0x05C
    // Light relinking distance defaults for levels without a Light element
    // (SystemInit forces 40/75 when zero).
    float    lightNoRescanDist = 0.0f;   // +0x070
    float    lightNoUpdateDist = 0.0f;   // +0x074
    float    field_088 = 0.0f;        // +0x088
    float    field_08C = 0.0f;        // +0x08C
    uint8_t  field_099 = 0;           // +0x099
    uint8_t  field_09A = 0;           // +0x09A
    uint8_t  field_09B = 0;           // +0x09B
    uint8_t  field_09C = 0;           // +0x09C
    uint8_t  field_09D = 0;           // +0x09D
    bool     field_0A1 = false;       // +0x0A1 set by the -c switch
    bool     field_0A2 = false;       // +0x0A2
    bool     field_0A3 = false;       // +0x0A3
    uint32_t field_0A4 = 0;           // +0x0A4
    uint32_t field_0A8 = 0;           // +0x0A8
    float    field_0AC = 0.0f;        // +0x0AC
    uint32_t field_144 = 0;           // +0x144
    uint32_t field_148 = 0;           // +0x148
    uint32_t field_14C = 0;           // +0x14C
    uint32_t field_150 = 0;           // +0x150
    uint32_t field_154 = 0;           // +0x154
    uint32_t field_158 = 0;           // +0x158
    // World object pool sizes (0 selects the module default).
    uint16_t geomInstanceCapacity = 0;    // +0x168
    uint16_t emitterInstanceCapacity = 0; // +0x16A
    uint16_t lightInstanceCapacity = 0;   // +0x16C
    uint16_t field_16E = 0;           // +0x16E point instance pool size
    uint16_t cameraInstanceCapacity = 0;  // +0x172
    uint16_t splineInstanceCapacity = 0;  // +0x174
    uint16_t geomDefCapacity = 0;         // +0x176
    uint16_t emitterDefCapacity = 0;      // +0x178
    uint16_t lightDefCapacity = 0;        // +0x17A
    uint16_t pointDefCapacity = 0;        // +0x17C
    uint16_t field_17E = 0;               // +0x17E
    uint16_t cameraDefCapacity = 0;       // +0x180
    uint16_t splineDefCapacity = 0;       // +0x182
    uint16_t field_18E = 0;           // +0x18E
    uint32_t field_1A4 = 0;           // +0x1A4 size of the general purpose heap created by SystemInit
    uint32_t field_1A8 = 0;           // +0x1A8
    uint16_t capacities_1AC[15] = {}; // +0x1AC..0x1C8 object pool capacities
    uint32_t field_1CC = 0;           // +0x1CC
    uint32_t field_1D0 = 0;           // +0x1D0
    uint32_t field_1D4 = 0;           // +0x1D4
    uint32_t field_1D8 = 0;           // +0x1D8
    uint32_t field_1DC = 0;           // +0x1DC
    uint32_t field_1E0 = 0;           // +0x1E0
    uint32_t field_1E4 = 0;           // +0x1E4
    uint32_t field_1E8 = 0;           // +0x1E8
    uint32_t field_1EC = 0;           // +0x1EC
    uint32_t field_1F0 = 0;           // +0x1F0
    uint32_t field_1F4 = 0;           // +0x1F4
    uint32_t field_1F8 = 0;           // +0x1F8
    uint32_t field_1FC = 0;           // +0x1FC
    uint32_t field_200 = 0;           // +0x200
    const char* titleId = nullptr;    // +0x208 "BCES01284"
    uint64_t field_210 = 0;           // +0x210 opaque 64-bit constant (purpose unknown)
};

extern EngineConfig g_engineConfig;
extern bool g_engineFlag_10F0EA8;     // set by the -t switch, cleared by main

// Brings up the requested engine subsystems. Returns false on failure.
bool SystemInit(uint32_t flags);

// Empty hooks kept by the shipping build around game init/shutdown.
void PreGameInitHook();
void PostGameShutdownHook();

} // namespace ape
