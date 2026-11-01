// cellSaveData (the "2" variants Sly 4 imports).
//
// Saves live in <hdd0>/home/00000001/savedata/<dir name>/ with a real
// PARAM.SFO, the same layout as the console and RPCS3. The library is
// callback driven: the game's own functions pick a directory (list/fixed),
// look at its status and describe its metadata (stat), then issue file
// operations one at a time (file) until they report OK_LAST.
//
// There is no save UI: where the console would ask the player, the first
// sensible choice is taken (the focused entry, else the newest, else new data).
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <mutex>

#include "../hle.h"
#include "../log.h"
#include "loader/sfo.h"

using namespace opentit;
using namespace opentit::rtm;
namespace fs = std::filesystem;

namespace {

enum : s32 {
    RET_OK = 0, RET_CANCEL = 1,
    ERR_CBRESULT = static_cast<s32>(0x8002B401), ERR_ACCESS = static_cast<s32>(0x8002B402),
    ERR_PARAM = static_cast<s32>(0x8002B404), ERR_NODATA = static_cast<s32>(0x8002B40B),
    ERR_BUSY = static_cast<s32>(0x8002B408),
    CB_OK_LAST_NOCONFIRM = 2, CB_OK_LAST = 1, CB_OK_NEXT = 0,
};
enum : u32 {
    FOCUS_DIRNAME = 0, FOCUS_LISTHEAD, FOCUS_LISTTAIL, FOCUS_LATEST, FOCUS_OLDEST, FOCUS_NEWDATA,
    OP_READ = 0, OP_WRITE, OP_DELETE, OP_WRITE_NOTRUNC,
    TYPE_SECURE = 0, TYPE_NORMAL, TYPE_ICON0, TYPE_ICON1, TYPE_PIC1, TYPE_SND0,
    RECREATE_YES = 2, RECREATE_YES_RESET_OWNER = 3,
};

// Guest structure sizes / offsets (see cellSaveData.h on the console SDK)
constexpr u32 kDirListSize = 48;        // dirName[32], listParam[8], reserved[8]
constexpr u32 kFileStatSize = 56;
constexpr u32 kSysParamTitle = 0, kSysParamSub = 128, kSysParamDetail = 256, kSysParamAttr = 1280,
              kSysParamParental = 1284, kSysParamList = 1288, kSysParamSize = 1552;
// CellSaveDataStatGet
constexpr u32 SG_FREE = 0, SG_NEW = 4, SG_DIR = 8, SG_PARAM = 64, SG_BIND = 1616, SG_SIZE = 1620, SG_SYS = 1624,
              SG_FILENUM = 1628, SG_FILELISTNUM = 1632, SG_FILELIST = 1636, SG_TOTAL = 1704;

// Scratch layout for the structures we hand to callbacks.
constexpr u32 S_CB = 0x000, S_LISTGET = 0x040, S_LISTSET = 0x100, S_FIXEDSET = 0x140, S_STATGET = 0x200,
              S_STATSET = 0x900, S_FILEGET = 0x940, S_FILESET = 0x9C0, S_SIZE = 0x1000;

std::mutex g_busy;
u32 g_scratch = 0;

fs::path save_root() { return fs::path(config().hdd0) / "home" / "00000001" / "savedata"; }

void put_str(u32 ea, const std::string& s, u32 size) {
    std::memset(mem::ptr(ea), 0, size);
    std::memcpy(mem::ptr(ea), s.data(), std::min<std::size_t>(s.size(), size - 1));
}

std::string sfo_str(const std::map<std::string, loader::SfoValue>& m, const char* k) {
    auto it = m.find(k);
    if (it == m.end()) return {};
    if (const auto* s = std::get_if<std::string>(&it->second)) return *s;
    return {};
}
u32 sfo_u32(const std::map<std::string, loader::SfoValue>& m, const char* k) {
    auto it = m.find(k);
    if (it == m.end()) return 0;
    if (const auto* v = std::get_if<u32>(&it->second)) return *v;
    return 0;
}

s64 unix_time(const fs::file_time_type& t) {
    using namespace std::chrono;
    const auto sys = time_point_cast<seconds>(t - fs::file_time_type::clock::now() + system_clock::now());
    return sys.time_since_epoch().count();
}

struct Entry {
    std::string name;
    std::map<std::string, loader::SfoValue> sfo;
    s64 mtime = 0;
};

// Saves whose names start with any of the '|'-separated prefixes.
std::vector<Entry> list_saves(const std::string& prefixes) {
    std::vector<std::string> pre;
    std::size_t at = 0;
    while (at <= prefixes.size()) {
        const std::size_t bar = prefixes.find('|', at);
        pre.push_back(prefixes.substr(at, bar == std::string::npos ? std::string::npos : bar - at));
        if (bar == std::string::npos) break;
        at = bar + 1;
    }
    std::vector<Entry> out;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(save_root(), ec)) {
        if (!e.is_directory()) continue;
        const std::string n = e.path().filename().string();
        if (!std::any_of(pre.begin(), pre.end(), [&](const std::string& p) { return n.rfind(p, 0) == 0; })) continue;
        auto sfo = loader::read_sfo((e.path() / "PARAM.SFO").string(), nullptr);
        if (!sfo) continue;  // not a complete save
        Entry en;
        en.name = n;
        en.sfo = *sfo;
        en.mtime = unix_time(fs::last_write_time(e.path() / "PARAM.SFO", ec));
        out.push_back(std::move(en));
    }
    std::sort(out.begin(), out.end(), [](const Entry& a, const Entry& b) { return a.mtime > b.mtime; });
    return out;
}

