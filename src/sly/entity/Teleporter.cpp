// openTIT - The teleporter actor (ObjectActor, vtable 0x00B18488): built from
// a "Sly_Teleporter" setting placed on a level object; using it asks the game
// for a trip to its destination.
#include "sly/entity/ActorDefs.h"
#include "sly/entity/ActorEntities.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/game/SAISettings.h"
#include "sly/objects/EntityPlacements.h"

#include "ape/core/ApeString.h"
#include "ape/setting/ApeSetting.h"
#include "ape/world/ApeObj.h"

#include <cstring>

namespace sly {
namespace entity {

// @func 0x001EB478 ObjectActor::ObjectActor
ObjectActor::ObjectActor(const objects::ObjectActorInstanceDesc& desc)
    : ActorEntity(desc), destObj(desc.destObj), destLevel(desc.destLevel), destSpawnPoint(desc.destSpawnPoint),
      noTeleport(desc.noTeleport), destControllableCurrentPlayer(desc.destControllableCurrentPlayer)
{
    if (destLevel == 0 || destLevel == GameWorldCurrentLevel()) {
        object_1F4 = ape::world::FindObject(desc.destObj, ape::world::kObjType_PointInstance);
        if (!object_1F4)
            object_1F4 = ape::world::FindInstance(desc.destObj, 0);
        if (object_1F4)
            object_1F4->AddRef();
        else if (!noTeleport)
            ape::TempHexString(desc.destObj);   // warning (stripped)
    }
    if (desc.destControllable && *desc.destControllable)
        ape::StrCopyBounded(destControllable, desc.destControllable, sizeof destControllable);
    else
        destControllable[0] = '\0';
}

// @func 0x001EB814 ObjectActor::SetEnabled
void ObjectActor::SetEnabled(bool enabled)
{
    if (object)
        object->SetVisible(enabled, false);
    if (EntityComponent* interaction = EntityInteraction(this)) {
        interaction->SetOption(enabled);
        InteractionSetActive(interaction, enabled);
    }
}

// @func 0x001EB8CC ObjectActor::InteractionBegin
bool ObjectActor::InteractionBegin(Entity* user)
{
    if (noTeleport)
        return false;
    TravelRequest request;
    request.destControllable = destControllable[0] ? destControllable : nullptr;
    request.player = int8_t(user->flags >> 24);
    request.level = destLevel;
    request.respawnPoint = destSpawnPoint;
    request.destObj = destObj;
    request.destObject = object_1F4;
    request.keepCharacterName = destControllableCurrentPlayer;
    request.interaction = EntityInteraction(this);
    GameWorldRequestTravel(request);
    return true;
}

namespace {

namespace params = sai_params;

// The deferred half of the association (0x001EBA20): the teleporter of the
// setting's definition (a "Teleporter" class definition) placed at the
// object `objectName` names.
void CreateTeleporter(void* target, uint32_t objectName)
{
    auto* setting = static_cast<ape::setting::Setting*>(target);
    ape::world::ApeObj* place = ape::world::FindObject(objectName, ape::world::kObjType_PointInstance);
    if (!place) {
        ape::TempFormat("Cannot find the teleporter object instance (%s) to associate with Entity Teleporter (%s)",
                        ape::TempHexString(objectName), setting->name);
        return;
    }
    const char* defName = setting->Value(params::TeleporterDef).str;
    EntityDef* def = FindEntityDef(defName ? ape::HashNameNoCase(defName) : 0);
    constexpr uint32_t kType2_Teleporter = 0x2;
    if (!def || !(def->registryFlags & kType2_Teleporter))
        return;
    objects::ObjectActorInstanceDesc desc(place->GetName(), def, nullptr, place->GetWorld(),
                                          ape::world::kWorldUnitScale, true, false);
    desc.typeBits[2] |= 2;   // type bit 65
    auto hashOf = [&](const ape::setting::SettingParam& param) {
        const char* text = setting->Value(param).str;
        return text ? ape::HashNameNoCase(text) : 0u;
    };
    desc.destControllable = setting->Value(params::DestControllable).str;
    desc.destObj = hashOf(params::DestObj);
    desc.destLevel = hashOf(params::DestLevel);
    desc.destSpawnPoint = hashOf(params::DestLevelSpawnPoint);
    desc.noTeleport = setting->Value(params::NoTeleport).b;
    desc.destControllableCurrentPlayer = setting->Value(params::DestControllableCurrentPlayer).b;
    auto* teleporter = new ObjectActor(desc);
    if (EntityComponent* interaction = EntityInteraction(teleporter))
        InteractionSetAlwaysHideContextual(interaction, setting->Value(params::AlwaysHideContextual).b);
    if (!setting->Value(params::SpawnOnCreate).b)
        teleporter->SetEnabled(false);
}

} // namespace

// @func 0x001EB9CC AssociateTeleporter
void AssociateTeleporter(ape::setting::Setting* setting, const char* objectName, bool reload)
{
    (void)reload;
    if (!objectName)
        return;
    using Callback = void (*)(void*, uint32_t);
    const Callback callback = &CreateTeleporter;
    QueueDeferredEvent(setting, ape::HashNameNoCase(objectName), reinterpret_cast<uintptr_t>(callback));
}

} // namespace entity
} // namespace sly
