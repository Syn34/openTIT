// Host input -> one DualShock 3.
//
// Filled by the window thread from an SDL game controller (any XInput/
// DualShock/DualSense pad) and the keyboard:
//   WASD left stick     arrows right stick   I/J/K/L d-pad
//   Space Cross   F Square   C Circle   R Triangle
//   Q L1   E R1   Left Shift L2   Left Ctrl R2   Z L3   X R3
//   Enter Start   Backspace Select
#pragma once

#include "common/types.h"

namespace opentit::input {

enum : u16 {
    // CELL_PAD_BTN_OFFSET_DIGITAL1
    BTN_SELECT = 0x01, BTN_L3 = 0x02, BTN_R3 = 0x04, BTN_START = 0x08,
    BTN_UP = 0x10, BTN_RIGHT = 0x20, BTN_DOWN = 0x40, BTN_LEFT = 0x80,
};
enum : u16 {
    // CELL_PAD_BTN_OFFSET_DIGITAL2
    BTN_L2 = 0x01, BTN_R2 = 0x02, BTN_L1 = 0x04, BTN_R1 = 0x08,
    BTN_TRIANGLE = 0x10, BTN_CIRCLE = 0x20, BTN_CROSS = 0x40, BTN_SQUARE = 0x80,
};

struct Pad {
    bool connected = true;  // the game always sees a pad on port 0
    u16 digital1 = 0, digital2 = 0;
    u8 lx = 128, ly = 128, rx = 128, ry = 128;  // 0 = left/up, 255 = right/down
    // Pressure, 0-255: right, left, up, down, triangle, circle, cross, square, L1, R1, L2, R2
    u8 pressure[12] = {};
};

Pad pad(int port);
void set_pad(int port, const Pad& p);  // used by the window thread (and tests)

// Fill CellPadData { s32 len; u16 button[64] } at guest `ea`.
void write_cell_pad_data(u32 ea, const Pad& p);

}  // namespace opentit::input
