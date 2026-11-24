// openTIT - Per-player notification flags.
#include "sly/levels/PlayerNotifyFlags.h"

#include "ape/core/ApeString.h"

namespace sly {
namespace levels {

namespace {
// Flag names by bit (console table 0x00F6453C). NOTE: the names behind the
// hashes are not known.
constexpr uint32_t kNotifyFlagNames[PlayerNotifyFlags::kFlagCount] = {
    0x4A4CFD32u, 0xAFD10295u, 0x8AFD0656u, 0x06F637F9u, 0xF8D84999u, 0x811B12AFu,
    0x2D8453C6u, 0x838099D7u, 0xC7AA004Fu, 0xB0C7745Au, 0xC7259516u, 0xFE7AD7FBu,
    0x3599CF10u, 0xFCE2BA11u, 0x5D4F7873u, 0x38AE6FF8u, 0x41C66898u, 0xFB756250u,
};
} // namespace

int PlayerNotifyFlags::FlagIndex(uint32_t nameHash)
{
    for (int i = 0; i < kFlagCount; ++i) {
        if (kNotifyFlagNames[i] == nameHash)
            return i;
    }
    return -1;
}

// @func 0x000AE2BC PlayerNotifyFlags::Set
void PlayerNotifyFlags::Set(uint32_t nameHash, bool value)
{
    if (nameHash == 0)
        return;
    int index = FlagIndex(nameHash);
    if (index < 0) {
        ape::TempHexString(nameHash);  // the release build's warning prints nothing
        return;
    }
    if (value)
        m_bits |= 1u << index;
    else
        m_bits &= ~(1u << index);
}

// @func 0x000AE35C PlayerNotifyFlags::IsSet
bool PlayerNotifyFlags::IsSet(uint32_t nameHash) const
{
    if (nameHash == 0)
        return false;
    int index = FlagIndex(nameHash);
    if (index < 0) {
        ape::TempHexString(nameHash);
        return false;
    }
    return (m_bits & (1u << index)) != 0;
}

// TODO: the player profile records live in the game world (not ported).
PlayerNotifyFlags* PlayerNotifyFlagsOf(int playerIndex)
{
    (void)playerIndex;
    return nullptr;
}

} // namespace levels
} // namespace sly
