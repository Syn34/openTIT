// openTIT - The instance render lists of the entity module: entities (list
// B) and components (list C) placed by the instance render pass each frame,
// and the components of list D.
#include "sly/entity/Character.h"
#include "sly/entity/Entity.h"
#include "sly/entity/GameEntityHooks.h"

#include "ape/core/ApeString.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeObj.h"
#include "sly/objects/SimpleWheeledVehicle.h"

namespace sly {
namespace entity {

namespace {

// Entity type bits of the classes the visibility restore knows.
constexpr uint32_t kTypeBit_Character = 3;
constexpr uint32_t kTypeBit_08 = 8;
constexpr uint32_t kTypeBit_09 = 9;
constexpr uint32_t kTypeBit_10 = 10;
constexpr uint32_t kTypeBit_11 = 11;     // also shows its world object
constexpr uint32_t kTypeBit_Vehicle = 77; // SimpleWheeledVehicle

void ShowIfWanted(Entity* entity, bool wanted)
{
    const bool visible = (entity->flags & kEnt_Visible) != 0;
    if (wanted != visible)
        entity->SetVisible(wanted, true, true, false);
}

} // namespace

// @func 0x000D0344 RestoreInstanceListVisibility
// Puts every entity of the instance render list back to the visibility its
// class wants: characters and vehicles keep the copy their fade holds, the
// other listed classes are shown.
void RestoreInstanceListVisibility()
{
    for (uint32_t i = 0; i < EntityCount(kList_B); ++i) {
        Entity* entity = EntityAt(kList_B, i);
        if (!entity)
            continue;
        if (entity->HasTypeBit(kTypeBit_Character)) {
            const auto* character = static_cast<const Character*>(entity);
            ShowIfWanted(entity, (character->flags3C8 & Character::kFlag3C8_Visible) != 0);
        } else if (entity->HasTypeBit(kTypeBit_11)) {
            entity->SetVisible(true, true, true, true);
            entity->object->SetVisible(true, true);
        } else if (entity->HasTypeBit(kTypeBit_09) || entity->HasTypeBit(kTypeBit_08) ||
                   entity->HasTypeBit(kTypeBit_10)) {
            entity->SetVisible(true, true, true, false);
        } else if (entity->HasTypeBit(kTypeBit_Vehicle)) {
            const auto* vehicle = static_cast<const objects::SimpleWheeledVehicle*>(entity);
            ShowIfWanted(entity, vehicle->fadeVisible);
        }
    }
}

// @func 0x000D016C RunInstanceRenderLists
// The fade pass of the listed entities, then the listed components place
// their geometry.
void RunInstanceRenderLists(void* renderContext)
{
    for (uint32_t i = 0; i < EntityCount(kList_B); ++i)
        EntityAt(kList_B, i)->UpdateFade();
    for (uint32_t i = 0; i < EntityCount(kList_C); ++i)
        reinterpret_cast<EntityComponent*>(EntityAt(kList_C, i))->RenderInstance();
    InstanceRenderPassDone(renderContext);
}

// @func 0x000D0298 RunListDComponents
void RunListDComponents()
{
    ListDPassBegin();
    for (uint32_t i = 0; i < EntityCount(kList_D); ++i)
        reinterpret_cast<EntityComponent*>(EntityAt(kList_D, i))->Slot9();
}

// @func 0x000D0628 ClearInstanceRenderLists
// End of the frame: the listed entities get their visibility back and the
// three lists are emptied.
void ClearInstanceRenderLists()
{
    if (EntityCount(kList_B) != 0) {
        RestoreInstanceListVisibility();
        ClearEntityList(kList_B);
    }
    ClearEntityList(kList_C);
    ClearEntityList(kList_D);
}

// @func 0x000D6770 AddToComponentSyncList
// Components whose list D slot runs each frame ("s_compSyncList"); a second
// add is refused with a warning.
void AddToComponentSyncList(EntityComponent* component)
{
    Entity* entry = reinterpret_cast<Entity*>(component);
    for (uint32_t i = 0; i < EntityCount(kList_D); ++i) {
        if (EntityAt(kList_D, i) == entry) {
            ape::TempFormat("entity '%s' component '%s' already on s_compSyncList",
                            component && component->owner ? component->owner->GetDebugName() : "NULL",
                            component ? ape::TempHexString(component->id) : "NULL");
            return;
        }
    }
    RegisterEntity(kList_D, entry);
}

// @func 0x000D8FC4 RefreshEntityRender
// A geometry instance object is shown, has its render data refreshed and is
// hidden again; with `children` the attached entities follow.
void RefreshEntityRender(Entity* entity, bool children)
{
    ape::world::ApeObj* object = entity->object;
    if (object && object->GetType() == ape::world::kObjType_GeomInstance) {
        object->SetVisible(true, false);
        QueueObjectRenderRefresh(object, true);
        object->SetVisible(false, false);
    }
    if (!children || !entity->children)
        return;
    for (EntityChildLink& link : *entity->children)
        RefreshEntityRender(link.child, true);
}

} // namespace entity
} // namespace sly
