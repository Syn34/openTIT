// openTIT - Ape engine input devices (game pads) and controller recording.
//
// ApeInputDevice holds the engine side state of one controller: a button
// mask using the engine's bit layout (ApeButton), 24 analog values, and two
// rumble channels. ApePadDevice fills it from a pad port through the
// PadBackend interface (cellPad on the console).
//
// Every Poll() can be mirrored to or replayed from "controller.bin"
// (SetControllerRecordMode), which the developers used for deterministic
// input playback.
#pragma once

#include "ape/input/PadDevice.h"
#include "ape/memory/ApeMem.h"

#include <cstdint>

namespace ape::input {

// Engine button bits (ApeInputDevice::GetButtons()).
enum ApeButton : uint32_t {
    kButton_Up       = 0x0001,
    kButton_Down     = 0x0002,
    kButton_Left     = 0x0004,
    kButton_Right    = 0x0008,
    kButton_Triangle = 0x0010,
    kButton_Square   = 0x0020,
    kButton_Circle   = 0x0040,
    kButton_Cross    = 0x0080,
    kButton_L1       = 0x0100,
    kButton_R1       = 0x0200,
    kButton_L2       = 0x0400,
    kButton_R2       = 0x0800,
    kButton_L3       = 0x1000,
    kButton_R3       = 0x2000,
    kButton_Start    = 0x4000,
    kButton_Select   = 0x8000,
};

// Indices into the analog value array. Bit i of GetAnalogValidMask() tells
// whether value i came from the hardware this frame.
enum ApeAnalog : int {
    kAnalog_LeftX = 0,      // sticks, -1..1
    kAnalog_LeftY,
    kAnalog_RightX,
    kAnalog_RightY,
    kAnalog_PressUp,        // button pressure, 0..1 (4..15 follow kButton_* order)
    kAnalog_PressDown,
    kAnalog_PressLeft,
    kAnalog_PressRight,
    kAnalog_PressTriangle,
    kAnalog_PressSquare,
    kAnalog_PressCircle,
    kAnalog_PressCross,
    kAnalog_PressL1,
    kAnalog_PressR1,
    kAnalog_PressL2,
    kAnalog_PressR2,
    kAnalog_Unused16,
    kAnalog_Unused17,
    kAnalog_SensorX = 18,   // motion sensors, -1..1
    kAnalog_SensorY,
    kAnalog_SensorZ,
    kAnalog_SensorG,
    kAnalog_Unused22,
    kAnalog_Unused23,
    kAnalog_Count
};

enum ApeRumbleMotor : int {
    kRumble_Small = 0,
    kRumble_Large = 1,
    kRumble_Count
};

// Device flag bits.
enum ApeInputDeviceFlags : uint16_t {
    kDeviceFlag_Active    = 0x4000,  // NOTE: set by the constructor, never cleared here
    kDeviceFlag_Connected = 0x8000,
};

class ApeInputDevice;
// Writes the device state to, or replaces it from, the controller recording.
void RecordOrReplayDeviceState(ApeInputDevice& device);

class ApeInputDevice : public ApeMemObject {
public:
    ApeInputDevice();
    virtual ~ApeInputDevice();

    ApeInputDevice(const ApeInputDevice&) = delete;
    ApeInputDevice& operator=(const ApeInputDevice&) = delete;

    // Clears buttons, analog values and rumble, and re-enables rumble.
    virtual void Reset();
    // Moves each rumble intensity by its rate (per second), clamped to 0..1.
    virtual void UpdateRumble();
    // Reads the device, then records/replays the state and updates the
    // button change masks and rumble.
    virtual void Poll();
    // @func 0x007A4D4C ApeInputDevice::VirtualSlot5
    virtual void VirtualSlot5() {}           // NOTE: empty in every device class
    // @func 0x007A4D50 ApeInputDevice::VirtualSlot6
    virtual void VirtualSlot6() {}           // NOTE: empty in every device class
    // True while the motor fades out (positive intensity, negative rate).
    virtual bool IsRumbleActive(int motor) const;
    virtual void StopRumble(int motor);
    // Pausing disables rumble output (the state is kept).
    virtual void SetRumblePaused(bool paused);
    // @func 0x007A4D54 ApeInputDevice::VirtualSlot10
    virtual int VirtualSlot10() { return 0; } // NOTE: returns 0 in every device class
    // @func 0x007A4D5C ApeInputDevice::VirtualSlot11
    virtual void VirtualSlot11() {}          // NOTE: empty in every device class
    // NOTE: 0 for the base class, 1 for pads; exact meaning unknown.
    // @func 0x007A4D60 ApeInputDevice::IsPad
    virtual int IsPad() const { return 0; }

