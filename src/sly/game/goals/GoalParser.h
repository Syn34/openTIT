// openTIT - Goal XML parser (0x00321D3C-0x003248B0): reads the attributes and
// child elements every goal shares into a GoalDesc, then hands it to the
// factory of the goal class named by the element.
#pragma once

#include "sly/game/goals/Goal.h"

namespace sly {
namespace goals {

class GoalGroup;

// Options of a goal group read by every group factory (0x00321D3C).
struct GoalGroupSetup {
    int32_t mode = 0;               // +0x00 GoalGroup::Mode ("op")
    int32_t maxMarkers = 0;         // +0x04 "MaxTargets"
    bool notifyAdvance = false;     // +0x08 attribute 0xA7BC1270. NOTE: name unknown
    bool waitActiveScripts = false; // +0x09 attribute 0x8E0B3401. NOTE: name unknown
    bool stopOnNewCompletion = false; // +0x0A attribute 0x9412EDCD. NOTE: name unknown

    // Copies the options into a freshly built group.
    void ApplyTo(GoalGroup* group) const;
};
// Reads the group options; false (and a warning) for a missing or unknown
// "op" (0x00321D3C).
bool ParseGoalGroupSetup(ape::xml::XmlElement* xml, GoalGroupSetup* out);

// Appends a script or sequence to script slot `slot` of a parse description
// (at most ten per slot); `bindings` are copied (0x00322AC0).
void AddGoalScript(GoalScriptCursor* scripts, int slot, uint32_t name, const GoalScriptBinding* bindings,
                   uint32_t bindingCount, bool isSequence, bool restart);

// Builds the goal described by `xml` for `job`; null for an unknown element
// or the elements that are not goals themselves (0x00322D88). `index` is
// handed on to the description (Goal::index).
Goal* ParseGoal(SlyGame* game, Job* job, ape::xml::XmlElement* xml, int32_t index);

// Frees what ParseGoal allocated in a description.
void ReleaseGoalDesc(GoalDesc& desc);

// Factories of goal classes that are not ported yet (TODO stubs).
Goal* CreateGoal9E69AF49(ape::xml::XmlElement* xml, const GoalDesc* desc);   // 0x003373E8
Goal* CreatePenguinMontageGoal(ape::xml::XmlElement* xml, const GoalDesc* desc);   // 0x003396A8
Goal* CreateBinocucomPhotoGoal(ape::xml::XmlElement* xml, const GoalDesc* desc);   // 0x0032B358
Goal* CreateShootingGalleryGoal(ape::xml::XmlElement* xml, const GoalDesc* desc);  // 0x0033DC0C
Goal* CreateQteGoal(ape::xml::XmlElement* xml, const GoalDesc* desc);        // 0x0033A520
// "GoalSet": a group whose children are the goal elements below it (0x00321EC8).
Goal* CreateGoalSet(ape::xml::XmlElement* xml, const GoalDesc* desc);

} // namespace goals
} // namespace sly
