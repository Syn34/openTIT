// openTIT - Save front end: error messages (see SaveMessages.h).
#include "sly/entity/SaveMessages.h"

#include "ape/level/ApeGui.h"
#include "ape/xml/ApeStringTable.h"

namespace sly {

// @func 0x000B00FC SaveRefusalMessage
// NOTE: the console reads the refusal from its global; it is a parameter here.
const char* SaveRefusalMessage(int refusal)
{
    static const char* const kUnknown = "[Missing save error code string]";
    const ape::StringTable* strings = ape::GetGuiStringTable();
    if (!strings)
        return kUnknown;
    switch (refusal) {
    case kSaveRefusal_NotYourData:
        return strings->Find("STR_SAVE_NOT_YOUR_DATA", "[Missing SAVEMSG_NOT_YOUR_SAVE_DATA]", false);
    case kSaveRefusal_AgeNotVerified:
        return strings->Find("STR_SAVE_UNABLE_TO_VERIFY_AGE", "[Missing SAVEMSG_UNABLE_TO_VERIFY_AGE]", false);
    case kSaveRefusal_Restricted:
        return strings->Find("STR_SAVE_PARENTAL_RESTRICTIONS", "[Missing SAVEMSG_RESTRICTED]", false);
    case kSaveRefusal_NotSignedIn:
        return strings->Find("STR_SAVE_UNABLE_TO_SIGNIN", "[Missing STR_SAVE_UNABLE_TO_SIGNIN]", false);
    default:
        return kUnknown;
    }
}

} // namespace sly
