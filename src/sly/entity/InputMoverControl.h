// openTIT - The control block of the input mover (ActorB17990): shared by
// its definition, placement and instance description.
#pragma once

#include <cstdint>

namespace sly {
namespace entity {

// The control block of the input mover (0x44 bytes, definition +0x1A4,
// placement +0x120): how the player drives it. The words from +0x10 on
// depend on the control mode.
struct InputMoverControl {
    enum Mode : int32_t {
        kMode_Buttons = 1,   // element 0x3257217B: press buttons
        kMode_Stick   = 2,   // element 0x89F1FF19: move a stick
        kMode_Mode3   = 3,   // element 0x89F23F47. NOTE: meaning unknown (a distance)
    };
    float field_00 = 0.0f;          // +0x00 attribute 0x11AA6429 (not negative)
    float field_04 = 0.0f;          // +0x04 attribute 0x26A56A39
    float field_08 = 0.0f;          // +0x08 attribute 0xCB13F34D (not negative)
    int32_t mode = kMode_Buttons;   // +0x0C
    union {
        struct {
            uint32_t buttons;       // +0x10 InputButtonMask of hash 0x3257B96C
            float field_14;         // attribute 0xE1555935
            float field_18;         // attribute 0x636A522B (not negative)
            float field_1C;         // attribute 0xE18D39E5
            float field_20;         // attribute 0x0C28FD40
            uint32_t flags24;       // bit 31: attribute 0x0E0CA78C
        } press;
        struct {
            int32_t stick;          // +0x10 0/1 (0x46BDBD34/35) or 2/3 (0x842320BA/BB) from hash 0xB456B7AA
            // +0x14.. attributes 0x82E789DB, 0x0BF5ADB7 (both not negative),
            // 0x96CB0082, 0x3CE11DA0, 0x46FFF512, 0x31D7B193, 0x036A8EAF,
            // 0x4C005DE1, 0xA4D3EB03, 0x57557B45, 0x8C06F037.
            float values[11];
            uint32_t flags40;       // bits 31/30/29: attributes 0xF6866423, 0x2443C7A5, 0x153912A7
        } stick;
        struct {
            float rangeSq;          // +0x10 square of attribute 0x82E789DB
            float field_14;         // attribute 0x02D738DF
            // +0x18 bit 31: hash 0x04B8E870 (0x04A5C9FC sets, 0x00239807
            // clears); bit 30: hash 0x906DDB56 (0x874 sets, 0x103F7 clears).
            uint32_t flags18;
        } mode3;
        uint32_t words[13] = {};
    };
};
static_assert(sizeof(InputMoverControl) == 0x44, "InputMoverControl is the console block");

} // namespace entity
} // namespace sly
