// openTIT - The cutscene subtitle line ("InGame.Cutscene.Letterbox.Subtitle",
// state at 0x010C1B88; 0x004A8B08-0x004A9084).
#pragma once

#include <cstdint>

namespace ape { class ApeGuiObject; class ApeGuiLabel; }

namespace sly {
namespace audio { struct SoundRequest; }

// Queues a subtitle line for the next update: `text` (a string table key
// or text), the colour style `colorStyle` of the HUD definitions, the
// screen object and label to show it on (the letterbox's subtitle screen
// and its "Clip.text" label by default) and the dialogue sound it belongs
// to (referenced while shown).
void ShowSubtitle(const char* text, uint32_t colorStyle, ape::ApeGuiObject* screen, ape::ApeGuiLabel* label,
                  audio::SoundRequest* sound);
// Takes the line down at once.
void ClearSubtitle();
// Shows a queued line (revealed at 0.08 s per character) or counts the
// shown line's time down, then times the line to its dialogue sound.
void UpdateSubtitle(float dt);

} // namespace sly
