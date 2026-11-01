// SPU regions -> C++ (see runtime/include/opentit/spu_recomp.h for the ABI).
#pragma once

#include <string>
#include <vector>

#include "spu/spu_analysis.h"

namespace opentit::recomp {

struct SpuEmitOptions {
    std::string out_dir;
    u32 instructions_per_file = 20000;
};

// Writes spu_NNNN.cpp files and spu_table.cpp. Returns false on I/O errors.
bool emit_spu(const std::vector<spu::Region>& regions, const SpuEmitOptions& opt, std::string* error);

}  // namespace opentit::recomp
