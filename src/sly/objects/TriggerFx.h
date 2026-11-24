// openTIT - FX trigger volumes (scripted effects and messages on entry) and
// their push-block variant.
//
// An FX trigger runs a "ScriptLifetime" anim script for its whole life and
// tells it about entities entering, staying and leaving; it can also show a
// text message (with random delays) to the player. Push-block triggers are FX
// triggers that only react to push blocks.
//
// Vtables: definition 0x00B1A728 (push block 0x00B1ACF0), placement 0x00B1A758,
// entity 0x00B1A770 (FX 0x00B1A8B0, push block 0x00B1AD20).
#pragma once

#include "sly/game/Upgrades.h"
#include "sly/objects/Trigger.h"

#include <cstdint>
#include <vector>

namespace sly {
namespace objects {

// Def class / placement flag of FX triggers.
constexpr uint32_t kDefClass_FxTrigger = 0x80000000;

// NOTE: attribute names unknown except "ScriptLifetime".
constexpr uint32_t kAttr_FxDelay       = 0x11CB4BE1;  // both delays at once
constexpr uint32_t kAttr_FxDelayMin    = 0xB8D446B1;  // "<name>Min"
constexpr uint32_t kAttr_FxDelayMax    = 0xB8D445C3;  // "<name>Max"
constexpr uint32_t kAttr_FxDelay2      = 0x49786225;
constexpr uint32_t kAttr_FxDelay2Min   = 0xD02659ED;
constexpr uint32_t kAttr_FxDelay2Max   = 0xD02658FF;
constexpr uint32_t kAttr_FxMessage     = 0x82BF2A4A;  // text id shown by the trigger
constexpr uint32_t kElem_FxTexts       = 0xC3B709C0;  // child element holding text fields
// The three FX option flags (table 0x00AD37C0; the placement table 0x00AD37E8
// lists the same names).
constexpr uint32_t kFxFlagAttrs[3] = {0xA8C8CA8F, 0x86FCD30D, 0xF0DAFF39};

// The FX parameters shared by definition, placement and instance.
struct FxTriggerParams {
    uint32_t flags = 0;          // bits 0..2
    uint32_t script = 0;         // "ScriptLifetime"
    float delayMin = 0.0f;
    float delayMax = 0.0f;
    float delay2Min = 0.0f;
    float delay2Max = 0.0f;
    uint32_t message = 0;
};

// Reads the FX attributes over `params` (defaults are its current values);
// delays are clamped to >= 0 and max >= min.
void ParseFxTriggerParams(ape::xml::XmlElement* elem, FxTriggerParams& params, const uint32_t (&flagAttrs)[3]);

struct FxTriggerDefDesc : TriggerDefDesc {
    FxTriggerParams params;      // 0x54..0x6F
    FxTriggerDefDesc(const EntityDefDesc& loader, const char* prefix);
    // Defaults come from an FX parent definition (0x0021DEE0).
    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

class FxTriggerDef : public TriggerDef {
public:
    explicit FxTriggerDef(const FxTriggerDefDesc& desc) : TriggerDef(desc), params(desc.params) {}
    ObjEntity* Create(EntityPlacement* placement) override;
    FxTriggerParams params;      // 0xB0..0xCB
};

class FxTriggerPlacement : public TriggerPlacement {
public:
    explicit FxTriggerPlacement(const char* placementName);
    // @func 0x00759B20 FxTriggerPlacement::~FxTriggerPlacement
    // @func 0x0075B730 FxTriggerPlacement::~FxTriggerPlacement (deleting)
    ~FxTriggerPlacement() override = default;
    void FillInstanceDesc(EntityInstanceDesc& desc) const override;
    std::vector<TextField> texts;   // 0xBC (up to 16)
    FxTriggerParams params;         // 0xD0..0xEB
};

struct FxTriggerInstanceDesc : TriggerInstanceDesc {
    FxTriggerInstanceDesc(const char* instanceName, FxTriggerDef* ownerDef);
    std::vector<TextField> texts;   // 0x9C
    FxTriggerParams params;         // 0xB0..0xCB
};

class FxTrigger : public TriggerBase {
public:
    explicit FxTrigger(const FxTriggerInstanceDesc& desc);
    ~FxTrigger() override;

    void OnStay(ObjEntity* entity) override;
    void SetEnabled(bool enabled) override;
    bool IsDebugDisplayed() const override;
    // Starts the lifetime script again if it is not running (0x002202BC).
    void RestartScript();
    // TODO(0x0021F240 FxTrigger::Update, 0x0021F9E8 OnEnter, 0x0021FF58 OnExit
    // and their helpers 0x0021F4E8 / 0x0021FB38): message delays and script
    // notifications are not ported.

protected:
    void* m_script = nullptr;        // 0x1AC instance of the lifetime script
    void* m_scriptDef = nullptr;     // 0x1B0
    FxTriggerParams m_params;        // 0x1B4..0x1CF
    uint32_t m_field1D0 = 0;         // 0x1D0
    bool m_messageShown = false;     // 0x1D4 bit 31
    bool m_messageSeen = false;      // 0x1D4 bit 30
    bool m_scriptRunning = false;    // 0x1D4 bit 29
    bool m_hasTexts = false;         // 0x1D4 bit 28
};

EntityDef* CreateFxTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                              const EntityDefDesc& loader, const EntityDef* parent);
EntityPlacement* ParseFxTriggerPlacement(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                         const char* name);

// ---------------------------------------------------------------------------
// Push block: an FX trigger fed only by push blocks.
// ---------------------------------------------------------------------------
class PushBlockTriggerDef : public FxTriggerDef {
public:
    using FxTriggerDef::FxTriggerDef;
    ObjEntity* Create(EntityPlacement* placement) override;
    // The member-wise destruction is TriggerDef::~TriggerDef (its copies are
    // tagged there); the deleting variant returns the block to the bucket
    // allocator.
    // @func 0x0075BA34 PushBlockTriggerDef::~PushBlockTriggerDef (deleting)
    ~PushBlockTriggerDef() override = default;
};

class PushBlockTrigger : public FxTrigger {
public:
    explicit PushBlockTrigger(const FxTriggerInstanceDesc& desc);
    ~PushBlockTrigger() override;
    void OnEnter(ObjEntity* entity) override;
    void OnExit(ObjEntity* entity) override;
};

EntityDef* CreatePushBlockTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                     const EntityDefDesc& loader, const EntityDef* parent);

} // namespace objects
} // namespace sly
