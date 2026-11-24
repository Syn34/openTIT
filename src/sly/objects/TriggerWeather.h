// openTIT - Weather trigger volumes: switch the weather script while a
// player is inside.
//
// Vtables: definition 0x00B1AE70, entity 0x00B1AEA0.
#pragma once

#include "sly/objects/Trigger.h"

#include <cstdint>

namespace ape { class ApeAnimScript; }

namespace sly {
namespace objects {

constexpr uint32_t kDefClass_WeatherTrigger = 0x20000000;
constexpr uint32_t kAttr_WeatherScript = 0xF988A90E;     // NOTE: name unknown (script of the default library)
constexpr uint32_t kAttr_WeatherBlendTime = 0x41A94847;  // NOTE: name unknown (seconds, default 15)

struct WeatherTriggerDefDesc : TriggerDefDesc {
    uint32_t script = 0;         // 0x54
    float blendTime = 15.0f;     // 0x58
    explicit WeatherTriggerDefDesc(const EntityDefDesc& loader);
    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

class WeatherTriggerDef : public TriggerDef {
public:
    explicit WeatherTriggerDef(const WeatherTriggerDefDesc& desc);
    ObjEntity* Create(EntityPlacement* placement) override;

    ape::ApeAnimScript* script = nullptr;   // 0xB0 looked up in the default library
    float blendTime = 15.0f;                // 0xB4
};

class WeatherTrigger : public TriggerBase {
public:
    explicit WeatherTrigger(const TriggerInstanceDesc& desc);
    void Release(bool immediate) override;
    bool IsDebugDisplayed() const override;
    // TODO(0x00222B18 Update, 0x00223890 OnEnter, 0x00223A7C OnExit and the
    // helpers 0x00222654 / 0x002228C8 / 0x00223640): the per-player weather
    // blend is not ported.

private:
    // Weather script instances per player (3 x 3 x 3 slots each, +0x1AC and
    // +0x218). NOTE: the indexing scheme is in the unported update code.
    void* m_scripts[3][3][3] = {};       // 0x1AC
    void* m_scripts2[3][3][3] = {};      // 0x218
    uint32_t m_field290 = 0;             // 0x290
    uint32_t m_field294 = 0;             // 0x294
    uint32_t m_field298 = 0;             // 0x298
    bool m_field2A0 = false;             // 0x2A0
};

EntityDef* CreateWeatherTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                   const EntityDefDesc& loader, const EntityDef* parent);

} // namespace objects
} // namespace sly
