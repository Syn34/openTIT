// openTIT - <ProcEyes> component (type 0x10, vtable 0x00B1C5D0, 0x24 bytes):
// procedural eyes. Each eye drives float constants of its eye material
// (lid pivots in 28..31, the rest copied from the material); animation
// events named "<eye>_<channel>" set its lid and rotation values.
#pragma once

#include "sly/entity/Entity.h"

#include <cstdint>

namespace agl { class AglMaterial; }

namespace sly {
namespace components {

struct ProcEyesComponentDesc;

class ProcEyesComponent : public entity::EntityComponent {
public:
    // Animation event channels (hash of the text from the first '_' on).
    // NOTE: names unknown; the first two are stored as given, the other two
    // converted from degrees.
    static constexpr uint32_t kChannel_A = 0x55CD7FC2u;   // -> value40
    static constexpr uint32_t kChannel_B = 0x55C04939u;   // -> angle48
    static constexpr uint32_t kChannel_C = 0xFF33E0B8u;   // -> angle44
    static constexpr uint32_t kChannel_D = 0xFF411741u;   // -> value3C

    struct Eye {                             // 0xDC bytes
        uint32_t name = 0;                   // +0x00
        uint32_t materialNames[2] = {};      // +0x04 material name per material set (see SwapMaterials)
        agl::AglMaterial* material = nullptr;   // +0x0C
        int32_t bones[4] = {};               // +0x10 eye, eyelid and two more bones
        float blinking = 0.0f;               // +0x20 1 when the eye blinks on its own
        float blink[3] = {};                 // +0x24 blink timing
        uint8_t field_30[0x0C] = {};
        float value3C = 0.0f;                // +0x3C
        float value40 = 0.0f;                // +0x40
        float angle44 = 0.0f;                // +0x44 radians
        float angle48 = 0.0f;                // +0x48 radians
        float pivots[4] = {};                // +0x4C upper/lower rotation pivots (constants 28..31)
        float constants[32] = {};            // +0x5C material constants
    };

    // 0x0025EBF8.
    explicit ProcEyesComponent(const ProcEyesComponentDesc* desc);
    // 0x002869A0 / 0x00286B0C.
    ~ProcEyesComponent() override;
    // Slot 6 (0x00286B54): queues the eye update on the game world's list 2.
    void Update(float dt) override;
    // Slot 9 (0x002878AC): writes the constants into the materials.
    void Slot9() override;

    // 0x00287260: constant 2 of every eye material.
    void SetConstant2(uint32_t value);
    // 0x00287318: switches every eye to its material of set `set` (0/1).
    void SwapMaterials(uint8_t set);
    // 0x002874E8: an animation event with a value for one eye channel.
    void OnAnimEvent(float value, const char* event);
    // 0x00286BB8 (TODO): per-frame eye update (bones, blinking, constants).
    void UpdateEyes();

    // Controller event callback (0x00287874) and world callback (0x00287988).
    static void AnimEventCallback(float value, void* unused, ProcEyesComponent* eyes, const char* event);
    static void WorldCallback(void* unused, ProcEyesComponent* eyes);

    Eye* eyes = nullptr;                     // +0x18
    uint32_t eyeCount = 0;                   // +0x1C
    bool flag80 = true;                      // +0x20 bit 7 (always set): events never override a blinking eye
    bool flag40 = false;                     // +0x20 bit 6 desc attribute 0x2A9B2CB7

private:
    void LoadMaterial(Eye& eye, uint32_t materialName);
};

} // namespace components
} // namespace sly
