// openTIT - Anim sets and anim clips (0x004B0850-0x004B6760).
//
// An *anim set* is a named collection of *anim clips* read from an "AnimSet" XML
// element: blend rules between animations ("AnimBlendInfoData"), blend rules between
// clip classes ("AnimBlendClassInfoData") and the clips themselves ("AnimClipData").
// A set may inherit from an earlier set; lookups fall back to the parent.
//
// A clip describes what to play when a character asks for it by name. Five kinds
// exist: Simple (one animation), Random (weighted pick), Selection (picked by
// comparing a control value), Container (weighted pick of other clips) and Blend
// (two animations weighted by a control value). Every clip, its elements and its
// events live in the set's clip pool, a fixed array of 0x4C-byte tagged entries.
//
// Resolving a clip fills an AnimPlayDesc: the animation(s) to start, their blend-in
// time and type (from the blend rules against the previous animation) and an
// optional transition clip.
//
// The manager (0x00F65F84) owns the sets, the shared string pool, the type tables
// the game passes in, and a handle list of "set slots" that characters acquire.
#pragma once

#include "ape/container/ApeTFixedArray.h"
#include "ape/container/ApeTHandleList.h"
#include "ape/container/ApeTSortedMap.h"
#include "ape/sys/ApeStringPool.h"

#include <cstdint>
#include <vector>

namespace ape {
class ApeDTD;
namespace xml {
class XmlDocument;
class XmlElement;
}
namespace iff {
struct IffReader;
}
}

