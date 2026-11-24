// openTIT - Debug network server (see DebugNetServer.h).
#include "sly/gameplay/DebugNetServer.h"

#include "ape/core/ApeString.h"
#include "ape/tuning/ApeTuning.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace sly {
namespace gameplay {

namespace {

struct LogLine {
    char text[0x100] = {};   // +0x000
    char tag[0x20] = {};     // +0x100
    int32_t type = 0;        // +0x120
};
constexpr size_t kMaxLogLines = 0x10;

// Queued messages of the server. NOTE: only the log ring has users in the
// reconstructed code; the other queues are kept for their sizes.
struct ServerQueues {
    std::vector<uint8_t> records84;      // 0x0108A110: 0x84-byte records
    std::vector<uint8_t> records10;      // 0x0108A128: 0x10-byte records
    std::vector<DebugPacket> packets;    // 0x0108A13C
    std::vector<LogLine> log;            // 0x0108A150
};

// @func 0x004285B8 StaticInit_DebugNetServer
// @func 0x004284F8 StaticDestroy_DebugNetServer
// Compiler-generated initialiser/destructor pair for the globals below.
ServerQueues g_queues = [] {
    ServerQueues queues;
    queues.records84.reserve(0x20 * 0x84);
    queues.records10.reserve(0x100 * 0x10);
    queues.log.reserve(kMaxLogLines);
    return queues;
}();
ape::TuningFloat g_netServerBroadcastTime("Game.Net.Server.BroadcastTime", 0.5f);

NetSocketSet* g_clientSockets = nullptr;      // 0x00F6584C
NetSocketSet* g_listenSockets = nullptr;      // 0x00F65850
NetSocketSet* g_broadcastSockets = nullptr;   // 0x00F65854

// Parses "a.b.c.d"; false unless all four parts are present.
bool ParseIp(const char* text, uint8_t out[4])
{
    int parts[4];
    if (std::sscanf(text, "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]) != 4)
        return false;
    for (int i = 0; i < 4; ++i)
        out[i] = static_cast<uint8_t>(parts[i]);
    return true;
}

// Local IP address, all zero when there is none.
void LocalIp(uint8_t out[4])
{
    uint8_t ip[4] = {};
    char text[0x100];
    if (NetLocalIpString(text, sizeof(text)))
        ParseIp(text, ip);
    std::memcpy(out, ip, 4);
}

void DestroySocketSet(NetSocketSet*& set)
{
    if (set == nullptr)
        return;
    set->Close();
    delete set;
    set = nullptr;
}

} // namespace

// ---------------------------------------------------------------------------
// NetSocketSet
// ---------------------------------------------------------------------------

// @func 0x004270C8 NetSocketSet::Receive
int NetSocketSet::Receive(int client, void* buffer, size_t size)
{
    return NetReceive(clients[client], buffer, size);
}

// @func 0x00427744 NetSocketSet::Send
bool NetSocketSet::Send(uint16_t client, const void* buffer, size_t size)
{
    return NetSend(clients[client], buffer, size) == static_cast<int>(size);
}

// @func 0x00427120 NetSocketSet::AcceptClient
bool NetSocketSet::AcceptClient()
{
    if (clientCount >= 3)
        return false;

    NetAddress peer;
    const int connection = NetAccept(socket, &peer);
    clients[clientCount] = connection;
    if (connection <= 0)
        return false;

    // Once a peer has connected, only that machine may become the active
    // connection again.
    const bool anyPeer = peerIp[0] == 0;
    if (anyPeer || std::memcmp(peerIp, peer.ip, 4) == 0) {
        std::memcpy(peerIp, peer.ip, 4);
        peerLastOctet = peer.ip[3];
        clients[0] = clients[clientCount];
    }
    clientCount = 1;
    return true;
}

// @func 0x004279A4 NetSocketSet::DropClient
void NetSocketSet::DropClient()
{
    if (clients[0] > 0)
        NetClose(clients[0]);
    clientCount = 0;
    clients[0] = 0;
    std::memset(peerIp, 0, sizeof(peerIp));
    peerLastOctet = 0;
}

// @func 0x0042725C NetSocketSet::Close
void NetSocketSet::Close()
{
    NetClose(socket);
    for (uint32_t i = 0; i < clientCount; ++i)
        NetClose(clients[i]);
    socket = 0;
}

// ---------------------------------------------------------------------------
// Sockets
// ---------------------------------------------------------------------------

// @func 0x00426CE0 GetLocalServerAddress
NetAddress GetLocalServerAddress()
{
    NetAddress address;
    LocalIp(address.ip);
    address.port = kDebugNetPort;
    return address;
}

// @func 0x00426DD4 CreateBroadcastSocket
void CreateBroadcastSocket()
{
    if (!NetInitialize())
        return;

    g_broadcastSockets = new NetSocketSet();
    g_broadcastSockets->socket = NetOpen(NetProtocol::Udp);
    const int socket = g_broadcastSockets->socket;
    if (!NetSetOption(socket, NetOption::ReuseAddress) ||
        !NetSetOption(socket, NetOption::Broadcast) || !NetSetOption(socket, NetOption::NoDelay))
        return;

    NetBind(socket, GetLocalServerAddress());
}

// @func 0x004274D0 CreateListenSocket
NetSocketSet* CreateListenSocket(const NetAddress& address)
{
    g_listenSockets = new NetSocketSet();
    g_listenSockets->socket = NetOpen(NetProtocol::Tcp);
    NetSocketSet* set = g_listenSockets;
    if (!NetSetOption(set->socket, NetOption::ReuseAddress))
        return nullptr;
    if (!NetBind(set->socket, address) || !NetListen(set->socket, 4)) {
        // NOTE: the failed set stays allocated in the global, as on the console.
        set->Close();
        return nullptr;
    }
    return set;
}

// @func 0x004277A8 BroadcastPacket
void BroadcastPacket(const DebugPacket& packet)
{
    if (g_broadcastSockets == nullptr)
        return;

    uint8_t ip[4];
    LocalIp(ip);
    // Every other host of the local /24 network.
    for (uint32_t host = 0; host < 0x100; ++host) {
        if (host == static_cast<uint32_t>(ip[3]) - 1)
            continue;
        NetAddress to;
        const char* text = ape::TempFormat("%d.%d.%d.%d", ip[0], ip[1], ip[2], host + 1);
        ParseIp(text, to.ip);
        to.port = kDebugNetPort;
        NetSendTo(g_broadcastSockets->socket, &packet, sizeof(DebugPacket), to);
    }
}

// @func 0x004272F4 ShutdownDebugNet
void ShutdownDebugNet()
{
    DestroySocketSet(g_clientSockets);
    DestroySocketSet(g_listenSockets);
    DestroySocketSet(g_broadcastSockets);
    g_queues.packets.clear();
}

// ---------------------------------------------------------------------------
// DebugNetServer
// ---------------------------------------------------------------------------

// @func 0x00427A0C DebugNetServer::DebugNetServer
// @func 0x00427BD0 DebugNetServer::DebugNetServer (forwarding copy)
DebugNetServer::DebugNetServer(const DebugNetServerConfig& config)
    : m_maxClients(config.maxClients), m_broadcastTimer(g_netServerBroadcastTime.Get())
{
    LocalIp(m_address.ip);
    m_address.port = config.port;
    m_listen = CreateListenSocket(m_address);
    if (m_maxClients != 0)
        m_clients = new Client[m_maxClients]();
}

// @func 0x00427E2C DebugNetServer::~DebugNetServer
// @func 0x00427BF0 DebugNetServer::~DebugNetServer (complete)
// @func 0x00427D00 DebugNetServer::~DebugNetServer (deleting)
DebugNetServer::~DebugNetServer()
{
    // Tell every connected tool that the server is going away.
    DebugPacket bye;
    bye.type = kDebugPacket_Disconnect;
    for (uint32_t i = 0; i < m_maxClients; ++i) {
        if (m_clients[i].socket > 0 && m_listen != nullptr)
            m_listen->Send(static_cast<uint16_t>(i), &bye, sizeof(bye));
    }
    // NOTE: the console never frees the client array.
    delete[] m_clients;
}

// @func 0x00427F3C DebugNetServer::Log
void DebugNetServer::Log(const char* text)
{
    ape::TempFormat("Server Log: %s\n", text);   // debug print, compiled out

    LogLine line;
    ape::StrNCopy(line.tag, "", sizeof(line.tag));
    ape::StrNCopy(line.text, text, sizeof(line.text));
    line.type = 1;

    std::vector<LogLine>& log = g_queues.log;
    if (log.size() == kMaxLogLines)
        log.erase(log.begin());
    log.push_back(line);
}

// @func 0x00428228 DebugNetServer::Update
void DebugNetServer::Update(float dt)
{
    if (m_state == 0) {
        uint8_t ip[4];
        LocalIp(ip);
        Log(ape::TempFormat("(%d.%d.%d.%d:%d): new server.", ip[0], ip[1], ip[2], ip[3],
                            kDebugNetPort));
        m_state = 1;
        return;
    }
    if (m_state != 1)
        return;

    m_broadcastTimer -= dt;
    if (m_broadcastTimer > 0.0f)
        return;

    DebugPacket announce;
    announce.type = kDebugPacket_Announce;
    std::memcpy(announce.senderIp, m_address.ip, 4);
    announce.senderPort = m_address.port;
    announce.capacity = static_cast<uint16_t>(((m_maxClients & 0x3F) << 6) |
                                              (m_clientCount & 0x3F));
    uint8_t ip[4];
    LocalIp(ip);
    ape::StrNCopy(announce.address, ape::TempFormat("%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]),
                  sizeof(announce.address));
    ape::StrNCopy(announce.levelName, CurrentLevelName(), sizeof(announce.levelName));
    BroadcastPacket(announce);
    m_broadcastTimer = g_netServerBroadcastTime.Get();
}

static_assert(sizeof(DebugPacket) == 0x130, "debug packets are 0x130 bytes on the wire");

} // namespace gameplay
} // namespace sly
