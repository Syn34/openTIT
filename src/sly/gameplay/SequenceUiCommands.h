// openTIT - Sequencer commands that drive HUD/GUI elements: dialog pages,
// speaker dialogs, HUD element animations and the cutscene letterbox.
#pragma once

#include "sly/gameplay/Sequencer.h"

#include <cstdint>
#include <vector>

namespace sly {
namespace gameplay {

struct GuiAnimation;
class ScriptCondition;

// Plays the animation of a fixed HUD element and waits for it (0x00B21088).
class SeqHudElementCommand : public SequenceCommand {
public:
    explicit SeqHudElementCommand(bool blocking);
    ~SeqHudElementCommand() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    void* m_element = nullptr;   // +0x0C
};

// One speaker of a dialog (0x0C-byte records).
struct DialogSpeaker {
    uint32_t nameHash = 0;            // +0x00 sequence entity name
    WorldObject* entity = nullptr;    // +0x04 resolved at Execute()
    uint8_t flags = 0;                // +0x08
};

// Shows a speaker dialog with portraits (0x00B210A0).
class SeqSpeakerDialogCommand : public SequenceCommand {
public:
    SeqSpeakerDialogCommand(const std::vector<DialogSpeaker>& speakers, const char* textKey,
                            const char* soundKey, uint32_t screenHash, uint32_t param,
                            bool notifyScreen, bool blocking, bool pauseGame);
    void Execute() override;
    bool Update(float dt) override;

private:
    char m_textKey[0x41] = {};             // +0x09
    char m_soundKey[0x41] = {};            // +0x4A
    std::vector<DialogSpeaker> m_speakers; // +0x8C
    uint32_t m_screenHash = 0;             // +0xA0
    uint32_t m_param = 0;                  // +0xA4
    bool m_pauseGame = false;              // +0xA8 bit 31
    bool m_notifyScreen = false;           // +0xA8 bit 30
};

// Shows one or more pages of text, optionally behind a yes/no prompt
// ("bumper" prompt, 0x00B210D0).
class SeqDialogPagesCommand : public SequenceCommand {
public:
    static constexpr int kMaxPages = 4;

    SeqDialogPagesCommand(const char (*pages)[0x80], int pageCount, const int32_t pos[2],
                          const int32_t size[2], ScriptCondition* condition, bool blocking,
                          bool letterboxOn, const uint8_t colorA[4], const uint8_t colorB[4],
                          bool restoreLetterbox, bool askFirst, bool flagA, bool flagB,
                          bool flag0);
    ~SeqDialogPagesCommand() override;
    void Execute() override;
    bool Update(float dt) override;

private:
    static void OnYes(void* user);
    static void OnNo(void* user);
    void ShowPages();

    char m_pages[kMaxPages][0x80] = {};    // +0x09
    int32_t m_pageCount = 0;               // +0x20C
    int32_t m_pos[2] = {};                 // +0x210
    int32_t m_size[2] = {};                // +0x218
    ScriptCondition* m_condition = nullptr;// +0x220
    uint8_t m_colorA[4] = {};              // +0x224
    uint8_t m_colorB[4] = {};              // +0x228
    // +0x22C flag byte
    bool m_flag0 = false;                  // bit 0
    bool m_flagB = false;                  // bit 1
    bool m_flagA = false;                  // bit 2
    bool m_askFirst = false;               // bit 3
    bool m_restoreLetterbox = false;       // bit 4
    bool m_letterboxOn = false;            // bit 5
    bool m_shown = false;                  // bit 6
    bool m_done = false;                   // bit 7
};

// Slides the cutscene letterbox in or out (0x00B212B0).
class SeqLetterboxCommand : public SequenceCommand {
public:
    SeqLetterboxCommand(bool show, bool blocking);
    void Execute() override;
    bool Update(float dt) override;

private:
    GuiAnimation* m_animation = nullptr;   // +0x0C
    bool m_show = false;                   // +0x10 bit 31
};

} // namespace gameplay
} // namespace sly