namespace ape::anim {

class AnimSet;
class AnimClipPool;

// Clip flags (AnimClipDef::flags).
enum AnimClipFlags : uint8_t {
    kClip_Flag01           = 0x01,   // NOTE: kept by the initialisers, never set here
    kClip_Flag02           = 0x02,   // NOTE: kept by the initialisers, never set here
    kClip_Flag04           = 0x04,   // attribute 0x57BAF55F; copied to AnimPlayAnim::flag2A
    kClip_ScaleExtMotion   = 0x08,
    kClip_ControlDriven    = 0x10,   // selection and blend clips
    kClip_ForceSyncAnimPos = 0x20,
    kClip_Loop             = 0x40,
    kClip_Invalid          = 0x80,
};

// Blend curve names of the "BlendType" attribute.
enum AnimBlendType : uint8_t {
    kBlend_Smooth = 0,
    kBlend_Linear = 1,
    kBlend_Slow   = 2,
};

// Common part of every clip (0x3C bytes on the console) plus the fields of the
// individual kinds.
struct AnimClipDef {
    int16_t typeIndex;         // +0x00 index in the clip type table
    const char* name;          // +0x04 (string pool)
    uint32_t nameHash;         // +0x08
    uint32_t classHash;        // +0x0C "Class"
    uint32_t channelMapHash;   // +0x10 "ChannelMap"
    uint8_t flags;             // +0x14 AnimClipFlags
    uint8_t blendType;         // +0x15 AnimBlendType
    float blendIn;             // +0x18 "BlendIn" (0.25)
    float speed;               // +0x1C "Speed" (1.0)
    float startTime;           // +0x20 "StartTime"
    const char* transitionName;// +0x24 "TranAnimClipName"
    uint32_t transitionHash;   // +0x28
    const char* playNode;      // +0x2C "PlayNode"
    uint32_t playNodeHash;     // +0x30
    int16_t firstEvent;        // +0x34 pool index of the first event, -1 for none
    uint16_t eventCount;       // +0x36
    int16_t controlType;       // +0x38 index in the anim control type table ("Type")
    int16_t syncType;          // +0x3A index in the sync type table ("SyncType")
    // Kind specific.
    int16_t animEntry;         // Simple: pool index of the animation (+0x3C)
    uint32_t elementCount;     // Random/Selection/Container/Blend (+0x3C)
    float totalWeight;         // Random/Container (+0x40)
    int16_t firstElement;      // pool index of the first element (+0x40 or +0x44)
    bool adjustAnimSpeed;      // Blend "AdjustAnimSpeed" (+0x42)
};

// An animation reference of a simple, random, selection or blend clip, or a
// clip reference of a container clip.
struct AnimClipElement {
    bool disabled;             // +0x00 random elements without a weight
    const char* animName;      // +0x04 animation (container: clip) name
    uint32_t animHash;         // +0x08
    const char* mapName;       // +0x0C "Map"
    uint32_t mapHash;          // +0x10
    float value;               // +0x14 random/container weight, selection value, blend position
    uint32_t param;            // +0x18 selection operator hash, blend attribute 0x044DDD31
    bool loop;                 // +0x1C selection "Loop"
};

// An event of a clip (0x2C bytes).
struct AnimClipEvent {
    enum Trigger : int32_t {
        kTrigger_Time = 0, kTrigger_Percentage = 1, kTrigger_Int = 2, kTrigger_Float = 3, kTrigger_Hash = 4,
        kTrigger_Bool = 5,
    };
    uint8_t field_00;          // +0x00
    float time;                // +0x04 attribute "AnimEvent"
    float percentage;          // +0x08 "Percentage", 0-100
    int32_t intValue;          // +0x0C attribute 0x1EAA8FE1
    uint32_t hashValue;        // +0x10 attribute 0xF7B7EBB0
    float floatValue;          // +0x14 attribute 0x27819325
    bool field_18;             // +0x18 attribute 0x0D17F9B7
    bool field_19;             // +0x19 always 1
    int32_t trigger;           // +0x1C the last trigger attribute present
    uint32_t animHash;         // +0x20 "AnimName"
    bool boolTrigger;          // +0x24 attribute 0x8B3AB37D
    int32_t userData;          // +0x28 from the manager's event callback, -1 without one
};

// Kinds of the clip pool entries.
enum AnimClipEntryKind : int32_t {
    kEntry_Free = 1, kEntry_Event = 2, kEntry_SimpleClip = 3, kEntry_SimpleAnim = 4, kEntry_RandomClip = 5,
    kEntry_RandomAnim = 6, kEntry_SelectionClip = 7, kEntry_SelectionAnim = 8, kEntry_BlendClip = 9,
    kEntry_BlendAnim = 10, kEntry_ContainerClip = 11, kEntry_ContainerRef = 12,
};

struct AnimClipEntry {
    int32_t kind;
    union {
        AnimClipDef clip;
        AnimClipElement element;
        AnimClipEvent event;
    };
};

// Fixed-size pool of tagged entries (console helpers 0x00789280...).
class AnimClipPool {
public:
    AnimClipPool(uint16_t capacity, AnimSet* owner);
    // Takes the next entry; -1 when the pool is full.
    int16_t Alloc(int32_t kind);
    // Entry `index`, or null outside the allocated range.
    AnimClipEntry* At(int index);
    AnimSet* Owner() const { return m_owner; }

private:
    ApeTFixedArray<AnimClipEntry> m_entries;   // +0x00 (default allocator, no index table)
    uint16_t m_count = 0;
    AnimSet* m_owner;          // +0x10
};

// One animation of a play request (0x28 bytes).
struct AnimPlayAnim {
    const char* animName;      // +0x00
    uint32_t animHash;
    const char* mapName;
    uint32_t mapHash;
    float blendIn;             // +0x10
    uint8_t blendType;         // +0x14
    bool loop;                 // +0x15
    bool flag2A;               // +0x16 clip flag 0x04
    float speed;               // +0x18
    float field_1C;
    float startTime;           // +0x20
    float weight;              // +0x24
};

// Result of resolving a clip.
struct AnimPlayDesc {
    enum Flags : uint8_t {
        kDesc_HasTransition  = 0x08,   // the blend rule names a transition clip
        kDesc_ClipTransition = 0x10,   // the clip names a transition clip
        kDesc_AdjustSpeed    = 0x20,   // blend clip "AdjustAnimSpeed"
        kDesc_BlendRule      = 0x40,   // blend-in came from a blend rule
        kDesc_Failed         = 0x80,   // clip not found
    };
    int32_t animCount;               // +0x00 1 or 2
    uint8_t flags;                   // +0x04
    const AnimClipDef* clip;         // +0x08
    const char* transitionName;      // +0x0C
    uint32_t transitionHash;         // +0x10
    AnimPlayAnim anims[2];           // +0x14, +0x3C
    uint32_t blendParam;             // +0x64 second element's attribute 0x044DDD31
};

// What is playing now, for the blend rules and the selection/blend clips.
struct AnimPlayContext {
    const char* prevAnimName;        // +0x00 null when nothing plays
    const AnimClipDef* prevClip;     // +0x04
    float controlValue;              // +0x08
};

// Blend rule between two animations (value of AnimSet::blendInfo).
struct AnimBlendInfo {
    float time;                      // -FLT_MAX when absent
    uint8_t blendType;
    uint32_t transitionHash;
    const char* transitionName;
};

// Blend rule between two clip classes (value of AnimSet::blendClassInfo).
struct AnimBlendClassInfo {
    uint32_t fromClass;
    uint32_t toClass;
    float time;                      // -FLT_MAX when absent
    uint8_t blendType;
    uint32_t transitionHash;
    const char* transitionName;
};

class AnimSet {
public:
    AnimSet() = default;
    ~AnimSet();
    AnimSet(const AnimSet&) = delete;
    AnimSet& operator=(const AnimSet&) = delete;

