#pragma once

#include "opentit/spu_recomp.h"

namespace opentit::spu {

bool dispatch_enabled();
// The recompiled region for `pc`, if local store holds its code; else null.
const CompiledRegion* find_region(SPUThread& t, u32 pc);
void release_region_cache(SPUThread& t);

}  // namespace opentit::spu
