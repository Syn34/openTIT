// openTIT - Entity layer calls into systems that are not ported yet (see EntityHooks.h).
#include "sly/entity/EntityHooks.h"
#include "sly/entity/Entity.h"
#include "sly/anim/AnimController.h"
#include "sly/anim/AnimLayer.h"

#include "ape/anim/ApeAnimScript.h"

#include "ape/world/ApeObj.h"

namespace sly {
namespace entity {

// TODO(0x00464B18 GameObject::OnSpawn)
void GameObjectSpawn(Entity*) {}
// TODO(0x00464BD4 GameObject::Update)
bool GameObjectUpdate(Entity*, float) { return true; }
// TODO(0x00464CBC GameObject::PostUpdate)
bool GameObjectPostUpdate(Entity*, float) { return true; }

// CreateComponent (0x002635E8) lives in sly/components/ComponentFactory.cpp.
// TODO(0x002BC1E0 CenterComponent::GetOffset)
ape::Vec3 CenterComponentOffset(EntityComponent*) { return { 0.0f, 0.0f, 0.0f, 0.0f }; }

// TODO(world object as script reference)
ape::ApeRefCounted* EntityWorldObjectRef(Entity*) { return nullptr; }
// TODO(0x000CE688 bone transforms)
ape::Matrix34 EntityBoneWorldTransform(Entity* entity, uint32_t) { return entity->transform; }
void AnimControllerSetRunning(void* controller, bool running)
{
    auto* ctl = static_cast<anim::AnimController*>(controller);
    if (running)
        ctl->flags |= anim::AnimController::kFlag_Running;
    else
        ctl->flags &= ~anim::AnimController::kFlag_Running;
}
// TODO(0x004705F8 / 0x00470660 / 0x00482D14): game events live in sly/game.
void EntitySendGameEvent(Entity*, Entity*, uint32_t, uint32_t, bool) {}
void EntityPostScriptMessage(Entity*, uint32_t, uint32_t, uint32_t) {}
void GameWorldPostEntityEvent(uint8_t, uint32_t, Entity*) {}
// TODO(spawner slot 9 0x00456258)
void SpawnerEntityKilled(void*, Entity*) {}
// TODO(0x01013B1C placement map / 0x0049FCC0)
void ApplyPlacementToScript(ape::ApeAnimScriptInstance*, ape::world::ApeObj*) {}
// TODO(listener slot 31)
bool ListenerIsAlly(EntityListener*) { return false; }
// TODO(scripted entity +0x16C record)
void DamageRecordSetType(void*, uint32_t) {}
// TODO(EntityMessage +0x00): messages carry no sender pointer in the port yet.
Entity* MessageSenderEntity(const EntityMessage&) { return nullptr; }
// TODO(0x01032148 physics step flag)
bool PhysicsStepInProgress() { return false; }
// TODO(0x0046B6CC GameWorld::ForgetEntity)
void GameWorldForgetEntity(Entity*) {}
// TODO(watcher slot 74): the watcher classes are not ported.
void WatcherEntityReleased(void*, Entity*) {}
// TODO(spawner slot 8 0x00455F54): sly::Spawner is reached as an opaque pointer here.
void SpawnerEntityReleased(void*, Entity*) {}
// TODO(0x01019BE0 / 0x0101DBF0 target tables)
void ClearEntityFromTargetTables(Entity*) {}

// TODO(0x004A2560 NewRenderId)
uint32_t NewRenderId()
{
    static uint32_t s_next = 1;
    return s_next++;
}
// TODO(0x002EF7B8 CullManager::Update)
void CullManagerUpdate(ape::world::ApeObj*) {}

// TODO: game world +0x128168 camera, position at +0x360.
ape::Vec3 GameCameraPosition() { return { 0.0f, 0.0f, 0.0f, 0.0f }; }

// TODO: game world byte +0x1D.
bool IsSinglePlayerMode() { return true; }
// TODO: game world player blocks (+0x1078, stride 0x20C80).
Entity* PlayerEntity(int) { return nullptr; }

// The "playing animation" handles are animation layers (sly/anim/AnimLayer.h).
// NOTE: the hook names predate the layer port: AnimSetWeight sets the
// playback speed (0x003541E0), AnimSetPaused is AnimLayer::Stop
// (0x00353CC0) and AnimStop is AnimLayer::IsDone (0x00353EB0).
void* AnimFindPlaying(void* controller, uint32_t anim)
{
    return static_cast<anim::AnimController*>(controller)->FindLayer(anim);
}
float AnimDuration(void* playing) { return static_cast<anim::AnimLayer*>(playing)->Duration(); }
float AnimElapsed(void* playing) { return static_cast<anim::AnimLayer*>(playing)->LocalTime(); }
void AnimSetElapsed(void* playing, float time) { static_cast<anim::AnimLayer*>(playing)->SetLocalTime(time); }
void AnimSetWeight(void* playing, float value) { static_cast<anim::AnimLayer*>(playing)->SetPlaybackSpeed(value); }
void AnimSetPaused(void* playing, bool flag) { static_cast<anim::AnimLayer*>(playing)->Stop(flag); }
bool AnimIsFinished(void* playing)
{
    return (static_cast<anim::AnimLayer*>(playing)->flags & anim::AnimLayer::kFlag_Looping) != 0;
}
bool AnimStop(void* playing, bool flag) { return static_cast<anim::AnimLayer*>(playing)->IsDone(flag); }

void WorldObjectSetVisible(ape::world::ApeObj* object, bool visible, bool recurse)
{
    object->SetVisible(visible, recurse);
}
// TODO: geometry instance byte +0x142 bit 7 (not modelled by ApeGeomInstance yet).
bool GeomGetHighlight(ape::world::ApeObj*) { return false; }
void GeomSetHighlight(ape::world::ApeObj*, bool) {}
// TODO: geometry instance byte +0xA8.
uint8_t GeomByteA8(ape::world::ApeObj* geom) { return geom->m_byteA8; }

// TODO: render node bounds entry byte +0x1C bit 5 (renderer owned data).
void RenderNodeSetHidden(ape::world::ApeObj*, bool) {}
// TODO: global byte 0x01022014.
bool ForwardVisibilityToComponents() { return false; }
// TODO: component 0x20 byte +0x140 bit 7 (component class not modelled).
bool ComponentHidesRender(EntityComponent*) { return false; }

void AnimControllerDestroy(void* controller) { delete static_cast<anim::AnimController*>(controller); }
// TODO(0x0048F298)
void EntityLeaveListenerGroup(Entity* entity) { (void)entity; }
void WorldObjectSetUserData(ape::world::ApeObj* object, uint32_t tag, uint32_t value)
{
    object->field_114 = reinterpret_cast<void*>(static_cast<uintptr_t>(tag));
    object->field_118 = reinterpret_cast<void*>(static_cast<uintptr_t>(value));
}

void* AnimClipRegistryFind(uint32_t nameHash) { return anim::FindAnimClip(nameHash); }

void Component1DOnMessage(EntityComponent* component) { (void)component; }
void EntityObjectDCStart(void* object) { (void)object; }
void EntityObjectDCStop(void* object) { (void)object; }
void ScriptInstanceSetState(void* instance, uint32_t stateName)
{
    static_cast<ape::ApeAnimScriptInstance*>(instance)->SetStateAndUpdate(stateName);
}

// TODO(0x00490518)
void PropertySystemForgetDef(EntityDef* def) { (void)def; }

} // namespace entity
} // namespace sly
