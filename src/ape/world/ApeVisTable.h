// openTIT - Instance draw records ("visibility table", 0x01193004).
//
// Geometry and emitter instances that are drawn get one 0x80-byte record in
// a shared table: their world transform plus renderer data, and a pointer
// back to the owner. The render node of the owner points at its record.
// Geometry records are taken from the front of the table, emitter records
// from the back, so each kind stays packed; a released record is filled
// with the last record of its kind.
#pragma once

#include "ape/math/ApeMath.h"

#include <cstdint>
#include <vector>

namespace ape::world {

class ApeRenderObj;

struct alignas(16) ApeVisRecord {
    Matrix34 world;                 // 0x00 (the fourth lanes stay 0)
    Vec4 field_40 = {};             // NOTE: renderer data, zero when the record is set up
    uint8_t field_50[0x0C] = {};
    uint32_t field_5C = 0;
    ApeRenderObj* owner = nullptr;  // 0x60 (0xFEEDF00D in a cleared record on the console)
    uint32_t field_64 = 0;
    uint8_t field_68[0x14] = {};
    uint16_t field_7C = 0;
    uint8_t field_7E = 0;
    uint8_t field_7F = 0;
};

struct ApeVisTable {
    std::vector<ApeVisRecord> records;
    uint16_t geomCount = 0;         // 0x01193008: records [0, geomCount) are geometry
    uint16_t emitterNext = 0;       // 0x0119300A: next free emitter record (counts down)
};

// The table, created on first use with EngineConfig +0x154 records (0x1000
// by default).
ApeVisTable& GetVisTable();

// Gives a geometry or emitter instance its record (other types get none).
void AllocVisRecord(ApeRenderObj& obj);
// Points the owner's render node at record 'index' (geometry and emitters).
void LinkVisRecord(uint16_t index);
// Replaces record 'index' with record 'from' (or clears it when they are
// the same), keeping the owners' record numbers in step.
void MoveVisRecord(uint16_t index, uint16_t from);
// Releases the record of a geometry or emitter instance.
void ReleaseVisRecord(ApeRenderObj& obj);

} // namespace ape::world
