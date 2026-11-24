// openTIT - Ape engine world object base class and shared constants.
#include "ape/world/ApeBase.h"

#include "ape/world/ApeTags.h"

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

void (*g_tagReleaseHook)(ApeBase* object) = nullptr;

// @func 0x00558A48 ApeBase::~ApeBase
// @func 0x00558B84 ApeBase::~ApeBase (deleting)
// @func 0x00558CE8 ApeBase::~ApeBase (copy called by derived destructors)
ApeBase::~ApeBase()
{
    if (m_tagFlags != 0 && g_tagReleaseHook)
        g_tagReleaseHook(this);
    if (m_tags) {
        m_tags->Release();
        m_tags = nullptr;
    }
}

// @func 0x00558F34 ApeBase::Release
void ApeBase::Release() {}

// @func 0x00558E24 ApeBase::ShareTags
void ApeBase::ShareTags(const ApeBase& other)
{
    AssignTagBlock(m_tags, other.m_tags);
}

void ApeBase::SetTags(ApeTagBlock* tags)
{
    AssignTagBlock(m_tags, tags);
}

} // namespace ape::world
