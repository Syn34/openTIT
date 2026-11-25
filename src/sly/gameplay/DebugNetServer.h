// openTIT - Debug network server.
//
// A development aid: the game opens a TCP listen socket on port 14120 for
// remote tools and announces itself every "Game.Net.Server.BroadcastTime"
// seconds with a UDP packet sent to every host of the local /24 network.
// Clients get a disconnect packet when the server goes away. Log lines are
// kept in a small ring for the tools to fetch.
//
// Console globals: client socket set 0x00F6584C, listen set 0x00F65850,
// broadcast set 0x00F65854, log ring 0x0108A150, packet queues 0x0108A110 /
// 0x0108A128 / 0x0108A13C.
#pragma once

#include "sly/gameplay/GameplayWorld.h"

#include <cstdint>

namespace sly {
namespace gameplay {

constexpr uint16_t kDebugNetPort = 0x3728;   // 14120

// Packets exchanged with the tools (0x130 bytes on the wire).
enum DebugPacketType : uint32_t {
    kDebugPacket_Announce = 2,
    kDebugPacket_Disconnect = 8,
};
struct DebugPacket {
    uint8_t senderIp[4] = {};      // +0x00
    uint16_t senderPort = 0;       // +0x04
    uint32_t type = 0;             // +0x08
    uint16_t capacity = 0;         // +0x0C bits 6-11 max clients, 0-5 clients
    char address[0x3C] = {};       // +0x0E "a.b.c.d"
    char levelName[0x3C] = {};     // +0x4A
    uint8_t payload[0xAA] = {};    // rest of the 0x130 bytes
};

// A socket with up to three accepted connections (0x20 bytes).
struct NetSocketSet {
    int socket = 0;                // +0x00
    int clients[4] = {};           // +0x04 slot 0 is the active connection
    uint32_t clientCount = 0;      // +0x14
    uint8_t peerIp[4] = {};        // +0x18 the only peer accepted once set
    uint8_t peerLastOctet = 0;     // +0x1C

    int Receive(int client, void* buffer, size_t size);
    bool Send(uint16_t client, const void* buffer, size_t size);
    bool AcceptClient();
    void DropClient();
    void Close();
};

// Address the server listens on (local IP, port 14120).
NetAddress GetLocalServerAddress();
void CreateBroadcastSocket();
NetSocketSet* CreateListenSocket(const NetAddress& address);
void BroadcastPacket(const DebugPacket& packet);
void ShutdownDebugNet();

struct DebugNetServerConfig {
    uint16_t port = kDebugNetPort;   // +0x00
    uint32_t maxClients = 0;         // +0x04
};

// vtable 0x00B20CB0.
class DebugNetServer {
public:
    explicit DebugNetServer(const DebugNetServerConfig& config);
    virtual ~DebugNetServer();
    DebugNetServer(const DebugNetServer&) = delete;
    DebugNetServer& operator=(const DebugNetServer&) = delete;

    // Slot 2: announces the server, then repeats the broadcast periodically.
    virtual void Update(float dt);

    // Appends a line to the log ring (16 lines, oldest dropped).
    static void Log(const char* text);

protected:
    struct Client {
        uint32_t field_00 = 0;
        uint16_t field_04 = 0;
        int32_t socket = 0;          // +0x08 > 0 while connected
    };

    NetAddress m_address;            // +0x04
    int32_t m_state = 0;             // +0x0C 0: starting, 1: running
    uint32_t m_maxClients = 0;       // +0x10
    uint32_t m_clientCount = 0;      // +0x14
    Client* m_clients = nullptr;     // +0x18
    NetSocketSet* m_listen = nullptr; // +0x1C
    float m_broadcastTimer = 0.0f;   // +0x20
};

} // namespace gameplay
} // namespace sly