// Fill CellSaveDataListGet with the matching saves (dir list goes in setBuf->buf).
void fill_list(u32 listget, u32 setbuf, const std::vector<Entry>& saves) {
    std::memset(mem::ptr(listget), 0, 76);
    const u32 max = mem::r32(setbuf);
    const u32 buf = mem::r32(setbuf + 36), bufsize = mem::r32(setbuf + 32);
    const u32 n = std::min<u32>({static_cast<u32>(saves.size()), max, bufsize / kDirListSize});
    mem::w32(listget, static_cast<u32>(saves.size()));
    mem::w32(listget + 4, n);
    mem::w32(listget + 8, buf);
    for (u32 k = 0; k < n; ++k) {
        const u32 d = buf + k * kDirListSize;
        std::memset(mem::ptr(d), 0, kDirListSize);
        put_str(d, saves[k].name, 32);
        put_str(d + 32, sfo_str(saves[k].sfo, "SAVEDATA_LIST_PARAM"), 8);
    }
}

std::string file_name_for(u32 type, u32 name_ptr) {
    switch (type) {
    case TYPE_ICON0: return "ICON0.PNG";
    case TYPE_ICON1: return "ICON1.PAM";
    case TYPE_PIC1: return "PIC1.PNG";
    case TYPE_SND0: return "SND0.AT3";
    default: return name_ptr ? mem::read_string(name_ptr, 13) : std::string();
    }
}

void write_param_sfo(const fs::path& dir, const std::string& dir_name, u32 param) {
    std::map<std::string, loader::SfoValue> v;
    v["CATEGORY"] = std::string("SD");
    v["TITLE"] = mem::read_string(param + kSysParamTitle, 128);
    v["SUB_TITLE"] = mem::read_string(param + kSysParamSub, 128);
    v["DETAIL"] = mem::read_string(param + kSysParamDetail, 1024);
    v["ATTRIBUTE"] = mem::r32(param + kSysParamAttr);
    v["PARENTAL_LEVEL"] = mem::r32(param + kSysParamParental);
    v["SAVEDATA_LIST_PARAM"] = mem::read_string(param + kSysParamList, 8);
    v["SAVEDATA_DIRECTORY"] = dir_name;
    v["PARAMS"] = std::string(1024, '\0');
    v["PARAMS2"] = std::string(12, '\0');
    const std::map<std::string, u32> sizes = {{"TITLE", 128}, {"SUB_TITLE", 128}, {"DETAIL", 1024},
                                              {"SAVEDATA_LIST_PARAM", 8}, {"SAVEDATA_DIRECTORY", 64}, {"CATEGORY", 4}};
    loader::write_sfo((dir / "PARAM.SFO").string(), v, sizes);
}

