#include "input.h"

#include <cstring>
#include <mutex>

#include "opentit/memory.h"

namespace opentit::input {

namespace {
std::mutex g_m;
Pad g_pads[7];
}  // namespace

Pad pad(int port) {
    std::lock_guard<std::mutex> lk(g_m);
    if (port < 0 || port >= 7) return Pad{false};
    Pad p = g_pads[port];
    if (port != 0) p.connected = false;
    return p;
}

void set_pad(int port, const Pad& p) {
    if (port < 0 || port >= 7) return;
    std::lock_guard<std::mutex> lk(g_m);
    g_pads[port] = p;
}

void write_cell_pad_data(u32 ea, const Pad& p) {
    std::memset(mem::ptr(ea), 0, 4 + 128);
    if (!p.connected) return;
    mem::w32(ea, 24);  // CELL_PAD_LEN_CHANGE_SENSOR_ON: digital, analog, pressure, sensors
    auto b = [&](u32 i, u16 v) { mem::w16(ea + 4 + 2 * i, v); };
    b(2, p.digital1);
    b(3, p.digital2);
    b(4, p.rx);
    b(5, p.ry);
    b(6, p.lx);
    b(7, p.ly);
    for (u32 k = 0; k < 12; ++k) b(8 + k, p.pressure[k]);
    b(20, 512);  // sensor X, Y, Z: level
    b(21, 399);
    b(22, 512);
    b(23, 512);  // gyro
}

}  // namespace opentit::input
