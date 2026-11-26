// openTIT - HUD model slots (see HudModelSlot.h).
#include "sly/game/hud/HudModelSlot.h"

#include "ape/world/ApeInstances.h"
#include "ape/world/ApeObj.h"
#include "sly/anim/AnimController.h"
#include "sly/entity/Entity.h"
#include "sly/entity/EntityDef.h"
#include "sly/game/hud/HudManager.h"

namespace sly {
namespace hud {

namespace {

// Light groups the HUD models are lit by. NOTE: group meanings unknown.
constexpr uint32_t kModelLightGroups = 0xD000u;

void ReleaseModel(entity::Entity*& model)
{
    if (!model)
        return;
    model->SetVisible(false, true, true, true);
    model->Release(false);
    // The slot's own reference: the spawn reference goes with the kill.
    if (model->flags & entity::kEnt_Alive)
        model->Release(true);
    model = nullptr;
}

} // namespace

// @func 0x0036A504 HudModelSlot::Clear
void HudModelSlot::Clear()
{
    style = nullptr;
    styleName = 0;
    ReleaseModel(entity);
}

// @func 0x0036A5F0 HudModelSlot::Set
void HudModelSlot::Set(const HudColorStyle* newStyle, uint32_t clip)
{
    Clear();
    if (!newStyle)
        return;

    entity::EntityPlacement placement;
    entity::Entity* model = entity::SpawnEntity(newStyle->field_04, placement);
    style = newStyle;
    entity = model;
    // NOTE: looked up in the third definition list (0x00F8F134), read as the
    // camera definitions.
    camera = ape::world::FindDef(newStyle->field_08, ape::world::kObjType_CameraDef);
    styleName = newStyle->name;
    if (!model)
        return;

    model->AddRef();
    model->SetBusyFlag(true);
    model->SetVisible(false, true, true, true);
    if (ape::world::ApeObj* object = model->object) {
        if (object->GetType() == ape::world::kObjType_GeomInstance) {
            auto* render = static_cast<ape::world::ApeRenderObj*>(object);
            render->SetLightGroupMask(kModelLightGroups);
            if (render->m_flags142 & 0x80) {
                render->m_flags142 &= uint8_t(~0x80);
                render->OnBoundsChanged();
            }
        }
    }
    if (!clip)
        clip = newStyle->field_18;
    if (clip)
        model->PlayClipByHash(1.0f, 0.25f, 0.0f, clip, 0, false);
    if (auto* controller = static_cast<anim::AnimController*>(model->animController))
        controller->flags &= uint16_t(~anim::AnimController::kFlag_DistanceLod);
}

} // namespace hud
} // namespace sly
