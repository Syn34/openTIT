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

// Last fixed-function state applied from a material or from the draw list
// (0x010DD598: blend word, 0x010DD5A0: depth word with the cull face at
// 0x010DD5A4). A material whose state matches is not applied again.
struct MaterialStateCache {
    bool blendValid = false;
    bool blendEnable = false;
    gpu::BlendFactor blendSrc = gpu::BlendFactor::One;
    gpu::BlendFactor blendDst = gpu::BlendFactor::Zero;
    gpu::BlendEquation blendEquation = gpu::BlendEquation::Add;

    bool depthValid = false;
    bool depthWrite = false;
    gpu::CompareFunc depthFunc = gpu::CompareFunc::LEqual;
    uint16_t cullFrontFace = 0;
};
MaterialStateCache g_materialCache;

void EmitBlendState(bool enable, gpu::BlendFactor src, gpu::BlendFactor dst, gpu::BlendEquation equation)
{
    gpu::CommandList& cmd = Cmd();
    cmd.SetBlendEnable(enable);
    if (enable) {
        cmd.SetBlendFunc(src, dst, src, dst);
        cmd.SetBlendEquation(equation, equation);
    }
}

void EmitDepthState(gpu::CompareFunc func, bool write)
{
    gpu::CommandList& cmd = Cmd();
    const bool depthEnabled = GetRenderState().depthEnabled;
    cmd.SetDepthTestEnable(depthEnabled);
    cmd.SetDepthFunc(func);
    cmd.SetDepthMask(write && depthEnabled);
}

void EmitCullState(uint16_t cullFrontFace)
{
    gpu::CommandList& cmd = Cmd();
    if (cullFrontFace == 0) {
        cmd.SetCullFaceEnable(false);
        return;
    }
    cmd.SetCullFaceEnable(true);
    // The value names the face that is culled; the front face is the other one.
    const bool cullCw = cullFrontFace == static_cast<uint16_t>(gpu::FrontFace::CW);
    cmd.SetFrontFace(cullCw ? gpu::FrontFace::CCW : gpu::FrontFace::CW);
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

void SetBlendStateCached(bool enable, gpu::BlendFactor src, gpu::BlendFactor dst, gpu::BlendEquation equation)
{
    EmitBlendState(enable, src, dst, equation);
    MaterialStateCache& c = g_materialCache;
    c.blendValid = true;
    c.blendEnable = enable;
    c.blendSrc = src;
    c.blendDst = dst;
    c.blendEquation = equation;
}

void SetDepthStateCached(gpu::CompareFunc func, bool write)
{
    EmitDepthState(func, write);
    MaterialStateCache& c = g_materialCache;
    c.depthValid = true;
    c.depthFunc = func;
    c.depthWrite = write;
}

void SetCullStateCached(uint16_t cullFrontFace)
{
    EmitCullState(cullFrontFace);
    g_materialCache.cullFrontFace = cullFrontFace;
}

// NOTE: equivalent of the program binding in the material apply function
// 0x0079CD0C (outside this range): the technique's vertex program block is
// called when it changes and its fragment program is committed with the
// next draw.
void BindTechniquePrograms(const AglShader* technique)
{
    AglRenderState& st = GetRenderState();
    AglVertexProgram* vp = technique->vertexProgram;
    if (st.vertexProgram != vp) {
        st.boundVertexBuffer = nullptr;
        st.vertexProgram = vp;
        if (vp && st.vertexProgramBlock != &vp->commands) {
            st.vertexProgramBlock = &vp->commands;
            Cmd().Call(vp->commands);
        }
    }
    if (st.fragmentProgram != technique->fragmentProgram) {
        st.fragmentProgramDirty = true;
        st.fragmentProgram = technique->fragmentProgram;
    }
}

// Fixed-function state of a material, the technique's programs, then the
// technique's own set-up.
// NOTE: stand-in for the material apply function 0x0079CD0C (outside this
// range, not tagged here): the console version also binds the material's
// textures.
void ApplyMaterial(AglMaterial* material, AglTechnique* technique)
{
    if (material->anim)
        material->anim->Update();
    MaterialStateCache& c = g_materialCache;
    if (!c.blendValid || c.blendEnable != material->blendEnable || c.blendSrc != material->blendSrc ||
        c.blendDst != material->blendDst || c.blendEquation != material->blendEquation)
        SetBlendStateCached(material->blendEnable, material->blendSrc, material->blendDst, material->blendEquation);
    if (!c.depthValid || c.depthFunc != material->depthFunc || c.depthWrite != material->depthTest ||
        c.cullFrontFace != material->cullFrontFace) {
        SetDepthStateCached(material->depthFunc, material->depthTest);
        SetCullStateCached(material->cullFrontFace);
    }
    if (!technique && material->shaderClass && material->shaderClass->GetTechniqueCount() != 0)
        technique = material->shaderClass->GetTechnique(0);
    if (!technique)
        return;
    BindTechniquePrograms(technique);
    technique->BeginMaterial(material);
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
        ApplyMaterial(material, technique);
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
