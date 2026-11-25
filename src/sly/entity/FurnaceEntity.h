// openTIT - The furnace entity ("Furnace%i"; vtable 0x00B11E88, constructor
// 0x000B9020) and its definition (vtable 0x00B11E50, constructor 0x000B76E4).
//
// Only the destructors and the small virtual overrides in 0x00743E54-
// 0x00744300 are ported; the furnace's update and parser live at
// 0x000B7000-0x000BE000. The arrays keep their offsets as names: their
// element types are not decoded.
#pragma once

#include "sly/entity/ScriptedEntity.h"
#include "ape/memory/ApeTArray.h"

#include <array>
#include <vector>

namespace sly {
namespace entity {

class FurnaceDef : public ScriptedEntityDef {
public:
    ~FurnaceDef() override;

    // Records of 0x30 bytes holding one array at +0x00 (rest not decoded).
    struct Stage {
        ape::ApeTArray<void*> list;
        uint8_t field_14[0x1C] = {};
    };

    ape::ApeTArray<void*> field_C0;        // 0xC0
    ape::ApeTArray<void*> field_D4;        // 0xD4
    ape::ApeTArray<void*> field_100[3];    // 0x100
    Stage stages[3];                       // 0x13C
    ape::ApeTArray<void*> field_230;       // 0x230
};

class FurnaceEntity : public ScriptedEntity {
public:
    ~FurnaceEntity() override;
    // Slot 36: the furnace's Havok object (+0x284). NOTE: see NOTES on slot 36.
    ape::world::ApeObj* GetRenderObject() override;
    // Only impacts (category 1, 0x000BD6D0, not ported) affect the furnace.
    void OnHitCategory0(EntityHitRecord& hit) override;
    void OnHitCategory2(EntityHitRecord& hit) override;
    void OnHitCategory3(EntityHitRecord& hit) override;
    void OnHitCategory4(EntityHitRecord& hit) override;

    ape::ApeTArray<void*> field_218;              // 0x218
    ape::ApeTArray<void*> field_22C;              // 0x22C
    ape::ApeTArray<void*> field_23C[3];           // 0x23C
    void* body = nullptr;                         // 0x284
    // Pool of 16-byte records (10 to start with, +0x2B8 holds the capacity).
    std::vector<std::array<uint8_t, 16>> field_2A8;   // 0x2A8
};

} // namespace entity
} // namespace sly
