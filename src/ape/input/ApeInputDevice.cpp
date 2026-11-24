// openTIT - Ape engine input devices (game pads) and controller recording.
#include "ape/input/ApeInputDevice.h"

#include "ape/file/ApeFile.h"
#include "ape/sys/ApeFrameClock.h"
#include "platform/Platform.h"

#include <cfloat>
#include <cmath>
#include <cstring>

namespace ape::input {

namespace {

// Next pad port to hand out (0x01164AF0).
int s_nextPort = 0;

// Controller recording state (0x01164AA4 file, 0x01164AA8 mode, 0x01164AAC stop request).
file::File* s_recordFile = nullptr;
int s_recordMode = kRecord_Off;
bool s_recordStopRequested = false;

// Pad port information, refreshed at most once per vertical blank
// (0x0116B2A4 / 0x0116B2A8).
uint64_t s_lastInfoVBlank = ~0ull;
PadInfo s_padInfo;

uint64_t GetVBlankCount()
{
    // cellGcmGetVBlankCount() on the console; 59.94 Hz here.
    return plat::GetSystemTimeUsec() / 16683u;
}

const PadPortInfo& PortInfo(uint8_t port)
{
    static const PadPortInfo s_none;
    return port < kMaxPadPorts ? s_padInfo.ports[port] : s_none;
}

// --- recording file I/O (big-endian, as written by the console) -------------

void PutBE32(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

uint32_t GetBE32(const uint8_t* p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

uint32_t FloatBits(float f)
{
    uint32_t u;
    std::memcpy(&u, &f, sizeof u);
    return u;
}

float BitsFloat(uint32_t u)
{
    float f;
    std::memcpy(&f, &u, sizeof f);
    return f;
}

// Stick value 0..255 to -1..1 with a dead zone of 10 around the centre.
float StickValue(uint16_t raw)
{
    const int v = (int16_t)raw - 0x80;
    if (v > 10)
        return (float)(v - 10) / 117.0f;
    if (v < -10)
        return ((float)(v + 10) + 1.0f) / 117.0f;
    return 0.0f;
}

// Sensor value 0..1023 to -1..1, values within 0.01 of rest read as 0.
float SensorValue(uint16_t raw)
{
    const float v = (float)(int16_t)(raw - 0x200) / 511.0f;
    return std::fabs(v) > 0.01f ? v : 0.0f;
}

// Pressure value 0..255 to 0..1, values up to 0.01 read as 0.
float PressureValue(uint16_t raw)
{
    const float v = (float)(int16_t)raw / 255.0f;
    return std::fabs(v) > 0.01f ? v : 0.0f;
}

class NullPadBackend final : public PadBackend {
public:
    bool GetInfo(PadInfo& info) override
    {
        info = PadInfo();
        return true;
    }
    bool GetData(int, PadRawData& data) override
    {
        data = PadRawData();
        return false;
    }
    void SetPortSetting(int, uint32_t) override {}
    void SetActuator(int, const PadActuator&) override {}
};

NullPadBackend s_nullBackend;
PadBackend* s_backend = nullptr;

} // namespace

PadBackend& GetPadBackend()
{
    return s_backend ? *s_backend : s_nullBackend;
}

void ResetPadSharedState()
{
    s_lastInfoVBlank = ~0ull;
    s_padInfo = PadInfo();
}

void SetPadBackend(PadBackend* backend)
{
    s_backend = backend;
    s_lastInfoVBlank = ~0ull;
}

// --- controller recording ------------------------------------------------------

// Mirrors the replicated part of a device to or from the recording:
// flags (2 bytes), analog values (0x60), analog mask (4), buttons (4).
// @func 0x00562998 ApeInputDevice::RecordOrReplay
void RecordOrReplayDeviceState(ApeInputDevice& device)
{
    if (!s_recordFile || (s_recordMode != kRecord_Write && s_recordMode != kRecord_Playback))
        return;
    const bool writing = (s_recordMode == kRecord_Write);

    uint8_t flags[2];
    uint8_t analog[kAnalog_Count * 4];
    uint8_t mask[4];
    uint8_t buttons[4];

    if (writing) {
        flags[0] = (uint8_t)(device.m_flags >> 8);
        flags[1] = (uint8_t)device.m_flags;
        for (int i = 0; i < kAnalog_Count; ++i)
            PutBE32(&analog[i * 4], FloatBits(device.m_analog[i]));
        PutBE32(mask, device.m_analogValid);
        PutBE32(buttons, device.m_buttons);
        s_recordFile->Write(flags, sizeof flags);
        s_recordFile->Write(analog, sizeof analog);
        s_recordFile->Write(mask, sizeof mask);
        s_recordFile->Write(buttons, sizeof buttons);
        return;
    }

    // NOTE: the console ignores short reads (the buffers then keep the old
    // state); here the old values are kept too.
    flags[0] = (uint8_t)(device.m_flags >> 8);
    flags[1] = (uint8_t)device.m_flags;
    for (int i = 0; i < kAnalog_Count; ++i)
        PutBE32(&analog[i * 4], FloatBits(device.m_analog[i]));
    PutBE32(mask, device.m_analogValid);
    PutBE32(buttons, device.m_buttons);
    s_recordFile->Read(flags, sizeof flags);
    s_recordFile->Read(analog, sizeof analog);
    s_recordFile->Read(mask, sizeof mask);
    s_recordFile->Read(buttons, sizeof buttons);
    device.m_flags = (uint16_t)((flags[0] << 8) | flags[1]);
    for (int i = 0; i < kAnalog_Count; ++i)
        device.m_analog[i] = BitsFloat(GetBE32(&analog[i * 4]));
    device.m_analogValid = GetBE32(mask);
    device.m_buttons = GetBE32(buttons);
}

// @func 0x0055BE90 SetControllerRecordMode
void SetControllerRecordMode(int mode)
{
    s_recordMode = mode;
    if (s_recordFile) {
        s_recordFile->Close();
        s_recordFile = nullptr;
    }
    if (mode == kRecord_Write || mode == kRecord_Playback) {
        GetFrameClock().fixedStep = 1.0f / 60.0f;
        s_recordFile = file::OpenFile("controller.bin", mode == kRecord_Write ? "wb" : "rb");
    }
}

int GetControllerRecordMode()
{
    return s_recordMode;
}

void RequestControllerRecordStop()
{
    s_recordStopRequested = true;
}

bool ConsumeControllerRecordStop()
{
    const bool requested = s_recordStopRequested;
    s_recordStopRequested = false;
    return requested;
}

// --- ApeInputDevice ------------------------------------------------------------

// @func 0x00562DDC ApeInputDevice::ApeInputDevice
ApeInputDevice::ApeInputDevice()
{
    m_flags |= kDeviceFlag_Connected | kDeviceFlag_Active;
    m_port = (uint8_t)s_nextPort++;
}

// @func 0x00562EB0 ApeInputDevice::~ApeInputDevice
// @func 0x00562F08 ApeInputDevice::~ApeInputDevice (base object copy)
// @func 0x00562EC0 ApeInputDevice::~ApeInputDevice (deleting)
ApeInputDevice::~ApeInputDevice() = default;

void ApeInputDevice::ClearState()
{
    m_buttons = 0;
    m_changed = 0;
    m_released = 0;
    std::memset(m_analog, 0, sizeof m_analog);
    m_analogValid = 0;
}

// @func 0x00562BAC ApeInputDevice::Reset
void ApeInputDevice::Reset()
{
    ClearState();
    m_rumble[kRumble_Small] = m_rumble[kRumble_Large] = 0.0f;
    m_rumbleRate[kRumble_Small] = m_rumbleRate[kRumble_Large] = 0.0f;
    m_rumbleEnabled = true;
}

// @func 0x00562C58 ApeInputDevice::UpdateRumble
void ApeInputDevice::UpdateRumble()
{
    if (!m_rumbleEnabled)
        return;
    const float dt = GetFrameClock().deltaTime;
    for (int motor = 0; motor < kRumble_Count; ++motor) {
        float value = m_rumble[motor] + m_rumbleRate[motor] * dt;
        if (value > 1.0f)
            value = 1.0f;
        if (value < 0.0f)
            value = 0.0f;
        m_rumble[motor] = value;
    }
}

void ApeInputDevice::FinishPoll()
{
    RecordOrReplayDeviceState(*this);
    const uint32_t previous = m_prevButtons;
    m_changed = m_buttons ^ previous;
    m_released = previous & m_changed;
    m_prevButtons = m_buttons;
    UpdateRumble();
}

// @func 0x00562F18 ApeInputDevice::Poll
void ApeInputDevice::Poll()
{
    FinishPoll();
}

// @func 0x00563000 ApeInputDevice::StartRumble
void ApeInputDevice::StartRumble(float intensity, float duration, int motor)
{
    if (intensity > 1.0f)
        intensity = 1.0f;
    if (intensity < 0.0f)
        intensity = 0.0f;
    m_rumble[motor] = intensity;

    float rate = 0.0f;
    if (duration <= 0.0f)
        rate = intensity * -60.0f;
    else if (duration < FLT_MAX)
        rate = -intensity / duration;
    m_rumbleRate[motor] = rate;
}

// @func 0x00562F88 ApeInputDevice::IsRumbleActive
bool ApeInputDevice::IsRumbleActive(int motor) const
{
    return m_rumbleRate[motor] < 0.0f && m_rumble[motor] > 0.0f;
}

// @func 0x00563098 ApeInputDevice::StopRumble
void ApeInputDevice::StopRumble(int motor)
{
    m_rumble[motor] = 0.0f;
    m_rumbleRate[motor] = 0.0f;
}

// @func 0x005630C0 ApeInputDevice::SetRumblePaused
void ApeInputDevice::SetRumblePaused(bool paused)
{
    m_rumbleEnabled = !paused;
}

// --- ApePadDevice --------------------------------------------------------------

// @func 0x0056FAC8 ApePadDevice::ApePadDevice
ApePadDevice::ApePadDevice()
{
    PadBackend& backend = GetPadBackend();
    backend.SetPortSetting(m_port, kPadSetting_PressMode | kPadSetting_SensorMode);
    const bool ok = backend.GetInfo(s_padInfo);
    if (ok && (PortInfo(m_port).status & kPadStatus_Connected))
        m_flags |= kDeviceFlag_Connected;
    else
        m_flags &= (uint16_t)~kDeviceFlag_Connected;
}

// @func 0x0056FB88 ApePadDevice::~ApePadDevice
// @func 0x0056FBB8 ApePadDevice::~ApePadDevice (deleting)
ApePadDevice::~ApePadDevice() = default;

void ApePadDevice::ClearActuator()
{
    m_actuator.smallMotor = 0;
    m_actuator.largeMotor = 0;
}

void ApePadDevice::SendActuator()
{
    if (PortInfo(m_port).capability & kPadCap_Actuator)
        GetPadBackend().SetActuator(m_port, m_actuator);
}

// @func 0x0056F95C ApePadDevice::Reset
void ApePadDevice::Reset()
{
    ApeInputDevice::Reset();
    ClearActuator();
    SendActuator();
}

// The small motor only knows on/off, so its intensity is turned into a duty
// cycle: the phase accumulates the intensity and the motor runs on the frames
// where it wraps. The large motor value is sent on those frames only.
// NOTE: so the large motor never runs while the small intensity is 0; this
// matches the console code.
// @func 0x0056F9D8 ApePadDevice::UpdateRumble
void ApePadDevice::UpdateRumble()
{
    ApeInputDevice::UpdateRumble();
    if (!m_rumbleEnabled)
        return;

    m_smallMotorPhase += m_rumble[kRumble_Small];
    bool wrapped = false;
    uint8_t large = 0;
    if (m_smallMotorPhase >= 1.0f) {
        wrapped = true;
        large = (uint8_t)(int)(m_rumble[kRumble_Large] * 255.0f);
        m_smallMotorPhase -= 1.0f;
    }
    m_actuator.smallMotor = (wrapped && m_rumble[kRumble_Small] > 0.0f) ? 1 : 0;
    m_actuator.largeMotor = large;
    SendActuator();
}

// @func 0x0056F7D8 ApePadDevice::ConvertButtons
uint32_t ApePadDevice::ConvertButtons(uint32_t digital)
{
    struct Mapping {
        uint32_t pad;
        uint32_t engine;
    };
    static const Mapping kMap[] = {
        { kPadCtrl_Up,                kButton_Up },
        { kPadCtrl_Down,              kButton_Down },
        { kPadCtrl_Left,              kButton_Left },
        { kPadCtrl_Right,             kButton_Right },
        { kPadCtrl_Start,             kButton_Start },
        { kPadCtrl_Select,            kButton_Select },
        { kPadCtrl_L3,                kButton_L3 },
        { kPadCtrl_R3,                kButton_R3 },
        { kPadCtrl_Triangle << 8u,    kButton_Triangle },
        { kPadCtrl_Square << 8u,      kButton_Square },
        { kPadCtrl_Circle << 8u,      kButton_Circle },
        { kPadCtrl_Cross << 8u,       kButton_Cross },
        { kPadCtrl_L1 << 8u,          kButton_L1 },
        { kPadCtrl_R1 << 8u,          kButton_R1 },
        { kPadCtrl_L2 << 8u,          kButton_L2 },
        { kPadCtrl_R2 << 8u,          kButton_R2 },
    };
    uint32_t buttons = 0;
    for (const Mapping& m : kMap) {
        if (digital & m.pad)
            buttons |= m.engine;
    }
    return buttons;
}

// @func 0x0056F6A8 ApePadDevice::SetDigitalPressure
void ApePadDevice::SetDigitalPressure(uint32_t buttons)
{
    // kAnalog_PressUp..kAnalog_PressR2 follow the order of the first twelve
    // button bits.
    for (int i = 0; i < 12; ++i)
        m_analog[kAnalog_PressUp + i] = (buttons & (1u << i)) ? 1.0f : 0.0f;
}

// @func 0x0056FC28 ApePadDevice::Poll
void ApePadDevice::Poll()
{
    PadBackend& backend = GetPadBackend();

    const uint64_t vblank = GetVBlankCount();
    if (vblank != s_lastInfoVBlank) {
        s_lastInfoVBlank = vblank;
        if (!backend.GetInfo(s_padInfo))
            return;
    }

    const PadPortInfo& port = PortInfo(m_port);
    bool connected = false;
    if (port.status & kPadStatus_Connected)
        connected = backend.GetData(m_port, m_raw);

    m_flags = (uint16_t)((m_flags & ~kDeviceFlag_Connected) | (connected ? kDeviceFlag_Connected : 0));
    if (connected) {
        // A length of 0 means the pad state did not change: keep the old one.
        if (m_raw.length != 0) {
            ClearState();
            m_buttons = ConvertButtons(((uint32_t)m_raw.digital2 << 8) | (m_raw.digital1 & 0xFFu));
            SetDigitalPressure(m_buttons);

            m_analogValid = 0xF;
            m_analog[kAnalog_LeftX] = StickValue(m_raw.leftStickX);
            m_analog[kAnalog_LeftY] = StickValue(m_raw.leftStickY);
            m_analog[kAnalog_RightX] = StickValue(m_raw.rightStickX);
            m_analog[kAnalog_RightY] = StickValue(m_raw.rightStickY);

            if (port.setting & kPadSetting_SensorMode) {
                m_analogValid |= 0x3C0000;
                m_analog[kAnalog_SensorX] = SensorValue(m_raw.sensorX);
                m_analog[kAnalog_SensorY] = SensorValue(m_raw.sensorY);
                m_analog[kAnalog_SensorZ] = SensorValue(m_raw.sensorZ);
                m_analog[kAnalog_SensorG] = SensorValue(m_raw.sensorG);
            }
            if (port.setting & kPadSetting_PressMode) {
                m_analogValid |= 0xFFF0;
                m_analog[kAnalog_PressUp] = PressureValue(m_raw.pressUp);
                m_analog[kAnalog_PressDown] = PressureValue(m_raw.pressDown);
                m_analog[kAnalog_PressLeft] = PressureValue(m_raw.pressLeft);
                m_analog[kAnalog_PressRight] = PressureValue(m_raw.pressRight);
                m_analog[kAnalog_PressTriangle] = PressureValue(m_raw.pressTriangle);
                m_analog[kAnalog_PressSquare] = PressureValue(m_raw.pressSquare);
                m_analog[kAnalog_PressCircle] = PressureValue(m_raw.pressCircle);
                m_analog[kAnalog_PressCross] = PressureValue(m_raw.pressCross);
                m_analog[kAnalog_PressL1] = PressureValue(m_raw.pressL1);
                m_analog[kAnalog_PressR1] = PressureValue(m_raw.pressR1);
                m_analog[kAnalog_PressL2] = PressureValue(m_raw.pressL2);
                m_analog[kAnalog_PressR2] = PressureValue(m_raw.pressR2);
            }
        }
        m_deviceClass = (port.capability == 0) ? 4u : 0u;
    }
    FinishPoll();
}

// @func 0x005704A0 ApePadDevice::IsRumbleActive
bool ApePadDevice::IsRumbleActive(int motor) const
{
    return m_rumbleEnabled && m_smallMotorPhase > 0.0f && ApeInputDevice::IsRumbleActive(motor);
}

// @func 0x00570520 ApePadDevice::StopRumble
void ApePadDevice::StopRumble(int motor)
{
    ApeInputDevice::StopRumble(motor);
    ClearActuator();
    m_smallMotorPhase = 0.0f;
    SendActuator();
}

// @func 0x00570590 ApePadDevice::SetRumblePaused
void ApePadDevice::SetRumblePaused(bool paused)
{
    ApeInputDevice::SetRumblePaused(paused);
    ClearActuator();
    SendActuator();
}

} // namespace ape::input
