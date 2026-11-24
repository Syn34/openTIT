// openTIT - Ape engine world object base class and shared constants.
#include "ape/world/ApeBase.h"

namespace ape::world {

const Vec3 kWorldUnitScale = {1.0f, 1.0f, 1.0f, 0.0f};
const Vec3 kWorldZero = {0.0f, 0.0f, 0.0f, 0.0f};
const Sphere kWorldEmptySphere = {0.0f, 0.0f, 0.0f, 0.0f};
const Matrix34 kWorldIdentity = {
    {1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
};

uint32_t g_nextObjectUniqueId = 0;

// The base constructor is inlined into every world object constructor on the
// console: reference count 1, type, next unique id, no tags.
ApeBase::ApeBase(uint32_t type)
    : m_type(uint8_t(type & 0x3F)), m_uniqueId(++g_nextObjectUniqueId)
{
}

// NOTE: the console destructor (0x00558A48, outside this module's range) also
// runs a global hook when m_tagFlags is set and releases the tag block with
// the parameter allocator (0x0056C730). Tag blocks are not ported yet, so
// there is nothing to release here.
ApeBase::~ApeBase() = default;

void ApeBase::Release() {}

// Inlined share of the tag block reference (0x00558E24 on the console). The
// reference count of the block is not maintained until tag blocks exist.
void ApeBase::ShareTags(const ApeBase& other)
{
    m_tags = other.m_tags;
}

} // namespace ape::world
