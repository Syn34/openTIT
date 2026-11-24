// openTIT - Sequencer commands that drive HUD/GUI elements (see
// SequenceUiCommands.h).
#include "sly/gameplay/SequenceUiCommands.h"

#include "ape/core/ApeString.h"
#include "sly/gameplay/GameplayWorld.h"

#include <cstring>

namespace sly {
namespace gameplay {

namespace {

// HUD element animated by SeqHudElementCommand.
constexpr uint32_t kHudElementHash = 0xF6B16611;

// GUI screen and scripts of the cutscene letterbox.
constexpr const char* kLetterboxScreen = "InGame.Cutscene.Letterbox";
constexpr const char* kLetterboxInit = "Letterbox_Init";
constexpr const char* kLetterboxShutdown = "Letterbox_Shutdown";

// Localisation keys of the "bumper" prompt.
constexpr const char* kBumperPromptText = "$HUD_PROMPT_BUMPER";
constexpr const char* kYesNoButtons = "$BUTTON_X $STR_YES $BUTTON_CIRCLE $STR_NO";

void CopyKey(char* dst, size_t dstSize, const char* src)
{
    if (src == nullptr || src[0] == '\0') {
        dst[0] = '\0';
        return;
    }
    ape::StrCopyBounded(dst, src, dstSize);
}

} // namespace

// ---------------------------------------------------------------------------
// SeqHudElementCommand
// ---------------------------------------------------------------------------

// @func 0x00433424 SeqHudElementCommand::SeqHudElementCommand
SeqHudElementCommand::SeqHudElementCommand(bool blocking)
    : SequenceCommand(blocking)
{
}

// @func 0x00433464 SeqHudElementCommand::~SeqHudElementCommand
// @func 0x00433494 SeqHudElementCommand::~SeqHudElementCommand (deleting)
SeqHudElementCommand::~SeqHudElementCommand() = default;

// @func 0x004334F0 SeqHudElementCommand::Execute
void SeqHudElementCommand::Execute()
{
    m_element = FindHudElement(kHudElementHash);
    HudPlayElement(m_element);
    MarkStarted();
}

// @func 0x00433570 SeqHudElementCommand::Update
bool SeqHudElementCommand::Update(float /*dt*/)
{
    return m_element == nullptr || HudElementDone(m_element);
}

// ---------------------------------------------------------------------------
// SeqSpeakerDialogCommand
// ---------------------------------------------------------------------------

// @func 0x004335C0 SeqSpeakerDialogCommand::SeqSpeakerDialogCommand
SeqSpeakerDialogCommand::SeqSpeakerDialogCommand(const std::vector<DialogSpeaker>& speakers,
                                                 const char* textKey, const char* soundKey,
                                                 uint32_t screenHash, uint32_t param,
                                                 bool notifyScreen, bool blocking, bool pauseGame)
    : SequenceCommand(blocking), m_speakers(speakers), m_screenHash(screenHash), m_param(param),
      m_pauseGame(pauseGame), m_notifyScreen(notifyScreen)
{
    CopyKey(m_textKey, 0x40, textKey);
    CopyKey(m_soundKey, 0x40, soundKey);
}

// @func 0x004337D0 SeqSpeakerDialogCommand::Execute
void SeqSpeakerDialogCommand::Execute()
{
    // Missing speakers were reported (compiled out) and stay null.
    for (DialogSpeaker& speaker : m_speakers)
        speaker.entity = FindSequenceEntity(speaker.nameHash);
    if (m_notifyScreen)
        HudScreenNotifyDialog(m_screenHash);
    ShowSpeakerDialog(m_speakers.data(), static_cast<int>(m_speakers.size()), m_textKey,
                      m_soundKey, m_screenHash, m_param, m_pauseGame);
    MarkStarted();
}

// @func 0x00433914 SeqSpeakerDialogCommand::Update
bool SeqSpeakerDialogCommand::Update(float /*dt*/)
{
    return !SpeakerDialogBusy();
}

// ---------------------------------------------------------------------------
// SeqDialogPagesCommand
// ---------------------------------------------------------------------------

// @func 0x00433B2C SeqDialogPagesCommand::SeqDialogPagesCommand
SeqDialogPagesCommand::SeqDialogPagesCommand(const char (*pages)[0x80], int pageCount,
                                             const int32_t pos[2], const int32_t size[2],
                                             ScriptCondition* condition, bool blocking,
                                             bool letterboxOn, const uint8_t colorA[4],
                                             const uint8_t colorB[4], bool restoreLetterbox,
                                             bool askFirst, bool flagA, bool flagB, bool flag0)
    : SequenceCommand(blocking), m_pageCount(pageCount), m_pos{pos[0], pos[1]},
      m_size{size[0], size[1]}, m_condition(condition), m_flag0(flag0), m_flagB(flagB),
      m_flagA(flagA), m_askFirst(askFirst), m_restoreLetterbox(restoreLetterbox),
      m_letterboxOn(letterboxOn)
{
    std::memcpy(m_colorA, colorA, sizeof(m_colorA));
    std::memcpy(m_colorB, colorB, sizeof(m_colorB));
    for (int i = 0; i < m_pageCount && i < kMaxPages; ++i)
        ape::StrNCopy(m_pages[i], pages[i], 0x7F);
}

// @func 0x00433D10 SeqDialogPagesCommand::~SeqDialogPagesCommand
// @func 0x00433D40 SeqDialogPagesCommand::~SeqDialogPagesCommand (deleting)
SeqDialogPagesCommand::~SeqDialogPagesCommand() = default;

void SeqDialogPagesCommand::ShowPages()
{
    m_shown = true;
    ShowDialogPages(m_pages, m_pageCount, m_pos, m_size, m_flagA, m_flagB, m_colorA, m_colorB);
}

// @func 0x00433D98 SeqDialogPagesCommand::Execute
void SeqDialogPagesCommand::Execute()
{
    // A condition that does not hold skips the pages altogether.
    if (m_condition != nullptr)
        m_done = !EvaluateScriptCondition(m_condition);

    if (!m_done) {
        if (m_askFirst)
            ShowYesNoPrompt(PlayerCharacter(0), kBumperPromptText, kYesNoButtons, &OnYes, &OnNo,
                            this);
        else
            ShowPages();
    }
    MarkStarted();
}

// @func 0x00433F04 SeqDialogPagesCommand::Update
bool SeqDialogPagesCommand::Update(float /*dt*/)
{
    if (m_done)
        return true;
    if (!m_shown)
        return false;   // still waiting for the prompt
    if (GameMode() == kGameModeModalDialog)
        return false;
    if (m_restoreLetterbox)
        SetLetterbox(m_letterboxOn);
    return true;
}

// @func 0x00433FD0 SeqDialogPagesCommand::OnYes
void SeqDialogPagesCommand::OnYes(void* user)
{
    static_cast<SeqDialogPagesCommand*>(user)->ShowPages();
}

// @func 0x00434078 SeqDialogPagesCommand::OnNo
void SeqDialogPagesCommand::OnNo(void* user)
{
    auto* self = static_cast<SeqDialogPagesCommand*>(user);
    self->m_done = true;
    if (self->m_restoreLetterbox)
        SetLetterbox(self->m_letterboxOn);
}

// ---------------------------------------------------------------------------
// SeqLetterboxCommand
// ---------------------------------------------------------------------------

// @func 0x004389D4 SeqLetterboxCommand::SeqLetterboxCommand
SeqLetterboxCommand::SeqLetterboxCommand(bool show, bool blocking)
    : SequenceCommand(blocking), m_show(show)
{
}

// @func 0x00438A3C SeqLetterboxCommand::Execute
void SeqLetterboxCommand::Execute()
{
    m_animation = nullptr;
    if (void* screen = FindGuiScreen(kLetterboxScreen)) {
        // An empty name stops every animation of the screen.
        GuiStopAnimation("", screen);
        const char* script = m_show ? kLetterboxInit : kLetterboxShutdown;
        m_animation = GuiStartAnimation(script, screen, !IsBlocking());
    }
    MarkStarted();
}

// @func 0x00438B00 SeqLetterboxCommand::Update
bool SeqLetterboxCommand::Update(float /*dt*/)
{
    if (m_animation != nullptr) {
        if (GuiAnimationRunning(m_animation))
            return false;
        GuiAnimationDelete(m_animation);
        m_animation = nullptr;
    }
    return true;
}

} // namespace gameplay
} // namespace sly
