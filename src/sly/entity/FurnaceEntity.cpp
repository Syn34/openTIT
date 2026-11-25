// openTIT - Furnace entity destructors and small overrides (see FurnaceEntity.h).
#include "sly/entity/FurnaceEntity.h"

namespace sly {
namespace entity {

// @func 0x00743E54 FurnaceDef::~FurnaceDef
// @func 0x00743F94 FurnaceDef::~FurnaceDef (deleting)
FurnaceDef::~FurnaceDef() = default;

// @func 0x007440EC FurnaceEntity::~FurnaceEntity
// @func 0x007441E0 FurnaceEntity::~FurnaceEntity (deleting)
FurnaceEntity::~FurnaceEntity() = default;

// @func 0x007442EC FurnaceEntity::GetRenderObject
ape::world::ApeObj* FurnaceEntity::GetRenderObject() { return static_cast<ape::world::ApeObj*>(body); }

// @func 0x007442F4 FurnaceEntity::OnHitCategory0
void FurnaceEntity::OnHitCategory0(EntityHitRecord& hit) { (void)hit; }
// @func 0x007442FC FurnaceEntity::OnHitCategory2
void FurnaceEntity::OnHitCategory2(EntityHitRecord& hit) { (void)hit; }
// @func 0x00744300 FurnaceEntity::OnHitCategory3
void FurnaceEntity::OnHitCategory3(EntityHitRecord& hit) { (void)hit; }
// @func 0x007442F8 FurnaceEntity::OnHitCategory4
void FurnaceEntity::OnHitCategory4(EntityHitRecord& hit) { (void)hit; }

} // namespace entity
} // namespace sly
