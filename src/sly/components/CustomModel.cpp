// openTIT - <CustomModel> component (0x0025C658-0x0025CC9C, 0x0028E9F8-0x0028F054).
#include "sly/components/CustomModel.h"

#include "ape/world/ApeDef.h"
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeInstances.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeSkeleton.h"
#include "sly/components/ComponentHooks.h"
#include "sly/game/SlyGame.h"
#include "sly/game/goals/JobManager.h"

#include <cstdio>

namespace sly {
namespace components {

namespace {
constexpr uint32_t kCloneFlags = ape::world::kClone_Children | ape::world::kClone_04 | ape::world::kClone_NoProcAnim;
constexpr uint32_t kModelLod = 10;
const ape::Vec3 kUnitScale{1.0f, 1.0f, 1.0f, 0.0f};

uint16_t SkeletonBoneCount(ape::world::ApeObj* object)
{
    ape::world::ApeSkeleton* skeleton = object ? object->GetSkeleton() : nullptr;
    return skeleton ? skeleton->numBones : 0;
}
} // namespace

// @func 0x0025CC9C CustomModelComponent::CustomModelComponent
CustomModelComponent::CustomModelComponent(const CustomModelComponentDesc* desc)
    : EntityComponent(desc)
    , groupCount(uint16_t(desc->groupCount))
{
    if (owner)
        boneCount = SkeletonBoneCount(owner->object);
    for (uint32_t i = 0; i < desc->groupCount && i < uint32_t(kMaxGroups); ++i)
        groups[i] = desc->groups[i];
    Rebuild();
}

// @func 0x0028E9F8 CustomModelComponent::~CustomModelComponent
// @func 0x0028EB70 CustomModelComponent::~CustomModelComponent (deleting)
CustomModelComponent::~CustomModelComponent()
{
    for (ape::world::ApeObj* model : models)
        if (model)
            model->Release();
}

// @func 0x0025C658 CustomModelComponent::Rebuild
void CustomModelComponent::Rebuild()
{
    for (ape::world::ApeObj*& model : models) {
        if (model) {
            model->Release();
            model = nullptr;
        }
    }
    if (!owner || (owner->classBits & 1) == 0 || !owner->object || !owner->object->GetSkeleton())
        return;

    // The costume the owner's player picked. NOTE: the console scans eight
    // choice records; only the named slots can match a group.
    const game::PlayerProgress& progress = game::ProfileProgress(g_game->players[owner->PlayerSlot()].index);
    for (int i = 0; i < groupCount; ++i) {
        const CustomModelGroup& group = groups[i];
        if (group.optionCount == 0)
            continue;
        uint32_t choice = 0;
        for (const game::PlayerProgress::CostumeChoice& picked : progress.costume) {
            if (picked.slot != group.nameHash)
                continue;
            for (uint32_t option = 0; option < group.optionCount; ++option) {
                if (group.options[option].nameHash == picked.option) {
                    choice = option;
                    break;
                }
            }
        }
        selected[i] = uint8_t(choice);
        const uint32_t objName = group.options[choice].objName;
        if (objName == 0)
            continue;

        // A placed instance of that name is cloned (and hidden); otherwise a
        // fresh instance of the geometry definition is made. Either must have
        // the owner's bone count.
        ape::world::ApeObj* model = nullptr;
        if (ape::world::ApeObj* placed = ape::world::FindGeomInstance(objName)) {
            if (SkeletonBoneCount(placed) == boneCount && placed->GetSkeleton()) {
                placed->SetVisible(false, false);
                model = placed->Clone(kCloneFlags);
            }
        } else if (auto* geomDef = ape::world::ApeTDefList<ape::world::ApeGeomDef>::Find(objName)) {
            if (geomDef->m_skeletonDef && geomDef->m_skeletonDef->numBones == boneCount)
                model = ape::world::CreateGeomInstance(geomDef, false);
        }
        models[i] = model;
        if (!model)
            continue;

        char name[0x40];
        std::snprintf(name, sizeof(name), "CMdl%02d_%s", i + 1, owner->GetDebugName());
        model->SetName(name, true);
        model->SetVisible((owner->flags & entity::kEnt_Visible) != 0, false);
        model->SetScale(owner->object ? owner->object->GetScale() : kUnitScale, false);
        model->VirtualSlot13(entity::EntityGeomByteA8(owner), false);
        model->SetLocalTransform(owner->transform);
        auto* renderObj = static_cast<ape::world::ApeRenderObj*>(model);
        if (renderObj->m_flags142 & 0x80) {
            renderObj->m_flags142 &= 0x7F;
            renderObj->OnBoundsChanged();
        }
        renderObj->SetLodField(kModelLod);
    }
}

// @func 0x0028EBB8 CustomModelComponent::ApplyPose
void CustomModelComponent::ApplyPose()
{
    if (!owner || !owner->IsAlive() || !owner->object)
        return;
    ape::world::ApeObj* source = owner->object;
    for (int i = 0; i < groupCount; ++i) {
        ape::world::ApeObj* model = models[i];
        if (!model)
            continue;
        model->SetLocalTransform(source->GetLocal());
        for (uint16_t bone = 0; bone < boneCount; ++bone)
            *model->GetSkeleton()->bones[bone].local = *source->GetSkeleton()->bones[bone].local;
        model->SetVisible(source->IsVisible(), false);
        model->GetSkeleton()->flags |= ape::world::kSkelFlag_Dirty;
        model->MarkDirty();
    }
}

// @func 0x0028EE68 CustomModelComponent::RefreshPose
void CustomModelComponent::RefreshPose()
{
    ApplyPose();
}

// @func 0x0028EE94 CustomModelComponent::Update
void CustomModelComponent::Update(float)
{
    QueueWorldCallback(2, reinterpret_cast<void*>(&WorldCallback), this, owner);
}

// @func 0x0028F054 CustomModelComponent::WorldCallback
void CustomModelComponent::WorldCallback(void*, CustomModelComponent* component)
{
    component->ApplyPose();
}

// @func 0x0028EED4 CustomModelComponent::SetVisible
void CustomModelComponent::SetVisible(bool visible, bool immediate)
{
    for (int i = 0; i < groupCount; ++i)
        if (models[i])
            models[i]->SetVisible(visible, immediate);
}

// @func 0x0028EF94 CustomModelComponent::SetAlpha
void CustomModelComponent::SetAlpha(uint8_t alpha, bool children)
{
    for (int i = 0; i < groupCount; ++i)
        if (models[i])
            models[i]->VirtualSlot13(alpha, children);
}

} // namespace components
} // namespace sly
