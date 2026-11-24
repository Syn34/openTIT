// openTIT - Save front end: error messages shown by the save menus.
#pragma once

namespace sly {

// Reasons the online save transfer (cross-save) can refuse to run
// (console global 0x00F9D1BC).
enum SaveRefusal : int {
    kSaveRefusal_NotYourData     = 0,
    kSaveRefusal_AgeNotVerified  = 1,
    kSaveRefusal_Restricted      = 2,
    kSaveRefusal_NotSignedIn     = 3,
};

// Localised message for a refusal; a placeholder when there is no string
// table or the code is unknown.
const char* SaveRefusalMessage(int refusal);

} // namespace sly
