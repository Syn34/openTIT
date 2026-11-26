// openTIT - Instance draw records (0x0057A1D8-0x0057A770, 0x00582F3C).
#include "ape/world/ApeVisTable.h"

#include "ape/core/ApeSystem.h"
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeRenderNode.h"

namespace ape::world {

namespace {

constexpr uint32_t kDefaultVisRecords = 0x1000;

void ResetRecord(ApeVisRecord& record, ApeRenderObj* owner)
{
    record = ApeVisRecord();
    record.world.xAxis = { 1.0f, 0.0f, 0.0f, 0.0f };
    record.world.yAxis = { 0.0f, 1.0f, 0.0f, 0.0f };
    record.world.zAxis = { 0.0f, 0.0f, 1.0f, 0.0f };
    record.world.translation = { 0.0f, 0.0f, 0.0f, 0.0f };
    record.owner = owner;
}

bool HasVisRecordType(const ApeObj& obj)
{
    return obj.GetType() == kObjType_GeomInstance || obj.GetType() == kObjType_EmitterInstance;
}

} // namespace

ApeVisTable& GetVisTable()
{
    static ApeVisTable s_table;
    if (s_table.records.empty()) {
        const uint32_t count = g_engineConfig.field_154 ? g_engineConfig.field_154 : kDefaultVisRecords;
        s_table.records.resize(count);
        s_table.geomCount = 0;
        s_table.emitterNext = static_cast<uint16_t>(count - 1);
    }
    return s_table;
}

// @func 0x0057A46C LinkVisRecord
void LinkVisRecord(uint16_t index)
{
    ApeVisTable& table = GetVisTable();
    ApeRenderObj* owner = table.records[index].owner;
    ApeRenderNode* node = owner ? GetRenderNodeOf(*owner) : nullptr;
    if (!node)
        return;
    ApeVisRecord* record = nullptr;
    if (HasVisRecordType(*owner) && owner->m_visSlot != 0xFFFF)
        record = &table.records[owner->m_visSlot];
    node->visRecord = record;
}

// @func 0x0057A53C AllocVisRecord
void AllocVisRecord(ApeRenderObj& obj)
{
    ApeVisTable& table = GetVisTable();
    uint16_t index;
    if (obj.GetType() == kObjType_GeomInstance)
        index = table.geomCount++;
    else if (obj.GetType() == kObjType_EmitterInstance)
        index = table.emitterNext--;
    else
        return;
    obj.m_visSlot = index;
    ResetRecord(table.records[index], &obj);
    LinkVisRecord(index);
}

// @func 0x0057A1D8 MoveVisRecord
void MoveVisRecord(uint16_t index, uint16_t from)
{
    ApeVisTable& table = GetVisTable();
    ApeVisRecord& record = table.records[index];
    if (record.owner)
        record.owner->m_visSlot = 0xFFFF;
    if (index == from) {
        ResetRecord(record, nullptr);
        return;
    }
    record = table.records[from];
    if (record.owner)
        record.owner->m_visSlot = index;
}

void ReleaseVisRecord(ApeRenderObj& obj)
{
    const uint16_t slot = obj.m_visSlot;
    if (slot == 0xFFFF)
        return;
    ApeVisTable& table = GetVisTable();
    if (obj.GetType() == kObjType_GeomInstance) {
        // The last geometry record fills the hole.
        const uint16_t last = static_cast<uint16_t>(table.geomCount - 1);
        MoveVisRecord(slot, last);
        if (slot != last)
            LinkVisRecord(slot);
        --table.geomCount;
    } else if (obj.GetType() == kObjType_EmitterInstance) {
        // Emitters grow downwards: the lowest emitter record fills the hole.
        const uint16_t first = static_cast<uint16_t>(table.emitterNext + 1);
        MoveVisRecord(slot, first);
        if (slot != first)
            LinkVisRecord(slot);
        ++table.emitterNext;
    }
}

// @func 0x00582F3C ApeRenderObj::SyncVisRecord
void ApeRenderObj::SyncVisRecord()
{
    if (m_visSlot == 0xFFFF)
        return;
    Matrix34& world = GetVisTable().records[m_visSlot].world;
    world.xAxis = { m_world.xAxis.x, m_world.xAxis.y, m_world.xAxis.z, world.xAxis.pad };
    world.yAxis = { m_world.yAxis.x, m_world.yAxis.y, m_world.yAxis.z, world.yAxis.pad };
    world.zAxis = { m_world.zAxis.x, m_world.zAxis.y, m_world.zAxis.z, world.zAxis.pad };
    world.translation = { m_world.translation.x, m_world.translation.y, m_world.translation.z,
                          world.translation.pad };
}

// @func 0x005A0158 ApeGeomInstance::~ApeGeomInstance
// @func 0x005A0268 ApeGeomInstance::~ApeGeomInstance (deleting)
ApeGeomInstance::~ApeGeomInstance()
{
    ReleaseVisRecord(*this);
}

uint32_t g_lightChangeStamp = 0;
uint32_t g_localLightsChanged = 0;
uint32_t g_allLightsChanged = 0;

// @func 0x005913E0 MarkAllLightsChanged
void MarkAllLightsChanged()
{
    g_allLightsChanged = g_lightChangeStamp;
}
uint32_t g_globalLightsChanged = 0;

ApeLightTable& GetLightTable()
{
    static ApeLightTable s_table;
    if (s_table.records.empty()) {
        // NOTE: the console sizes this table outside the ported code; the
        // record count of the draw record table is used here.
        const uint32_t count = g_engineConfig.field_154 ? g_engineConfig.field_154 : kDefaultVisRecords;
        s_table.records.resize(count);
        s_table.localCount = 0;
        s_table.globalNext = static_cast<uint16_t>(count - 1);
    }
    return s_table;
}

// @func 0x005A2024 MoveLightRecord
void MoveLightRecord(uint16_t index, uint16_t from)
{
    ApeLightTable& table = GetLightTable();
    ApeLightRecord& record = table.records[index];
    if (record.owner)
        record.owner->m_visSlot = 0xFFFF;
    if (index == from) {
        record.field_00 = {};
        record.field_2C = 0;
        record.owner = nullptr;
        return;
    }
    const uint8_t keptFlags = record.flags3E & 0x7F;
    record = table.records[from];
    record.flags3E = static_cast<uint8_t>((record.flags3E & 0x80) | keptFlags);
    if (record.owner)
        record.owner->m_visSlot = index;
}

// @func 0x005A21B4 ApeLightInstance::~ApeLightInstance
// @func 0x005A22AC ApeLightInstance::~ApeLightInstance (deleting)
ApeLightInstance::~ApeLightInstance()
{
    if (m_visSlot != 0xFFFF) {
        ApeLightTable& table = GetLightTable();
        if (m_lightType == 2) {
            MoveLightRecord(m_visSlot, static_cast<uint16_t>(table.localCount - 1));
            --table.localCount;
        } else if (m_lightType == 0 || m_lightType == 1) {
            MoveLightRecord(m_visSlot, static_cast<uint16_t>(table.globalNext + 1));
            ++table.globalNext;
        }
    }
    const auto* def = static_cast<const ApeLightDef*>(GetDef());
    if (def && def->m_lightType < 2)
        g_globalLightsChanged = g_lightChangeStamp;
    else
        g_localLightsChanged = g_lightChangeStamp;
}

// @func 0x005A3DC0 ApeEmitterInstance::~ApeEmitterInstance
// @func 0x005A3ED0 ApeEmitterInstance::~ApeEmitterInstance (deleting)
ApeEmitterInstance::~ApeEmitterInstance()
{
    ReleaseVisRecord(*this);
}

} // namespace ape::world
