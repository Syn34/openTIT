#include "recomp/symbols.h"

#include <cctype>
#include <fstream>

namespace opentit::recomp {

std::map<u32, Symbol> read_symbols(const std::string& path, std::string* error) {
    std::map<u32, Symbol> out;
    std::ifstream in(path);
    if (!in) {
        if (error) *error = "cannot open " + path;
        return out;
    }
    std::string line;
    int n = 0;
    while (std::getline(in, line)) {
        ++n;
        std::string note;
        if (const auto c = line.find("//"); c != std::string::npos) {
            note = line.substr(c + 2);
            line.resize(c);
            while (!note.empty() && note.front() == ' ') note.erase(0, 1);
        }
        if (const auto h = line.find('#'); h != std::string::npos) line.resize(h);
        std::size_t p = 0;
        while (p < line.size() && std::isspace(static_cast<unsigned char>(line[p]))) ++p;
        if (p == line.size()) continue;
        std::size_t q = p;
        while (q < line.size() && std::isxdigit(static_cast<unsigned char>(line[q]))) ++q;
        std::size_t r = q;
        while (r < line.size() && std::isspace(static_cast<unsigned char>(line[r]))) ++r;
        std::size_t e = r;
        while (e < line.size() && !std::isspace(static_cast<unsigned char>(line[e]))) ++e;
        if (q == p || e == r) {
            if (error) *error = path + ":" + std::to_string(n) + ": expected '<hex address> <name>'";
            continue;
        }
        out[static_cast<u32>(std::stoul(line.substr(p, q - p), nullptr, 16))] = {line.substr(r, e - r), note};
    }
    return out;
}

}  // namespace opentit::recomp
