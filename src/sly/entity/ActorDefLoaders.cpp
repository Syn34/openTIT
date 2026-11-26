// openTIT - Factories of the actor definition classes: each builds its class
// description on top of the actor description (ActorDefDesc), reads its own
// attributes (taking the defaults from a "from" parent of the same class) and
// constructs the definition. The loader (EntityDefLoader.cpp) calls them by
// element name hash.
#include "sly/entity/ActorDefLoaders.h"

#include "sly/entity/ActorDefs.h"

#include "ape/xml/ApeXml.h"

#include <algorithm>
#include <cmath>

namespace sly {
namespace entity {

namespace {

// The description every class factory starts from: the actor defaults with
// the class's type bit set.
struct ClassDefDesc : ActorDefDesc {
    ClassDefDesc(const EntityDefDesc& loader, uint32_t classType) : ActorDefDesc(loader) { typeBits |= classType; }
};

// Reads boolean attribute `attr` into bit `bit` of `word` (default: the bit of `defaults`).
void ReadBitAttr(ape::xml::XmlElement* elem, uint32_t attr, uint32_t& word, uint32_t defaults, int bit)
{
    bool on = false;
    elem->GetBool(attr, &on, ((defaults >> bit) & 1) != 0);
    word = (word & ~(1u << bit)) | (uint32_t(on) << bit);
}

// The parent supplies the defaults when it is of the same class.
template <class Def>
const Def* SameClassParent(const EntityDef* parent, uint32_t classType)
{
    return parent && (parent->typeBits & classType) ? static_cast<const Def*>(parent) : nullptr;
}

// -- ActorGroup ------------------------------------------------------------------
constexpr uint32_t kType_ActorGroup = 0x2000;

struct ActorGroupDefDesc : ClassDefDesc {
    explicit ActorGroupDefDesc(const EntityDefDesc& loader) : ClassDefDesc(loader, kType_ActorGroup) {}
    // Placement defaults (ActorGroupPlacement +0xF0..+0x100).
    float field_E0[4] = {};     // 0xE0 attributes 0x4E14723F, 0x6A96C7DB, 0xEAEDFCE1, 0x0A924DA2
    bool field_F0 = false;      // 0xF0 attribute 0x317F6BB4

