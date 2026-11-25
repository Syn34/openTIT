// openTIT - <ContextualMarker> component (0x0025FA1C-0x0025FACC,
// 0x0027BB20-0x0027BDC8).
#include "sly/components/ContextualMarker.h"

#include "ape/world/ApeObj.h"
#include "sly/components/ComponentHooks.h"

namespace sly {
namespace components {

// @func 0x0025FACC ContextualMarkerComponent::ContextualMarkerComponent
// The attach point is a locator of the owner's object definition, else a
// bone, else an object below the owner.
ContextualMarkerComponent::ContextualMarkerComponent(const ContextualMarkerComponentDesc* desc)
    : EntityComponent(desc)
    , attachRot(desc->attachRot)
    , scaleMin(desc->scaleMin)
    , scaleMax(desc->scaleMax)
    , scaleDistMin(desc->scaleDistMin)
    , scaleDistMax(desc->scaleDistMax)
    , scaleFactorFOV(desc->scaleFactorFOV)
    , hasScaleFactorFOV(desc->hasScaleFactorFOV)
    , applyRotation(desc->applyRotation)
{
    attachOffset = { desc->attachOffset.x, desc->attachOffset.y, desc->attachOffset.z, 0.0f };
    attachCameraOffset = { desc->attachCameraOffset.x, desc->attachCameraOffset.y, desc->attachCameraOffset.z,
                           0.0f };
    geometry = desc->geomDef ? MarkerGeometry(desc->geomDef) : nullptr;

    if (desc->attachPart && owner) {
        locator = owner->object ? EntityFindLocator(owner, desc->attachPart) : nullptr;
        if (!locator) {
            bone = owner->FindBone(desc->attachPart);
            if (bone == -1)
                attachObject = entity::EntityFindObject(owner, desc->attachPart);
        }
    }
    if (geometry)
        geometry->SetVisible(true, true);
    enabled = desc->startEnabled;
    SetVisible(enabled, true);
}

// @func 0x0027BB20 ContextualMarkerComponent::~ContextualMarkerComponent
// @func 0x0027BC64 ContextualMarkerComponent::~ContextualMarkerComponent (deleting)
ContextualMarkerComponent::~ContextualMarkerComponent()
{
    entity::UnregisterObject(entity::kList_C, this);
    if (geometry) {
        geometry->Release();
        geometry = nullptr;
    }
}

// @func 0x0027BDC8 ContextualMarkerComponent::Update
void ContextualMarkerComponent::Update(float dt)
{
    (void)dt;
    if (geometry && geometry->IsVisible())
        entity::AddToInstanceRenderList(this);
}

// TODO(0x0027BE38 ContextualMarkerComponent::RenderInstance): needs the
// attach transform (0x0049FFE0) and the camera of the game world.
void ContextualMarkerComponent::RenderInstance() {}

// @func 0x0025FA1C ContextualMarkerComponent::SetOption
void ContextualMarkerComponent::SetOption(bool on)
{
    enabled = on;
    SetVisible(on, true);
}

// @func 0x0025FA64 ContextualMarkerComponent::SetVisible
void ContextualMarkerComponent::SetVisible(bool visible, bool immediate)
{
    if (geometry)
        geometry->SetVisible(visible, immediate);
}

} // namespace components
} // namespace sly
