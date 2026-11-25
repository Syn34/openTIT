// openTIT - The four effect kinds the FX manager's play request builds
// (0x002F0E0C): only their "attached to" overrides are ported. Names are
// vtable based; their constructors and updates (0x002ED6F4, 0x002EF938,
// 0x002F0058, 0x002F0CD8 and neighbours) are not.
#pragma once

#include "sly/game/FxManager.h"

namespace sly {
namespace game {

// Kill and value requests only apply when they name the object the effect
// is attached to (each class has its own copy).

// 0x9C bytes, vtable 0x00B1D528 (object at +0x0C).
class FxEffectB1D528 : public FxEffect {
public:
    void KillIfAttachedTo(ape::world::ApeObj* object) override;
    void SetValueIfAttachedTo(ape::world::ApeObj* object, float newValue) override;
    ape::world::ApeObj* attachedTo = nullptr;   // +0x0C
};

// 0x100 bytes, vtable 0x00B1D550 (object at +0xD4).
class FxEffectB1D550 : public FxEffect {
public:
    void KillIfAttachedTo(ape::world::ApeObj* object) override;
    void SetValueIfAttachedTo(ape::world::ApeObj* object, float newValue) override;
    ape::world::ApeObj* attachedTo = nullptr;   // +0xD4
};

// 0x14 bytes, vtable 0x00B1D578 (object at +0x0C).
class FxEffectB1D578 : public FxEffect {
public:
    void KillIfAttachedTo(ape::world::ApeObj* object) override;
    void SetValueIfAttachedTo(ape::world::ApeObj* object, float newValue) override;
    ape::world::ApeObj* attachedTo = nullptr;   // +0x0C
};

// 0x84 bytes, vtable 0x00B1D5A0 (object at +0x0C).
class FxEffectB1D5A0 : public FxEffect {
public:
    void KillIfAttachedTo(ape::world::ApeObj* object) override;
    void SetValueIfAttachedTo(ape::world::ApeObj* object, float newValue) override;
    ape::world::ApeObj* attachedTo = nullptr;   // +0x0C
};

} // namespace game
} // namespace sly
