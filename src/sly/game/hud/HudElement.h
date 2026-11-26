// openTIT - The common HUD element (vtable 0x00B1F6B0, 0x190 bytes): an
// element with text fields, state animations (intro, update, idle, outro),
// a hold time and a text that waits for the next intro. Most element kinds
// derive from it.
#pragma once

#include "sly/game/hud/HudManager.h"

namespace sly {
namespace hud {

class HudElement : public HudObject {
public:
    // 0x0035A868: text fields (cloning their GUI objects where the
    // definition asks) and the "<prefix>_Intro/_OnUpdate/_Idle/_Outro" clips.
    explicit HudElement(HudObjectDef* def);
    // 0x0036ABF8 / 0x0036AF54.
    ~HudElement() override;
    // Slot 3 (0x0036BCAC): back to the hidden state, keeping a persistent
    // element shown.
    void Refresh() override;
    // Slot 5 (0x0036AACC): the object, the text field objects and the anim
    // script state ("visibleON"/"visibleOFF").
    void SetActive(bool active) override;
    // Slot 6 (0x0036BF38): disabling plays the outro (`flag` as its first
    // reason); enabling shows a hidden element.
    void SetEnabled(bool enabled, bool flag) override;
    // Slot 2 (0x0036B240): pending text and outro requests, label colours,
    // the spring, and the state machine (intro -> idle -> outro; update
    // animations return to idle).
    void Update(float dt) override;
    // Slot 7 (0x0036BAA0, idle state): slides along the stack and counts the
    // hold time down; when it runs out the element leaves.
    virtual void TickIdle(float dt);
    // Slot 8 (0x0036BBC0, update state): once the update animation and
    // script are done the element idles for its hold time.
    virtual void TickUpdate();
    // 0x0036AFA8: pulls the object towards springTarget; queues the element
    // to hide once it has settled there.
    void TickSpring(float dt);
    // 0x0036C160: draws the field frames at their scene objects (world
    // callback 0x0036C33C).
    void DrawFields();
    static void DrawFieldsCallback(void* unused, void* context);
    // 0x0036C36C (world callback): queues a shown element's object and its
    // visible children for drawing.
    static void DrawObjectCallback(void* unused, void* context);
};

// Element of definition type bit 0x20000 (vtable 0x00B1FA48, 0x1B0 bytes),
// built inline by the element factory. The game world's yes/no prompt
// (SlyGame::ShowPrompt) fills the answer callbacks. NOTE: also looked up
// by the AR server's target view, hence the name.
class HudArElement : public HudElement {
public:
    explicit HudArElement(HudObjectDef* def) : HudElement(def) { typeFlags |= kHud_Type20000; }
    // @func 0x00773724 HudArElement::~HudArElement
    // @func 0x00773750 HudArElement::~HudArElement (deleting)
    ~HudArElement() override = default;
    // Slot 3 (0x00389C3C): the common element's reset.
    void Refresh() override;

    using AnswerCallback = void (*)(void* user);
    AnswerCallback onYes = nullptr;   // +0x190
    AnswerCallback onNo = nullptr;    // +0x194
    void* yesUser = nullptr;          // +0x198
    void* noUser = nullptr;           // +0x19C
    entity::Entity* player = nullptr; // +0x1A0 the player the prompt is for
    bool field_1A4 = true;        // +0x1A4 NOTE: prompt options, meaning unknown
    bool field_1A5 = true;        // +0x1A5
};

// The plain element the element factory (0x0035AC7C, 0x003677C4) builds
// for definitions without a special type (vtable 0x00B1F6E0): it adds
// nothing to HudElement.
class HudPlainElement : public HudElement {
public:
    using HudElement::HudElement;
    // @func 0x00772A3C HudPlainElement::~HudPlainElement
    // @func 0x00772A68 HudPlainElement::~HudPlainElement (deleting)
    ~HudPlainElement() override = default;
};

} // namespace hud
} // namespace sly
