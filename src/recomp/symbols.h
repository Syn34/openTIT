// Function names for the decomp (decomp/symbols.txt).
//
//     # comment
//     004FC7B4 main
//     00463658 Game_InitFileSystem   // free text after // is a note
#pragma once

#include <map>
#include <string>

#include "common/types.h"

namespace opentit::recomp {

struct Symbol {
    std::string name;
    std::string note;
};

std::map<u32, Symbol> read_symbols(const std::string& path, std::string* error);

}  // namespace opentit::recomp
