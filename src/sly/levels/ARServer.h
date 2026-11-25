// openTIT - AR debug server ("Art Capture" link to a handheld AR client).
//
// A debug network server (port 333, one client) the level code creates on
// demand (vtable 0x00B11D80, derived from gameplay::DebugNetServer). It
// renders the AR target view into the "RenderTarget_SlyAR" render target and
// exchanges target/capture packets with the client; a background thread
// ("Discover AR Server") looks for the client on the local network.
// The shared state lives in a global block (0x00F558C4..0x00F558E3).
#pragma once

#include "sly/gameplay/DebugNetServer.h"

#include <cstdint>

namespace agl { class AglRenderTarget; }

namespace sly {
namespace levels {

constexpr uint16_t kARServerPort = 0x14D;   // 333

// Global AR state (0x00F558C4).
struct ARState {
    const void* cameraSource = nullptr;      // 0x00F558C4 object whose matrix (+0x2F0) the view uses
    bool captured = false;                   // 0x00F558C8
    bool dirty = false;                      // 0x00F558C9 target info changed, resend
    int32_t targetId = -1;                   // 0x00F558CC
    agl::AglRenderTarget* renderTarget = nullptr;  // 0x00F558D0
    uint32_t sentCount = 0;                  // 0x00F558D4 queued entries already sent
    int16_t field_D8 = 0;                    // 0x00F558D8
    int16_t field_DA = 0;                    // 0x00F558DA
    int32_t field_DC = 0;                    // 0x00F558DC
    uint8_t field_E0 = 0;                    // 0x00F558E0
    bool resendRequested = false;            // 0x00F558E1
    uint8_t discoveredHost = 0;              // 0x00F558E2 last octet of the client found
    bool discoveryDone = false;              // 0x00F558E3
};
ARState& GetARState();

// Number of queued AR entries (0x00F9CDB0). NOTE: what the entries are is not
// decoded; they are sent to the client by the server update.
uint16_t& ARQueuedCount();

class ARServer : public gameplay::DebugNetServer {
public:
    ARServer();
    ~ARServer() override;
    // TODO(0x000A9768 ARServer::Update): AR protocol and target view (8 KB).

    uint32_t field_24 = 0;   // +0x24
};

// Forgets the queued entries (0x000A884C).
void ARResetQueue();
// Asks for the queued entries to be sent again if there are any (0x000A8868).
void ARRequestResend();
// Starts the discovery thread (0x000A9670).
void ARStartDiscovery();
// Last octet of the discovered client, or -1 while discovery runs (0x000A96B8).
int ARDiscoveredHost();
// Byte order swap of a 32-bit value (0x000A96D8).
uint32_t ARSwap32(uint32_t value);
agl::AglRenderTarget* ARRenderTarget();        // 0x000AB93C
void ARSetCameraSource(const void* source);   // 0x000AB948
// TODO(0x000AB7B0): resets the three "InGame.ARTarget.Image*" HUD objects
// (two GUI virtual slots whose meaning is not settled).

} // namespace levels
} // namespace sly
