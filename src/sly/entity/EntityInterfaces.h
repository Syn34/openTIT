// openTIT - The two listener interfaces many entity classes mix in.
//
// Entity classes such as the physical entities, the scripted actors and the
// characters derive from their entity base plus two small interfaces that sit
// at +0x17C and +0x180 of the object on the console. Their own vtables are
// 0x00B1200C and 0x00B12528 (defaults at 0x0074354C-0x007435B4 and
// 0x00746BC0-0x00746CB0). Their purpose is not known yet, so the methods
// keep slot names.
#pragma once

#include <cstdint>

namespace sly {
namespace entity {

// Interface at +0x17C (vtable 0x00B1200C): three callbacks, the destructor,
// three more callbacks. NOTE: looks like a physics step listener; the
// default camera mode receives slot 0 with the state of its probe bodies.
class EntityInterface17C {
public:
    // @func 0x0074354C EntityInterface17C::OnStep
    virtual void OnStep(const void* info) { (void)info; }    // slot 0
    // @func 0x00743550 EntityInterface17C::Interface17CSlot1
    virtual void Interface17CSlot1() {}
    // @func 0x00743554 EntityInterface17C::Interface17CSlot2
    virtual void Interface17CSlot2() {}
    // @func 0x00743558 EntityInterface17C::~EntityInterface17C
    // @func 0x00743568 EntityInterface17C::~EntityInterface17C (deleting)
    virtual ~EntityInterface17C() = default;                 // slots 3/4
    // Slot 5: NOTE: with the slot layout of hkpContactListener this is the
    // "contact point added" callback; the event is passed through untouched.
    // @func 0x007435AC EntityInterface17C::Interface17CSlot5
    virtual void Interface17CSlot5(const void* event) { (void)event; }
    // @func 0x007435B0 EntityInterface17C::Interface17CSlot6
    virtual void Interface17CSlot6() {}
    // @func 0x007435B4 EntityInterface17C::Interface17CSlot7
    virtual void Interface17CSlot7() {}
};

// Event handed to the +0x180 interface (only the word at +0x08 is read here).
struct EntityInterface180Event {
    uint32_t field_00 = 0;
    uint32_t field_04 = 0;
    uint32_t field_08 = 0;   // non-zero: forwarded to slots 3 and 2
};

// Interface at +0x180 (vtable 0x00B12528): the destructor, then five callbacks.
class EntityInterface180 {
public:
    // @func 0x00746BC0 EntityInterface180::~EntityInterface180
    // @func 0x00746BD0 EntityInterface180::~EntityInterface180 (deleting)
    virtual ~EntityInterface180() = default;                 // slots 0/1
    // @func 0x00746C14 EntityInterface180::Interface180Slot2
    virtual void Interface180Slot2(const EntityInterface180Event& e) { (void)e; }
    // @func 0x00746C18 EntityInterface180::Interface180Slot3
    virtual void Interface180Slot3(const EntityInterface180Event& e) { (void)e; }
    // Slot 4: events with the word at +0x08 set go to slot 3, then slot 2.
    // @func 0x00746C1C EntityInterface180::Interface180Slot4
    virtual void Interface180Slot4(const EntityInterface180Event& e)
    {
        if (e.field_08) {
            Interface180Slot3(e);
            Interface180Slot2(e);
        }
    }
    // @func 0x00746CAC EntityInterface180::Interface180Slot5
    virtual void Interface180Slot5() {}
    // Slot 6: a body the entity listens to is being deleted. NOTE: matches
    // hkpEntityListener::entityDeletedCallback; the rigid body entities stop
    // listening to its contacts (their +0x17C interface) here.
    // @func 0x00746CB0 EntityInterface180::Interface180Slot6
    virtual void Interface180Slot6(void* body) { (void)body; }
};

} // namespace entity
} // namespace sly
