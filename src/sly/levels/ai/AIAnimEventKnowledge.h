// openTIT - Knowledge lists behind animation events (see AIAnimEventKnowledge.cpp).
#pragma once

#include <cstdint>

namespace ape::anim { class AnimSet; struct AnimClipDef; }
namespace ape::xml { class XmlElement; }

namespace sly {
namespace levels {
namespace ai {

// The anim clip manager callbacks the AI manager installs (0x000650B0,
// 0x00065290; the slot release 0x000653CC is not ported).
int32_t AnimEventKnowledgeUserData(ape::xml::XmlElement* element, ape::anim::AnimSet* set,
                                   ape::anim::AnimClipDef* clip);
void AnimEventKnowledgeAcquireSlot(uint32_t* outUserData, uint32_t setData, uint32_t user);

} // namespace ai
} // namespace levels
} // namespace sly
