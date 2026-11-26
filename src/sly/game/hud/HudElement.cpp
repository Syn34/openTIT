// openTIT - The common HUD element (vtable 0x00B1F6B0).
#include "sly/game/hud/HudElement.h"

#include "sly/anim/AnimController.h"
#include "sly/anim/AnimLayer.h"
#include "sly/game/hud/HudHooks.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "ape/level/ApeGuiScript.h"
#include "ape/level/ApeGuiTypes.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeObj.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace sly {
namespace hud {

namespace {

constexpr uint32_t kScript_VisibleOn = 0x152FB9B1u;    // "visibleON"
constexpr uint32_t kScript_VisibleOff = 0x90C77BBDu;   // "visibleOFF"

// The object named `name` in the hierarchy of `root` (the engine's 0x004A5AE8).
ape::world::ApeObj* FindSceneChild(ape::world::ApeObj* root, uint32_t name)
{
    if (!root || name == 0)
        return nullptr;
    if (root->GetNameHash() == name)
        return root;
    return anim::FindDescendant(root, name);
}

uint32_t ClipName(const char* prefix, const char* suffix)
{
    char clip[0x60];
    std::snprintf(clip, sizeof clip, "%s_%s", prefix, suffix);
    return ape::HashNameNoCase(clip);
}

// Running number appended to cloned field objects (0x00F64F84).
int s_cloneCounter = 0;

} // namespace

// @func 0x0035A868 HudElement::HudElement
HudElement::HudElement(HudObjectDef* def_) : HudObject(0, def_)
{
    altIdleAnim = 0;
    altOutroAnim = 0;
    for (uint8_t& c : textColor)
        c = 0;   // transparent
    const bool persistent = (def->elementFlags & 1) != 0;
    stateFlags = (stateFlags & 0x0013FFFFu) | (persistent ? (kState_Persistent | kState_Shown) : 0u);
    defaultFade = def->defaultFade;
    linked = nullptr;
    animSpeed = 1.0f;
    fadeTime = -1.0f;
    text[0] = '\0';
    if (def->textFieldCount) {
        textFields = static_cast<HudTextField*>(std::calloc(def->textFieldCount, sizeof(HudTextField)));
        for (uint32_t i = 0; i < def->textFieldCount; ++i) {
            const HudObjectDef::TextFieldDef& info = def->textFieldDefs[i];
            HudTextField& field = textFields[i];
            field.sceneObject = FindSceneChild(object, info.objectName);
            if (!(info.field_1C & HudObjectDef::TextFieldDef::kField_Clone)) {
                field.object = info.templateObject;
                field.field_08 = info.framePath;
                field.label = info.labelPath;
                continue;
            }
            ape::ApeGuiObject* source = info.templateObject;
            if (!source)
                continue;
            char name[0x40];
            std::snprintf(name, sizeof name, "%s_%i", source->name, ++s_cloneCounter);
            ape::ApeGuiObject* clone = source->Clone(name, source->parent, source->owner, true);
            field.object = clone;
            if (!clone)
                continue;
            ape::ApeGuiObject* frame = field.field_08;
            if (info.framePath) {
                frame = ape::FindGuiObject(info.framePath->name, clone);
                field.field_08 = frame;
            }
            if (!frame)
                frame = clone;
            if (info.labelPath) {
                ape::ApeGuiObject* label = ape::FindGuiObject(info.labelPath->name, frame);
                if (label && (label->GetType() == ape::kGuiObject_Text || label->GetType() == ape::kGuiObject_Type5))
                    field.label = label;
            }
        }
    }
    for (uint32_t& clip : stateAnims)
        clip = 0;
    const char* prefix = def->animPrefix[0] ? def->animPrefix : def->defName;
    if (prefix[0]) {
        stateAnims[1] = ClipName(prefix, "Intro");
        stateAnims[2] = ClipName(prefix, "OnUpdate");
        stateAnims[3] = ClipName(prefix, "Idle");
        stateAnims[4] = ClipName(prefix, "Outro");
    }
}

// @func 0x0036ABF8 HudElement::~HudElement
// @func 0x0036AF54 HudElement::~HudElement (deleting)
HudElement::~HudElement()
{
    GameWorldRemoveHudCallback(this);
    if (object)
        object->SetVisible(false, true);
    for (uint32_t i = 0; i < def->textFieldCount; ++i) {
        if (ape::ApeGuiObject* fieldObject = textFields[i].object)
            fieldObject->SetVisible(false);
    }
    SetScriptState(kScript_VisibleOff);
    for (uint32_t i = 0; i < def->textFieldCount; ++i) {
        if (textFields[i].object)
            ape::StopGuiScripts("", textFields[i].object);
    }
    // Field objects the element made itself (definition flag bit 31) go with it.
    for (uint32_t i = 0; i < def->textFieldCount; ++i) {
        if ((def->textFieldDefs[i].field_1C & HudObjectDef::TextFieldDef::kField_Clone) && textFields[i].object)
            delete textFields[i].object;
    }
    std::free(textFields);
    textFields = nullptr;
}

// @func 0x0036AACC HudElement::SetActive
void HudElement::SetActive(bool active)
{
    if (object)
        object->SetVisible(active, true);
    for (uint32_t i = 0; i < def->textFieldCount; ++i) {
        if (ape::ApeGuiObject* fieldObject = textFields[i].object)
            fieldObject->SetVisible(active);
    }
    SetScriptState(active ? kScript_VisibleOn : kScript_VisibleOff);
}

// @func 0x0036BCAC HudElement::Refresh
void HudElement::Refresh()
{
    SetActive(false);
    const float origin[3] = {ape::world::kWorldZero.x, ape::world::kWorldZero.y, ape::world::kWorldZero.z};
    SetStackOffset(origin, true);
    linked = nullptr;
    holdTime = 0.0f;
    animState &= 1u;
    fadeTime = -1.0f;
    text[0] = '\0';
    altIdleAnim = 0;
    // Keep the suppression, reasons and the persistent bit; a persistent
    // element stays shown unless it was told to leave.
    const uint32_t old = stateFlags;
    uint32_t flags = old & 0x927FFFFFu;
    if (!(old & kState_OutroA))
        flags = (old & 0xB27FFFFFu) | ((old & kState_Persistent) ? kState_Shown : 0u);
    stateFlags = flags;
    if ((stateFlags & kState_OutroB) && !*Hud().hideList.Find(RegisteredName()))
        Hud().hideList.Add(RegisteredName(), this);
    ape::ApeTArray<HudObject*>& list = Hud().suspended;
    const int at = list.FindLast(this);
    if (at >= 0)
        list.RemoveAt(size_t(at));
}

// @func 0x0036BF38 HudElement::SetEnabled
void HudElement::SetEnabled(bool enabled, bool flag)
{
    if (!enabled) {
        PlayIntro(flag, false);
    } else if (!(stateFlags & kState_Shown)) {
        static const uint8_t kNoTint[4] = {0, 0, 0, 0};
        ShowTimed(-1.0f, nullptr, false, nullptr, kNoTint);
    }
}

// @func 0x0036BAA0 HudElement::TickIdle
void HudElement::TickIdle(float dt)
{
    if (!offsetTween.IsFinished()) {
        offsetTween.Update(dt);
        if (object) {
            ape::Matrix34 local = object->GetLocal();
            local.translation.x = offset.x;
            local.translation.y = offset.y;
            local.translation.z = offset.z;
            object->SetLocal(local);
            object->MarkDirty();
        }
    }
    if (holdTime > 0.0f) {
        // The hold time only runs while the element stands still.
        if (offsetTween.IsFinished())
            holdTime -= dt;
        if (holdTime <= 0.0f)
            PlayIntro((stateFlags & kState_Bit22) != 0, (stateFlags & kState_Bit21) != 0);
    }
}

// @func 0x0036BBC0 HudElement::TickUpdate
void HudElement::TickUpdate()
{
    if (stateFlags & kState_AnimOk) {
        anim::AnimLayer* layer = animController ? animController->FindLayer(0) : nullptr;
        if (layer && !layer->IsDone(false))
            return;
    }
    if (stateFlags & kState_ScriptOk) {
        if (!animScript || !static_cast<ape::ApeAnimScriptInstance*>(animScript)->IsFinished(false))
            return;
    }
    animState = (animState & 1u) | (3u << 1);
    holdTime = fadeTime < 0.0f ? defaultFade : fadeTime;
    const bool loop = altIdleAnim ? (stateFlags & kState_Bit19) != 0 : true;
    PlayState(3, loop);
}

namespace {

void QueueHide(HudObject* element)
{
    if (!*Hud().hideList.Find(element->RegisteredName()))
        Hud().hideList.Add(element->RegisteredName(), element);
}

bool IntroFinished(HudObject* element)
{
    if ((element->stateFlags & HudObject::kState_AnimOk) && !HudClipDone(element->animController))
        return false;
    if (element->stateFlags & HudObject::kState_ScriptOk) {
        auto* script = static_cast<ape::ApeAnimScriptInstance*>(element->animScript);
        if (!script || !script->IsFinished(false))
            return false;
    }
    return true;
}

} // namespace

// @func 0x0036AFA8 HudElement::TickSpring
void HudElement::TickSpring(float dt)
{
    if (!object)
        return;
    ape::Matrix34 local = object->GetLocal();
    ape::Vec3& pos = local.translation;
    // Damped spring, integrated with the average velocity over the step.
    const float dvx = ((springTarget.x - pos.x) * springStiffness - springVelocity.x * springDamping) * dt;
    const float dvy = ((springTarget.y - pos.y) * springStiffness - springVelocity.y * springDamping) * dt;
    const float dvz = ((springTarget.z - pos.z) * springStiffness - springVelocity.z * springDamping) * dt;
    const float nx = dvx * dt * 0.5f + (springVelocity.x * dt + pos.x);
    const float ny = dvy * dt * 0.5f + (springVelocity.y * dt + pos.y);
    const float nz = dvz * dt * 0.5f + (springVelocity.z * dt + pos.z);
    springVelocity.x += dvx;
    springVelocity.y += dvy;
    springVelocity.z += dvz;
    pos.x = nx;
    pos.y = ny;
    pos.z = nz;
    object->SetLocal(local);
    object->MarkDirty();
    const float ex = nx - springTarget.x, ey = ny - springTarget.y, ez = nz - springTarget.z;
    const float distance = std::sqrt(ez * ez + ex * ex + ey * ey);
    const float speed = std::sqrt(springVelocity.z * springVelocity.z + springVelocity.x * springVelocity.x +
                                  springVelocity.y * springVelocity.y);
    if (distance < 0.1f && speed < 1.0f)
        QueueHide(this);
}

// @func 0x0036B240 HudElement::Update
void HudElement::Update(float dt)
{
    if ((stateFlags & kState_Suppressed) || !(stateFlags & kState_Shown))
        return;
    if ((animState >> 1) == 0 && !(stateFlags & kState_Bit27) && !(stateFlags & kState_Leaving) && !linked)
        return;
    GameWorldAddHudCallback(this, 8);
    if (stateFlags & kState_Bit18)
        GameWorldAddHudCallback(this, 11);

    // Requests made since the last update.
    if (!(stateFlags & kState_Bit27)) {
        if (stateFlags & kState_Leaving) {
            PlayIntro((stateFlags & kState_OutroA) != 0, (stateFlags & kState_OutroB) != 0);
            stateFlags &= ~kState_Leaving;
        }
    } else {
        const char* current = nullptr;
        if (def->textFieldCount && textFields[0].label)
            current = static_cast<ape::ApeGuiLabel*>(textFields[0].label)->GetText();
        static const uint8_t kNoTint[4] = {0, 0, 0, 0};
        ShowTimed(-1.0f, current, false, nullptr, kNoTint);
        if (text[0] && def->textFieldCount) {
            if (ape::ApeGuiObject* label = textFields[0].label) {
                label->SetText(text, false);
                label->Invalidate();
            }
        }
        text[0] = '\0';
        stateFlags &= ~kState_Bit27;
    }

    // Label colours: the element's tint, else the field's own colour or the
    // player's; geometry elements pass their alpha on.
    if (def->textFieldCount) {
        ape::world::ApeObj* geometry =
            (object && object->GetType() == ape::world::kObjType_GeomInstance) ? object : nullptr;
        const bool tinted = textColor[3] != 0;
        for (uint32_t i = 0; i < def->textFieldCount; ++i) {
            ape::ApeGuiObject* label = textFields[i].label;
            if (!label)
                continue;
            const HudObjectDef::TextFieldDef& info = def->textFieldDefs[i];
            ape::xml::Color8 color = label->GetColor();
            if (tinted) {
                color = {textColor[0], textColor[1], textColor[2], textColor[3]};
            } else if ((info.field_1C & HudObjectDef::TextFieldDef::kField_PlayerColor) && PlayerHudColorAvailable()) {
                uint8_t player[4];
                PlayerHudColor(player);
                color = {player[0], player[1], player[2], player[3]};
            } else if (info.color[3]) {
                color = {info.color[0], info.color[1], info.color[2], info.color[3]};
            }
            if (geometry)
                color.a = geometry->m_byteA8;
            label->SetColor(color);
        }
    }
    if ((stateFlags & kState_Shown) && (stateFlags & kState_Bit20))
        TickSpring(dt);

    switch (animState >> 1) {
    case 4:   // outro: once it ended, reset and show the linked element
        if (IntroFinished(this)) {
            HudObject* next = linked;
            Refresh();
            if (next)
                BeginShow();
        }
        break;
    case 3:
        TickIdle(dt);
        break;
    case 2:
        TickUpdate();
        break;
    case 1:   // intro: idle for the hold time once it ended
        if (IntroFinished(this)) {
            animState = (animState & 1u) | (3u << 1);
            holdTime = fadeTime < 0.0f ? defaultFade : fadeTime;
            const bool loop = altIdleAnim ? (stateFlags & kState_Bit19) != 0 : true;
            PlayState(3, loop);
            for (uint32_t i = 0; i < def->textFieldCount; ++i) {
                if (!textFields[i].field_08 && textFields[i].object)
                    textFields[i].object->SetVisible(true);
            }
        }
        break;
    case 0:
        if (linked && (!linked->object || !linked->object->IsVisible()))
            BeginShow();
        break;
    default:
        break;
    }

    if (animController)
        animController->UpdateLayers(dt);
    if (!object || !object->IsVisible())
        return;
    const ape::Vec3& zero = ape::world::kWorldZero;
    if ((baseOffset.x == zero.x && baseOffset.y == zero.y && baseOffset.z == zero.z) || !HudPlacementEnabled())
        return;
    ape::Matrix34 local = object->GetLocal();
    local.translation.x = zero.x + baseOffset.x + offset.x;
    local.translation.y = zero.y + baseOffset.y + offset.y;
    local.translation.z = zero.z + baseOffset.z + offset.z;
    object->SetLocal(local);
    object->MarkDirty();
}

// @func 0x0036C160 HudElement::DrawFields
void HudElement::DrawFields()
{
    for (uint32_t i = 0; i < def->textFieldCount; ++i) {
        const HudTextField& field = textFields[i];
        if (!field.field_08 || !field.sceneObject || !field.sceneObject->IsVisible())
            continue;
        // The GUI's x and y axes point the other way from the scene's.
        ape::Matrix34 transform = field.sceneObject->GetWorld();
        transform.xAxis = {-transform.xAxis.x, -transform.xAxis.y, -transform.xAxis.z, 0.0f};
        transform.yAxis = {-transform.yAxis.x, -transform.yAxis.y, -transform.yAxis.z, 0.0f};
        field.field_08->SetTransform(transform);
        field.field_08->Tick(0.0f);
        field.field_08->Draw();
    }
}

// @func 0x0036C33C HudElement::DrawFieldsCallback
void HudElement::DrawFieldsCallback(void* unused, void* context)
{
    (void)unused;
    if (context)
        static_cast<HudElement*>(context)->DrawFields();
}

// @func 0x0036C36C HudElement::DrawObjectCallback
void HudElement::DrawObjectCallback(void* unused, void* context)
{
    (void)unused;
    auto* element = static_cast<HudObject*>(context);
    if (!element || !(element->stateFlags & kState_Shown))
        return;
    HudQueueObjectDraw2(element->object, false);
    for (ape::world::ApeObj* child = element->object->GetFirstChild(); child; child = child->GetNextSibling()) {
        if (child->IsVisible())
            HudQueueObjectDraw2(child, true);
    }
}

// @func 0x00389C3C HudArElement::Refresh
void HudArElement::Refresh()
{
    HudElement::Refresh();
}

} // namespace hud
} // namespace sly
