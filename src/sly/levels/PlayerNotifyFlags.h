// openTIT - Per-player notification flags.
//
// Every player profile record (game world +0x50, 0xFD8 bytes each, indexed by
// the player's profile byte +0x1070 of the 0x20C80-byte player block) keeps
// a 32-bit word of named notification flags at +0x15C. Scripts set them
// (command 0x19EA0BF7) and test them (IfNotify-style command 0x0007F4EC).
// The names are 18 hashes in a fixed table (console 0x00F6453C).
#pragma once

#include <cstdint>

namespace sly {
namespace levels {

// NOTE: the class name is not known; only the flag word of the record is
// reconstructed.
class PlayerNotifyFlags {
public:
    static constexpr int kFlagCount = 18;

    // Bit index of a flag name, -1 for an unknown name.
    static int FlagIndex(uint32_t nameHash);

    // Sets or clears a flag by name; unknown names are ignored (0x000AE2BC).
    void Set(uint32_t nameHash, bool value);
    // True if the named flag is set; false for unknown names (0x000AE35C).
    bool IsSet(uint32_t nameHash) const;

    uint32_t Bits() const { return m_bits; }

private:
    uint32_t m_bits = 0;  // record +0x15C
};

// Flags of player `playerIndex`'s profile record, null when there is none.
PlayerNotifyFlags* PlayerNotifyFlagsOf(int playerIndex);

} // namespace levels
} // namespace sly
