// openTIT - Minimal view of the engine's global frame clock.
#include "ape/sys/ApeFrameClock.h"

namespace ape {

FrameClock& GetFrameClock()
{
    static FrameClock s_clock;
    return s_clock;
}

// @func 0x005DBB60 RunFrameClockProgress
void RunFrameClockProgress()
{
    FrameClock& clock = GetFrameClock();
    if (clock.progressCallback)
        clock.progressCallback(clock.progressArg);
}

} // namespace ape
