// openTIT - A list of entity references that drops dead entities (see EntityRefList.h).
#include "sly/entity/EntityRefList.h"

#include "sly/entity/Entity.h"

namespace sly {
namespace entity {

// @func 0x000B4524 EntityRefList::Purge
void EntityRefList::Purge(uint32_t name)
{
    for (size_t i = 0; i < entries.Count();) {
        Entity* e = entries[i];
        if (e && (!e->IsAlive() || e->nameHash == name))
            entries.RemoveAtSwap(i);   // re-examine the entry moved into slot i
        else
            ++i;
    }
    nonEmpty = entries.Count() != 0;
}

// @func 0x000B566C EntityRefList::Reset
void EntityRefList::Reset()
{
    field_E4 = nullptr;
    nonEmpty = false;
}

} // namespace entity
} // namespace sly
