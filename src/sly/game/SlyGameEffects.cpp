// openTIT - Screen effects the game world animates: the colour fade of the
// "Common.Fade" GUI object and the depth of field, blur, desaturation and
// bloom noise effects of the main view's post-process chain.
#include "sly/game/SlyGame.h"

#include "agl/AglDrawList.h"
#include "agl/AglResources.h"
#include "agl/AglPostProc.h"
#include "agl/gpu/GpuTypes.h"
#include "ape/world/ApeBase.h"
#include "agl/AglRenderTarget.h"
#include "ape/level/ApeGui.h"
#include "ape/level/ApeLevelDef.h"
#include "sly/entity/Entity.h"
#include "sly/entity/GameCamera.h"
#include "sly/tuning/GameTuning.h"

#include <cmath>

namespace sly {

ape::ApeGuiObject* g_fadeObject = nullptr;

namespace {

using agl::AglPostProcChain;

AglPostProcChain* MainChain()
{
    if (!g_game || !g_game->mainView)
        return nullptr;
    return g_game->mainView->postProc;
}

template <class Effect>
Effect* MainEffect(int index)
{
    AglPostProcChain* chain = MainChain();
    return chain ? static_cast<Effect*>(chain->effects[index]) : nullptr;
}

// While the game drives one of the camera-controlled effects, the camera
// must not apply its own post effects.
void SetCameraPostFx(bool on)
{
    if (!g_game || !g_game->camera)
        return;
    uint32_t& flags = g_game->camera->cameraFlags;
    flags = on ? (flags | entity::kCamFlag_PostFx) : (flags & ~uint32_t(entity::kCamFlag_PostFx));
}

// Advances `elapsed` by dt within [0, duration]; returns true once the end
// is reached.
bool AdvanceClamped(float& elapsed, float duration, float dt)
{
    const float next = elapsed + dt;
    const bool done = next >= duration;
    const float capped = done ? duration : next;
    elapsed = capped > 0.0f ? capped : 0.0f;
    return done;
}

float Lerp(float a, float b, float t) { return a + (b - a) * t; }
float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
float ClampRange(float v) { return v < 0.01f ? 0.01f : (v > 999.0f ? 999.0f : v); }

// Channel blend with an 8-bit weight (0..255), as the console does it.
uint8_t BlendChannel(uint8_t from, uint8_t to, int weight)
{
    return uint8_t(from + (((int(to) - int(from)) * weight) >> 8));
}

} // namespace

// @func 0x00468C40 ScreenFade::Start
void ScreenFade::Start(float time, ape::xml::Color8 fromColor, ape::xml::Color8 toColor, bool hide, bool force)
{
    from = fromColor;
    to = toColor;
    duration = time;
    elapsed = 0.0f;
    hideWhenDone = hide;
    active = true;

    ape::ApeGuiObject* object = g_fadeObject;
    if (!object) {
        active = false;
        return;
    }
    object->BringToFront(true);
    const bool targetVisible = to.a * (1.0f / 255.0f) > 0.0f;
    if (!force && object->IsVisible() == targetVisible) {
        const ape::xml::Color8 current = object->GetColor();
        if (current.r == to.r && current.g == to.g && current.b == to.b && current.a == to.a)
            duration = 0.0f;
    }
    object->SetColor(duration <= 0.0f ? to : from);
    object->SetVisible(true);
}

// @func 0x00469B90 ScreenFade::Update
void ScreenFade::Update(float dt)
{
    if (!active)
        return;
    ape::ApeGuiObject* object = g_fadeObject;
    if (!object) {
        active = false;
        return;
    }

    ape::xml::Color8 color = to;
    if (duration <= elapsed) {
        active = false;
    } else {
        elapsed += dt;
        const uint32_t weight = uint32_t(elapsed / duration * 255.0f);
        if (weight < 0xFF) {
            color.r = BlendChannel(from.r, to.r, int(weight));
            color.g = BlendChannel(from.g, to.g, int(weight));
            color.b = BlendChannel(from.b, to.b, int(weight));
            color.a = BlendChannel(from.a, to.a, int(weight));
        }
    }
    const bool hide = !active && hideWhenDone;
    object->BringToFront(true);
    object->SetColor(color);
    object->SetVisible(!hide);
}

// @func 0x004697D8 DofAnimation::Update
void DofAnimation::Update(float dt)
{
    if (!active)
        return;
    auto* dofEffect = MainEffect<agl::AglDofBase>(AglPostProcChain::kDoF);
    if (!dofEffect)
        return;

    dofEffect->SetEnabled(true);
    SetCameraPostFx(!dofEffect->IsActive());
    if (!dofEffect->IsActive()) {
        active = false;
        return;
    }

    dofEffect->nearBlur = nearBlur;
    dofEffect->quality = quality == 0 ? 1 : (quality > 4 ? 4 : quality);
    const float next = elapsed + dt;
    if (next >= duration && !hold)
        active = false;
    const float capped = next >= duration ? duration : next;
    elapsed = capped > 0.0f ? capped : 0.0f;
    const float t = elapsed / duration;

    float focus;
    float range;
    if (!target) {
        focus = Lerp(focusStart, focusEnd, t);
        range = Lerp(rangeStart, rangeEnd, t);
    } else {
        // Focus on the target: its distance to the camera.
        const ape::Vec3& eye = g_game->mainView->camera.translation;
        const ape::Vec3& pos = target->transform.translation;
        const float dx = pos.x - eye.x;
        const float dy = pos.y - eye.y;
        const float dz = pos.z - eye.z;
        focus = std::sqrt(dz * dz + dx * dx + dy * dy);
        range = focus;
    }
    dofEffect->focusDistance = focus;
    dofEffect->focusRange = ClampRange(range);
    dofEffect->intensity = Lerp(intensityStart, intensityEnd, t) * g_gameIGCDOFIntensity;

    if (!active && disableWhenDone) {
        dofEffect->SetEnabled(false);
        SetCameraPostFx(!dofEffect->IsActive());
    }
}

// @func 0x004695A8 NoisePulse::Update
void NoisePulse::Update(float dt)
{
    if (step >= 2 || !MainChain())
        return;
    AglPostProcChain* chain = MainChain();
    agl::AglPostProcEffect* bloomEffect = chain->effects[AglPostProcChain::kBloom];
    auto* noise = static_cast<agl::AglBloomNoiseBase*>(chain->effects[AglPostProcChain::kBloomNoise]);

    switch (steps[step]) {
    case kStep_Start:
        bloomWasOn = bloomEffect->IsActive();
        bloomEffect->SetEnabled(false);
        noise->SetEnabled(true);
        noise->pulseAmount = from;
        ++step;
        break;
    case kStep_Ramp:
        if (AdvanceClamped(elapsed, duration, dt))
            ++step;
        noise->pulseAmount = Lerp(from, to, elapsed / duration);
        break;
    case kStep_Restore:
        noise->SetEnabled(false);
        bloomEffect->SetEnabled(bloomWasOn);
        ++step;
        break;
    default:
        break;
    }
}

// @func 0x004691E8 DesaturationQueue::Update
void DesaturationQueue::Update(float dt)
{
    auto* desat = MainEffect<agl::AglDesaturationBase>(AglPostProcChain::kDesaturation);
    if (!desat) {
        count = 0;
        return;
    }

    // Advance every fade, newest first; finished ones either hold their end
    // values or leave the queue.
    for (int i = int(count) - 1; i >= 0; --i) {
        DesaturationFade& fade = fades[i];
        if (fade.duration <= fade.elapsed + dt) {
            if (!fade.removeWhenDone) {
                fade.elapsed = fade.duration;
            } else {
                for (uint32_t j = uint32_t(i); j + 1 < count; ++j)
                    fades[j] = fades[j + 1];
                --count;
            }
        } else {
            fade.elapsed += dt;
        }
    }

    if (count == 0) {
        desat->SetEnabled(false);
        SetCameraPostFx(true);
        return;
    }

    const DesaturationFade& fade = fades[count - 1];
    const float t = fade.duration > 0.0f ? fade.elapsed / fade.duration : 1.0f;
    SetCameraPostFx(false);
    desat->SetEnabled(true);
    desat->blendModeB = fade.blendModeB;
    desat->blendModeA = fade.blendModeA;
    desat->amount = Clamp01(Lerp(fade.amountFrom, fade.amountTo, t));
    const int weight = int(t * 255.0f);
    for (int c = 0; c < 3; ++c)
        desat->color[c] = float(BlendChannel(fade.colorFrom[c + 1], fade.colorTo[c + 1], weight));
}

// @func 0x00468EF8 BlurAnimation::Update
void BlurAnimation::Update(float dt)
{
    if (!active)
        return;
    auto* blurEffect = MainEffect<agl::AglBlurBase>(AglPostProcChain::kBlur);
    if (!blurEffect)
        return;

    blurEffect->SetEnabled(true);
    SetCameraPostFx(!blurEffect->IsActive());
    if (!blurEffect->IsActive()) {
        active = false;
        return;
    }

    const uint32_t q = quality & 0xFF;
    blurEffect->quality = uint8_t(q == 0 ? 1 : (q > 5 ? 5 : q));
    if (AdvanceClamped(elapsed, duration, dt))
        active = false;
    blurEffect->amount = Clamp01(Lerp(from, to, elapsed / duration));

    if (!active && disableWhenDone) {
        blurEffect->SetEnabled(false);
        SetCameraPostFx(!blurEffect->IsActive());
    }
}

// @func 0x0048EF50 ValueRamp::Start
void ValueRamp::Start(float fromValue, float toValue, float time, float holdTime)
{
    from = fromValue;
    to = toValue;
    duration = time;
    hold = holdTime;
    elapsed = 0.0f;
    active = true;
    value = time <= 0.0f ? toValue : fromValue;
}

// @func 0x0048EF9C DofAnimation::Start
void DofAnimation::Start(float intensityFrom, float intensityTo, float focusFrom, float focusTo, float rangeFrom,
                         float rangeTo, float time, entity::Entity* focusTarget, uint8_t samples, bool disableAtEnd,
                         bool nearBlurOn, bool holdAtEnd)
{
    target = focusTarget;
    intensityStart = intensityFrom;
    intensityEnd = intensityTo;
    focusStart = focusFrom;
    focusEnd = focusTo;
    rangeStart = rangeFrom;
    rangeEnd = rangeTo;
    duration = time;
    quality = samples;
    elapsed = 0.0f;
    disableWhenDone = disableAtEnd;
    nearBlur = nearBlurOn;
    hold = holdAtEnd;
    active = true;
}

// @func 0x0048F008 NoisePulse::FadeIn
void NoisePulse::FadeIn(float time)
{
    AglPostProcChain* chain = MainChain();
    if (!chain)
        return;
    auto* bloomEffect = static_cast<agl::AglBloomBase*>(chain->effects[AglPostProcChain::kBloom]);
    auto* noise = static_cast<agl::AglBloomNoiseBase*>(chain->effects[AglPostProcChain::kBloomNoise]);

    from = step < 2 ? noise->pulseAmount : 0.0f;
    while (step < 2)
        Update(3.4028235e38f);
    steps[0] = kStep_Start;
    steps[1] = kStep_Ramp;
    to = 1.0f;
    duration = time;
    elapsed = 0.0f;
    // The noise takes over the bloom's look.
    noise->field_A0 = bloomEffect->field_30;
    noise->field_90 = bloomEffect->field_20[0];
    noise->field_94 = bloomEffect->field_20[1];
    noise->field_98 = bloomEffect->field_20[2];
    step = 0;
    Update(0.0f);
}

// @func 0x0048F158 NoisePulse::FadeOut
void NoisePulse::FadeOut(float time)
{
    AglPostProcChain* chain = MainChain();
    if (!chain)
        return;
    auto* noise = static_cast<agl::AglBloomNoiseBase*>(chain->effects[AglPostProcChain::kBloomNoise]);

    from = step < 2 ? noise->pulseAmount : 1.0f;
    while (step < 2)
        Update(3.4028235e38f);
    steps[0] = kStep_Ramp;
    steps[1] = kStep_Restore;
    to = 0.0f;
    duration = time;
    elapsed = 0.0f;
    step = 0;
    Update(0.0f);
}

// @func 0x00488B78 SlyGame::SetDofIntensity
void SlyGame::SetDofIntensity(float value)
{
    if (!mainView || dof.active)
        return;
    if (auto* effect = MainEffect<agl::AglDofBase>(AglPostProcChain::kDoF)) {
        effect->intensity = value * g_gameIGCDOFIntensity;
        effect->SetEnabled(value > 0.0f);
    }
}

// @func 0x00488C40 SlyGame::SetDofRange
void SlyGame::SetDofRange(float range)
{
    if (!mainView || dof.active)
        return;
    if (auto* effect = MainEffect<agl::AglDofBase>(AglPostProcChain::kDoF))
        effect->focusRange = ClampRange(range);
}

// @func 0x00488CC8 SlyGame::SetDofFocus
void SlyGame::SetDofFocus(float distance)
{
    if (!mainView || dof.active)
        return;
    if (auto* effect = MainEffect<agl::AglDofBase>(AglPostProcChain::kDoF))
        effect->focusDistance = distance;
}

// @func 0x00488D14 SlyGame::SetDofNearBlur
void SlyGame::SetDofNearBlur(bool on)
{
    if (!mainView || dof.active)
        return;
    if (auto* effect = MainEffect<agl::AglDofBase>(AglPostProcChain::kDoF))
        effect->nearBlur = on;
}

// @func 0x0048C5C8 SlyGame::ApplyLevelDof
void SlyGame::ApplyLevelDof()
{
    if (camera && !(camera->cameraFlags & entity::kCamFlag_PostFx))
        return;
    if (!levelDef || !levelDef->dofEnable)
        return;
    auto* effect = MainEffect<agl::AglDofBase>(AglPostProcChain::kDoF);
    if (!effect)
        return;
    effect->focusDistance = levelDef->dofStartingDistance;
    effect->focusRange = ClampRange(levelDef->dofRange);
    effect->intensity = levelDef->dofIntensity;
    const uint8_t passes = levelDef->dofPasses;
    effect->quality = passes < 1 ? 1 : (passes > 4 ? 4 : passes);
    effect->SetEnabled(levelDef->dofEnable);
}

// @func 0x004583F8 SlyGame::DrawScreenGradient
// A full screen quad at the far end of the depth range, `top` colour at
// the top edge, `bottom` at the bottom, drawn with the built-in vertex
// colour material into the frame's draw list.
// NOTE: the console also unprojects the four screen corners through the
// view's inverse view-projection but then draws in clip space; the result
// is unused and the view only matters for that.
void SlyGame::DrawScreenGradient(agl::AglRenderTarget* view, const uint8_t top[4], const uint8_t bottom[4])
{
    (void)view;
    constexpr float kFarZ = 0.9999f;
    agl::AglDrawList& list = agl::GetDrawList();   // NOTE: the game world's frame list (+0x1285E4)
    list.SetMaterial(agl::GetBuiltinMaterial(3), 0);
    list.SetWorldMatrix(ape::world::kWorldIdentity, 0);
    agl::AglDrawPrimitive primitive;
    list.AllocPrimitive(&primitive, static_cast<uint8_t>(agl::gpu::PrimitiveType::TriangleFan), 4, 0);
    const struct {
        float x, y;
        const uint8_t* color;
    } corners[4] = {{-1.0f, -1.0f, bottom}, {1.0f, -1.0f, bottom}, {1.0f, 1.0f, top}, {-1.0f, 1.0f, top}};
    for (int i = 0; i < 4; ++i) {
        agl::AglDrawVertex& v = primitive.vertices[i];
        v.position[0] = corners[i].x;
        v.position[1] = corners[i].y;
        v.position[2] = kFarZ;
        for (int c = 0; c < 4; ++c)
            v.color[c] = corners[i].color[c];
    }
    list.EndPrimitive();
}

} // namespace sly
