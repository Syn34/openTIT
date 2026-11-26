// openTIT - The game's system message dialog (save errors, trophies, the
// "do not turn off" notices): one dialog at a time, its answer polled by the
// game shell (0x00468944, 0x004688C8, 0x0048CE2C, callback 0x00491180).
#include "sly/game/SystemMessages.h"

#include "ape/core/ApeSysUtil.h"
#include "ape/sys/ApeGameContent.h"
#include "sly/save/SaveGame.h"
#include "platform/Platform.h"

namespace sly {

namespace {

// Dialog state (0x010BB850): 0 none, 1 answered yes/ok, 2 answered no,
// 3 open.
int g_systemMessageState = kSystemMessage_None;

} // namespace

// @func 0x00491180 OnSystemMessageAnswer
void OnSystemMessageAnswer(int result, void* /*userdata*/)
{
    if (result == plat::kDialog_Yes)
        g_systemMessageState = kSystemMessage_Yes;
    else if (result == plat::kDialog_No)
        g_systemMessageState = kSystemMessage_No;
}

// @func 0x00468944 ShowSystemMessage
bool ShowSystemMessage(const char* text, SystemMessageKind kind)
{
    if (g_systemMessageState != kSystemMessage_None) {
        plat::AbortMessageDialog();
        g_systemMessageState = kSystemMessage_None;
    }
    plat::MessageDialogStyle style;
    style.showBackground = kind == kMessage_Notice;
    style.canCancel = false;
    switch (kind) {
    case kMessage_Notice:
    case kMessage_Ok:
        style.buttons = plat::DialogButtons::Ok;
        break;
    case kMessage_YesNo:
        style.buttons = plat::DialogButtons::YesNo;
        style.defaultNo = true;
        break;
    case kMessage_Busy:
        style.buttons = plat::DialogButtons::None;
        break;
    default:
        // Plain text that can be cancelled.
        style.buttons = plat::DialogButtons::None;
        style.canCancel = true;
        break;
    }
    if (!plat::OpenMessageDialog(text, style, OnSystemMessageAnswer, nullptr))
        return false;
    g_systemMessageState = kSystemMessage_Open;
    return true;
}

// @func 0x0048CE2C ShowSystemErrorMessage
bool ShowSystemErrorMessage(uint32_t errorCode)
{
    if (g_systemMessageState != kSystemMessage_None)
        return false;
    if (!plat::OpenErrorDialog(errorCode, OnSystemMessageAnswer, nullptr))
        return false;
    g_systemMessageState = kSystemMessage_Open;
    return true;
}

// @func 0x004688C8 PollSystemMessage
int PollSystemMessage()
{
    if (g_systemMessageState == kSystemMessage_None)
        return kSystemMessage_None;
    ape::SysUtilCheckCallbacks();
    if (g_systemMessageState == kSystemMessage_Open)
        return kSystemMessage_Open;
    plat::CloseMessageDialog();
    const int answer = g_systemMessageState;
    g_systemMessageState = kSystemMessage_None;
    return answer;
}

// @func 0x0049AF80 QuerySaveSpace
// Space the save data needs (waiting for the save worker to measure it)
// and the free space of the hard disk, in kilobytes.
bool QuerySaveSpace(int64_t* required, int64_t* freeKB)
{
    int32_t needed = RequiredSpace();
    while (needed < 0)
        needed = RequiredSpace();
    *required = needed;
    *freeKB = ape::GetHddFreeSizeKB();
    return true;
}

} // namespace sly