    // 0x001A9EA8
    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001A9EA8 ActorGroupDefDesc::Parse
void ActorGroupDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                              const EntityDef* parent)
{
    ActorDefDesc::Parse(root, elem, loader, parent);
    float defaults[4] = {};
    bool defaultFlag = false;
    if (const auto* group = SameClassParent<ActorGroupDef>(parent, kType_ActorGroup)) {
        for (int i = 0; i < 4; ++i)
            defaults[i] = group->field_130[i];
        defaultFlag = group->field_140;
    }
    static constexpr uint32_t kAttrs[4] = {0x4E14723Fu, 0x6A96C7DBu, 0xEAEDFCE1u, 0x0A924DA2u};
    for (int i = 0; i < 4; ++i)
        elem->GetFloat(kAttrs[i], &field_E0[i], defaults[i]);
    elem->GetBool(0x317F6BB4u, &field_F0, defaultFlag);
}

// -- HitCounterActor -------------------------------------------------------------
constexpr uint32_t kType_HitCounter = 0x4000;

struct HitCounterActorDefDesc : ClassDefDesc {
    explicit HitCounterActorDefDesc(const EntityDefDesc& loader) : ClassDefDesc(loader, kType_HitCounter) {}
    float field_E0[5] = {};     // 0xE0 attributes 0x8722570B, 0x1E6104E5, 0x08B6A993, 0x14EE18CD, 0x4E14723F
    int32_t field_F4 = -1;      // 0xF4 attribute 0x4C238808, one-based in the file
    uint32_t flags_F8 = 0;      // 0xF8 bits 31 (0x609A61B6) and 30 (0x994327AD)

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001AB190 HitCounterActorDefDesc::Parse
void HitCounterActorDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                   const EntityDefDesc& loader, const EntityDef* parent)
{
    ActorDefDesc::Parse(root, elem, loader, parent);
    float defaults[5] = {};
    int32_t defaultIndex = -1;
    uint32_t defaultFlags = 0x80000000u;
    if (const auto* counter = SameClassParent<HitCounterActorDef>(parent, kType_HitCounter)) {
        for (int i = 0; i < 5; ++i)
            defaults[i] = counter->field_130[i];
        defaultIndex = counter->field_144;
        defaultFlags = counter->flags_148 & 0xC0000000u;
    }
    static constexpr uint32_t kAttrs[5] = {0x8722570Bu, 0x1E6104E5u, 0x08B6A993u, 0x14EE18CDu, 0x4E14723Fu};
    for (int i = 0; i < 5; ++i)
        elem->GetFloat(kAttrs[i], &field_E0[i], defaults[i]);
    if (elem->GetInt(0x4C238808u, &field_F4, defaultIndex) == ape::xml::kQuery_Ok)
        field_F4 = std::max(field_F4 - 1, 0);
    ReadBitAttr(elem, 0x609A61B6u, flags_F8, defaultFlags, 31);
    ReadBitAttr(elem, 0x994327ADu, flags_F8, defaultFlags, 30);
}

// -- WatchActor ------------------------------------------------------------------
constexpr uint32_t kType_Watch = 0x8000;

struct WatchActorDefDesc : ClassDefDesc {
    explicit WatchActorDefDesc(const EntityDefDesc& loader) : ClassDefDesc(loader, kType_Watch) {}
    uint32_t field_E0 = 0;      // 0xE0 hash 0xB90456D0
    uint32_t field_E4 = 0;      // 0xE4 hash 0xB0804865
    float field_E8 = 0.0f;      // 0xE8 attribute 0x80E60B5A
    uint32_t flags_EC = 0;      // 0xEC bits 31 (0x91863F0B) and 30 (0x0CF2D02B)

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001AC540 WatchActorDefDesc::Parse
void WatchActorDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                              const EntityDef* parent)
{
    ActorDefDesc::Parse(root, elem, loader, parent);
    WatchActorDefDesc defaults(loader);
    if (const auto* watch = SameClassParent<WatchActorDef>(parent, kType_Watch)) {
        defaults.field_E0 = watch->field_130;
        defaults.field_E4 = watch->field_134;
        defaults.field_E8 = watch->field_138;
        defaults.flags_EC = watch->flags_13C & 0xC0000000u;
    }
    elem->GetHash(0xB90456D0u, &field_E0, defaults.field_E0);
    elem->GetHash(0xB0804865u, &field_E4, defaults.field_E4);
    elem->GetFloat(0x80E60B5Au, &field_E8, defaults.field_E8);
    ReadBitAttr(elem, 0x91863F0Bu, flags_EC, defaults.flags_EC, 31);
    ReadBitAttr(elem, 0x0CF2D02Bu, flags_EC, defaults.flags_EC, 30);
}

// -- ActorB15E60 -----------------------------------------------------------------
constexpr uint32_t kType_B15E60 = 0x10000;

struct ActorB15E60DefDesc : ClassDefDesc {
    explicit ActorB15E60DefDesc(const EntityDefDesc& loader) : ClassDefDesc(loader, kType_B15E60) {}
    uint32_t field_E0[3] = {};  // 0xE0 hashes 0x8CD5EC64, 0xF7DACE90, 0x4A104549
    // 0xEC attributes 0xCEA8E637, 0x06CD2139, 0x0DE78502, 0x0B0BD5E6, 0x285C1380.
    float field_EC[5] = {1.0f, 2.0f, 1.0f, 1.0f, 0.0f};

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001B2118 ActorB15E60DefDesc::Parse
// The first four values are not negative, and the first is at most the second.
void ActorB15E60DefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                               const EntityDef* parent)
{
    ActorDefDesc::Parse(root, elem, loader, parent);
    ActorB15E60DefDesc defaults(loader);
    if (const auto* other = SameClassParent<ActorB15E60Def>(parent, kType_B15E60)) {
        for (int i = 0; i < 3; ++i)
            defaults.field_E0[i] = other->field_130[i];
        for (int i = 0; i < 5; ++i)
            defaults.field_EC[i] = other->field_13C[i];
    }
    static constexpr uint32_t kHashAttrs[3] = {0x8CD5EC64u, 0xF7DACE90u, 0x4A104549u};
    static constexpr uint32_t kFloatAttrs[5] = {0xCEA8E637u, 0x06CD2139u, 0x0DE78502u, 0x0B0BD5E6u, 0x285C1380u};
    for (int i = 0; i < 3; ++i)
        elem->GetHash(kHashAttrs[i], &field_E0[i], defaults.field_E0[i]);
    for (int i = 0; i < 5; ++i)
        elem->GetFloat(kFloatAttrs[i], &field_EC[i], defaults.field_EC[i]);
    for (int i = 0; i < 4; ++i)
        field_EC[i] = std::max(field_EC[i], 0.0f);
    if (field_EC[1] < field_EC[0])
        field_EC[0] = field_EC[1];
}

} // namespace

// -- SignalActor -----------------------------------------------------------------
constexpr uint32_t kType_Signal = 0x40000;

struct SignalActorDefDesc : ClassDefDesc {
    explicit SignalActorDefDesc(const EntityDefDesc& loader) : ClassDefDesc(loader, kType_Signal) {}
    ape::Vec3 direction{0.0f, 0.0f, 1.0f, 0.0f};   // 0xE0 attribute 0xB6EFDEE3 (normalised)
    // 0xF0 attributes 0xD62F9754, 0x8A8E31BE, 0xC73B5193, 0x2BFE2EDD, 0x53CAF854,
    // 0x6A4BC424, 0xDF300BED.
    float field_F0[7] = {1.0f, 6.0f, 3.5f, 2.0f, 0.5f, 6.0f, 3.5f};
    float field_10C = 18.849556f;   // 0x10C attribute 0x26903F9C, degrees in the file
    float field_110 = 0.08726646f;  // 0x110 attribute 0xC3017FF2, degrees in the file
    float field_114 = 0.1f;         // 0x114 attribute 0xA63D55BC
    float field_118[3] = {};        // 0x118 attributes 0xAA8C2237, 0x59DEB9E0, 0x721A4F25
    // 0x124 top byte: attribute 0xF2D444A7 (one-based in the file; 0xFF none);
    // bits 23-20: 0x4D97C801, 0x3AF62E9E, 0x71C52C75, 0x66B71F2E.
    uint32_t flags_124 = 0xFF900000u;

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

namespace {

// @func 0x001AFA78 SignalActorDefDesc::Parse
void ParseSignalActorDefDesc(SignalActorDefDesc& desc, ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                             const EntityDefDesc& loader, const EntityDef* parent)
{
    desc.ActorDefDesc::Parse(root, elem, loader, parent);
    SignalActorDefDesc defaults(loader);
    if (const auto* signal = SameClassParent<SignalActorDef>(parent, kType_Signal)) {
        defaults.direction = signal->direction;
        for (int i = 0; i < 7; ++i)
            defaults.field_F0[i] = signal->field_140[i];
        defaults.field_10C = signal->field_15C;
        defaults.field_110 = signal->field_160;
        defaults.field_114 = signal->field_164;
        defaults.field_118[0] = signal->field_168;
        defaults.field_118[1] = signal->field_16C;
        defaults.field_118[2] = signal->field_170;
        defaults.flags_124 = signal->flags_174 & 0x00F00000u;
    }
    elem->GetVec3(0xB6EFDEE3u, &desc.direction.x, &defaults.direction.x);
    elem->GetFloat(0xD62F9754u, &desc.field_F0[0], defaults.field_F0[0]);
    const float lenSq = desc.direction.x * desc.direction.x + desc.direction.y * desc.direction.y +
                        desc.direction.z * desc.direction.z;
    if (lenSq >= 1e-05f) {
        const float inv = 1.0f / std::sqrt(lenSq);
        desc.direction.x *= inv;
        desc.direction.y *= inv;
        desc.direction.z *= inv;
    }
    static constexpr uint32_t kAttrs[6] = {0x8A8E31BEu, 0xC73B5193u, 0x2BFE2EDDu, 0x53CAF854u, 0x6A4BC424u,
                                           0xDF300BEDu};
    for (int i = 0; i < 6; ++i)
        elem->GetFloat(kAttrs[i], &desc.field_F0[i + 1], defaults.field_F0[i + 1]);
    constexpr float kDegToRad = 0.017453292f;
    if (elem->GetFloat(0x26903F9Cu, &desc.field_10C, defaults.field_10C) == ape::xml::kQuery_Ok)
        desc.field_10C *= kDegToRad;
    if (elem->GetFloat(0xC3017FF2u, &desc.field_110, defaults.field_110) == ape::xml::kQuery_Ok)
        desc.field_110 *= kDegToRad;
    elem->GetFloat(0xA63D55BCu, &desc.field_114, defaults.field_114);
    int32_t index = 0;
    elem->GetInt(0xF2D444A7u, &index, 0);
    desc.flags_124 = (desc.flags_124 & 0x00FFFFFFu) | (uint32_t(uint8_t(index - 1)) << 24);
    ReadBitAttr(elem, 0x4D97C801u, desc.flags_124, defaults.flags_124, 23);
    ReadBitAttr(elem, 0x3AF62E9Eu, desc.flags_124, defaults.flags_124, 22);
    ReadBitAttr(elem, 0x71C52C75u, desc.flags_124, defaults.flags_124, 21);
    ReadBitAttr(elem, 0x66B71F2Eu, desc.flags_124, defaults.flags_124, 20);
    elem->GetFloat(0xAA8C2237u, &desc.field_118[0], defaults.field_118[0]);
    elem->GetFloat(0x59DEB9E0u, &desc.field_118[1], defaults.field_118[1]);
    elem->GetFloat(0x721A4F25u, &desc.field_118[2], defaults.field_118[2]);
}

} // namespace

void SignalActorDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                               const EntityDef* parent)
{
    ParseSignalActorDefDesc(*this, root, elem, loader, parent);
}

// @func 0x001AF598 SignalActorDef::SignalActorDef
SignalActorDef::SignalActorDef(const SignalActorDefDesc& desc)
    : ActorDef(desc), direction(desc.direction), field_15C(desc.field_10C), field_160(desc.field_110),
      field_164(desc.field_114), field_168(desc.field_118[0]), field_16C(desc.field_118[1]),
      field_170(desc.field_118[2])
{
    for (int i = 0; i < 7; ++i)
        field_140[i] = desc.field_F0[i];
    flags_174 = (flags_174 & 0x000FFFFFu) | (desc.flags_124 & 0xFFF00000u);
}

// @func 0x001AA158 CreateActorGroupDef
EntityDef* CreateActorGroupDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                               const EntityDef* parent)
{
    ActorGroupDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new ActorGroupDef(desc);
    for (int i = 0; i < 4; ++i)
        def->field_130[i] = desc.field_E0[i];
    def->field_140 = desc.field_F0;
    delete desc.parts;
    return def;
}

// @func 0x001AB548 CreateHitCounterActorDef
EntityDef* CreateHitCounterActorDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                    const EntityDefDesc& loader, const EntityDef* parent)
{
    HitCounterActorDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new HitCounterActorDef(desc);
    for (int i = 0; i < 5; ++i)
        def->field_130[i] = desc.field_E0[i];
    def->field_144 = desc.field_F4;
    def->flags_148 = (def->flags_148 & 0x3FFFFFFFu) | (desc.flags_F8 & 0xC0000000u);
    delete desc.parts;
    return def;
}

// @func 0x001AC858 CreateWatchActorDef
EntityDef* CreateWatchActorDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                               const EntityDef* parent)
{
    WatchActorDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new WatchActorDef(desc);
    def->field_130 = desc.field_E0;
    def->field_134 = desc.field_E4;
    def->field_138 = desc.field_E8;
    def->flags_13C = (def->flags_13C & 0x3FFFFFFFu) | (desc.flags_EC & 0xC0000000u);
    delete desc.parts;
    return def;
}

// @func 0x001B0120 CreateSignalActorDef
EntityDef* CreateSignalActorDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                                const EntityDef* parent)
{
    SignalActorDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new SignalActorDef(desc);
    delete desc.parts;
    return def;
}

// @func 0x001B24D0 CreateActorB15E60Def
EntityDef* CreateActorB15E60Def(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                                const EntityDef* parent)
{
    ActorB15E60DefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new ActorB15E60Def(desc);
    for (int i = 0; i < 3; ++i)
        def->field_130[i] = desc.field_E0[i];
    for (int i = 0; i < 5; ++i)
        def->field_13C[i] = desc.field_EC[i];
    delete desc.parts;
    return def;
}

} // namespace entity
} // namespace sly
