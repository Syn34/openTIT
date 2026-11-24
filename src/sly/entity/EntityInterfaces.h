// openTIT - The two listener interfaces many entity classes mix in.
//
// Entity classes such as the physical entities, the scripted actors and the
// characters derive from their entity base plus two small interfaces that sit
// at +0x17C and +0x180 of the object on the console. Their own vtables are
// 0x00B1200C and 0x00B12528; the default methods live at 0x0074354C-0x007435B4
// and 0x00746BC0-0x00746CB0 (shared helper range). Their purpose is not
// known yet, so the methods keep slot names.
#pragma once

namespace sly {
namespace entity {

// Interface at +0x17C (vtable 0x00B1200C): three callbacks, the destructor,
// three more callbacks. NOTE: looks like a physics step listener; the
// default camera mode receives slot 0 with the state of its probe bodies.
class EntityInterface17C {
public:
    virtual void OnStep(const void* info) { (void)info; }    // slot 0
    virtual void Interface17CSlot1() {}
    virtual void Interface17CSlot2() {}
    virtual ~EntityInterface17C() = default;                 // slots 3/4
    virtual void Interface17CSlot5() {}
    virtual void Interface17CSlot6() {}
    virtual void Interface17CSlot7() {}
};

// Interface at +0x180 (vtable 0x00B12528): the destructor, then five callbacks.
class EntityInterface180 {
public:
    virtual ~EntityInterface180() = default;                 // slots 0/1
    virtual void Interface180Slot2() {}
    virtual void Interface180Slot3() {}
    virtual void Interface180Slot4() {}
    virtual void Interface180Slot5() {}
    virtual void Interface180Slot6() {}
};

} // namespace entity
} // namespace sly
