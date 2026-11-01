// cellSaveData flows driven by host-side stand-ins for the game's callbacks.
#include <filesystem>
#include <fstream>

#include "../runtime/src/hle.h"
#include "loader/sfo.h"
#include "opentit/memory.h"
#include "test.h"

using namespace opentit;
namespace fs = std::filesystem;

namespace {
u32 g_param = 0, g_data = 0, g_name = 0;
int g_file_calls = 0;
bool g_saving = false;
u32 g_seen_new = 99, g_seen_files = 99, g_list_dirs = 99;

void cb_stat(PPUContext& c) {
    const u32 cb = static_cast<u32>(c.gpr[3]), sg = static_cast<u32>(c.gpr[4]), ss = static_cast<u32>(c.gpr[5]);
    g_seen_new = mem::r32(sg + 4);
    g_seen_files = mem::r32(sg + 1628);
    if (g_saving) mem::w32(ss, g_param);  // setParam
    mem::w32(cb, 0);                      // OK_NEXT
    c.gpr[3] = 0;
}
void cb_file(PPUContext& c) {
    const u32 cb = static_cast<u32>(c.gpr[3]), fs_ = static_cast<u32>(c.gpr[5]);
    if (g_file_calls++ == 0) {
        mem::w32(fs_, g_saving ? 1 : 0);  // WRITE / READ
        mem::w32(fs_ + 8, 1);             // normal file
        mem::w32(fs_ + 28, g_name);
        mem::w32(fs_ + 32, 0);
        mem::w32(fs_ + 36, 16);
        mem::w32(fs_ + 40, 16);
        mem::w32(fs_ + 44, g_data);
        mem::w32(cb, 0);
    } else {
        mem::w32(cb, 1);  // OK_LAST
    }
    c.gpr[3] = 0;
}
void cb_list(PPUContext& c) {
    const u32 cb = static_cast<u32>(c.gpr[3]), lg = static_cast<u32>(c.gpr[4]), ls = static_cast<u32>(c.gpr[5]);
    g_list_dirs = mem::r32(lg + 4);
    mem::w32(ls, 3);  // FOCUS_LATEST
    mem::w32(cb, 0);
    c.gpr[3] = 0;
}

u64 invoke(const char* name, std::initializer_list<u64> args) {
    static PPUContext c;
    static bool init = false;
    if (!init) {
        c.stack_size = 0x20000;
        c.stack_addr = rtm::alloc_stack(c.stack_size);
        init = true;
    }
    c.gpr[1] = c.stack_addr + c.stack_size - 0x200;
    int k = 3;
    for (u64 a : args) c.gpr[k++] = a;
    hle::find(name)(c);
    return c.gpr[3];
}
}  // namespace

TEST(SaveDataAutoSaveLoadAndList) {
    const fs::path hdd = fs::temp_directory_path() / "opentit_savedata_test";
    fs::remove_all(hdd);
    rtm::config().hdd0 = hdd.string();
    const u32 stat = hle::make_internal_function("test_stat", &cb_stat);
    const u32 file = hle::make_internal_function("test_file", &cb_file);
    const u32 list = hle::make_internal_function("test_list", &cb_list);
    const u32 work = rtm::alloc_guest(0x4000, 0x80);
    g_param = work;           // CellSaveDataSystemFileParam
    std::memset(mem::ptr(g_param), 0, 1552);
    std::strcpy(reinterpret_cast<char*>(mem::ptr(g_param)), "Sly Cooper");
    std::strcpy(reinterpret_cast<char*>(mem::ptr(g_param + 128)), "Episode 1");
    g_data = work + 0x800;
    for (u32 k = 0; k < 16; ++k) mem::w8(g_data + k, static_cast<u8>(0xA0 + k));
    g_name = work + 0x900;
    std::strcpy(reinterpret_cast<char*>(mem::ptr(g_name)), "DATA.BIN");
    const u32 dir = work + 0x940;
    std::strcpy(reinterpret_cast<char*>(mem::ptr(dir)), "BCES01284-SAVE0");
    const u32 setbuf = work + 0x980;  // dirListMax, fileListMax, reserved[6], bufSize, buf
    std::memset(mem::ptr(setbuf), 0, 40);
    mem::w32(setbuf, 8);
    mem::w32(setbuf + 4, 8);
    mem::w32(setbuf + 32, 0x1000);
    mem::w32(setbuf + 36, work + 0x2000);

    g_saving = true;
    g_file_calls = 0;
    CHECK_EQ(invoke("cellSaveDataAutoSave2", {1, dir, 0, setbuf, stat, file, 0, 0}), 0u);
    CHECK_EQ(g_seen_new, 1u);
    const fs::path save = hdd / "home" / "00000001" / "savedata" / "BCES01284-SAVE0";
    CHECK(fs::exists(save / "DATA.BIN"));
    CHECK_EQ(fs::file_size(save / "DATA.BIN"), 16u);
    const auto sfo = loader::read_sfo((save / "PARAM.SFO").string(), nullptr);
    CHECK(sfo.has_value());
    if (sfo) {
        CHECK_STR(std::get<std::string>(sfo->at("TITLE")), "Sly Cooper");
        CHECK_STR(std::get<std::string>(sfo->at("SUB_TITLE")), "Episode 1");
        CHECK_STR(std::get<std::string>(sfo->at("SAVEDATA_DIRECTORY")), "BCES01284-SAVE0");
    }

    std::memset(mem::ptr(g_data), 0, 16);
    g_saving = false;
    g_file_calls = 0;
    CHECK_EQ(invoke("cellSaveDataAutoLoad2", {1, dir, 0, setbuf, stat, file, 0, 0}), 0u);
    CHECK_EQ(g_seen_new, 0u);
    CHECK_EQ(g_seen_files, 1u);
    CHECK_EQ(mem::r8(g_data + 15), 0xAFu);

    // ListLoad2 finds it by prefix.
    const u32 setlist = work + 0x9C0, prefix = work + 0xA00;
    std::strcpy(reinterpret_cast<char*>(mem::ptr(prefix)), "NOPE|BCES01284-");
    mem::w32(setlist, 0);
    mem::w32(setlist + 4, 0);
    mem::w32(setlist + 8, prefix);
    mem::w32(setlist + 12, 0);
    std::memset(mem::ptr(g_data), 0, 16);
    g_file_calls = 0;
    CHECK_EQ(invoke("cellSaveDataListLoad2", {1, setlist, setbuf, list, stat, file, 0, 0}), 0u);
    CHECK_EQ(g_list_dirs, 1u);
    CHECK_EQ(mem::r8(g_data), 0xA0u);
    // Nothing matching: the player "cancels".
    std::strcpy(reinterpret_cast<char*>(mem::ptr(prefix)), "OTHERGAME");
    CHECK_EQ(invoke("cellSaveDataListLoad2", {1, setlist, setbuf, list, stat, file, 0, 0}), 1u);
    fs::remove_all(hdd);
}
