// Loader for decrypted PS3 PPU executables (ELF64, big endian, CellOS LV2 ABI).
//
// The tools never ship or embed game data: this loader reads the user's own
// decrypted EBOOT.elf at recompile time (and the runtime reads it again at
// boot to populate guest memory).
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/types.h"

namespace opentit::loader {

enum : u32 {
    PT_LOAD = 1,
    PT_TLS = 7,
    PT_PROC_PARAM = 0x60000001,
    PT_PROC_PRX_PARAM = 0x60000002,
};

enum : u32 { PF_X = 1, PF_W = 2, PF_R = 4 };

struct Segment {
    u32 type = 0;
    u32 flags = 0;
    u64 offset = 0;
    u64 vaddr = 0;
    u64 filesz = 0;
    u64 memsz = 0;
    u64 align = 0;
};

struct Section {
    u32 name = 0;
    u32 type = 0;
    u64 flags = 0;
    u64 addr = 0;
    u64 offset = 0;
    u64 size = 0;
    u64 entsize = 0;
};

// sys_process_param_t (PT_PROC_PARAM)
struct ProcessParam {
    u32 size = 0;
    u32 magic = 0;  // 0x13BCC5F6
    u32 version = 0;
    u32 sdk_version = 0;
    s32 primary_prio = 1001;
    u32 primary_stacksize = 0x10000;
    u32 malloc_pagesize = 0x100000;
    u32 ppc_seg = 0;
};

// One imported function or variable resolved through a PRX stub table.
struct Import {
    std::string library;
    u32 nid = 0;
    u32 slot = 0;      // guest address of the fstub/vstub table entry the loader patches
    u32 initial = 0;   // value the slot holds in the file (for functions: the stub trampoline)
    bool is_variable = false;
};

// A function or variable exported by a PRX module.
struct Export {
    std::string library;
    u32 nid = 0;
    u32 addr = 0;  // functions: address of the OPD; variables: address of the data
    bool is_variable = false;
};

// 32-bit PS3 function descriptor (.opd entry).
struct Opd {
    u32 addr = 0;  // where the descriptor lives
    u32 func = 0;  // code address
    u32 toc = 0;   // r2 value
};

class Elf {
public:
    static std::optional<Elf> load(const std::string& path, std::string* error);

    // Load a decrypted PRX (ET_SCE_PPURELEXEC) and relocate it so its first
    // segment starts at `base`; later segments follow at 64 KiB alignment.
    // The result behaves like an executable mapped at fixed addresses.
    static std::optional<Elf> load_prx(const std::string& path, u32 base, std::string* error);
    static std::optional<Elf> load_prx_from_memory(std::vector<u8> file, u32 base, std::string* error);

    bool is_prx() const { return is_prx_; }
    const std::string& module_name() const { return module_name_; }
    const std::vector<Export>& exports() const { return exports_; }
    u32 module_start_opd() const { return module_start_; }
    u32 module_stop_opd() const { return module_stop_; }
    u32 prx_toc() const { return prx_toc_; }
    u32 image_end() const;  // highest mapped address + 1
    const std::vector<u8>& data() const { return data_; }
    u64 entry() const { return entry_; }
    const std::vector<Segment>& segments() const { return segments_; }
    const std::vector<Section>& sections() const { return sections_; }
    const ProcessParam& process_param() const { return proc_param_; }
    const std::vector<Import>& imports() const { return imports_; }
    std::optional<Segment> tls() const { return tls_; }

    // Virtual address helpers (only covers file-backed bytes of PT_LOAD segments).
    const u8* ptr(u64 va, u64 size = 1) const;
    bool is_exec(u64 va) const;
    bool is_mapped(u64 va) const;
    u32 read_u32(u64 va) const;
    u16 read_u16(u64 va) const;
    u8 read_u8(u64 va) const;
    std::string read_cstr(u64 va, std::size_t limit = 256) const;

    // Executable range(s): [start, end)
    struct Range { u64 start, end; };
    std::vector<Range> exec_ranges() const;

    // Entry point descriptor (e_entry points at an OPD on PS3).
    Opd entry_opd() const;

    // Scan the section that holds the entry OPD (the .opd section) for every
    // function descriptor. These are the primary function-start seeds.
    std::vector<Opd> scan_opds() const;

private:
    bool parse(std::string* error);
    bool parse_imports(u32 prx_param_va, std::string* error);
    void parse_stub_records(u32 start, u32 end);
    bool relocate_prx(u32 base, std::string* error);

    std::vector<u8> data_;
    u64 entry_ = 0;
    std::vector<Segment> segments_;
    std::vector<Section> sections_;
    ProcessParam proc_param_;
    std::vector<Import> imports_;
    std::optional<Segment> tls_;

    bool is_prx_ = false;
    std::string module_name_;
    std::vector<Export> exports_;
    u32 module_start_ = 0;
    u32 module_stop_ = 0;
    u32 prx_toc_ = 0;
};

}  // namespace opentit::loader
