// openTIT - A list of entity references that drops dead entities.
#pragma once

#include "ape/memory/ApeTArray.h"

#include <cstdint>

namespace sly {
namespace entity {

class Entity;

// NOTE: owner class unknown (code at 0x000B4524/0x000B566C, between the
// loading screen tuning and the mine laser tutorial).
struct EntityRefList {
    bool nonEmpty = false;                     // 0x08 bit 31
    ape::ApeTArray<Entity*> entries;           // 0x0C
    void* field_E4 = nullptr;                  // 0xE4

    // Removes entities that are no longer alive and the
    // entity named `name` (null entries stay); updates nonEmpty.
    void Purge(uint32_t name);
    void Reset();
};

} // namespace entity
} // namespace sly
