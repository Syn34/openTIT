// openTIT - Minimal view of the engine's global frame clock.
#include "ape/sys/ApeFrameClock.h"

namespace ape {

FrameClock& GetFrameClock()
{
    static FrameClock s_clock;
    return s_clock;
}

} // namespace ape
