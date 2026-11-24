// openTIT - Agl occlusion queries: how much of a sphere is visible, measured by the GPU.
#include "agl/AglOcclusion.h"

#include "agl/AglDisplay.h"
#include "agl/AglPrimitives.h"
#include "agl/AglRenderTarget.h"
#include "agl/AglRenderThread.h"
#include "agl/AglResources.h"
#include "agl/AglShaderClass.h"
#include "agl/AglState.h"
#include "agl/gpu/Device.h"

#include <cstring>

namespace agl {

namespace {

constexpr int kMaxQueries = 16;
// The queries use report slots 1914..2041, handed out round robin.
constexpr uint32_t kFirstReport = 0x77A;
constexpr uint32_t kReportCount = 128;

AglOcclusionQuery g_queries[kMaxQueries];        // 0x010DFAD0
AglMaterial* g_queryMaterial = nullptr;          // 0x010DFAC0
uint32_t g_nextReport = 0;                       // 0x011640EC
bool g_reportsInMainMemory = false;              // 0x011640E4
bool g_queriesEnabled = true;                    // 0x00F66FD5
bool g_anyActive = false;                        // 0x010E05D0
// Reports issued but not read back yet (the console marks them in the
// report memory and the GPU clears the mark when it writes the value).
bool g_reportPending[kReportCount] = {};

uint32_t AllocReport()
{
    const uint32_t slot = g_nextReport;
    g_nextReport = (g_nextReport + 1) & (kReportCount - 1);
    g_reportPending[slot] = true;
    return kFirstReport + slot;
}

bool ReadReport(uint32_t index, uint32_t* value)
{
    gpu::Device* device = gpu::GetDevice();
    if (!device)
        return false;
    g_reportPending[(index - kFirstReport) & (kReportCount - 1)] = false;
    *value = device->GetReport(index);
    return true;
}

bool IsReportPending(uint32_t index)
{
    return g_reportPending[(index - kFirstReport) & (kReportCount - 1)];
}

// @func 0x00527C4C AglOcclusion::BeginQueryState
// Pixel counting setup: no blending, no depth writes, no culling, the
// view's matrix in vertex constants 4..7 and no color writes.
void BeginQueryState()
{
    gpu::CommandList& cmd = Cmd();
    cmd.SetBlendEnable(false);
    cmd.SetDepthMask(false);
    cmd.SetCullFaceEnable(false);
    const AglViewConstants& view = GetViewConstants();
    cmd.SetVertexProgramConstants(4, 16, &view.viewProjT.rows[0].x);
    cmd.SetColorMask(0);
}

// @func 0x00527D34 AglOcclusion::EndQueryState
void EndQueryState()
{
    gpu::CommandList& cmd = Cmd();
    cmd.SetColorMask(gpu::kColorMask_All);
    cmd.SetCullFaceEnable(true);
    cmd.SetFrontFace(gpu::FrontFace::CCW);
}

// Draws the query sphere with its transform.
// TODO(0x00527708 AglOcclusionQuery::DrawMesh)
// The console version selects the material's technique, calls its program
// block and uploads transform * viewProj into vertex constants 0..3 before
// the draw; the rest of the listing could not be recovered.
void DrawQueryMesh(const AglOcclusionQuery& query)
{
    const AglSubMesh* mesh = query.mesh;
    if (!mesh || !mesh->vertices.data)
        return;
    const ape::Matrix34& t = query.transform;
    const ape::Matrix44 world = {
        { { t.xAxis.x, t.xAxis.y, t.xAxis.z, 0.0f }, { t.yAxis.x, t.yAxis.y, t.yAxis.z, 0.0f },
          { t.zAxis.x, t.zAxis.y, t.zAxis.z, 0.0f }, { t.translation.x, t.translation.y, t.translation.z, 1.0f } }
    };
    ape::Matrix44 mvp;
    ape::Matrix44Mul(mvp, world, GetViewConstants().viewProj);
    float transposed[16];
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            transposed[c * 4 + r] = (&mvp.rows[r].x)[c];
    gpu::CommandList& cmd = Cmd();
    cmd.SetVertexProgramConstants(0, 16, transposed);
    const uint32_t stride = mesh->vertexDecl ? mesh->vertexDecl->stride : 24;
    cmd.SetVertexAttribute(0, 0, static_cast<uint8_t>(stride), 3, gpu::VertexAttribType::Float, mesh->vertices.Ptr());
    cmd.DrawArrays(static_cast<gpu::PrimitiveType>(mesh->primitive), 0, mesh->vertexCount);
}

} // namespace

// @func 0x005276AC AglOcclusionQueryBase::AglOcclusionQueryBase
AglOcclusionQueryBase::AglOcclusionQueryBase() : active(false) {}

// @func 0x005485B4 AglOcclusionQuery::AglOcclusionQuery
AglOcclusionQuery::AglOcclusionQuery() = default;

// @func 0x004FCC10 AglOcclusionQueryBase::~AglOcclusionQueryBase
// @func 0x004FCC20 AglOcclusionQueryBase::~AglOcclusionQueryBase (copy 2)
// @func 0x005276C4 AglOcclusionQueryBase::~AglOcclusionQueryBase (deleting)
AglOcclusionQueryBase::~AglOcclusionQueryBase() = default;

// @func 0x005485F4 AglOcclusionQuery::Init
void AglOcclusionQuery::Init(AglSubMesh* newMesh, const ape::Matrix34& newTransform)
{
    transform = newTransform;
    mesh = newMesh;
    for (Slot& slot : slots)
        slot = Slot();
    current = 0;
}

// @func 0x00548694 AglOcclusionQuery::Issue
void AglOcclusionQuery::Issue()
{
    Slot& slot = slots[current];
    slot.flags = kSlot_Issued;
    gpu::CommandList& cmd = Cmd();

    // Visible pixels: with the depth test of the current target.
    slot.visibleReport = AllocReport();
    cmd.ClearReport(gpu::ReportType::ZPassPixelCount);
    cmd.SetDepthTestEnable(GetRenderState().depthEnabled);
    DrawQueryMesh(*this);
    cmd.GetReport(gpu::ReportType::ZPassPixelCount, slot.visibleReport);

    // Total pixels: depth test off.
    slot.totalReport = AllocReport();
    cmd.ClearReport(gpu::ReportType::ZPassPixelCount);
    cmd.SetDepthTestEnable(false);
    DrawQueryMesh(*this);
    cmd.GetReport(gpu::ReportType::ZPassPixelCount, slot.totalReport);
}

// @func 0x00548848 AglOcclusionQuery::GetResult
bool AglOcclusionQuery::GetResult(uint32_t out[2], int framesBack) const
{
    const Slot& slot = slots[(current - framesBack + kFrames) % kFrames];
    out[0] = slot.totalPixels;
    out[1] = slot.visiblePixels;
    return slot.flags == kSlot_Complete;
}

void AglOcclusionQuery::Update()
{
    for (Slot& slot : slots) {
        if (!(slot.flags & kSlot_Issued))
            continue;
        if (!(slot.flags & kSlot_TotalReady) && !IsReportPending(slot.totalReport) &&
            ReadReport(slot.totalReport, &slot.totalPixels))
            slot.flags |= kSlot_TotalReady;
        if (!(slot.flags & kSlot_VisibleReady) && !IsReportPending(slot.visibleReport) &&
            ReadReport(slot.visibleReport, &slot.visiblePixels))
            slot.flags |= kSlot_VisibleReady;
    }
    current = static_cast<uint16_t>((current + 1) % kFrames);
    slots[current].flags = 0;
}

// @func 0x00527ACC AglOcclusionQuery::ReleaseMesh
void AglOcclusionQuery::ReleaseMesh()
{
    if (mesh) {
        mesh->Release();
        mesh = nullptr;
    }
}

// @func 0x00548A10 UpdateOcclusionQueries
void UpdateOcclusionQueries()
{
    if (!g_queriesEnabled)
        return;
    // Reports written by the device since the last frame are complete.
    std::memset(g_reportPending, 0, sizeof(g_reportPending));
    for (AglOcclusionQuery& query : g_queries) {
        if (query.active)
            query.Update();
    }
}

// @func 0x00527A70 FreeOcclusionQuery
bool FreeOcclusionQuery(AglOcclusionQuery* query)
{
    for (AglOcclusionQuery& slot : g_queries) {
        if (&slot == query) {
            slot.active = false;
            return true;
        }
    }
    return false;
}

// @func 0x00548938 ReleaseOcclusionQuery
void ReleaseOcclusionQuery(AglOcclusionQuery* query)
{
    query->ReleaseMesh();
    FreeOcclusionQuery(query);
}

// @func 0x00527E64 CreateOcclusionQuery
AglOcclusionQuery* CreateOcclusionQuery(const ape::Matrix34& transform, int rings, int segments)
{
    AglSubMesh* mesh = CreateSphereMesh(g_queryMaterial, rings, segments);
    for (AglOcclusionQuery& query : g_queries) {
        if (!query.active) {
            query.active = true;
            query.Init(mesh, transform);
            return &query;
        }
    }
    if (mesh)
        mesh->Release();
    return nullptr;
}

namespace {

// @func 0x00527B28 AglOcclusion::CreateMaterial
void CreateQueryMaterial()
{
    ResetEndOfFrameReleases();
    const AglMaterial* base = GetBuiltinMaterial(0);
    g_queryMaterial = AglMaterial::Clone(*base);
    g_queryMaterial->depthTest = false;
}

// @func 0x00527BA8 AglOcclusion::ReleaseAll
void ReleaseAllQueries()
{
    if (g_queryMaterial) {
        g_queryMaterial->Release();
        g_queryMaterial = nullptr;
    }
    // Live queries are released at the end of the frame.
    for (AglOcclusionQuery& query : g_queries) {
        if (query.active)
            QueueEndOfFrameRelease(&query);
    }
}

} // namespace

// @func 0x00548974 InitOcclusionQueries
void InitOcclusionQueries()
{
    CreateQueryMaterial();
    g_nextReport = 0;
    g_reportsInMainMemory = false;   // the console asks libgcm where report memory is
}

// @func 0x005489CC ShutdownOcclusionQueries
void ShutdownOcclusionQueries()
{
    ReleaseAllQueries();
    // The console frees the report memory here when it lives in main memory.
}

// @func 0x005488AC AglOcclusion::Begin
static void BeginOcclusionPass()
{
    BeginQueryState();
    Cmd().SetZPassPixelCountEnable(true);
    Cmd().SetReportLocation(g_reportsInMainMemory ? gpu::MemoryLocation::Main : gpu::MemoryLocation::Local);
}

// @func 0x00548900 AglOcclusion::End
static void EndOcclusionPass()
{
    EndQueryState();
    Cmd().SetZPassPixelCountEnable(false);
}

// @func 0x00527DAC DrawOcclusionQueries
void DrawOcclusionQueries()
{
    if (!g_queriesEnabled || !g_anyActive)
        return;
    BeginOcclusionPass();
    for (AglOcclusionQuery& query : g_queries) {
        if (query.active)
            query.Issue();
    }
    EndOcclusionPass();
    InvalidateRenderState();
}

// @func 0x00548B88 DrawOcclusionQueriesForEye
void DrawOcclusionQueriesForEye()
{
    // Both eyes see nearly the same thing: count once.
    if (GetStereoMode() != kAglStereo_Right)
        DrawOcclusionQueries();
}

void SetOcclusionQueriesEnabled(bool enabled)
{
    g_queriesEnabled = enabled;
}

// Called at the start of each frame (part of 0x0051008C): remembers whether
// any query needs drawing.
void RefreshOcclusionQueryState()
{
    g_anyActive = false;
    for (const AglOcclusionQuery& query : g_queries) {
        if (query.active) {
            g_anyActive = true;
            break;
        }
    }
}

} // namespace agl