// The stat + file phase for one directory.
s32 run_dir(PPUContext& ctx, const std::string& dir_name, bool saving, u32 setbuf, u32 func_stat, u32 func_file,
            u32 userdata) {
    const u32 cb = g_scratch + S_CB, sg = g_scratch + S_STATGET, ss = g_scratch + S_STATSET;
    const u32 fg = g_scratch + S_FILEGET, fset = g_scratch + S_FILESET;
    const fs::path dir = save_root() / dir_name;
    std::error_code ec;
    const bool exists = fs::exists(dir / "PARAM.SFO", ec);
    const auto sfo = exists ? loader::read_sfo((dir / "PARAM.SFO").string(), nullptr) : std::nullopt;

    // ---- CellSaveDataStatGet
    std::memset(mem::ptr(sg), 0, SG_TOTAL);
    mem::w32(sg + SG_FREE, 1024 * 1024);  // 1 GiB free
    mem::w32(sg + SG_NEW, exists ? 0 : 1);
    put_str(sg + SG_DIR + 24, dir_name, 32);
    std::vector<fs::directory_entry> files;
    u64 bytes = 0;
    if (exists) {
        const s64 t = unix_time(fs::last_write_time(dir, ec));
        mem::w64(sg + SG_DIR, static_cast<u64>(t));
        mem::w64(sg + SG_DIR + 8, static_cast<u64>(t));
        mem::w64(sg + SG_DIR + 16, static_cast<u64>(t));
        if (sfo) {
            put_str(sg + SG_PARAM + kSysParamTitle, sfo_str(*sfo, "TITLE"), 128);
            put_str(sg + SG_PARAM + kSysParamSub, sfo_str(*sfo, "SUB_TITLE"), 128);
            put_str(sg + SG_PARAM + kSysParamDetail, sfo_str(*sfo, "DETAIL"), 1024);
            mem::w32(sg + SG_PARAM + kSysParamAttr, sfo_u32(*sfo, "ATTRIBUTE"));
            mem::w32(sg + SG_PARAM + kSysParamParental, sfo_u32(*sfo, "PARENTAL_LEVEL"));
            put_str(sg + SG_PARAM + kSysParamList, sfo_str(*sfo, "SAVEDATA_LIST_PARAM"), 8);
        }
        for (const auto& e : fs::directory_iterator(dir, ec))
            if (e.is_regular_file() && e.path().filename() != "PARAM.SFO") {
                files.push_back(e);
                bytes += e.file_size(ec);
            }
    }
    const u32 file_max = mem::r32(setbuf + 4);
    const u32 buf = mem::r32(setbuf + 36), bufsize = mem::r32(setbuf + 32);
    const u32 nfiles = std::min<u32>({static_cast<u32>(files.size()), file_max, bufsize / kFileStatSize});
    mem::w32(sg + SG_BIND, 0);
    mem::w32(sg + SG_SIZE, static_cast<u32>((bytes + 1023) / 1024));
    mem::w32(sg + SG_SYS, 35);
    mem::w32(sg + SG_FILENUM, static_cast<u32>(files.size()));
    mem::w32(sg + SG_FILELISTNUM, nfiles);
    mem::w32(sg + SG_FILELIST, buf);
    for (u32 k = 0; k < nfiles; ++k) {
        const u32 f = buf + k * kFileStatSize;
        std::memset(mem::ptr(f), 0, kFileStatSize);
        const std::string n = files[k].path().filename().string();
        const u32 type = n == "ICON0.PNG" ? TYPE_ICON0 : n == "ICON1.PAM" ? TYPE_ICON1 : n == "PIC1.PNG" ? TYPE_PIC1
                         : n == "SND0.AT3" ? TYPE_SND0 : TYPE_NORMAL;
        mem::w32(f, type);
        mem::w64(f + 8, files[k].file_size(ec));
        const s64 t = unix_time(files[k].last_write_time(ec));
        mem::w64(f + 16, static_cast<u64>(t));
        mem::w64(f + 24, static_cast<u64>(t));
        mem::w64(f + 32, static_cast<u64>(t));
        put_str(f + 40, n, 13);
    }

    // ---- stat callback
    std::memset(mem::ptr(cb), 0, 20);
    mem::w32(cb + 16, userdata);
    std::memset(mem::ptr(ss), 0, 12);
    hle::call_guest(ctx, func_stat, {cb, sg, ss});
    const s32 stat_result = static_cast<s32>(mem::r32(cb));
    if (stat_result < 0) {
        LOG_INFO("savedata", "'%s': stat callback returned %d", dir_name.c_str(), stat_result);
        return ERR_CBRESULT;
    }
    const u32 set_param = mem::r32(ss), recreate = mem::r32(ss + 4) & 0xFFFF;
    if (saving || set_param) {
        if (!exists && !set_param) return ERR_PARAM;  // new data needs its metadata
        fs::create_directories(dir, ec);
        if (recreate == RECREATE_YES || recreate == RECREATE_YES_RESET_OWNER) {
            for (const auto& f : files) fs::remove(f.path(), ec);
        }
        if (set_param) write_param_sfo(dir, dir_name, set_param);
    } else if (!exists) {
        LOG_INFO("savedata", "'%s' does not exist", dir_name.c_str());
    }
    if (stat_result == CB_OK_LAST || stat_result == CB_OK_LAST_NOCONFIRM) return RET_OK;

    // ---- file callbacks
    for (int guard = 0; guard < 4096; ++guard) {
        std::memset(mem::ptr(cb), 0, 20);
        mem::w32(cb + 16, userdata);
        std::memset(mem::ptr(fg), 0, 68);
        std::memset(mem::ptr(fset), 0, 48);
        hle::call_guest(ctx, func_file, {cb, fg, fset});
        const s32 r = static_cast<s32>(mem::r32(cb));
        if (r < 0) {
            LOG_INFO("savedata", "'%s': file callback returned %d", dir_name.c_str(), r);
            return ERR_CBRESULT;
        }
        if (r == CB_OK_LAST || r == CB_OK_LAST_NOCONFIRM) break;
        const u32 op = mem::r32(fset), type = mem::r32(fset + 8);
        const std::string name = file_name_for(type, mem::r32(fset + 28));
        const u32 offset = mem::r32(fset + 32), size = mem::r32(fset + 36), bufsz = mem::r32(fset + 40);
        const u32 fbuf = mem::r32(fset + 44);
        if (name.empty() || name.find('/') != std::string::npos || name.find("..") != std::string::npos) {
            LOG_WARN("savedata", "bad file name in save '%s'", dir_name.c_str());
            return ERR_PARAM;
        }
        const fs::path path = dir / name;
        u32 done = 0;
        switch (op) {
        case OP_READ: {
            std::ifstream in(path, std::ios::binary);
            if (!in) {
                LOG_WARN("savedata", "read of missing file %s/%s", dir_name.c_str(), name.c_str());
                break;
            }
            in.seekg(offset);
            in.read(reinterpret_cast<char*>(mem::ptr(fbuf)), std::min(size, bufsz));
            done = static_cast<u32>(in.gcount());
            break;
        }
        case OP_WRITE: case OP_WRITE_NOTRUNC: {
            fs::create_directories(dir, ec);
            std::fstream out;
            if (op == OP_WRITE || !fs::exists(path, ec)) {
                out.open(path, std::ios::binary | std::ios::out | std::ios::trunc);
            } else {
                out.open(path, std::ios::binary | std::ios::in | std::ios::out);
            }
            if (!out) return ERR_ACCESS;
            out.seekp(offset);
            const u32 n = std::min(size, bufsz);
            out.write(reinterpret_cast<const char*>(mem::ptr(fbuf)), n);
            done = out ? n : 0;
            break;
        }
        case OP_DELETE:
            fs::remove(path, ec);
            break;
        default:
            return ERR_PARAM;
        }
        mem::w32(fg, done);
        LOG_DEBUG("savedata", "%s %s/%s: %u bytes", op == OP_READ ? "read" : op == OP_DELETE ? "delete" : "write",
                  dir_name.c_str(), name.c_str(), done);
    }
    LOG_INFO("savedata", "%s '%s'", saving ? "saved" : "loaded", dir_name.c_str());
    return RET_OK;
}

