// openTIT - <CustomModel> component (type 0x15, vtable 0x00B1C6D8, 0xB84
// bytes): costume pieces. Each option group of the description names one
// model per option; the model of the option the player picked for that
// costume slot (the profile's costume choices) is cloned and follows the
// owner's world object and skeleton pose every frame.
#pragma once

#include "sly/components/ComponentDescs2.h"
#include "sly/entity/Entity.h"

#include <cstdint>

namespace ape::world { class ApeObj; }

namespace sly {
namespace components {

class CustomModelComponent : public entity::EntityComponent {
public:
    static constexpr int kMaxGroups = CustomModelComponentDesc::kMaxGroups;

    // 0x0025CC9C
    explicit CustomModelComponent(const CustomModelComponentDesc* desc);
    ~CustomModelComponent() override;
    // Slot 6: queues the pose copy on the game world's list 2.
    void Update(float dt) override;
    // Slots 12/14 forward to every model.
    void SetVisible(bool visible, bool immediate) override;
    void SetAlpha(uint8_t alpha, bool children) override;

    // 0x0025C658: releases the models and builds the ones the owner's
    // player has picked (also called when the costume changes).
    void Rebuild();
    // 0x0028EBB8: copies the owner object's transform, bone pose and
    // visibility onto every model.
    void ApplyPose();
    // 0x0028EE68: the same, called from the character code.
    void RefreshPose();

    CustomModelGroup groups[kMaxGroups] = {};          // 0x18 copied from the description
    uint8_t selected[kMaxGroups] = {};                 // 0xB58 option index per group
    ape::world::ApeObj* models[kMaxGroups] = {};       // 0xB60
    uint16_t groupCount = 0;                           // 0xB80
    uint16_t boneCount = 0;                            // 0xB82 the owner's skeleton

private:
    // World callback list entry (0x0028F054).
    static void WorldCallback(void* unused, CustomModelComponent* component);
};

} // namespace components
} // namespace sly