    // Reads an "AnimSet" element (null elements only record the file name).
    void Load(const char* fileName, xml::XmlElement* element);

    // Blend rule for switching from one animation to another; tries the pair,
    // then the wildcard rule of the target, then the parent set.
    bool FindBlendInfo(const char* fromAnim, const char* toAnim, AnimBlendInfo* out) const;
    // Blend rule between two clip classes, falling back to the parent set.
    bool FindBlendClassInfo(uint32_t fromClass, uint32_t toClass, AnimBlendClassInfo* out) const;

    // Fills `desc` for the clip named `clipHash` (searching the parents).
    void ResolveClip(uint32_t clipHash, AnimPlayDesc& desc, const AnimPlayContext& ctx, int index);
    // Fills `desc` for a known clip; a parent's clip gets this set's channel
    // map override.
    void PlayClip(const AnimClipDef* clip, AnimPlayDesc& desc, const AnimPlayContext& ctx, int index);

    AnimClipPool* Pool() const { return pool; }

    AnimSet* parent = nullptr;                               // +0x00 "Inherit"
    uint32_t channelMapOverride = 0;                         // +0x04 attribute 0xA80F1E14
    ApeTSortedMap<AnimClipDef*>* clips = nullptr;            // +0x08 by name hash
    ApeTSortedMap<AnimBlendInfo>* blendInfo = nullptr;       // +0x0C
    ApeTSortedMap<AnimBlendClassInfo>* blendClassInfo = nullptr; // +0x10
    const char* name = nullptr;                              // +0x14
    uint32_t nameHash = 0;                                   // +0x18
    const char* fileName = nullptr;                          // +0x1C
    uint32_t fileHash = 0;                                   // +0x20
    AnimClipPool* pool = nullptr;                            // +0x24
    uint32_t field_28 = 0;                                   // +0x28 passed to the slot callback
    int32_t clipCountHint = 0;                               // +0x2C attribute 0xE98F593E (64)
};

// Entry of the anim control type and sync type tables the game passes in.
struct AnimNamedType {
    const char* name;
    uint32_t hash;
};

// A set slot a character holds (8 bytes).
struct AnimSetSlot {
    AnimSet* set;
    uint32_t userData;   // from the acquire callback; the low half is cleared on release
};

// Request passed to AcquireSlot.
struct AnimSetSlotRequest {
    AnimSet* set;
    uint32_t user;
};

using AnimEventUserDataFn = int32_t (*)(xml::XmlElement* element, AnimSet* set, AnimClipDef* clip);
using AnimSlotAcquireFn = void (*)(uint32_t* outUserData, uint32_t setData, uint32_t user);
using AnimSlotReleaseFn = void (*)(uint32_t userData);

// Description of one clip kind (the creation table, 0x00F65F88).
struct AnimClipType {
    uint32_t hash;
    const char* name;
    AnimClipDef* (*create)(xml::XmlElement* element, AnimClipPool* pool, AnimSet* set);
    void (*initDTD)(ApeDTD* dtd);
    void (*fill)(const AnimClipDef* clip, AnimClipPool* pool, AnimPlayDesc& desc, const AnimPlayContext& ctx,
                 int index);
};
extern const AnimClipType g_animClipTypes[5];

class AnimClipManager {
public:
    AnimClipManager(int slotCapacity, const AnimNamedType* controlTypes, uint32_t controlTypeCount,
                    const AnimNamedType* syncTypes, uint32_t syncTypeCount, AnimEventUserDataFn eventUserData,
                    void* field11C, AnimSlotAcquireFn acquire, AnimSlotReleaseFn release);
    ~AnimClipManager();
    AnimClipManager(const AnimClipManager&) = delete;
    AnimClipManager& operator=(const AnimClipManager&) = delete;

