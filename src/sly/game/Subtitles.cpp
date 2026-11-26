// openTIT - The cutscene subtitle line (see Subtitles.h).
#include "sly/game/Subtitles.h"

#include "ape/core/ApeString.h"
#include "ape/level/ApeGui.h"
#include "ape/level/ApeGuiTypes.h"
#include "sly/game/AudioSystem.h"
#include "sly/game/EconomyTuning.h"
#include "sly/game/hud/HudManager.h"

#include <algorithm>

namespace sly {

namespace {

// Seconds per revealed character of a new line.
constexpr float kRevealInterval = 0.08f;

// The subtitle state (0x010C1B88, 0x98 bytes).
struct SubtitleState {
    char text[0x80] = {};                    // +0x00
    uint32_t colorStyle = 0;                 // +0x80
    ape::ApeGuiObject* screen = nullptr;     // +0x84
    ape::ApeGuiLabel* label = nullptr;       // +0x88
    float timer = 0.0f;                      // +0x8C time left on screen
    audio::SoundRequest* sound = nullptr;    // +0x90 referenced
    bool shown = false;                      // +0x94
    bool pending = false;                    // +0x95 waits for the next update
};
SubtitleState g_subtitle;

// Default subtitle colour (0x01164CC4). NOTE: value assumed (opaque white).
const ape::xml::Color8 kDefaultColor{0xFF, 0xFF, 0xFF, 0xFF};

void ReleaseSound(SubtitleState& state)
{
    if (state.sound) {
        audio::Requests().Release(state.sound);
        state.sound = nullptr;
    }
}

// Once the dialogue sound has started, the line's reveal rate and time on
// screen follow its length; the label scrolls to its last lines.
// @func 0x004A8B08 SyncSubtitleToSound
void SyncToSound(SubtitleState& state)
{
    if (state.sound && audio::IsRequestReady(state.sound)) {
        const float duration = audio::RequestDuration(state.sound);
        float time = std::max(0.5f, duration - g_subtitleDisplayRateFudgeFactor.Get());
        if (state.label) {
            const uint32_t glyphs = state.label->glyphCount;
            const float perGlyph = time / float(glyphs);
            state.label->revealInterval = perGlyph;
            if (glyphs != 0)
                time = float(glyphs) * perGlyph + g_subtitleExtraTime.Get();
        }
        state.timer = time;
        ReleaseSound(state);
    }
    // Only the last "UI.Subtitle.MaxLines" lines stay in view: the label
    // moves up by the lines above them.
    if (ape::ApeGuiLabel* label = state.label) {
        const uint32_t lines = uint32_t(label->progress[1]);
        const float spacing = label->GetLineSpacing();
        const uint32_t maxLines = uint32_t(g_subtitleMaxLines.Get());
        float y = 0.0f;
        if (lines > maxLines)
            y -= float(lines - maxLines) * spacing;
        ape::Vec3 position = label->GetPosition();
        position.y = y;
        label->SetPosition(position);
    }
}

} // namespace

// @func 0x004A8CD0 ShowSubtitle
void ShowSubtitle(const char* text, uint32_t colorStyle, ape::ApeGuiObject* screen, ape::ApeGuiLabel* label,
                  audio::SoundRequest* sound)
{
    SubtitleState& state = g_subtitle;
    state.text[0] = '\0';
    if (text && text[0] != '\0')
        ape::StrNCopy(state.text, text, 0x7F);
    state.colorStyle = colorStyle;
    if (!screen)
        screen = ape::FindGuiObject("InGame.Cutscene.Letterbox.Subtitle", nullptr);
    if (!label)
        label = static_cast<ape::ApeGuiLabel*>(ape::FindGuiObject("Clip.text", screen));
    state.label = label;
    state.screen = screen;
    if (sound)
        ++sound->refCount;
    state.sound = sound;
    state.shown = false;
    state.pending = true;
}

// @func 0x004A9084 ClearSubtitle
void ClearSubtitle()
{
    SubtitleState& state = g_subtitle;
    if (state.label)
        state.label->SetText("", false);
    state.label = nullptr;
    ReleaseSound(state);
}

// @func 0x004A8DF0 UpdateSubtitle
void UpdateSubtitle(float dt)
{
    SubtitleState& state = g_subtitle;
    if (state.pending) {
        if (state.screen) {
            state.screen->SetVisible(true);
            ape::xml::Color8 color = kDefaultColor;
            if (void** found = hud::Hud().defBuffers.Find(state.colorStyle); found && *found) {
                const uint8_t* c = static_cast<const hud::HudColorStyle*>(*found)->colors[2];
                color = ape::xml::Color8{c[0], c[1], c[2], c[3]};
            }
            if (state.label) {
                state.label->SetColor(color);
                state.label->SetText(state.text, false);
                state.label->revealInterval = kRevealInterval;
                state.timer = float(state.label->glyphCount) * kRevealInterval + g_subtitleExtraTime.Get();
            }
        }
        state.shown = true;
        state.pending = false;
    } else if (state.shown) {
        state.timer -= dt;
        if (state.timer <= 0.0f) {
            state.shown = false;
            ClearSubtitle();
        }
    }
    SyncToSound(state);
}

} // namespace sly