    // Starts a rumble that fades from `intensity` (clamped to 0..1) to zero
    // over `duration` seconds. A duration of 0 or less stops after one frame
    // at 60 Hz, FLT_MAX (or more) keeps the intensity constant.
    void StartRumble(float intensity, float duration, int motor);

    void SetRumble(int motor, float intensity, float rate)
    {
        m_rumble[motor] = intensity;
        m_rumbleRate[motor] = rate;
    }
    float GetRumble(int motor) const { return m_rumble[motor]; }

    bool IsConnected() const { return (m_flags & kDeviceFlag_Connected) != 0; }
    uint16_t GetFlags() const { return m_flags; }
    uint32_t GetDeviceClass() const { return m_deviceClass; }
    uint8_t GetPort() const { return m_port; }

    uint32_t GetButtons() const { return m_buttons; }
    uint32_t GetPreviousButtons() const { return m_prevButtons; }
    // Buttons whose state changed in the last Poll().
    uint32_t GetChangedButtons() const { return m_changed; }
    uint32_t GetPressedButtons() const { return m_buttons & m_changed; }
    uint32_t GetReleasedButtons() const { return m_released; }
    float GetAnalog(int index) const { return m_analog[index]; }
    uint32_t GetAnalogValidMask() const { return m_analogValid; }

protected:
    void ClearState();
    // Recording/playback, button change masks and rumble update.
    void FinishPoll();

    uint16_t m_flags = 0;               // +0x04 ApeInputDeviceFlags
    uint32_t m_deviceClass = 0;         // +0x08 NOTE: pads set 4 when the port reports no capability, else 0
    uint8_t m_port = 0;                 // +0x0C pad port, assigned in creation order
    uint32_t m_buttons = 0;             // +0x10 ApeButton bits
    uint32_t m_changed = 0;             // +0x14
    uint32_t m_released = 0;            // +0x18
    uint32_t m_prevButtons = 0;         // +0x1C
    float m_analog[kAnalog_Count] = {}; // +0x20
    uint32_t m_analogValid = 0;         // +0x80
    float m_rumble[kRumble_Count] = {};      // +0x84 intensity 0..1
    float m_rumbleRate[kRumble_Count] = {};  // +0x8C change per second
    bool m_rumbleEnabled = true;        // +0x94

    friend void RecordOrReplayDeviceState(ApeInputDevice& device);
};

// A controller on a pad port.
class ApePadDevice final : public ApeInputDevice {
public:
    ApePadDevice();
    ~ApePadDevice() override;

    void Reset() override;
    void UpdateRumble() override;
    void Poll() override;
    bool IsRumbleActive(int motor) const override;
    void StopRumble(int motor) override;
    void SetRumblePaused(bool paused) override;
    // @func 0x007A5DD4 ApePadDevice::IsPad
    int IsPad() const override { return 1; }

    // Engine button mask for a cellPad digital word (digital2 << 8 | digital1).
    static uint32_t ConvertButtons(uint32_t digital);

private:
    // Pressure values 0/1 for the digital buttons (used when the pad has no
    // pressure sensors).
    void SetDigitalPressure(uint32_t buttons);
    void ClearActuator();
    void SendActuator();

    PadRawData m_raw;                   // +0x98 last sample (CellPadData)
    PadActuator m_actuator;             // +0x11C
    float m_smallMotorPhase = 0.0f;     // +0x124
};

// Forgets the cached pad port information (part of PadSystemInit).
void ResetPadSharedState();

// Controller recording ("controller.bin" in the data root).
enum ControllerRecordMode : int {
    kRecord_Off = 0,
    kRecord_Write = 1,
    kRecord_Playback = 2,
};
// Closes any open recording and starts a new one. Recording and playback
// also lock the frame clock to 1/60 s so replays are deterministic.
void SetControllerRecordMode(int mode);
int GetControllerRecordMode();
// Makes the next ApeInputMgr::Update() switch recording off.
// NOTE: the console flag (0x01164AAC) is set by code outside this module.
void RequestControllerRecordStop();
bool ConsumeControllerRecordStop();

} // namespace ape::input
