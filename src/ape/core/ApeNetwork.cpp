// openTIT - PSN (NP) bring-up.
#include "ape/core/ApeNetwork.h"

namespace ape {

// @func 0x0056C0B4 NpInit
// The console version loads the NP system module, allocates the NP memory
// pool from the engine heap and calls sceNpInit. There is no PlayStation
// Network on PC, so the online layer reports itself as unavailable.
bool NpInit(uint32_t poolSize)
{
    (void)poolSize;
    return false;
}

} // namespace ape
