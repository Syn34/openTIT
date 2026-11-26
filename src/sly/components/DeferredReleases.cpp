// openTIT - Deferred entity releases (see DeferredReleases.h).
#include "sly/components/DeferredReleases.h"

#include "sly/entity/Entity.h"

namespace sly {
namespace components {

namespace {
DeferredRelease g_records[kMaxDeferredReleases];   // 0x010573A8
uint8_t g_count = 0;                                // 0x01057528
} // namespace

DeferredRelease* DeferredReleases() { return g_records; }
uint8_t& DeferredReleaseCount() { return g_count; }

// @func 0x0026F8D4 ResetDeferredReleases
void ResetDeferredReleases()
{
    for (DeferredRelease& record : g_records)
        record = DeferredRelease{};
    g_count = 0;
}

// @func 0x0026F798 FlushDeferredReleases
void FlushDeferredReleases()
{
    // The table is walked up to the first empty record; a released record
    // takes the last one's place (which is then checked on the next update).
    for (int i = 0; i < kMaxDeferredReleases && g_records[i].entity; ++i) {
        DeferredRelease& record = g_records[i];
        if (!record.due) {
            record.due = true;
            continue;
        }
        record.entity->Release(false);
        record.entity = nullptr;
        --g_count;
        DeferredRelease& last = g_records[g_count];
        record = last;
        last.entity = nullptr;
        last.due = true;
    }
}

} // namespace components
} // namespace sly
