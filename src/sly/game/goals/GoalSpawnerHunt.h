// openTIT - Goal type 0x22 (vtable 0x00B1EDE8, 0xBB0 bytes; job XML element
// 0x9E69AF49, name not recovered): a hunt over the entities of five spawners
// (named spawner base + 0..4 in the spawn manager). The goal counts the
// entities left (GetProgress reports five minus the remainder), picks the
// one the player catches, and keeps a list of up to 32 level objects named
// "<markerBase>NN" for its markers.
//
// NOTE: the class name is descriptive; only the creation, destruction and
// the small virtuals are ported. HandleMessage (0x00336AE8), Update
// (0x003379A0), CollectMarkers (0x00338950), SetStatus (0x00338ABC) and the
// spawn helper 0x00337690 are TODO.
#pragma once

#include "sly/game/goals/Goal.h"

#include "ape/math/ApeMath.h"

namespace ape::xml { class XmlElement; }
namespace sly { class Spawner; namespace entity { class Entity; } }
namespace sly { namespace hud { class HudObject; } }

namespace sly {
namespace goals {

class GoalSpawnerHunt : public Goal {
public:
    using Goal::Goal;
    // 0x003366F0 / 0x00336A80.
    ~GoalSpawnerHunt() override;
    // 0x003373E8.
    static Goal* Create(ape::xml::XmlElement* xml, const GoalDesc* desc);
    int TypeId() const override { return 0x22; }
    void ResetHints(bool stopHintScript) override;       // slot 7 (0x0033876C)
    void ReapplyHints(Goal* owner) override;             // slot 9 (0x003386C0)
    bool GetProgress(GoalProgress* out) override;        // slot 13 (0x0033891C)

    // 0x00338560: the level objects "<markerBase>01".."<markerBase>32" that
    // exist (stops at the first missing one).
    void CollectMarkerObjects();

    uint32_t spawnerBase = 0;           // +0xA0C spawners are spawnerBase + 0..4
    Spawner* spawners[5] = {};          // +0xA10
    int32_t field_A24 = -1;             // +0xA24
    bool field_A28 = false;             // +0xA28 NOTE: set by the message handler; SetStatus spawns when set
    uint32_t field_A2C = 0;             // +0xA2C attribute 0x35107D59
    entity::Entity* spawned[5] = {};    // +0xA30 entities the spawners made (references held)
    int32_t remaining = 0;              // +0xA44 entities still to find
    bool field_A48 = false;             // +0xA48
    bool field_A49 = false;             // +0xA49
    bool field_A4A = false;             // +0xA4A
    uint32_t field_A4C = 0;             // +0xA4C attribute 0x6E6EDD6B
    ape::Vec3 offset{};                 // +0xA50 attribute 0x4A4D4D4D
    bool field_A60 = false;             // +0xA60 attribute 0x9446A097
    entity::Entity* caught = nullptr;   // +0xA64 the entity being caught
    float field_AB0 = 8.0f;             // +0xAB0 attribute 0x0A1A79FB
    char markerBase[0x20] = {};         // +0xAB4 attribute 0xBFB1D754
    void* markerObjects[32] = {};       // +0xAD4
    int32_t markerCount = 0;            // +0xB54
    ape::Matrix34 playerTransform{};    // +0xB60 the player is put here on message 0x7D22CF63
    hud::HudObject* hudElement = nullptr;   // +0xBA0
    bool hudTaken = false;              // +0xBA4 the goal turned the HUD counters off

private:
    void GiveHudBack();
};

} // namespace goals
} // namespace sly
