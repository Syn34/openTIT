// openTIT - ApeDebug primitives: the engine's debug-draw queue.
//
// Debug primitives (text labels, screen lines and rectangles, plus 3D
// shapes drawn elsewhere) are pooled in two ApeTListAllocator<ApeDebug::Prim>
// pools that alternate every frame: game code adds to the current pool while
// the renderer draws the pool filled during the previous frame.
#pragma once

#include "ape/math/ApeMath.h"
#include "ape/memory/ApeTFreeList.h"

#include <cstdint>

namespace ape {
namespace ApeDebug {

enum PrimType : int32_t {
    kPrim_Text   = 3,   // screen text (AddText3D fills it)
    kPrim_Line   = 7,   // screen line
    kPrim_Rect   = 8,   // filled screen rectangle
    kPrim_Dead   = 9,   // drawn and retired; skipped until the pool is cleared
    // NOTE: the other values are 3D shapes; their renderer is not in this module.
};

struct Prim {
    int32_t type;
    uint8_t color[4];   // r, g, b, a
    uint8_t flag;       // NOTE: set to 1 for text; meaning unknown (depth test?)
    union {
        struct {
            float position[3];   // x, y used on screen
            float scale;
            char text[0x60];     // NUL-terminated, at most 0x5F characters
        } text;
        struct {
            float x0, y0, x1, y1;
            float depth;
        } line;
        struct {
            float x, y, width, height;
            float depth;
        } rect;
    };
};
static_assert(sizeof(Prim) == 0x7C, "ApeDebug::Prim layout");

using PrimList = ApeTFreeList<Prim>;

// One of the two primitive pools (an ApeTListAllocator<Prim> instance):
// the list is created on first use.
struct PrimPool {
    PrimList* list = nullptr;
    uint32_t initialCount = 0x10;

    PrimList& Get();
};

// Takes a primitive from the current pool and fills in the common header.
Prim* AllocPrim(int32_t type, const uint8_t color[4], uint8_t flag);
// Queues a text label.
void AddText3D(float scale, const Vec3& position, const char* text, const uint8_t color[4]);

// Bitmap font of the debug text (0x2C-byte console object).
struct Font;

// Font data the engine embeds (descriptor and BMP image); the PC build
// supplies them through SetFontData before Init.
void SetFontData(const void* descriptor, const void* bmp, uint32_t bmpSize);

// Creates the font, the pools and the shared debug materials. False when
// the font texture cannot be created.
bool Init();
// Releases the pools.
bool Shutdown();
// Draws the screen primitives of the previous frame (2D views).
void DrawScreenPrims();
// Retires the previous frame's 3D primitives once a 3D view was drawn.
void RetireWorldPrims();
// Frees every primitive of the current pool.
void FreeCurrent();
// Starts a new frame: the current pool becomes the one drawn.
void SwapBuffers();
// Empties both pools.
void Clear();

} // namespace ApeDebug
} // namespace ape