    // Releases every slot, deletes every set and forgets the file list.
    void Clear();
    // Releases every slot (the sets stay).
    void ReleaseAllSlots();
    // NOTE: empty on the console; called when a level is reset.
    void OnLevelReset();
    // Loads the files of the file list.
    void LoadAnimSetFiles();
    // Reads the "AnimSet" elements of a document; sets already known are skipped.
    bool LoadAnimSetFile(xml::XmlDocument& document, const char* path);

    AnimSetSlot* AcquireSlot(const AnimSetSlotRequest& request, uint32_t* outHandle);
    void ReleaseSlot(uint32_t handle);

    AnimSet* FindSet(uint32_t nameHash) { return *sets.Find(nameHash); }
    const AnimClipType* FindClipType(uint32_t hash) { return *clipTypes.Find(hash); }
    int16_t FindControlType(uint32_t hash) const;
    int16_t FindSyncType(uint32_t hash) const;

    ApeStringPool strings;                          // +0x000
    ApeTSortedMap<const AnimClipType*> clipTypes;   // +0x018 by type name hash
    ApeTSortedMap<void*> field_30;                  // +0x030 NOTE: never filled here
    ApeTSortedMap<AnimSet*> sets;                   // +0x048 by name hash
    const char* files[32] = {};                     // +0x060
    uint32_t fileCount = 0;                         // +0x0E0
    ApeTHandleList<AnimSetSlot> slots;              // +0x0E4
    const AnimNamedType* controlTypes;              // +0x108
    uint32_t controlTypeCount;                      // +0x10C
    const AnimNamedType* syncTypes;                 // +0x110
    uint32_t syncTypeCount;                         // +0x114
    AnimEventUserDataFn eventUserData;              // +0x118
    void* field_11C;                                // +0x11C NOTE: not used here
    AnimSlotAcquireFn acquireSlot;                  // +0x120
    AnimSlotReleaseFn releaseSlot;                  // +0x124
};

extern AnimClipManager* g_animClipManager;   // 0x00F65F84

// Event of a clip by index, or null.
AnimClipEvent* GetClipEvent(const AnimClipDef* clip, int index, AnimClipPool* pool);

// Schema of the attributes all clips share.
void InitClipDTD(ApeDTD* dtd);

// IFF handlers of the 'ACLF/ACLH/ACLD' chunks (anim clip files).
int AnimClipFileChunk(iff::IffReader* reader, void* arg);
int AnimClipFileHeaderChunk(iff::IffReader* reader, void* arg);
int AnimClipFileDataChunk(iff::IffReader* reader, void* arg);

} // namespace ape::anim
