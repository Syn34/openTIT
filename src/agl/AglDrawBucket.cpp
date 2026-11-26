// openTIT - Draw buckets and the per-item draw loop (0x00512940-0x00513984).
#include "agl/AglDrawBucket.h"

#include "agl/AglResources.h"
#include "agl/AglShaderClass.h"
#include "agl/AglState.h"

#include <algorithm>
#include <cstring>

namespace agl {

namespace {

AglItemDrawState g_itemDraw;
uint32_t g_bucketDrawCounter = 0;   // 0x010C5A6C

struct MaterialStateCache {
    bool blendValid = false;    // 0x010DD598 != -1
    bool depthValid = false;    // 0x010DD5A0 != -1
};
MaterialStateCache g_materialCache;

// Fixed-function state of a material, then the technique's own set-up.
// NOTE: stand-in for the material apply function 0x0079CD0C (outside this
// range, not tagged here): the console version also binds the textures and
// uploads the UV animation.
void ApplyMaterialState(AglMaterial* material, AglTechnique* technique)
{
    gpu::CommandList& cmd = Cmd();
    if (!g_materialCache.blendValid) {
        cmd.SetBlendEnable(material->blendEnable);
        if (material->blendEnable) {
            cmd.SetBlendFunc(material->blendSrc, material->blendDst, material->blendSrc, material->blendDst);
            cmd.SetBlendEquation(material->blendEquation, material->blendEquation);
        }
        g_materialCache.blendValid = true;
    }
    if (!g_materialCache.depthValid) {
        const AglRenderState& st = GetRenderState();
        cmd.SetDepthTestEnable(st.depthEnabled);
        cmd.SetDepthFunc(material->depthFunc);
        cmd.SetDepthMask(material->depthTest && st.depthEnabled);
        if (material->cullFrontFace == 0) {
            cmd.SetCullFaceEnable(false);
        } else {
            cmd.SetCullFaceEnable(true);
            const bool cullCw = material->cullFrontFace == static_cast<uint16_t>(gpu::FrontFace::CW);
            cmd.SetFrontFace(cullCw ? gpu::FrontFace::CCW : gpu::FrontFace::CW);
        }
        g_materialCache.depthValid = true;
    }
    technique->BeginMaterial(material);
}

void EndCurrentTechnique()
{
    if (g_itemDraw.technique)
        g_itemDraw.technique->End(nullptr);
}

} // namespace

AglItemDrawState& GetItemDrawState()
{
    return g_itemDraw;
}

void InvalidateMaterialStateCache()
{
    g_materialCache.blendValid = false;
    g_materialCache.depthValid = false;
}

uint32_t BucketDrawCounter()
{
    return g_bucketDrawCounter;
}

// @func 0x005131E8 AglRenderCallbackList::Insert
void AglRenderCallbackList::Insert(void (*fn)(void*), void* arg, uint32_t pass, int8_t priority)
{
    int slot = count;
    while (slot > 0 && entries[slot - 1].priority > priority) {
        if (slot < kMax)
            entries[slot] = entries[slot - 1];
        --slot;
    }
    if (slot >= kMax)
        return;   // NOTE: the console does not check the capacity
    entries[slot].fn = fn;
    entries[slot].arg = arg;
    entries[slot].pass = pass;
    entries[slot].priority = priority;
    ++count;
}

// @func 0x005132D8 AglRenderCallbackList::Run
void AglRenderCallbackList::Run() const
{
    const AglRenderTarget* view = GetRenderingView();
    for (uint8_t i = 0; i < count; ++i) {
        const AglRenderCallback& cb = entries[i];
        if (!cb.fn || !view || view->pass != cb.pass)
            continue;
        cb.fn(cb.arg);
        InvalidateMaterialStateCache();
    }
}

// @func 0x00512C30 DrawRenderItem
void DrawRenderItem(AglRenderItem* item)
{
    AglMaterial* material = item->material;
    AglTechnique* technique = material->shaderClass->SelectTechnique(material, item);
    if (const AglRenderTarget* view = GetRenderingView()) {
        if (view->techniqueOverride)
            technique = view->techniqueOverride;
    }
    if (g_itemDraw.technique != technique) {
        EndCurrentTechnique();
        g_itemDraw.material = nullptr;
        g_itemDraw.field_8 = 0;
        g_itemDraw.technique = technique;
    }
    if (g_itemDraw.material != material) {
        if (g_itemDraw.material)
            EndCurrentTechnique();
        g_itemDraw.material = material;
        ApplyMaterialState(material, technique);
    }
    technique->Draw(item);
}

// @func 0x00512EF4 BeginBucketDraw
void BeginBucketDraw(uint8_t writeMask)
{
    gpu::CommandList& cmd = Cmd();
    AglRenderState& st = GetRenderState();
    cmd.SetStencilTestEnable(true);
    AglStencilState& front = st.stencilFront;
    front.fail = gpu::StencilOp::Keep;
    front.depthFail = gpu::StencilOp::Keep;
    front.depthPass = gpu::StencilOp::Replace;
    front.func = gpu::CompareFunc::Always;
    front.ref = 0;
    front.mask = 0;
    front.writeMask = writeMask;
    cmd.SetStencilOp(front.fail, front.depthFail, front.depthPass);
    cmd.SetStencilMask(front.writeMask);
    cmd.SetStencilFunc(front.func, front.ref, front.mask);
    AglItemStencil& stencil = GetItemStencil();
    stencil.ref = 0;
    stencil.writeMask = writeMask;
}

// @func 0x00512DA0 EndBucketDraw
void EndBucketDraw()
{
    EndCurrentTechnique();
    g_itemDraw = AglItemDrawState();

    gpu::CommandList& cmd = Cmd();
    AglRenderState& st = GetRenderState();
    cmd.SetStencilTestEnable(false);
    AglStencilState& front = st.stencilFront;
    front.fail = front.depthFail = front.depthPass = gpu::StencilOp::Keep;
    front.func = gpu::CompareFunc::Always;
    front.ref = 0;
    front.mask = 0;
    front.writeMask = 0;
    cmd.SetStencilOp(front.fail, front.depthFail, front.depthPass);
    cmd.SetStencilMask(front.writeMask);
    cmd.SetStencilFunc(front.func, front.ref, front.mask);
    AglItemStencil& stencil = GetItemStencil();
    stencil.ref = 0;
    stencil.writeMask = 0;
}

AglDrawBucket::AglDrawBucket(uint32_t capacity) : m_capacity(capacity)
{
    m_items.reserve(capacity);
}

// @func 0x0051384C AglDrawBucket::Add
void AglDrawBucket::Add(AglRenderItem* item, uint32_t key)
{
    m_items.push_back({ key, item });
}

// @func 0x00513868 AglDrawBucket::AddUnsorted
void AglDrawBucket::AddUnsorted(AglRenderItem* item)
{
    m_items.push_back({ 0, item });
}

// @func 0x0051300C AglDrawBucket::Terminate
void AglDrawBucket::Terminate()
{
    m_items.push_back({ 0xFFFFFFFFu, nullptr });
}

// @func 0x00513894 AglDrawBucket::Sort
void AglDrawBucket::Sort()
{
    const uint32_t count = Count();
    m_peakCount = std::max(m_peakCount, count);
    if (count > m_capacity) {
        m_items.clear();
        return;
    }
    if (count == 0)
        return;
    if (count <= 0x200) {
        // NOTE: the console sorts small buckets with its pair sort
        // (0x0056A81C, 32-bit values); any sort by key gives the same draws.
        std::sort(m_items.begin(), m_items.end(),
                  [](const AglBucketEntry& a, const AglBucketEntry& b) { return a.key < b.key; });
    } else {
        RadixSort();
    }
}

// @func 0x00513394 AglDrawBucket::RadixSort
void AglDrawBucket::RadixSort()
{
    const size_t count = m_items.size();
    std::vector<AglBucketEntry> scratch(count);
    std::vector<AglBucketEntry>* from = &m_items;
    std::vector<AglBucketEntry>* to = &scratch;
    // Four counting-sort passes, least significant key byte first.
    for (int pass = 0; pass < 4; ++pass) {
        const int shift = pass * 8;
        uint32_t histogram[256] = {};
        for (const AglBucketEntry& e : *from)
            ++histogram[(e.key >> shift) & 0xFF];
        uint32_t offset[256];
        uint32_t running = 0;
        for (int b = 0; b < 256; ++b) {
            offset[b] = running;
            running += histogram[b];
        }
        for (const AglBucketEntry& e : *from)
            (*to)[offset[(e.key >> shift) & 0xFF]++] = e;
        std::swap(from, to);
    }
    // After an even number of passes the result is back in m_items.
}

// @func 0x00513984 AglDrawBucket::Draw
void AglDrawBucket::Draw()
{
    if (m_flags & 0x80)
        return;
    m_before.Run();
    if (!m_items.empty() && m_items.front().item) {
        ++g_bucketDrawCounter;
        BeginBucketDraw(m_stencilWriteMask);
        for (const AglBucketEntry& entry : m_items) {
            if (!entry.item)
                break;   // terminator
            DrawRenderItem(entry.item);
        }
        EndBucketDraw();
    }
    m_after.Run();
}

} // namespace agl