void ensure_scratch() {
    if (!g_scratch) g_scratch = alloc_guest(S_SIZE, 0x80);
}

// ListSave2 / ListLoad2:
// (version, setList, setBuf, funcList, funcStat, funcFile, container, userdata)
s32 list_op(PPUContext& ctx, bool saving) {
    const u32 set_list = arg32(ctx, 1), setbuf = arg32(ctx, 2), func_list = arg32(ctx, 3);
    const u32 func_stat = arg32(ctx, 4), func_file = arg32(ctx, 5), userdata = arg32(ctx, 7);
    if (!set_list || !setbuf || !func_list || !func_stat || !func_file) return ERR_PARAM;
    const std::string prefix = mem::read_string(mem::r32(set_list + 8), 256);
    const auto saves = list_saves(prefix);
    const u32 cb = g_scratch + S_CB, lg = g_scratch + S_LISTGET, ls = g_scratch + S_LISTSET;
    fill_list(lg, setbuf, saves);
    std::memset(mem::ptr(cb), 0, 20);
    mem::w32(cb + 16, userdata);
    std::memset(mem::ptr(ls), 0, 24);
    hle::call_guest(ctx, func_list, {cb, lg, ls});
    if (static_cast<s32>(mem::r32(cb)) < 0) return ERR_CBRESULT;

    // The game may narrow the list down to fixedList; otherwise the full list stands.
    std::vector<std::string> shown;
    const u32 fixed_n = mem::r32(ls + 8), fixed = mem::r32(ls + 12);
    if (fixed && fixed_n) {
        for (u32 k = 0; k < fixed_n; ++k) shown.push_back(mem::read_string(fixed + k * kDirListSize, 32));
    } else {
        for (const auto& s : saves) shown.push_back(s.name);
    }
    const u32 focus = mem::r32(ls);
    const u32 new_data = mem::r32(ls + 16);
    std::string pick;
    if (focus == FOCUS_DIRNAME && mem::r32(ls + 4)) pick = mem::read_string(mem::r32(ls + 4), 32);
    if (saving && new_data && (focus == FOCUS_NEWDATA || shown.empty())) pick = mem::read_string(mem::r32(new_data + 4), 32);
    if (pick.empty() && !shown.empty()) {
        if (focus == FOCUS_LISTTAIL || focus == FOCUS_OLDEST) pick = shown.back();
        else pick = shown.front();  // head / latest (the list is newest first)
    }
    if (pick.empty() && saving && new_data) pick = mem::read_string(mem::r32(new_data + 4), 32);
    if (pick.empty()) {
        LOG_INFO("savedata", "no save data to %s (prefix '%s')", saving ? "overwrite" : "load", prefix.c_str());
        return RET_CANCEL;  // what the player would see as "no data" / backing out
    }
    return run_dir(ctx, pick, saving, setbuf, func_stat, func_file, userdata);
}

}  // namespace

