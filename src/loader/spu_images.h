// Discovery of SPU ELF images embedded in the PPU executable (SPURS tasks,
// EDGE libraries, raw SPU programs).
#pragma once

#include <string>
#include <vector>

#include "common/types.h"
#include "loader/elf.h"

namespace opentit::loader {

struct SpuSegment {
    u32 ls_addr, file_offset, filesz, memsz, flags;
};

struct SpuImage {
    u32 file_offset = 0;  // offset of the ELF header inside the PPU ELF file
    u32 vaddr = 0;        // PPU address of the image (0 if outside loaded segments)
    u32 size = 0;         // bytes spanned by the image in the file
    u32 entry = 0;        // LS entry point
    std::vector<SpuSegment> segments;
    std::string hint;     // a recognisable string from the image (e.g. source file name)
};

std::vector<SpuImage> find_spu_images(const Elf& elf);

}  // namespace opentit::loader
