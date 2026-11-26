// openTIT - Upgrade shop tuning and the table of upgrade records.
#pragma once

#include "ape/tuning/ApeTuning.h"

#include <cstdint>
#include <vector>

namespace ape::xml { class XmlElement; }
namespace ape { class ApeAnimScriptVar; }

namespace sly {

extern ape::TuningInt g_upgradePurchaseFirstAwardNum;  // 0x010BE820
extern ape::TuningInt g_upgradePurchaseAllAwardNum;    // 0x010BE870

class WorldObject;

// Kinds of script variable bindings (the XML element name of a binding).
enum ScriptBindingType : uint8_t {
    kBinding_None         = 0,
    kBinding_Var          = 1,    // "var": copy another script's variable
    kBinding_Entity       = 2,    // "entity"
    kBinding_Trigger      = 3,    // "trigger"
    kBinding_Spawner      = 4,    // "Spawner"
    kBinding_SpawnerGroup = 5,    // "SpawnerGroup"
    kBinding_Laser        = 6,    // "laser"
    kBinding_LaserField   = 7,    // "LaserField"
    kBinding_LaserGroup   = 8,    // "laserGroup"
    kBinding_Hash         = 10,   // "hash"
    kBinding_String       = 11,   // "string"
    kBinding_Float        = 12,   // "float"
    kBinding_UInt         = 13,   // "uint"
    kBinding_Bool         = 14,   // "bool"
    kBinding_Vec3         = 15,   // "vec3"
    kBinding_Matrix       = 16,   // "matrix"
};

// A value bound to an anim script variable (0x4C bytes on the console): the
// value, the variable's name hash (`id`) and the binding type (`flagA`,
// zero when the record holds nothing). The jobs also keep the texts of
// their scripts in these records (flagB marks a stored copy).
// NOTE: the console keeps an entity binding's object pointer in the first
// value word; here it has its own member next to the entity name.
struct TextField {
    union {
        char text[0x44];          // +0x00 string bindings
        uint32_t hash;            // +0x00 var / spawner / laser / hash bindings
        uint32_t uintValue;       // +0x00
        float floatValue;         // +0x00
        bool boolValue;           // +0x00
        float vec3[3];            // +0x00
        float matrix[16];         // +0x00 four rows of four
        struct {
            WorldObject* object;  // +0x00 resolved entity (null: look the name up)
            uint32_t name;        // +0x04 entity / trigger name hash
        } entity;
    };
    uint32_t id = 0;              // +0x44 variable name hash
    uint8_t flagA = 0;            // +0x48 ScriptBindingType
    bool flagB = false;           // +0x49

    TextField() : text{} {}
    void Clear();
    void Reset(uint32_t newId);
};

// Binding type named by an element name hash (0 when unknown).
ScriptBindingType ScriptBindingTypeFromName(uint32_t nameHash);
// Reads a binding element (<float name="..." value="..."/> etc.) into
// `binding`; false for unknown elements, a missing name or a missing value.
bool ParseScriptBinding(ape::xml::XmlElement* element, TextField* binding);

// One upgrade record (0x98 bytes), an <Upgrade> element of the upgrade
// file. NOTE: the attribute names other than "id", "type" and "cost" are
// unknown (hashes in Upgrades.cpp).
struct UpgradeEntry {
    uint32_t id = 0;            // +0x00 "id"
    uint32_t item = 0;          // +0x04 what it gives (an ability name for characters)
    uint32_t type = 0;          // +0x08 "type": the character / kind it belongs to
    int32_t cost = 0;           // +0x0C "cost" in coins
    char name[0x40] = {};       // +0x10 display name
    char desc[0x40] = {};       // +0x50 description
    uint32_t unlockJob = 0;     // +0x90 job whose completion unlocks it
    bool notAbility = false;    // +0x94 not listed with the bought abilities
};

constexpr uint32_t kUpgradeCount = 0x54;

// Bits of PlayerProgress::upgradeStates.
enum UpgradeState : uint8_t {
    kUpgrade_Unlocked = 0x01,
    kUpgrade_Bought   = 0x02,
    kUpgrade_New      = 0x04,   // unlocked and not looked at yet
};

// Reads the upgrade table ("Upgrades" root, "Upgrade" elements; at most
// 0x53) and clears the bought/unlocked bits of each record. False if the
// file cannot be loaded.
bool LoadUpgrades(const char* path);
// The upgrades a completed job unlocks become unlocked and new (the
// profile's notification bit 7 is raised).
void UnlockUpgradesOfJob(uint32_t jobId);
// Buys upgrade `id` with the profile's coins: it must be unlocked (outside
// episodes any may be bought), affordable and not bought yet. The first and
// the last purchase award trophies.
bool BuyUpgrade(uint32_t id);
// Whether upgrade `id` could be bought now (not counting a repeat buy).
bool CanBuyUpgrade(uint32_t id);
// Appends the index of every unlocked upgrade with an id to `out`.
void CollectUnlockedUpgrades(std::vector<uint32_t>& out);
// Appends the item of every bought, unlocked ability upgrade of `type`.
void CollectBoughtItems(uint32_t type, std::vector<uint32_t>& out);
// Some upgrade is new.
bool AnyNewUpgrade();
// Upgrade `item` of `type` is bought and unlocked; outside episodes every
// upgrade counts.
bool UpgradeOwned(uint32_t type, uint32_t item);

// Stores the current value of a script variable in a binding record.
void CopyScriptVarToBinding(TextField& out, const ape::ApeAnimScriptVar* var);
// qsort comparator of string pointers (case-insensitive).
int CompareStringPointers(const void* a, const void* b);

// Upgrade record by index, null when out of range.
UpgradeEntry* GetUpgradeEntry(uint32_t index);

// Appends a binding of an entity / an unsigned value to `list` (nothing
// when the variable name or the value is zero).
void AddEntityBinding(std::vector<TextField>& list, uint32_t varName, WorldObject* entity);
void AddUIntBinding(std::vector<TextField>& list, uint32_t varName, uint32_t value);

} // namespace sly