OPENTIT_HLE(cellSysutil, cellSaveDataListSave2) {
    std::unique_lock<std::mutex> lk(g_busy, std::try_to_lock);
    if (!lk) return ret(ctx, static_cast<u32>(ERR_BUSY));
    ensure_scratch();
    ret(ctx, static_cast<u32>(list_op(ctx, true)));
}

OPENTIT_HLE(cellSysutil, cellSaveDataListLoad2) {
    std::unique_lock<std::mutex> lk(g_busy, std::try_to_lock);
    if (!lk) return ret(ctx, static_cast<u32>(ERR_BUSY));
    ensure_scratch();
    ret(ctx, static_cast<u32>(list_op(ctx, false)));
}

// (version, setList, setBuf, funcFixed, funcStat, funcFile, container, userdata)
OPENTIT_HLE(cellSysutil, cellSaveDataFixedLoad2) {
    std::unique_lock<std::mutex> lk(g_busy, std::try_to_lock);
    if (!lk) return ret(ctx, static_cast<u32>(ERR_BUSY));
    ensure_scratch();
    const u32 set_list = arg32(ctx, 1), setbuf = arg32(ctx, 2), func_fixed = arg32(ctx, 3);
    const u32 func_stat = arg32(ctx, 4), func_file = arg32(ctx, 5), userdata = arg32(ctx, 7);
    if (!set_list || !setbuf || !func_fixed || !func_stat || !func_file) return ret(ctx, static_cast<u32>(ERR_PARAM));
    const auto saves = list_saves(mem::read_string(mem::r32(set_list + 8), 256));
    const u32 cb = g_scratch + S_CB, lg = g_scratch + S_LISTGET, fx = g_scratch + S_FIXEDSET;
    fill_list(lg, setbuf, saves);
    std::memset(mem::ptr(cb), 0, 20);
    mem::w32(cb + 16, userdata);
    std::memset(mem::ptr(fx), 0, 12);
    hle::call_guest(ctx, func_fixed, {cb, lg, fx});
    if (static_cast<s32>(mem::r32(cb)) < 0) return ret(ctx, static_cast<u32>(ERR_CBRESULT));
    const u32 name = mem::r32(fx);
    if (!name) return ret(ctx, static_cast<u32>(ERR_PARAM));
    ret(ctx, static_cast<u32>(run_dir(ctx, mem::read_string(name, 32), false, setbuf, func_stat, func_file, userdata)));
}

// (version, dirName, errDialog, setBuf, funcStat, funcFile, container, userdata)
static void auto_op(PPUContext& ctx, bool saving) {
    std::unique_lock<std::mutex> lk(g_busy, std::try_to_lock);
    if (!lk) return ret(ctx, static_cast<u32>(ERR_BUSY));
    ensure_scratch();
    const u32 dir = arg32(ctx, 1), setbuf = arg32(ctx, 3), func_stat = arg32(ctx, 4), func_file = arg32(ctx, 5);
    if (!dir || !setbuf || !func_stat || !func_file) return ret(ctx, static_cast<u32>(ERR_PARAM));
    ret(ctx, static_cast<u32>(run_dir(ctx, mem::read_string(dir, 32), saving, setbuf, func_stat, func_file, arg32(ctx, 7))));
}

OPENTIT_HLE(cellSysutil, cellSaveDataAutoSave2) { auto_op(ctx, true); }
OPENTIT_HLE(cellSysutil, cellSaveDataAutoLoad2) { auto_op(ctx, false); }

// The console shows a delete dialog here; there is none, so the player "backs out".
OPENTIT_HLE(cellSysutil, cellSaveDataDelete2) { ret(ctx, static_cast<u32>(RET_CANCEL)); }
