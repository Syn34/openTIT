// openTIT - Placed sound emitters (entity type bit 0x49, console vtable
// 0x00B13718, 0x170 bytes; constructed by 0x000D3BB4 in the entity module).
#pragma once

#include "sly/entity/Entity.h"
#include "sly/game/AudioSystem.h"

namespace sly {
namespace entity {

// Script state event handed to entity virtual slot 9.
struct EntityStateEvent {
    int32_t state = 0;      // 0x00 0: entered, 1: changed, 2: polled while active
    int32_t arg = 0;        // 0x04
    bool finished = false;  // 0x08 set by the entity when the state is done
};

class SoundEntity : public Entity {
public:
    static constexpr uint32_t kTypeBit = 0x49;

    ~SoundEntity() override;
    // Restarts the sound; returns the new voice (0x000F5FFC).
    audio::VoiceId Play();
    // 0x000F60DC
    bool IsPlaying() const;
    // Entity virtual slot 9: plays on entry when set to, lets a loop finish when
    // the state changes, reports completion while polled (0x000F6208).
    // NOTE: the slot is not declared on Entity yet.
    bool OnStateEvent(EntityStateEvent& event);

    audio::VoiceId voice = 0;            // 0x15C
    uint32_t attachment = 0;             // 0x160 NOTE: handed to the sound system as an attachment
    audio::SoundName* sound = nullptr;   // 0x164
    bool attached = false;               // 0x16C bit 31: play at `attachment`, not at the entity
    bool autoPlay = false;               // 0x16C bit 30
};

// Sound emitter registered under `nameHash`, or null (0x000F6118).
SoundEntity* FindSoundEntity(uint32_t nameHash);

} // namespace entity
} // namespace sly
