// openTIT - Anim sets and anim clips (0x004B0850-0x004B6760).
#include "ape/anim/ApeAnimClip.h"

#include "ape/core/ApeString.h"
#include "ape/file/ApeFile.h"
#include "ape/level/ApeDTD.h"
#include "ape/level/ApeXmlForEach.h"
#include "ape/math/ApeMath.h"
#include "ape/memory/ApeMemStack.h"
#include "ape/sys/ApeIff.h"
#include "ape/xml/ApeXml.h"
#include "platform/Platform.h"

#include <cfloat>
#include <cstring>

namespace ape::anim {

using xml::XmlDocument;
using xml::XmlElement;

AnimClipManager* g_animClipManager = nullptr;

namespace {

// Compile-time HashNameNoCase().
constexpr uint32_t NameHash(const char* name)
{
    uint32_t hash = 0;
    for (; *name; ++name) {
        int c = (signed char)*name;
        if (c >= 'a' && c <= 'z')
            c -= 'a' - 'A';
        hash = hash * 31u + (uint32_t)c;
    }
    return hash;
}

// Attribute name hashes. The ones without a name are only known by hash.
constexpr uint32_t kAttr_Name = NameHash("Name");
constexpr uint32_t kAttr_Inherit = NameHash("Inherit");
constexpr uint32_t kAttr_Class = NameHash("Class");
constexpr uint32_t kAttr_ChannelMap = NameHash("ChannelMap");
constexpr uint32_t kAttr_Loop = NameHash("Loop");
constexpr uint32_t kAttr_ForceSyncAnimPos = NameHash("ForceSyncAnimPos");
constexpr uint32_t kAttr_BlendIn = NameHash("BlendIn");
constexpr uint32_t kAttr_Speed = NameHash("Speed");
constexpr uint32_t kAttr_PlayNode = NameHash("PlayNode");
constexpr uint32_t kAttr_SyncType = NameHash("SyncType");
constexpr uint32_t kAttr_StartTime = NameHash("StartTime");
constexpr uint32_t kAttr_BlendType = NameHash("BlendType");
constexpr uint32_t kAttr_TranAnimClipName = NameHash("TranAnimClipName");
constexpr uint32_t kAttr_From = NameHash("From");
constexpr uint32_t kAttr_To = NameHash("To");
constexpr uint32_t kAttr_AnimName = NameHash("AnimName");
constexpr uint32_t kAttr_AnimName1 = NameHash("AnimName1");
constexpr uint32_t kAttr_AnimName2 = NameHash("AnimName2");
constexpr uint32_t kAttr_BlendValue = NameHash("BlendValue");
constexpr uint32_t kAttr_Map = NameHash("Map");
constexpr uint32_t kAttr_Value = NameHash("Value");
constexpr uint32_t kAttr_Operator = NameHash("Operator");
constexpr uint32_t kAttr_Type = NameHash("Type");
constexpr uint32_t kAttr_Weight = NameHash("Weight");
constexpr uint32_t kAttr_ClipName = NameHash("ClipName");
constexpr uint32_t kAttr_AdjustAnimSpeed = NameHash("AdjustAnimSpeed");
constexpr uint32_t kAttr_Percentage = NameHash("Percentage");
constexpr uint32_t kAttr_Size = NameHash("Size");
constexpr uint32_t kAttr_ClipFlag04 = 0x57BAF55F;          // NOTE: name unknown
constexpr uint32_t kAttr_TransitionClip = 0x91EEB2F8;      // NOTE: transition clip of a blend rule
constexpr uint32_t kAttr_SetChannelMap = 0xA80F1E14;       // NOTE: channel map override of a set
constexpr uint32_t kAttr_ClipCountHint = 0xE98F593E;       // NOTE: name unknown
constexpr uint32_t kAttr_EventInt = 0x1EAA8FE1;            // NOTE: name unknown
constexpr uint32_t kAttr_EventFloat = 0x27819325;          // NOTE: name unknown
constexpr uint32_t kAttr_EventHash = 0xF7B7EBB0;           // NOTE: name unknown
constexpr uint32_t kAttr_EventFlag18 = 0x0D17F9B7;         // NOTE: name unknown
constexpr uint32_t kAttr_EventBool = 0x8B3AB37D;           // NOTE: name unknown
constexpr uint32_t kAttr_BlendParam = 0x044DDD31;          // NOTE: name unknown

constexpr uint32_t kBlendLinear = NameHash("Linear");
constexpr uint32_t kBlendSlow = NameHash("Slow");
constexpr uint32_t kAnimNone = NameHash("None");

// Comparison operators of selection elements: the control value against the
// element's value. NOTE: only "LessThan" and "NotEqual" are known by name.
constexpr uint32_t kOp_ControlLessEqual = 0x34212D3C;     // default
constexpr uint32_t kOp_ControlGreaterEqual = 0xC83F23FE;
constexpr uint32_t kOp_LessThan = NameHash("LessThan");
constexpr uint32_t kOp_ControlGreater = 0x9791C2B8;
constexpr uint32_t kOp_NotEqual = NameHash("NotEqual");
static_assert(kOp_LessThan == 0xAB426ABA && kOp_NotEqual == 0x8CF17841, "operator hashes");

AnimClipElement* EntryElement(AnimClipEntry* entry) { return entry ? &entry->element : nullptr; }

ApeStringPool& Strings() { return g_animClipManager->strings; }

uint8_t ParseBlendType(XmlElement* element)
{
    uint32_t type = 0;
    element->GetHash(kAttr_BlendType, &type, 0);
    if (type == kBlendLinear)
        return kBlend_Linear;
    if (type == kBlendSlow)
        return kBlend_Slow;
    return kBlend_Smooth;
}

// Pools a string and returns it with its hash (both zero for null).
const char* PoolString(const char* text, uint32_t* hash)
{
    const char* pooled = text ? Strings().Add(text) : nullptr;
    *hash = pooled ? HashNameNoCase(pooled) : 0;
    return pooled;
}

// Index of `name` in the clip type table, -1 (with a warning) if absent.
int16_t FindClipTypeIndex(const char* name)
{
    const uint32_t hash = HashNameNoCase(name);
    for (int16_t i = 0; i < 5; ++i) {
        if (g_animClipTypes[i].hash == hash)
            return i;
    }
    plat::Log(plat::LogLevel::Warning, "Fail to find the anim clip def (%s) from from the creation map!\n", name);
    return -1;
}

// The part of every clip initialiser that is the same for all kinds.
void InitClipCommon(AnimClipDef& clip, const char* typeName)
{
    clip.typeIndex = FindClipTypeIndex(typeName);
    clip.name = nullptr;
    clip.nameHash = 0;
    clip.classHash = 0;
    clip.channelMapHash = 0;
    clip.blendIn = 0.25f;
    clip.speed = 1.0f;
    clip.eventCount = 0;
    clip.playNode = nullptr;
    clip.playNodeHash = 0;
    clip.flags &= kClip_Flag01 | kClip_Flag02 | kClip_ScaleExtMotion;
    clip.transitionName = nullptr;
    clip.transitionHash = 0;
    clip.startTime = 0.0f;
    clip.blendType = kBlend_Smooth;
    clip.firstEvent = -1;
    clip.controlType = -1;
    clip.syncType = -1;
}

// Allocates an entry and returns it, or null when the pool is full.
AnimClipEntry* AllocEntry(AnimClipPool* pool, int32_t kind, int16_t* outIndex = nullptr)
{
    const int16_t index = pool->Alloc(kind);
    if (outIndex)
        *outIndex = index;
    return pool->At(index);
}

// Reads the "AnimName"/"Map" pair of an animation element.
void ReadAnimReference(AnimClipElement& element, XmlElement* xml, const char* animAttr, const char* mapAttr,
                       const char* nullMessage)
{
    const char* animName = xml->GetString(animAttr);
    element.animName = animName;
    if (!animName) {
        plat::Log(plat::LogLevel::Warning, nullMessage, xml->GetLocationString());
        return;
    }
    element.animName = Strings().Add(animName);
    element.animHash = HashNameNoCase(element.animName);
    const char* mapName = xml->GetString(mapAttr);
    element.mapName = mapName;
    if (mapName) {
        element.mapName = Strings().Add(mapName);
        element.mapHash = HashNameNoCase(element.mapName);
    }
}

// Same, with the attributes given by hash.
void ReadAnimReferenceByHash(AnimClipElement& element, XmlElement* xml, const char* nullMessage)
{
    const char* animName = xml->GetString(kAttr_AnimName);
    element.animName = animName;
    if (!animName) {
        plat::Log(plat::LogLevel::Warning, nullMessage, xml->GetLocationString());
        return;
    }
    element.animName = Strings().Add(animName);
    element.animHash = HashNameNoCase(element.animName);
    const char* mapName = xml->GetString(kAttr_Map);
    element.mapName = mapName;
    if (mapName) {
        element.mapName = Strings().Add(mapName);
        element.mapHash = HashNameNoCase(element.mapName);
    }
}

void ZeroElement(AnimClipElement& element)
{
    element.disabled = false;
    element.animName = nullptr;
    element.animHash = 0;
    element.mapName = nullptr;
    element.mapHash = 0;
    element.value = 0.0f;
}

// Copies the animation fields of an element into a play slot.
void CopyAnim(AnimPlayAnim& anim, const AnimClipElement* element)
{
    anim.animName = element ? element->animName : nullptr;
    anim.animHash = element ? element->animHash : 0;
    anim.mapName = element ? element->mapName : nullptr;
    anim.mapHash = element ? element->mapHash : 0;
}

void AddEnumNames(DTDAttribute* attribute, const AnimNamedType* types, uint32_t count, uint32_t boundCount)
{
    attribute->AllocEnumNames(count);
    for (uint32_t i = 0; i < count; ++i) {
        // NOTE: the sync type list is bounded by the *control type* count on
        // the console; an index outside it would dereference null there.
        if ((int16_t)i < (int32_t)boundCount)
            attribute->AddEnumName(types[i].name);
    }
}

// Name of the anim clip file being parsed (0x00F65FEC), set by the header chunk.
const char* g_animClipFileName = nullptr;
char g_animClipFileNameBuffer[0x41];   // 0x010C2708

} // namespace

// ---------------------------------------------------------------------------
// AnimClipPool
// ---------------------------------------------------------------------------

AnimClipPool::AnimClipPool(uint16_t capacity, AnimSet* owner)
    : m_entries(capacity), m_owner(owner)
{
    for (AnimClipEntry& entry : m_entries)
        entry.kind = kEntry_Free;
}

int16_t AnimClipPool::Alloc(int32_t kind)
{
    if (m_count >= m_entries.size())
        return -1;
    const int16_t index = (int16_t)m_count++;
    m_entries[index].kind = kind;
    return index;
}

AnimClipEntry* AnimClipPool::At(int index)
{
    if (index < 0 || index >= m_count)
        return nullptr;
    return &m_entries[index];
}

// ---------------------------------------------------------------------------
// Clip pieces shared by all kinds
// ---------------------------------------------------------------------------

// @func 0x004B3624 GetClipEvent
AnimClipEvent* GetClipEvent(const AnimClipDef* clip, int index, AnimClipPool* pool)
{
    if (!pool)
        return nullptr;
    AnimClipEntry* entry = pool->At((int16_t)(clip->firstEvent + index));
    return entry ? &entry->event : nullptr;
}

namespace {

// @func 0x004B3870 AllocClipEvent
AnimClipEvent* AllocClipEvent(AnimClipDef& clip, AnimClipPool* pool)
{
    int16_t index = -1;
    AnimClipEntry* entry = AllocEntry(pool, kEntry_Event, &index);
    if (!entry)
        return nullptr;
    if (clip.eventCount == 0)
        clip.firstEvent = index;
    ++clip.eventCount;
    return &entry->event;
}

// @func 0x004B3940 ReadClipEvent
void ReadClipEvent(AnimClipEvent& event, XmlElement* element, AnimSet* set, AnimClipDef* clip)
{
    if (element->GetFloat("AnimEvent", &event.time, 0.0f) == xml::kQuery_Ok)
        event.trigger = AnimClipEvent::kTrigger_Time;
    if (element->GetFloat(kAttr_Percentage, &event.percentage, 0.0f) == xml::kQuery_Ok) {
        if (event.percentage < 0.0f || event.percentage > 100.0f) {
            plat::Log(plat::LogLevel::Warning,
                      "The anim event percentage (%.2f) is not in range, it will set to 100 percentage! (%s)\n",
                      (double)event.percentage, element->GetLocationString());
            event.percentage = 100.0f;
        }
        event.trigger = AnimClipEvent::kTrigger_Percentage;
    }
    if (element->GetInt(kAttr_EventInt, &event.intValue, 0) == xml::kQuery_Ok)
        event.trigger = AnimClipEvent::kTrigger_Int;
    if (element->GetFloat(kAttr_EventFloat, &event.floatValue, 0.0f) == xml::kQuery_Ok)
        event.trigger = AnimClipEvent::kTrigger_Float;
    if (element->GetHash(kAttr_EventHash, &event.hashValue, 0) == xml::kQuery_Ok)
        event.trigger = AnimClipEvent::kTrigger_Hash;
    element->GetBool(kAttr_EventFlag18, &event.field_18, false);
    element->GetHash(kAttr_AnimName, &event.animHash, 0);
    element->GetBool(kAttr_EventBool, &event.boolTrigger, false);
    if (event.boolTrigger)
        event.trigger = AnimClipEvent::kTrigger_Bool;
    if (g_animClipManager->eventUserData)
        event.userData = g_animClipManager->eventUserData(element, set, clip);
}

struct ClipReadContext {
    AnimClipDef* clip;
    AnimClipPool* pool;
    AnimSet* set;
};

// @func 0x004B3BC8 ReadClipEventElement
bool ReadClipEventElement(XmlElement* element, void* userData)
{
    ClipReadContext* ctx = static_cast<ClipReadContext*>(userData);
    AnimClipEvent* event = AllocClipEvent(*ctx->clip, ctx->pool);
    if (!event)
        return false;
    event->time = 0.0f;
    event->percentage = 0.0f;
    event->intValue = 0;
    event->floatValue = 0.0f;
    event->field_18 = false;
    event->field_19 = true;
    event->trigger = AnimClipEvent::kTrigger_Time;
    event->field_00 = 0;
    event->userData = -1;
    event->animHash = 0;
    ReadClipEvent(*event, element, ctx->set, ctx->clip);
    return true;
}

// @func 0x004B31D8 ReadClipCommon
void ReadClipCommon(AnimClipDef& clip, XmlElement* element, AnimClipPool* pool, AnimSet* set)
{
    const char* name = element->GetString(kAttr_Name);
    clip.name = name;
    if (!name) {
        clip.flags |= kClip_Invalid;
        return;
    }
    clip.name = Strings().Add(name);
    if (!clip.name) {
        plat::Log(plat::LogLevel::Warning, "Fail to add the clip name (%s) to the anim clip string buffer!\n",
                  name);
        clip.flags |= kClip_Invalid;
        return;
    }
    clip.nameHash = HashNameNoCase(clip.name);
    element->GetHash(kAttr_Class, &clip.classHash, 0);
    element->GetHash(kAttr_ChannelMap, &clip.channelMapHash, 0);

    bool flag = false;
    element->GetBool(kAttr_Loop, &flag, false);
    clip.flags = (uint8_t)((clip.flags & ~kClip_Loop) | (flag ? kClip_Loop : 0));
    flag = false;
    element->GetBool(kAttr_ClipFlag04, &flag, false);
    clip.flags = (uint8_t)((clip.flags & ~kClip_Flag04) | (flag ? kClip_Flag04 : 0));
    flag = false;
    element->GetBool(kAttr_ForceSyncAnimPos, &flag, false);
    clip.flags = (uint8_t)((clip.flags & ~kClip_ForceSyncAnimPos) | (flag ? kClip_ForceSyncAnimPos : 0));
    element->GetFloat(kAttr_BlendIn, &clip.blendIn, 0.25f);
    flag = false;
    element->GetBool("ScaleExtMotion", &flag, false);
    clip.flags = (uint8_t)((clip.flags & ~kClip_ScaleExtMotion) | (flag ? kClip_ScaleExtMotion : 0));
    element->GetFloat(kAttr_Speed, &clip.speed, 1.0f);

    const char* playNode = element->GetString(kAttr_PlayNode);
    clip.playNode = playNode;
    if (playNode) {
        clip.playNode = Strings().Add(playNode);
        if (!clip.playNode) {
            plat::Log(plat::LogLevel::Warning,
                      "Run out anim clip string buffer for clip name (%s) for play node name (%s)\n", clip.name,
                      playNode);
            clip.flags |= kClip_Invalid;
            return;
        }
        clip.playNodeHash = HashNameNoCase(clip.playNode);
    }

    uint32_t syncHash = 0;
    element->GetHash(kAttr_SyncType, &syncHash, 0);
    clip.syncType = g_animClipManager->FindSyncType(syncHash);
    element->GetFloat(kAttr_StartTime, &clip.startTime, 0.0f);
    clip.blendType = ParseBlendType(element);

    const char* transition = element->GetString(kAttr_TranAnimClipName);
    clip.transitionName = transition;
    if (!transition) {
        clip.transitionHash = 0;
    } else {
        clip.transitionHash = HashNameNoCase(transition);
        clip.transitionName = Strings().Add(transition);
        if (!clip.transitionName) {
            plat::Log(plat::LogLevel::Warning, "Run out of string buffer for transition name %s\n", transition);
            clip.flags |= kClip_Invalid;
            return;
        }
    }

    ClipReadContext ctx = { &clip, pool, set };
    ForEachNamedChild(element, "AnimEvent", &ReadClipEventElement, &ctx, false);
}

// @func 0x004B36A0 ApplyClipBlend
// Blend-in of the first animation: from the clip when nothing plays, from the
// clip's own transition, from a blend rule against the previous animation or
// clip class, or from the clip's defaults.
void ApplyClipBlend(const AnimClipDef* clip, AnimClipPool* pool, AnimPlayDesc& desc, const AnimPlayContext& ctx)
{
    AnimPlayAnim& anim = desc.anims[0];
    if (!ctx.prevAnimName) {
        desc.flags &= (uint8_t)~AnimPlayDesc::kDesc_BlendRule;
        anim.blendIn = clip->blendIn;
        anim.blendType = clip->blendType;
        return;
    }
    if (clip->transitionHash != 0) {
        desc.transitionName = clip->transitionName;
        desc.transitionHash = clip->transitionHash;
        desc.flags |= AnimPlayDesc::kDesc_ClipTransition;
        return;
    }

    AnimSet* set = pool->Owner();
    AnimBlendInfo rule = { -FLT_MAX, 0, 0, nullptr };
    bool found = set->FindBlendInfo(ctx.prevAnimName, anim.animName, &rule);
    float blendIn = rule.time;
    uint8_t blendType = rule.blendType;
    if (!found) {
        AnimBlendClassInfo classRule = { 0, 0, -FLT_MAX, 0, 0, nullptr };
        if (ctx.prevClip)
            found = set->FindBlendClassInfo(ctx.prevClip->classHash, clip->classHash, &classRule);
        if (found) {
            blendIn = classRule.time;
            blendType = classRule.blendType;
            rule.transitionHash = classRule.transitionHash;
            rule.transitionName = classRule.transitionName;
        } else {
            blendIn = clip->blendIn;
            blendType = clip->blendType;
        }
    }
    desc.flags = (uint8_t)((desc.flags & ~AnimPlayDesc::kDesc_BlendRule) | (found ? AnimPlayDesc::kDesc_BlendRule : 0));
    anim.blendIn = blendIn;
    anim.blendType = blendType;
    desc.transitionName = rule.transitionName;
    desc.transitionHash = rule.transitionHash;
    desc.flags = (uint8_t)((desc.flags & ~AnimPlayDesc::kDesc_HasTransition) |
                           (rule.transitionHash ? AnimPlayDesc::kDesc_HasTransition : 0));
}

// Common play fields of the first animation.
void FillClipAnimFlags(AnimPlayAnim& anim, const AnimClipDef* clip)
{
    anim.loop = (clip->flags & kClip_Loop) != 0;
    anim.speed = clip->speed;
    anim.startTime = clip->startTime;
    anim.flag2A = (clip->flags & kClip_Flag04) != 0;
}

// ----- Simple ---------------------------------------------------------------

// @func 0x004B3C70 InitSimpleClip
void InitSimpleClip(AnimClipDef& clip)
{
    InitClipCommon(clip, "AnimClipSimple");
    clip.animEntry = -1;
}

// @func 0x004B3F28 ReadSimpleClip
void ReadSimpleClip(AnimClipDef& clip, XmlElement* element, AnimClipPool* pool, AnimSet* set)
{
    ReadClipCommon(clip, element, pool, set);
    XmlElement* animElement = element->FirstChildElement("Anim");
    AnimClipElement* anim = EntryElement(pool->At(clip.animEntry));
    if (anim && animElement)
        ReadAnimReference(*anim, animElement, "AnimName", "Map",
                          "Null anim name for animation simple element in anim clip (%s)\n");
}

// @func 0x004B40CC InitSimpleClipDTD
void InitSimpleClipDTD(ApeDTD* dtd)
{
    dtd->AddChild("Anim", 0);
    InitClipDTD(dtd);
}

// @func 0x004B4110 CreateSimpleClip
AnimClipDef* CreateSimpleClip(XmlElement* element, AnimClipPool* pool, AnimSet* set)
{
    AnimClipEntry* entry = AllocEntry(pool, kEntry_SimpleClip);
    // NOTE: the console does not check the allocations.
    if (!entry)
        return nullptr;
    AnimClipDef& clip = entry->clip;
    InitSimpleClip(clip);
    AnimClipEntry* animEntry = AllocEntry(pool, kEntry_SimpleAnim, &clip.animEntry);
    if (animEntry)
        ZeroElement(animEntry->element);
    ReadSimpleClip(clip, element, pool, set);
    return &clip;
}

// @func 0x004B3D84 FillSimpleClip
void FillSimpleClip(const AnimClipDef* clip, AnimClipPool* pool, AnimPlayDesc& desc, const AnimPlayContext& ctx,
                    int index)
{
    desc.animCount = 1;
    CopyAnim(desc.anims[0], EntryElement(pool->At(clip->animEntry)));
    FillClipAnimFlags(desc.anims[0], clip);
    ApplyClipBlend(clip, pool, desc, ctx);
}

// ----- Random ---------------------------------------------------------------

// @func 0x004B42CC InitRandomClip
void InitRandomClip(AnimClipDef& clip)
{
    InitClipCommon(clip, "AnimClipRandom");
    clip.elementCount = 0;
    clip.totalWeight = 0.0f;
    // NOTE: the first element index is only set by the first element.
    clip.firstElement = -1;
}

// Weighted random element (the random and container clips).
AnimClipElement* PickWeighted(const AnimClipDef* clip, AnimClipPool* pool, bool emptyReturnsNull)
{
    const float threshold = clip->totalWeight * RandomUnit(g_gameRandom);
    float sum = 0.0f;
    for (uint32_t i = 0; i < clip->elementCount; ++i) {
        AnimClipElement* element = EntryElement(pool->At((int16_t)(clip->firstElement + i)));
        sum += element ? element->value : 0.0f;
        if (threshold <= sum)
            return element;
    }
    if (emptyReturnsNull && clip->elementCount == 0)
        return nullptr;
    return EntryElement(pool->At(clip->firstElement));
}

// @func 0x004B43E8 PickRandomElement
AnimClipElement* PickRandomElement(const AnimClipDef* clip, AnimClipPool* pool)
{
    return PickWeighted(clip, pool, true);
}

// @func 0x004B453C FillRandomClip
void FillRandomClip(const AnimClipDef* clip, AnimClipPool* pool, AnimPlayDesc& desc, const AnimPlayContext& ctx,
                    int index)
{
    desc.animCount = 1;
    AnimClipElement* element;
    if (index < 0 || (uint32_t)index >= clip->elementCount)
        element = PickRandomElement(clip, pool);
    else
        element = EntryElement(pool->At((int16_t)(clip->firstElement + index)));
    CopyAnim(desc.anims[0], element);
    FillClipAnimFlags(desc.anims[0], clip);
    desc.anims[0].blendType = clip->blendType;
    // A "None" animation plays nothing, so it needs no blend.
    if (desc.anims[0].animHash != kAnimNone)
        ApplyClipBlend(clip, pool, desc, ctx);
}

// @func 0x004B46A8 ReadRandomClip
void ReadRandomClip(AnimClipDef& clip, XmlElement* element, AnimClipPool* pool, AnimSet* set)
{
    ReadClipCommon(clip, element, pool, set);
    for (XmlElement* child = element->FirstChildElement("AnimRandom"); child;
         child = child->NextSiblingElement("AnimRandom")) {
        int16_t index = -1;
        AnimClipEntry* entry = AllocEntry(pool, kEntry_RandomAnim, &index);
        if (!entry) {
            plat::Log(plat::LogLevel::Warning, "Fail to allocate random anim element for anim random clip (%s)\n",
                      clip.name);
            clip.flags |= kClip_Invalid;
            return;
        }
        AnimClipElement& anim = entry->element;
        ZeroElement(anim);
        ReadAnimReference(anim, child, "AnimName", "Map",
                          "Null anim name for animation simple element in anim clip (%s)\n");
        if (child->GetFloat(kAttr_Weight, &anim.value, 0.0f) != xml::kQuery_Ok) {
            plat::Log(plat::LogLevel::Warning,
                      "Cannot get the weight for the anim element (%s). This anim will not be played!\n",
                      anim.animName);
            anim.disabled = true;
        }
        if (clip.elementCount == 0)
            clip.firstElement = index;
        ++clip.elementCount;
        clip.totalWeight += anim.value;
    }
}

// @func 0x004B499C InitRandomClipDTD
void InitRandomClipDTD(ApeDTD* dtd)
{
    dtd->AddChild("AnimRandom", 1);
    InitClipDTD(dtd);
}

// @func 0x004B49E0 CreateRandomClip
AnimClipDef* CreateRandomClip(XmlElement* element, AnimClipPool* pool, AnimSet* set)
{
    AnimClipEntry* entry = AllocEntry(pool, kEntry_RandomClip);
    if (!entry)
        return nullptr;
    InitRandomClip(entry->clip);
    ReadRandomClip(entry->clip, element, pool, set);
    return &entry->clip;
}

// ----- Selection ------------------------------------------------------------

// @func 0x004B4AEC InitSelectionClip
void InitSelectionClip(AnimClipDef& clip)
{
    InitClipCommon(clip, "AnimClipSelection");
    clip.firstElement = -1;
    clip.flags |= kClip_ControlDriven;
    clip.elementCount = 0;
}

bool SelectionMatches(uint32_t op, float control, float value)
{
    switch (op) {
    case kOp_ControlLessEqual: return control <= value;
    case kOp_ControlGreaterEqual: return value <= control;
    case kOp_LessThan: return control < value;
    case kOp_ControlGreater: return value < control;
    case kOp_NotEqual: return control != value;
    default: return control == value;
    }
}

// @func 0x004B4C08 FillSelectionClip
void FillSelectionClip(const AnimClipDef* clip, AnimClipPool* pool, AnimPlayDesc& desc, const AnimPlayContext& ctx,
                       int index)
{
    AnimClipElement* element = nullptr;
    if (index >= 0 && (uint32_t)index < clip->elementCount) {
        element = EntryElement(pool->At((int16_t)(clip->firstElement + index)));
    } else {
        if (clip->elementCount == 0)
            return;
        // First element whose condition holds; the last one otherwise.
        for (uint32_t i = 0; i < clip->elementCount; ++i) {
            element = EntryElement(pool->At((int16_t)(clip->firstElement + i)));
            if (element && SelectionMatches(element->param, ctx.controlValue, element->value))
                break;
        }
    }
    if (!element)
        return;
    desc.animCount = 1;
    CopyAnim(desc.anims[0], element);
    FillClipAnimFlags(desc.anims[0], clip);
    desc.anims[0].loop = (clip->flags & kClip_Loop) != 0 || element->loop;
    ApplyClipBlend(clip, pool, desc, ctx);
}

// Reads the "Type" attribute of the selection and blend clips.
void ReadControlType(AnimClipDef& clip, XmlElement* element, const char* nullMessage)
{
    const char* typeName = element->GetString(kAttr_Type);
    if (!typeName) {
        plat::Log(plat::LogLevel::Warning, "%s\n", nullMessage);
        clip.flags |= kClip_Invalid;
        return;
    }
    const int16_t index = g_animClipManager->FindControlType(HashNameNoCase(typeName));
    if (index < 0)
        plat::Log(plat::LogLevel::Warning,
                  "Fail to find the anim control data type (%s) define in the data type table!\n", typeName);
    clip.controlType = index;
}

// @func 0x004B4EC8 ReadSelectionClip
void ReadSelectionClip(AnimClipDef& clip, XmlElement* element, AnimClipPool* pool, AnimSet* set)
{
    ReadClipCommon(clip, element, pool, set);
    for (XmlElement* child = element->FirstChildElement("AnimSelection"); child;
         child = child->NextSiblingElement("AnimSelection")) {
        int16_t index = -1;
        AnimClipEntry* entry = AllocEntry(pool, kEntry_SelectionAnim, &index);
        if (!entry) {
            plat::Log(plat::LogLevel::Warning,
                      "Fail to allocate anim clip element for anim clip selection (%s)\n", clip.name);
            clip.flags |= kClip_Invalid;
            return;
        }
        AnimClipElement& anim = entry->element;
        ZeroElement(anim);
        anim.param = kOp_ControlLessEqual;
        ReadAnimReferenceByHash(anim, child, "Null clip name for animation selection element in anim (%s)\n");
        if (anim.animName) {
            child->GetFloat(kAttr_Value, &anim.value, 0.0f);
            child->GetHash(kAttr_Operator, &anim.param, kOp_ControlLessEqual);
            child->GetBool(kAttr_Loop, &anim.loop, false);
        }
        if (clip.elementCount == 0)
            clip.firstElement = index;
        ++clip.elementCount;
    }
    ReadControlType(clip, element, "ApeAnimClipSelectionDef: Null anim control value type pass in!");
}

void AddControlTypeAttribute(ApeDTD* dtd)
{
    DTDAttribute* type = dtd->AddAttribute("Type", kDTD_String, 0);
    const AnimClipManager* manager = g_animClipManager;
    AddEnumNames(type, manager->controlTypes, manager->controlTypeCount, manager->controlTypeCount);
}

// @func 0x004B5274 InitSelectionClipDTD
void InitSelectionClipDTD(ApeDTD* dtd)
{
    dtd->AddChild("AnimSelection", 1);
    InitClipDTD(dtd);
    AddControlTypeAttribute(dtd);
}

// @func 0x004B53CC CreateSelectionClip
AnimClipDef* CreateSelectionClip(XmlElement* element, AnimClipPool* pool, AnimSet* set)
{
    AnimClipEntry* entry = AllocEntry(pool, kEntry_SelectionClip);
    if (!entry)
        return nullptr;
    InitSelectionClip(entry->clip);
    ReadSelectionClip(entry->clip, element, pool, set);
    return &entry->clip;
}

// ----- Container ------------------------------------------------------------

// @func 0x004B54D0 InitContainerClip
void InitContainerClip(AnimClipDef& clip)
{
    InitClipCommon(clip, "AnimClipContainer");
    clip.elementCount = 0;
    clip.totalWeight = 0.0f;
    clip.firstElement = -1;
}

// @func 0x004B5708 PickContainerElement
AnimClipElement* PickContainerElement(const AnimClipDef* clip, AnimClipPool* pool)
{
    // NOTE: unlike the random clip this falls back to the first element even
    // when there is none (the console then returns a bogus pointer).
    return PickWeighted(clip, pool, false);
}

// @func 0x004B584C FillContainerClip
void FillContainerClip(const AnimClipDef* clip, AnimClipPool* pool, AnimPlayDesc& desc,
                       const AnimPlayContext& ctx, int index)
{
    AnimClipElement* element;
    if (index < 0 || (uint32_t)index >= clip->elementCount)
        element = PickContainerElement(clip, pool);
    else
        element = EntryElement(pool->At((int16_t)(clip->firstElement + index)));
    if (!element)
        return;
    pool->Owner()->ResolveClip(element->animHash, desc, ctx, -1);
}

// @func 0x004B5918 ReadContainerClip
void ReadContainerClip(AnimClipDef& clip, XmlElement* element, AnimClipPool* pool, AnimSet* set)
{
    ReadClipCommon(clip, element, pool, set);
    for (XmlElement* child = element->FirstChildElement("AnimClip"); child;
         child = child->NextSiblingElement("AnimClip")) {
        int16_t index = -1;
        AnimClipEntry* entry = AllocEntry(pool, kEntry_ContainerRef, &index);
        if (!entry) {
            plat::Log(plat::LogLevel::Warning,
                      "Fail to allocate anim clip element for anim clip container (%s)\n", clip.name);
            clip.flags |= kClip_Invalid;
            return;
        }
        AnimClipElement& ref = entry->element;
        ref.animName = nullptr;
        ref.animHash = 0;
        ref.value = 0.0f;
        ref.disabled = false;
        const char* clipName = child->GetString(kAttr_ClipName);
        ref.animName = clipName;
        if (!clipName) {
            plat::Log(plat::LogLevel::Warning,
                      "Null clip name for animation container element in anim clip (%s)\n",
                      child->GetLocationString());
        } else {
            ref.animName = Strings().Add(clipName);
            ref.animHash = HashNameNoCase(ref.animName);
            child->GetFloat(kAttr_Weight, &ref.value, 100.0f);
        }
        if (clip.elementCount == 0)
            clip.firstElement = index;
        ++clip.elementCount;
        clip.totalWeight += ref.value;
    }
}

// @func 0x004B5B74 InitContainerClipDTD
void InitContainerClipDTD(ApeDTD* dtd)
{
    dtd->AddAttribute("Name", kDTD_String, 0);
    DTDAttribute* type = dtd->AddAttribute("Type", kDTD_Hash, 0);
    type->AllocEnumNames(1);
    type->AddEnumName("Random");
    dtd->AddChild("AnimClip", 1);
}

// @func 0x004B5C70 CreateContainerClip
AnimClipDef* CreateContainerClip(XmlElement* element, AnimClipPool* pool, AnimSet* set)
{
    AnimClipEntry* entry = AllocEntry(pool, kEntry_ContainerClip);
    if (!entry)
        return nullptr;
    InitContainerClip(entry->clip);
    ReadContainerClip(entry->clip, element, pool, set);
    return &entry->clip;
}

// ----- Blend ----------------------------------------------------------------

// @func 0x004B5D78 InitBlendClip
void InitBlendClip(AnimClipDef& clip)
{
    InitClipCommon(clip, "AnimClipBlend");
    clip.firstElement = -1;
    clip.flags |= kClip_ControlDriven;
    clip.adjustAnimSpeed = false;
    clip.elementCount = 0;
}

// @func 0x004B5EB0 FillBlendClip
void FillBlendClip(const AnimClipDef* clip, AnimClipPool* pool, AnimPlayDesc& desc, const AnimPlayContext& ctx,
                   int index)
{
    desc.flags = (uint8_t)((desc.flags & ~(AnimPlayDesc::kDesc_Failed | AnimPlayDesc::kDesc_AdjustSpeed)) |
                           (clip->adjustAnimSpeed ? AnimPlayDesc::kDesc_AdjustSpeed : 0));
    const uint32_t count = clip->elementCount;
    if (count == 0)
        return;

    AnimClipElement* first = nullptr;
    AnimClipElement* second = nullptr;
    // NOTE: the console's test reads "index < 0 && index >= count", which is
    // never true, so the control value search below is dead code and the
    // element at firstElement + index is always used (index -1 included).
    if (index < 0 && (int)count <= index) {
        AnimClipElement* previous = nullptr;
        for (uint32_t i = 0; i < count; ++i) {
            AnimClipElement* element = EntryElement(pool->At((int16_t)(clip->firstElement + i)));
            if (ctx.controlValue <= element->value) {
                first = previous;
                second = element;
                break;
            }
            previous = element;
        }
        if (!first) {
            first = second;
            second = nullptr;
        }
    } else {
        first = EntryElement(pool->At((int16_t)(clip->firstElement + index)));
    }
    if (!first)
        return;

    desc.animCount = 1;
    AnimPlayAnim& anim0 = desc.anims[0];
    CopyAnim(anim0, first);
    anim0.loop = (clip->flags & kClip_Loop) != 0;
    anim0.speed = clip->speed;
    anim0.blendIn = clip->blendIn;
    anim0.startTime = clip->startTime;
    anim0.flag2A = (clip->flags & kClip_Flag04) != 0;
    anim0.blendType = clip->blendType;
    if (second) {
        desc.animCount = 2;
        AnimPlayAnim& anim1 = desc.anims[1];
        CopyAnim(anim1, second);
        anim1.speed = clip->speed;
        anim1.blendIn = clip->blendIn;
        anim1.startTime = clip->startTime;
        anim1.blendType = clip->blendType;
        anim1.loop = (clip->flags & kClip_Loop) != 0;
        desc.blendParam = second->param;
        float t = (ctx.controlValue - first->value) / (second->value - first->value);
        if (1.0f - t < 0.0f)
            t = 1.0f;
        if (t < 0.0f)
            t = 0.0f;
        anim1.weight = t;
        anim0.weight = 1.0f - t;
    }
}

// @func 0x004B6120 ReadBlendClip
void ReadBlendClip(AnimClipDef& clip, XmlElement* element, AnimClipPool* pool, AnimSet* set)
{
    ReadClipCommon(clip, element, pool, set);
    for (XmlElement* child = element->FirstChildElement("AnimBlend"); child;
         child = child->NextSiblingElement("AnimBlend")) {
        int16_t index = -1;
        AnimClipEntry* entry = AllocEntry(pool, kEntry_BlendAnim, &index);
        if (!entry) {
            plat::Log(plat::LogLevel::Warning, "Fail to allocate anim clip element for anim clip blend (%s)\n",
                      clip.name);
            clip.flags |= kClip_Invalid;
            return;
        }
        AnimClipElement& anim = entry->element;
        ZeroElement(anim);
        anim.param = 0;
        ReadAnimReferenceByHash(anim, child, "Null clip name for animation blend element in anim (%s)\n");
        if (anim.animName) {
            child->GetFloat(kAttr_BlendValue, &anim.value, 0.0f);
            child->GetHash(kAttr_BlendParam, &anim.param, 0);
        }
        if (clip.elementCount == 0)
            clip.firstElement = index;
        ++clip.elementCount;
    }
    ReadControlType(clip, element, "ApeAnimClipBlendDef: Null anim control value type pass in!");
    element->GetBool(kAttr_AdjustAnimSpeed, &clip.adjustAnimSpeed, false);
}

// @func 0x004B64D4 InitBlendClipDTD
void InitBlendClipDTD(ApeDTD* dtd)
{
    dtd->AddChild("AnimBlend", 1);
    InitClipDTD(dtd);
    AddControlTypeAttribute(dtd);
    dtd->AddAttribute("AdjustAnimSpeed", kDTD_Bool, 1);
}

// @func 0x004B6658 CreateBlendClip
AnimClipDef* CreateBlendClip(XmlElement* element, AnimClipPool* pool, AnimSet* set)
{
    AnimClipEntry* entry = AllocEntry(pool, kEntry_BlendClip);
    if (!entry)
        return nullptr;
    InitBlendClip(entry->clip);
    ReadBlendClip(entry->clip, element, pool, set);
    return &entry->clip;
}

} // namespace

// The creation table (0x00F65F88).
const AnimClipType g_animClipTypes[5] = {
    { NameHash("AnimClipContainer"), "AnimClipContainer", &CreateContainerClip, &InitContainerClipDTD,
      &FillContainerClip },
    { NameHash("AnimClipSimple"), "AnimClipSimple", &CreateSimpleClip, &InitSimpleClipDTD, &FillSimpleClip },
    { NameHash("AnimClipRandom"), "AnimClipRandom", &CreateRandomClip, &InitRandomClipDTD, &FillRandomClip },
    { NameHash("AnimClipSelection"), "AnimClipSelection", &CreateSelectionClip, &InitSelectionClipDTD,
      &FillSelectionClip },
    { NameHash("AnimClipBlend"), "AnimClipBlend", &CreateBlendClip, &InitBlendClipDTD, &FillBlendClip },
};

// @func 0x004B2DA0 InitClipDTD
void InitClipDTD(ApeDTD* dtd)
{
    dtd->AddAttribute("Name", kDTD_String, 0);
    dtd->AddAttribute("Class", kDTD_Hash, 1);
    dtd->AddAttribute("ChannelMap", kDTD_Hash, 1);
    dtd->AddAttribute("Loop", kDTD_Bool, 1);
    dtd->AddAttribute("BlendIn", kDTD_Float, 1);
    dtd->AddAttribute("ScaleExtMotion", kDTD_Bool, 1);
    dtd->AddAttribute("Speed", kDTD_Float, 1);
    dtd->AddAttribute("PlayNode", kDTD_String, 1);
    dtd->AddAttribute("ForceSyncAnimPos", kDTD_Bool, 1);
    dtd->AddAttribute("StartTime", kDTD_Float, 1);
    dtd->AddAttribute("TranAnimClipName", kDTD_String, 1);
    DTDAttribute* blendType = dtd->AddAttribute("BlendType", kDTD_Hash, 1);
    blendType->AllocEnumNames(3);
    blendType->AddEnumName("Smooth");
    blendType->AddEnumName("Linear");
    blendType->AddEnumName("Slow");
    DTDAttribute* syncType = dtd->AddAttribute("SyncType", kDTD_Hash, 1);
    const AnimClipManager* manager = g_animClipManager;
    AddEnumNames(syncType, manager->syncTypes, manager->syncTypeCount, manager->controlTypeCount);
    dtd->AddChild("AnimEvent", 3);
}

// ---------------------------------------------------------------------------
// AnimSet
// ---------------------------------------------------------------------------

// @func 0x004B0C70 AnimSet::~AnimSet
AnimSet::~AnimSet()
{
    delete clips;
    clips = nullptr;
    delete blendInfo;
    blendInfo = nullptr;
    delete blendClassInfo;
    blendClassInfo = nullptr;
    delete pool;
    pool = nullptr;
}

namespace {

// @func 0x004B14F8 ReadBlendInfo
bool ReadBlendInfo(AnimBlendInfo& info, XmlElement* element, uint32_t* key)
{
    const char* from = element->GetString(kAttr_AnimName1);
    if (!from)
        return false;
    const char* to = element->GetString(kAttr_AnimName2);
    if (!to)
        return false;
    // "*" matches any previous animation; the key is then the target alone.
    // NOTE: the console leaves its buffer uninitialised in that case.
    char buffer[0x200] = {};
    if (from[0] != '*')
        StrNCopy(buffer, from, sizeof(buffer));
    std::strncat(buffer, to, sizeof(buffer) - std::strlen(buffer) - 1);
    *key = HashNameNoCase(buffer);
    if (element->GetFloat(kAttr_BlendValue, &info.time, 0.0f) != xml::kQuery_Ok)
        return false;
    info.blendType = ParseBlendType(element);
    info.transitionName = PoolString(element->GetString(kAttr_TransitionClip), &info.transitionHash);
    return true;
}

// @func 0x004B1330 ReadBlendClassInfo
bool ReadBlendClassInfo(AnimBlendClassInfo& info, XmlElement* element, uint32_t* key)
{
    element->GetHash(kAttr_From, &info.fromClass, info.fromClass);
    if (info.fromClass == 0)
        return false;
    element->GetHash(kAttr_To, &info.toClass, info.toClass);
    if (info.toClass == 0)
        return false;
    if (element->GetFloat(kAttr_BlendValue, &info.time, 0.0f) != xml::kQuery_Ok)
        return false;
    info.blendType = ParseBlendType(element);
    info.transitionName = PoolString(element->GetString(kAttr_TransitionClip), &info.transitionHash);
    *key = (info.fromClass << 16) | (info.toClass >> 16);
    return true;
}

} // namespace

// @func 0x004B1708 AnimSet::Load
void AnimSet::Load(const char* file, XmlElement* element)
{
    AnimClipManager& manager = *g_animClipManager;
    fileName = manager.strings.Add(file);
    fileHash = HashNameNoCase(fileName);
    if (!element) {
        plat::Log(plat::LogLevel::Warning,
                  "Cannot find the root element (%s) in the anim set file (%s), the anim set will be ignored!\n",
                  "AnimSet", fileName);
        return;
    }

    const char* setName = element->GetString(kAttr_Name);
    name = manager.strings.Add(setName);
    nameHash = HashNameNoCase(name);
    if (const char* inherit = element->GetString(kAttr_Inherit)) {
        parent = manager.FindSet(HashNameNoCase(inherit));
        if (!parent)
            plat::Log(plat::LogLevel::Warning,
                      "Cannot find the set to inherit from. Current Set: (%s), Inheriting from: (%s)\n", setName,
                      inherit);
    }
    element->GetHash(kAttr_SetChannelMap, &channelMapOverride, channelMapOverride);

    // Blend rules between animations. NOTE: elements the reader rejects are
    // still added (with an unset key on the console).
    XmlElement* blendData = element->FirstChildElement("AnimBlendInfoData");
    if (blendData) {
        if (const int count = blendData->CountChildren(nullptr)) {
            blendInfo = new ApeTSortedMap<AnimBlendInfo>((uint16_t)count, nullptr,
                                                         AnimBlendInfo{ -FLT_MAX, 0, 0, nullptr });
            for (XmlElement* child = blendData->FirstChildElement(); child; child = child->NextSiblingElement()) {
                AnimBlendInfo info = { -FLT_MAX, 0, 0, nullptr };
                uint32_t key = 0;
                ReadBlendInfo(info, child, &key);
                blendInfo->Add(key, info);
            }
        }
    }

    // Blend rules between clip classes.
    XmlElement* classData = element->FirstChildElement("AnimBlendClassInfoData");
    if (classData) {
        if (const int count = classData->CountChildren(nullptr)) {
            blendClassInfo = new ApeTSortedMap<AnimBlendClassInfo>(
                (uint16_t)count, nullptr, AnimBlendClassInfo{ 0, 0, -FLT_MAX, 0, 0, nullptr });
            for (XmlElement* child = classData->FirstChildElement(); child; child = child->NextSiblingElement()) {
                AnimBlendClassInfo info = { 0, 0, -FLT_MAX, 0, 0, nullptr };
                uint32_t key = 0;
                ReadBlendClassInfo(info, child, &key);
                blendClassInfo->Add(key, info);
            }
        }
    }

    // The clips.
    XmlElement* clipData = element->FirstChildElement("AnimClipData");
    if (!clipData)
        return;
    const int count = clipData->CountChildren(nullptr);
    clipData->GetInt(kAttr_ClipCountHint, &clipCountHint, 0x40);
    if (!count)
        return;
    int32_t poolSize = 0x40;
    clipData->GetInt(kAttr_Size, &poolSize, 0x40);
    pool = new AnimClipPool((uint16_t)poolSize, this);
    clips = new ApeTSortedMap<AnimClipDef*>((uint16_t)count);
    for (XmlElement* child = clipData->FirstChildElement(); child; child = child->NextSiblingElement()) {
        const AnimClipType* type = manager.FindClipType(child->GetNameHash());
        if (!type || !type->create)
            continue;
        AnimClipDef* clip = type->create(child, pool, this);
        if (clip && !(clip->flags & kClip_Invalid))
            clips->Add(clip->nameHash, clip);
    }
}

// @func 0x00789BC0 AnimSet::FindBlendInfo
bool AnimSet::FindBlendInfo(const char* fromAnim, const char* toAnim, AnimBlendInfo* out) const
{
    if (!blendInfo)
        return parent ? parent->FindBlendInfo(fromAnim, toAnim, out) : false;

    char buffer[0x200] = {};
    if (fromAnim[0] != '*')
        StrNCopy(buffer, fromAnim, sizeof(buffer));
    std::strncat(buffer, toAnim, sizeof(buffer) - std::strlen(buffer) - 1);
    *out = *blendInfo->Find(HashNameNoCase(buffer));
    if (out->time != -FLT_MAX)
        return true;
    // The wildcard rule of the target.
    *out = *blendInfo->Find(HashNameNoCase(toAnim));
    if (out->time != -FLT_MAX)
        return true;
    return parent ? parent->FindBlendInfo(fromAnim, toAnim, out) : false;
}

// @func 0x00789E00 AnimSet::FindBlendClassInfo
bool AnimSet::FindBlendClassInfo(uint32_t fromClass, uint32_t toClass, AnimBlendClassInfo* out) const
{
    if (!blendClassInfo)
        return parent ? parent->FindBlendClassInfo(fromClass, toClass, out) : false;
    *out = *blendClassInfo->Find((fromClass << 16) | (toClass >> 16));
    if (out->time == -FLT_MAX && parent)
        return parent->FindBlendClassInfo(fromClass, toClass, out);
    return out->time != -FLT_MAX;
}

// @func 0x004B55E0 AnimSet::ResolveClip
void AnimSet::ResolveClip(uint32_t clipHash, AnimPlayDesc& desc, const AnimPlayContext& ctx, int index)
{
    if (!clips) {
        desc.flags |= AnimPlayDesc::kDesc_Failed;
        return;
    }
    AnimClipDef* clip = *clips->Find(clipHash);
    if (!clip) {
        if (!parent)
            desc.flags |= AnimPlayDesc::kDesc_Failed;
        else
            parent->ResolveClip(clipHash, desc, ctx, -1);
        return;
    }
    g_animClipTypes[clip->typeIndex].fill(clip, pool, desc, ctx, index);
    desc.clip = clip;
}

// @func 0x004B6760 AnimSet::PlayClip
void AnimSet::PlayClip(const AnimClipDef* clip, AnimPlayDesc& desc, const AnimPlayContext& ctx, int index)
{
    const bool ownClip = clips && *clips->Find(clip->nameHash);
    if (!ownClip && parent) {
        parent->PlayClip(clip, desc, ctx, -1);
        if (channelMapOverride) {
            desc.anims[0].mapHash = channelMapOverride;
            desc.anims[1].mapHash = channelMapOverride;
        }
        return;
    }
    g_animClipTypes[clip->typeIndex].fill(clip, pool, desc, ctx, index);
    desc.clip = clip;
}

// ---------------------------------------------------------------------------
// AnimClipManager
// ---------------------------------------------------------------------------

// @func 0x004B0850 AnimClipManager::AnimClipManager
// @func 0x004B0C40 AnimClipManager::AnimClipManager (complete object)
AnimClipManager::AnimClipManager(int slotCapacity, const AnimNamedType* controlTypes, uint32_t controlTypeCount,
                                 const AnimNamedType* syncTypes, uint32_t syncTypeCount,
                                 AnimEventUserDataFn eventUserData, void* field11C, AnimSlotAcquireFn acquire,
                                 AnimSlotReleaseFn release)
    : strings(0x12000, false, false),
      clipTypes(5),
      field_30(0x100),
      sets(0x20),
      slots(slotCapacity),
      controlTypes(controlTypes),
      controlTypeCount(controlTypeCount),
      syncTypes(syncTypes),
      syncTypeCount(syncTypeCount),
      eventUserData(eventUserData),
      field_11C(field11C),
      acquireSlot(acquire),
      releaseSlot(release)
{
    g_animClipManager = this;
    for (const AnimClipType& type : g_animClipTypes)
        clipTypes.Add(type.hash, &type);
    slots.SetGrowable(false);
    iff::RegisterChunkHandler(iff::MakeId('A', 'C', 'L', 'F'), &AnimClipFileChunk, 0, nullptr, 0);
    iff::RegisterChunkHandler(iff::MakeId('A', 'C', 'L', 'H'), &AnimClipFileHeaderChunk, 0, nullptr, 0);
    iff::RegisterChunkHandler(iff::MakeId('A', 'C', 'L', 'D'), &AnimClipFileDataChunk, 0, nullptr, 0);
}

// @func 0x004B1114 AnimClipManager::~AnimClipManager
// @func 0x004B1308 AnimClipManager::~AnimClipManager (complete object)
AnimClipManager::~AnimClipManager()
{
    g_animClipManager = nullptr;
    Clear();
}

int16_t AnimClipManager::FindControlType(uint32_t hash) const
{
    for (uint32_t i = 0; i < controlTypeCount; ++i) {
        if (controlTypes[i].hash == hash)
            return (int16_t)i;
    }
    return -1;
}

int16_t AnimClipManager::FindSyncType(uint32_t hash) const
{
    for (uint32_t i = 0; i < syncTypeCount; ++i) {
        if (syncTypes[i].hash == hash)
            return (int16_t)i;
    }
    return -1;
}

// @func 0x004B2398 AnimClipManager::ReleaseAllSlots
void AnimClipManager::ReleaseAllSlots()
{
    for (int32_t i = slots.FirstIndex(); i != ApeTHandleList<AnimSetSlot>::kNone; i = slots.NextIndex(i)) {
        AnimSetSlot& slot = slots.At(i);
        slot.set = nullptr;
        if (releaseSlot)
            releaseSlot(slot.userData);
        slot.userData &= 0xFFFF0000u;
    }
    slots.Clear();
}

// @func 0x004B0E70 AnimClipManager::Clear
void AnimClipManager::Clear()
{
    ReleaseAllSlots();
    strings.Reset(strings.Capacity());
    field_30.Clear();
    for (uint16_t i = 0; i < sets.Count(); ++i)
        delete sets.Data()[i].value;
    sets.Clear();
    fileCount = 0;
}

// @func 0x004B25C0 AnimClipManager::OnLevelReset
void AnimClipManager::OnLevelReset()
{
}

// @func 0x004B25C8 AnimClipManager::AcquireSlot
AnimSetSlot* AnimClipManager::AcquireSlot(const AnimSetSlotRequest& request, uint32_t* outHandle)
{
    if (slots.Count() >= slots.Capacity()) {
        plat::Log(plat::LogLevel::Warning,
                  "Run out of anim set slot, please increase the size of the anim set list (%d)\n", slots.Count());
        return nullptr;
    }
    // NOTE: the console reuses the node's old slot block, so the user data of
    // a previous owner survives when there is no acquire callback.
    const uint32_t handle = slots.PushBack(AnimSetSlot{ nullptr, 0 });
    *outHandle = handle;
    AnimSetSlot* slot = slots.Get(handle);
    slot->set = request.set;
    if (acquireSlot) {
        uint32_t userData = 0;
        acquireSlot(&userData, request.set->field_28, request.user);
        slot->userData = userData;
    }
    return slot;
}

// @func 0x004B2A04 AnimClipManager::ReleaseSlot
void AnimClipManager::ReleaseSlot(uint32_t handle)
{
    AnimSetSlot* slot = slots.Get(handle);
    if (!slot)
        return;
    slot->set = nullptr;
    if (releaseSlot)
        releaseSlot(slot->userData);
    slot->userData &= 0xFFFF0000u;
    slots.Remove(handle);
}

// @func 0x004B20B8 AnimClipManager::LoadAnimSetFile
bool AnimClipManager::LoadAnimSetFile(XmlDocument& document, const char* path)
{
    // NOTE: after the first "AnimSet" every following sibling element is read.
    for (XmlElement* element = document.FirstChildElement("AnimSet"); element;
         element = element->NextSiblingElement()) {
        const uint32_t hash = HashNameNoCase(element->GetString("Name"));
        if (*sets.Find(hash))
            continue;
        AnimSet* set = new AnimSet;
        set->Load(path, element);
        sets.Add(hash, set);
    }
    return true;
}

// @func 0x004B229C AnimClipManager::LoadAnimSetFiles
void AnimClipManager::LoadAnimSetFiles()
{
    for (uint32_t i = 0; i < fileCount; ++i) {
        const char* path = files[i];
        XmlDocument document(path);
        if (!document.LoadFile(0))
            plat::Log(plat::LogLevel::Warning, "Fail to load AnimClip file (%s)\n", path);
        else
            LoadAnimSetFile(document, path);
    }
}

// ---------------------------------------------------------------------------
// Chunk handlers
// ---------------------------------------------------------------------------

// @func 0x004B2B18 AnimClipFileChunk
int AnimClipFileChunk(iff::IffReader* reader, void* arg)
{
    g_animClipFileName = nullptr;
    reader->ParseChunks(nullptr);
    return 1;
}

// @func 0x004B2B50 AnimClipFileHeaderChunk
int AnimClipFileHeaderChunk(iff::IffReader* reader, void* arg)
{
    // One byte (ignored) and the 64-character file name.
    uint8_t ignored = 0;
    reader->ReadU8(&ignored);
    for (int i = 1; i <= 0x40; ++i)
        reader->ReadU8(reinterpret_cast<uint8_t*>(&g_animClipFileNameBuffer[i]));
    g_animClipFileNameBuffer[0x40] = '\0';
    g_animClipFileName = &g_animClipFileNameBuffer[1];
    return 1;
}

// @func 0x004B2C1C AnimClipFileDataChunk
int AnimClipFileDataChunk(iff::IffReader* reader, void* arg)
{
    const uint32_t size = reader->GetChunkSize();
    if (!size)
        return 0;
    void* data = GetMemStack()->Alloc(size);
    reader->Read(data, size);
    file::File* stream = file::CreateMemoryFile(data, size, g_animClipFileName);
    AnimClipManager* manager = g_animClipManager;
    {
        XmlDocument document;
        if (stream && document.LoadFromStream(*stream, nullptr))
            manager->LoadAnimSetFile(document, stream->GetName());
        else
            plat::Log(plat::LogLevel::Warning, "Fail to load AnimClip file (%s)\n",
                      stream ? stream->GetName() : "");
    }
    if (stream)
        stream->Close();
    GetMemStack()->Free(data);
    g_animClipFileName = nullptr;
    return 0;
}

} // namespace ape::anim
