// openTIT - The game's system message dialog (see SystemMessages.cpp).
#pragma once

#include <cstdint>

namespace sly {

// Dialog kinds of ShowSystemMessage. NOTE: named from their button sets.
enum SystemMessageKind : int {
    kMessage_Text = 0,     // no buttons, cancellable
    kMessage_Ok = 1,       // OK button
    kMessage_YesNo = 2,    // yes/no, the cursor on "no"
    kMessage_Busy = 3,     // no buttons, not cancellable (wait notices)
    kMessage_Notice = 4,   // OK button over the visible game
};

// What PollSystemMessage reports.
enum SystemMessageState : int {
    kSystemMessage_None = 0,
    kSystemMessage_Yes = 1,    // answered yes / OK
    kSystemMessage_No = 2,
    kSystemMessage_Open = 3,   // still up
};

// Opens a message (taking down any dialog that is up). False if the
// system refused it.
bool ShowSystemMessage(const char* text, SystemMessageKind kind);
// Opens the system's text for an error code, unless a dialog is up.
bool ShowSystemErrorMessage(uint32_t errorCode);
// The state of the dialog; an answered dialog is closed and forgotten.
int PollSystemMessage();
// Save space check: the space the save data needs and the free disk space
// (kilobytes).
bool QuerySaveSpace(int64_t* required, int64_t* freeKB);
// Answer callback of the dialogs.
void OnSystemMessageAnswer(int result, void* userdata);

} // namespace sly
