// openTIT - AR debug server (0x000A85C0-0x000ABAD4).
#include "sly/levels/ARServer.h"

#include "agl/AglDisplay.h"
#include "agl/AglRenderTarget.h"

#include <cstring>
#include <thread>

namespace sly {
namespace levels {

namespace {
ARState g_arState;
uint16_t g_arQueuedCount = 0;   // 0x00F9CDB0

// TODO(0x000A88C0 DiscoverARClient): probes every host of the local /24
// network on port 333 with non-blocking connects and returns the last octet
// of the one that answered; needs a connect/select layer in the net
// abstraction. Reports none (0) until then.
uint8_t DiscoverARClient()
{
    return 0;
}

// @func 0x000ABAD4 ARDiscoveryThread
void ARDiscoveryThread()
{
    g_arState.discoveredHost = 0;
    g_arState.discoveryDone = false;
    g_arState.discoveredHost = DiscoverARClient();
    g_arState.discoveryDone = true;
}

gameplay::DebugNetServerConfig ARServerConfig()
{
    gameplay::DebugNetServerConfig config;
    config.port = kARServerPort;
    config.maxClients = 1;
    return config;
}
} // namespace

ARState& GetARState() { return g_arState; }
uint16_t& ARQueuedCount() { return g_arQueuedCount; }

// @func 0x000A85C0 ARServer::ARServer
// Resets the shared state and creates the full-screen orthographic render
// target the AR view is drawn into.
ARServer::ARServer()
    : DebugNetServer(ARServerConfig())
{
    g_arState.targetId = -1;
    g_arState.field_D8 = 0;
    g_arState.field_E0 = 0;
    g_arState.field_DA = 0;
    g_arState.dirty = false;
    g_arState.field_DC = 0;
    g_arState.captured = false;
    g_arState.sentCount = 0;
    field_24 = 0;

    agl::AglRenderTarget* target = agl::AglRenderTarget::CreatePrimary(2, false);
    g_arState.renderTarget = target;
    std::strncpy(target->name, "RenderTarget_SlyAR", sizeof(target->name) - 1);
    target->name[sizeof(target->name) - 1] = '\0';
    target->SetProjectionType(agl::kAglProjection_Ortho);
    const agl::AglDisplaySettings& display = agl::GetDisplaySettings();
    const uint16_t viewport[4] = { 0, 0, display.width, display.height };
    target->SetViewport(viewport);
}

// @func 0x000A86D8 ARServer::~ARServer
// @func 0x000A8784 ARServer::~ARServer (deleting)
ARServer::~ARServer()
{
    if (m_listen) {
        m_listen->Close();
        delete m_listen;
    }
    m_listen = nullptr;
    if (g_arState.renderTarget)
        g_arState.renderTarget->Release();
    g_arState.renderTarget = nullptr;
}

// @func 0x000A884C ARResetQueue
void ARResetQueue()
{
    g_arQueuedCount = 0;
    g_arState.sentCount = 0;
}

// @func 0x000A8868 ARRequestResend
void ARRequestResend()
{
    if (g_arQueuedCount != 0)
        g_arState.resendRequested = true;
}

// @func 0x000A9670 ARStartDiscovery
// The console runs the discovery on its own thread (priority 0x600, 8 KB
// stack, joinable).
void ARStartDiscovery()
{
    std::thread(ARDiscoveryThread).detach();
}

// @func 0x000A96B8 ARDiscoveredHost
int ARDiscoveredHost()
{
    return g_arState.discoveryDone ? int(g_arState.discoveredHost) : -1;
}

// @func 0x000A96D8 ARSwap32
uint32_t ARSwap32(uint32_t value)
{
    return (value >> 24) | ((value >> 8) & 0xFF00u) | ((value << 8) & 0xFF0000u) | (value << 24);
}

// @func 0x000AB93C ARRenderTarget
agl::AglRenderTarget* ARRenderTarget()
{
    return g_arState.renderTarget;
}

// @func 0x000AB948 ARSetCameraSource
void ARSetCameraSource(const void* source)
{
    g_arState.cameraSource = source;
}

} // namespace levels
} // namespace sly
