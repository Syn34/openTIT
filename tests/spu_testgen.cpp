// Writes the SPU test programs as a region record file for spu-recompile.
#include <cstdio>
#include <vector>

#include "spu/spu_analysis.h"
#include "spu_programs.h"

using namespace opentit;

int main(int argc, char** argv) {
    if (argc < 2) return 2;
    std::vector<u8> ls(0x40000, 0);
    auto put = [&](u32 at, const std::vector<u32>& p) {
        for (std::size_t k = 0; k < p.size(); ++k) store_be<u32>(&ls[at + 4 * k], p[k]);
    };
    put(spu_programs::kLoopAt, spu_programs::loop_program());
    put(spu_programs::kCallAt, spu_programs::call_program());
    put(spu_programs::kFuncAt, spu_programs::func_program());
    std::vector<spu::Region> regions = {spu::discover(ls.data(), spu_programs::kLoopAt),
                                        spu::discover(ls.data(), spu_programs::kCallAt),
                                        spu::discover(ls.data(), spu_programs::kFuncAt)};
    return spu::write_records(argv[1], regions) ? 0 : 1;
}
