// openTIT - Knowledge lists behind animation events (0x000650B0-0x000653CC).
//
// The game hands three callbacks to the anim clip manager: an AnimEvent
// element of a clip becomes a standalone result in a knowledge list owned by
// the anim set, and every character slot holding the set gets its own copy of
// that list (released with the slot).
#include "ape/anim/ApeAnimClip.h"
#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"
#include "sly/knowledge/Knowledge.h"

namespace sly {
namespace levels {
namespace ai {

namespace {

constexpr uint16_t kSlotListCapacity = 0x40;

// The list a library handle (index << 16 | serial) names, or null when the
// handle is stale.
knowledge::KnowledgeList* ListFromHandle(uint32_t handle)
{
    knowledge::KnowledgeLibrary* library = knowledge::Library();
    const uint16_t serial = uint16_t(handle & 0xFFFF);
    const uint32_t index = handle >> 16;
    if (!library || serial == 0 || index >= (uint32_t)library->capacity)
        return nullptr;
    if (!library->used[index] || library->serial[index] != serial)
        return nullptr;
    return library->slots[index].get();
}

} // namespace

// @func 0x000650B0 AnimEventKnowledgeUserData
// The set's list is made on first use (empty, with runtime, sized by the
// set's clip count hint); only the first child of the event element is
// read. Returns the entry index of the new result, -1 on failure.
int32_t AnimEventKnowledgeUserData(ape::xml::XmlElement* element, ape::anim::AnimSet* set, ape::anim::AnimClipDef* clip)
{
    if ((set->field_28 & 0xFFFF) == 0 && knowledge::Library()) {
        knowledge::KListRequest request;
        request.empty = true;
        request.withRuntime = true;
        request.entryCapacity = uint16_t(set->clipCountHint);
        request.name = set->name;
        uint32_t handle = 0;
        knowledge::Library()->Add(request, &handle);
        set->field_28 = handle;
    }
    knowledge::KnowledgeList* list = ListFromHandle(set->field_28);
    if (!list || !list->data)
        return -1;
    ape::xml::XmlElement* first = element->FirstChildElement();
    if (first && first->NextSiblingElement()) {
        ape::xml::XmlDocument* document = element->GetDocument();
        ape::TempFormat("%s(%ld): (AnimSet %s, clip %s): AnimEvent element has more than one child - only the "
                        "first will be parsed.",
                        document ? document->GetDocumentPath() : "", 0L, set->name, clip->name);
    }
    int16_t entry = -1;
    knowledge::AddStandaloneResult(list, element->FirstChildElement(), &entry);
    return entry;
}

// @func 0x00065290 AnimEventKnowledgeAcquireSlot
// Clones the set's list for the slot's user. NOTE: on the console `user`
// points at the owning entity; the port's 32-bit callback argument cannot
// carry it, so the clone has no owner.
void AnimEventKnowledgeAcquireSlot(uint32_t* outUserData, uint32_t setData, uint32_t user)
{
    (void)user;
    *outUserData = 0;
    knowledge::KnowledgeList* source = ListFromHandle(setData);
    if (!source || !source->data)
        return;
    if (!(source->data->flags & knowledge::kList_Template)) {
        ape::TempFormat("The source knowledge list is not a def, cannot clone it.");
        return;
    }
    knowledge::KListRequest request;
    request.source = source;
    request.entryCapacity = kSlotListCapacity;
    uint32_t handle = 0;
    knowledge::Library()->Add(request, &handle);
    *outUserData = handle;
}

// @func 0x000653CC AnimEventKnowledgeReleaseSlot
void AnimEventKnowledgeReleaseSlot(uint32_t userData)
{
    knowledge::Library()->Remove(userData);
}

} // namespace ai
} // namespace levels
} // namespace sly
