// openTIT - Game pad backend interface (replaces Sony's libpad / cellPad).
//
// The engine's pad device (ApePadDevice) talks to the hardware only through
// this interface. Its data layout follows cellPad so the engine keeps its
// original conversion code: digital buttons use the CELL_PAD_CTRL_* bits,
// sticks and pressure values are 0..255 and motion sensors 0..1023.
//
// A PC frontend (SDL, XInput, ...) implements PadBackend and installs it with
// SetPadBackend(). The default backend reports no controller connected, so
// the engine builds and runs without any external library.
#pragma once

#include <cstdint>

namespace ape::input {

constexpr int kMaxPadPorts = 7;   // CELL_PAD_MAX_PORT_NUM

// PadPortInfo::status
enum PadPortStatus : uint32_t {
    kPadStatus_Connected = 0x1,
};

// PadPortInfo::setting / PadBackend::SetPortSetting()
enum PadPortSetting : uint32_t {
    kPadSetting_PressMode  = 0x2,   // report pressure sensitive button values
    kPadSetting_SensorMode = 0x4,   // report motion sensor values
};

// PadPortInfo::capability
enum PadCapability : uint32_t {
    kPadCap_PS3Conformity = 0x01,
    kPadCap_PressMode     = 0x02,
    kPadCap_SensorMode    = 0x04,
    kPadCap_HpAnalogStick = 0x08,
    kPadCap_Actuator      = 0x10,
};

struct PadPortInfo {
    uint32_t status = 0;
    uint32_t setting = 0;
    uint32_t capability = 0;
    uint32_t deviceType = 0;
};

struct PadInfo {
    uint32_t maxConnect = kMaxPadPorts;
    uint32_t nowConnect = 0;
    uint32_t systemInfo = 0;
    PadPortInfo ports[kMaxPadPorts];
};

// Bits of PadRawData::digital1 (CELL_PAD_CTRL_*).
enum PadDigital1 : uint16_t {
    kPadCtrl_Select = 0x01,
    kPadCtrl_L3     = 0x02,
    kPadCtrl_R3     = 0x04,
    kPadCtrl_Start  = 0x08,
    kPadCtrl_Up     = 0x10,
    kPadCtrl_Right  = 0x20,
    kPadCtrl_Down   = 0x40,
    kPadCtrl_Left   = 0x80,
};

// Bits of PadRawData::digital2.
enum PadDigital2 : uint16_t {
    kPadCtrl_L2       = 0x01,
    kPadCtrl_R2       = 0x02,
    kPadCtrl_L1       = 0x04,
    kPadCtrl_R1       = 0x08,
    kPadCtrl_Triangle = 0x10,
    kPadCtrl_Circle   = 0x20,
    kPadCtrl_Cross    = 0x40,
    kPadCtrl_Square   = 0x80,
};

// One sample of a pad, in the order of CellPadData::button[].
struct PadRawData {
    // Number of valid values; 0 means "nothing changed since the last read".
    int32_t length = 0;
    uint16_t reserved[2] = {};
    uint16_t digital1 = 0;        // PadDigital1 bits
    uint16_t digital2 = 0;        // PadDigital2 bits
    uint16_t rightStickX = 0x80;  // 0..255, 0x80 = centre
    uint16_t rightStickY = 0x80;
    uint16_t leftStickX = 0x80;
    uint16_t leftStickY = 0x80;
    // Pressure, 0..255 (press mode only).
    uint16_t pressRight = 0;
    uint16_t pressLeft = 0;
    uint16_t pressUp = 0;
    uint16_t pressDown = 0;
    uint16_t pressTriangle = 0;
    uint16_t pressCircle = 0;
    uint16_t pressCross = 0;
    uint16_t pressSquare = 0;
    uint16_t pressL1 = 0;
    uint16_t pressR1 = 0;
    uint16_t pressL2 = 0;
    uint16_t pressR2 = 0;
    // Motion sensors, 0..1023, 512 = rest (sensor mode only).
    uint16_t sensorX = 512;
    uint16_t sensorY = 512;
    uint16_t sensorZ = 512;
    uint16_t sensorG = 512;
};

// Vibration request: the small motor is on/off, the large one 0..255.
struct PadActuator {
    uint8_t smallMotor = 0;
    uint8_t largeMotor = 0;
};

class PadBackend {
public:
    virtual ~PadBackend() = default;
    // Fills in the connection state of every port. Returns false on error.
    virtual bool GetInfo(PadInfo& info) = 0;
    // Reads the latest sample of a connected port. Returns false on error
    // (the port is then treated as disconnected for this frame).
    virtual bool GetData(int port, PadRawData& data) = 0;
    // Selects which optional values (PadPortSetting) a port reports.
    virtual void SetPortSetting(int port, uint32_t setting) = 0;
    // Drives the vibration motors of a port with kPadCap_Actuator.
    virtual void SetActuator(int port, const PadActuator& actuator) = 0;
};

// The backend used by the pad devices. Never null: until a frontend installs
// one (or after SetPadBackend(nullptr)), a backend without any pad is used.
PadBackend& GetPadBackend();
void SetPadBackend(PadBackend* backend);

} // namespace ape::input
