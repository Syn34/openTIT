// openTIT - Placed sound emitters (see SoundEntity.h).
#include "sly/entity/SoundEntity.h"

#include "sly/entity/GameEntityHooks.h"

namespace sly {
namespace entity {

// @func 0x000F5EFC SoundEntity::~SoundEntity
// @func 0x000F5F68 SoundEntity::~SoundEntity (deleting)
SoundEntity::~SoundEntity()
{
    audio::StopVoice(voice, true);
    audio::SoundNameRelease(sound);
}

// @func 0x000F5FFC SoundEntity::Play
audio::VoiceId SoundEntity::Play()
{
    audio::StopVoice(voice, true);
    voice = PlayEntitySound(sound, attached ? nullptr : &transform.translation, attached ? &attachment : nullptr);
    return voice;
}

// @func 0x000F60DC SoundEntity::IsPlaying
bool SoundEntity::IsPlaying() const
{
    return audio::IsVoicePlaying(voice);
}

// @func 0x000F6208 SoundEntity::OnStateEvent
bool SoundEntity::OnStateEvent(EntityStateEvent& event)
{
    switch (event.state) {
    case 0:
        if (autoPlay && audio::SoundNameHasDef(sound))
            voice = PlayEntitySound(sound, attached ? nullptr : &transform.translation,
                                    attached ? &attachment : nullptr);
        break;
    case 1:
        if (event.arg == 0 && EntityGetState(this, 0) == 0)
            ReleaseVoiceLoop(voice);
        break;
    case 2:
        if (EntityGetState(this, 0) == 0 && audio::IsVoiceValid(voice)) {
            audio::SoundParams params;
            if (!GetVoiceParams(voice, params))
                event.finished = true;
            audio::SetRequestBusA(params, nullptr);
            audio::SetRequestBusB(params, nullptr);
        }
        break;
    default:
        break;
    }
    return true;
}

// @func 0x000F6118 FindSoundEntity
SoundEntity* FindSoundEntity(uint32_t nameHash)
{
    if (nameHash == 0)
        return nullptr;
    ape::ApeTArray<Entity*> found;
    CollectEntitiesOfType(SoundEntity::kTypeBit, found);
    for (uint32_t i = 0; i < found.Count(); ++i) {
        if (found[i]->nameHash == nameHash)
            return static_cast<SoundEntity*>(found[i]);
    }
    return nullptr;
}

} // namespace entity
} // namespace sly
