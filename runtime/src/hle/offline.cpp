// Online services, answered the way an offline console answers them: PSN is
// signed out, sockets fail, connection dialogs fail. Trophy queries return
// empty data (trophies are logged when unlocked, not stored).
#include <cstring>

#include "../hle.h"
#include "../log.h"

using namespace opentit;
using namespace opentit::rtm;

namespace opentit::hle {
u32 net_errno_addr();
}

namespace {
constexpr u32 SCE_NP_ERROR_INVALID_STATE = 0x8002AA0A;
constexpr u32 SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID = 0x8002A1A4;
constexpr u32 CELL_NET_CTL_ERROR_NOT_CONNECTED = 0x80130106;
constexpr u32 SYS_NET_EBADF = 9;

void net_fail(PPUContext& ctx) {
    mem::w32(hle::net_errno_addr(), SYS_NET_EBADF);
    ret(ctx, static_cast<u64>(-1));
}
}  // namespace

// ---------------------------------------------------------------- sys_net (no sockets ever open)
OPENTIT_HLE(sys_net, accept) { net_fail(ctx); }
OPENTIT_HLE(sys_net, bind) { net_fail(ctx); }
OPENTIT_HLE(sys_net, connect) { net_fail(ctx); }
OPENTIT_HLE(sys_net, listen) { net_fail(ctx); }
OPENTIT_HLE(sys_net, recv) { net_fail(ctx); }
OPENTIT_HLE(sys_net, recvfrom) { net_fail(ctx); }
OPENTIT_HLE(sys_net, send) { net_fail(ctx); }
OPENTIT_HLE(sys_net, sendto) { net_fail(ctx); }
OPENTIT_HLE(sys_net, setsockopt) { net_fail(ctx); }
OPENTIT_HLE(sys_net, socketclose) { net_fail(ctx); }
OPENTIT_HLE(sys_net, sys_net_get_sockinfo) { net_fail(ctx); }
OPENTIT_HLE(sys_net, sys_net_show_ifconfig) { net_fail(ctx); }
OPENTIT_HLE(sys_net, socketselect) { ret(ctx, 0); }  // nothing is ever ready

OPENTIT_HLE(sys_net, inet_addr) {
    // Dotted quad -> address in network byte order (big endian, as on the PS3)
    const std::string s = mem::read_string(arg32(ctx, 0), 64);
    u32 parts[4] = {}, n = 0, cur = 0;
    bool digit = false, ok = true;
    for (char c : s) {
        if (c >= '0' && c <= '9') {
            cur = cur * 10 + static_cast<u32>(c - '0');
            digit = true;
            if (cur > 255) ok = false;
        } else if (c == '.' && digit && n < 3) {
            parts[n++] = cur;
            cur = 0;
            digit = false;
        } else {
            ok = false;
        }
    }
    if (!digit || n != 3) ok = false;
    parts[3] = cur;
    ret(ctx, ok ? (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3] : 0xFFFFFFFFu);
}

// ---------------------------------------------------------------- HTTP / SSL / NetCtl
OPENTIT_HLE_OK(cellHttp, cellHttpInit)
OPENTIT_HLE_OK(cellHttp, cellHttpEnd)
OPENTIT_HLE_OK(cellSsl, cellSslInit)
OPENTIT_HLE_OK(cellSsl, cellSslEnd)
OPENTIT_HLE(cellNetCtl, cellNetCtlNetStartDialogLoadAsync) { ret(ctx, CELL_NET_CTL_ERROR_NOT_CONNECTED); }
OPENTIT_HLE_OK(cellNetCtl, cellNetCtlNetStartDialogUnloadAsync)

// ---------------------------------------------------------------- NP (signed out)
OPENTIT_HLE_OK(sceNp, sceNpInit)
OPENTIT_HLE_OK(sceNp, sceNpTerm)
OPENTIT_HLE_OK(sceNp2, sceNp2Init)
OPENTIT_HLE_OK(sceNp, sceNpManagerRegisterCallback)
OPENTIT_HLE_OK(sceNp, sceNpDrmIsAvailable2)  // disc copy: no licence needed
OPENTIT_HLE(sceNp, sceNpManagerGetNpId) { ret(ctx, SCE_NP_ERROR_INVALID_STATE); }
OPENTIT_HLE(sceNp, sceNpManagerGetOnlineId) { ret(ctx, SCE_NP_ERROR_INVALID_STATE); }
OPENTIT_HLE(sceNp, sceNpManagerGetContentRatingFlag) { ret(ctx, SCE_NP_ERROR_INVALID_STATE); }

OPENTIT_HLE_OK(sceNpTus, sceNpTusInit)
OPENTIT_HLE_OK(sceNpTus, sceNpTusTerm)
OPENTIT_HLE(sceNpTus, sceNpTusCreateTitleCtx) { ret(ctx, SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID); }
OPENTIT_HLE(sceNpTus, sceNpTusCreateTransactionCtx) { ret(ctx, SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID); }
OPENTIT_HLE_OK(sceNpTus, sceNpTusDestroyTitleCtx)
OPENTIT_HLE_OK(sceNpTus, sceNpTusDestroyTransactionCtx)
OPENTIT_HLE(sceNpTus, sceNpTusGetData) { ret(ctx, SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID); }
OPENTIT_HLE(sceNpTus, sceNpTusSetData) { ret(ctx, SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID); }

// ---------------------------------------------------------------- trophies (queries)
OPENTIT_HLE(sceNpTrophy, sceNpTrophyGetGameInfo) {
    // (context, handle, SceNpTrophyGameDetails* (1176 bytes), SceNpTrophyGameData* (20 bytes))
    if (arg32(ctx, 2)) std::memset(mem::ptr(arg32(ctx, 2)), 0, 1176);
    if (arg32(ctx, 3)) std::memset(mem::ptr(arg32(ctx, 3)), 0, 20);
    ret(ctx, CELL_OK);
}
OPENTIT_HLE(sceNpTrophy, sceNpTrophyGetTrophyInfo) {
    // (context, handle, trophyId, SceNpTrophyDetails* (1164 bytes), SceNpTrophyData* (16 bytes))
    const u32 id = arg32(ctx, 2);
    if (arg32(ctx, 3)) {
        std::memset(mem::ptr(arg32(ctx, 3)), 0, 1164);
        mem::w32(arg32(ctx, 3), id);
    }
    if (arg32(ctx, 4)) {
        std::memset(mem::ptr(arg32(ctx, 4)), 0, 16);
        mem::w32(arg32(ctx, 4) + 8, id);
    }
    ret(ctx, CELL_OK);
}

// ---------------------------------------------------------------- misc
OPENTIT_HLE(sys_fs, cellFsSdataOpenByFd) { ret(ctx, CELL_ENOENT); }  // no EDATA content
