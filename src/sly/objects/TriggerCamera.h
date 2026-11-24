// openTIT - Camera trigger volumes: switch the player camera to a scripted
// behaviour ("Spline": a position/target spline pair, or "Track": a tracking
// offset) while a player in an allowed physics state is inside.
#pragma once

#include "sly/objects/Trigger.h"

#include <cstdint>

namespace sly {
namespace objects {

constexpr uint32_t kAttr_EaseIn       = 0x79792193;   // "EaseIn"
constexpr uint32_t kAttr_EaseOut      = 0xB5AB2880;   // "EaseOut"
constexpr uint32_t kAttr_Behavior     = 0x88B4A252;   // "Behavior"
constexpr uint32_t kAttr_SplinePos    = 0xBDE51CC3;   // "SplinePos"
constexpr uint32_t kAttr_SplineTgt    = 0xBDE52AD0;   // "SplineTgt"
constexpr uint32_t kAttr_SplineParent = 0x484957BB;   // "SplineParent"
constexpr uint32_t kAttr_OffsetPos    = 0x313AB0C1;   // "OffsetPos"
constexpr uint32_t kAttr_OffsetRot    = 0x313AB844;   // "OffsetRot" (degrees)
constexpr uint32_t kAttr_Fov          = 0x000110AD;   // "FOV"
constexpr uint32_t kAttr_InvertDir    = 0x2DE05CD7;   // "InvertDir"
constexpr uint32_t kAttr_Name         = 0x0024728B;   // "Name"

constexpr uint32_t kCameraBehavior_Spline = 0x922D3BD1;   // "Spline"
constexpr uint32_t kCameraBehavior_Track  = 0x04C5F92B;   // "Track"

// Physics states a camera volume reacts to (bit mask).
enum CameraStateBits : uint16_t {
    kCamState_Ground = 0x01,
    kCamState_Air    = 0x02,
    kCamState_Climb  = 0x04,
    kCamState_Crawl  = 0x08,
    kCamState_All    = 0xFFFF,
};

// Reads the <PhysicsState Name=".."/> children and the PhysicsState attribute
// into an allowed mask and an excluded mask (shared by the def and placement).
void ParseCameraStates(ape::xml::XmlElement* elem, uint16_t& allowed, uint16_t& excluded);

struct CameraTriggerDefDesc : TriggerDefDesc {
    uint16_t allowedStates = 0xFFFF;   // 0x54
    uint16_t excludedStates = 0;       // 0x56
    uint32_t field_58 = 0;             // 0x58
    explicit CameraTriggerDefDesc(const EntityDefDesc& loader);
    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

class CameraTriggerDef : public TriggerDef {
public:
    explicit CameraTriggerDef(const CameraTriggerDefDesc& desc);
    ObjEntity* Create(EntityPlacement* placement) override;
    uint16_t allowedStates = 0xFFFF;   // 0xB0
    uint16_t excludedStates = 0;       // 0xB2
    uint32_t field_B4 = 0;             // 0xB4
};

class CameraTriggerPlacement : public TriggerPlacement {
public:
    explicit CameraTriggerPlacement(const char* placementName);
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;

    uint32_t field_BC = 0;             // 0xBC
    uint16_t allowedStates = 0xFFFF;   // 0xC0
    uint16_t excludedStates = 0;       // 0xC2
    uint32_t behavior = kCameraBehavior_Spline;   // 0xC4
    float easeIn = -1.0f;              // 0xC8
    float easeOut = -1.0f;             // 0xCC
    uint32_t splinePos = 0;            // 0xD0
    uint32_t splineTgt = 0;            // 0xD4
    uint32_t splineParent = 0;         // 0xD8
    ape::Vec3 offsetPos{};             // 0xE0
    float offsetRot[2] = {};           // 0xF0 (radians)
    float fov = 0.0f;                  // 0xF8
    bool invertDir = false;            // 0xFC
};

struct CameraTriggerInstanceDesc : TriggerInstanceDesc {
    CameraTriggerInstanceDesc(const char* instanceName, CameraTriggerDef* ownerDef);
    uint32_t field_9C = 0;             // 0x9C
    uint16_t allowedStates = 0xFFFF;   // 0xA0
    uint16_t excludedStates = 0;       // 0xA2
    uint32_t behavior = kCameraBehavior_Spline;   // 0xA4
    float easeIn = -1.0f;              // 0xA8
    float easeOut = -1.0f;             // 0xAC
    uint32_t splinePos = 0;            // 0xB0
    uint32_t splineTgt = 0;            // 0xB4
    uint32_t splineParent = 0;         // 0xB8
    ape::Vec3 offsetPos{};             // 0xC0
    float offsetRot[2] = {};           // 0xD0
    float fov = 0.0f;                  // 0xD8
    bool invertDir = false;            // 0xDC
};

class CameraTrigger : public TriggerBase {
public:
    explicit CameraTrigger(const CameraTriggerInstanceDesc& desc);
    ~CameraTrigger() override;
    bool IsDebugDisplayed() const override;

private:
    uint16_t m_allowedStates;          // 0x1AC
    uint16_t m_excludedStates;         // 0x1AE
    uint32_t m_behavior;               // 0x1B0
    uint32_t m_state = 1;              // 0x1B4 (top bits)
    void* m_cameraMode = nullptr;      // 0x1B8
    float m_easeIn;                    // 0x1BC
    float m_easeOut;                   // 0x1C0
    // Spline behaviour: position/target splines and the parent object hash.
    void* m_splinePos = nullptr;       // 0x1C4
    void* m_splineTgt = nullptr;       // 0x1C8
    uint32_t m_splineParent = 0;       // 0x1CC
    // Track behaviour.
    ape::Vec3 m_offsetPos{};           // 0x1C4
    float m_offsetRot[2] = {};         // 0x1D0
    float m_fov = 0.0f;                // 0x1D8
    bool m_invertDir = false;          // 0x1DC
};

EntityDef* CreateCameraTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                  const EntityDefDesc& loader, const EntityDef* parent);
EntityPlacement* ParseCameraTriggerPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                             const char* name);

} // namespace objects
} // namespace sly
