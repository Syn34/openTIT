// openTIT - Factories of the actor definition classes: each builds its class
// description on top of the actor description (ActorDefDesc), reads its own
// attributes (taking the defaults from a "from" parent of the same class) and
// constructs the definition. The loader (EntityDefLoader.cpp) calls them by
// element name hash.
#include "sly/entity/ActorDefLoaders.h"

#include "sly/entity/ActorDefs.h"

#include "ape/math/ApeMath.h"
#include "ape/xml/ApeXml.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace sly {
namespace entity {

namespace {

// The description every class factory starts from: the actor defaults with
// the class's type bit set.
struct ClassDefDesc : ActorDefDesc {
    ClassDefDesc(const EntityDefDesc& loader, uint32_t classType) : ActorDefDesc(loader) { typeBits |= classType; }
    // Classes whose bit lives in the second type word (EntityDef::registryFlags).
    struct SecondWord {};
    ClassDefDesc(const EntityDefDesc& loader, uint32_t classType, SecondWord) : ActorDefDesc(loader)
    {
        registryFlags |= classType;
    }
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
template <class Def>
const Def* SameClassParent2(const EntityDef* parent, uint32_t classType)
{
    return parent && (parent->registryFlags & classType) ? static_cast<const Def*>(parent) : nullptr;
}

constexpr uint32_t kType_SoundActor = 0x80000;

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

// -- ObjectActor (the "Teleporter" element) ---------------------------------------
constexpr uint32_t kType2_ObjectActor = 0x2;

struct ObjectActorDefDesc : ClassDefDesc {
    explicit ObjectActorDefDesc(const EntityDefDesc& loader)
        : ClassDefDesc(loader, kType2_ObjectActor, SecondWord{}) {}
    // Nothing of its own: only the actor attributes (0x001EB118; a parent of
    // another class earns a warning, stripped).
    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent)
    {
        ActorDefDesc::Parse(root, elem, loader, parent);
    }
};

// -- InteractivePropA ------------------------------------------------------------
constexpr uint32_t kType2_InteractivePropA = 0x4;

struct InteractivePropAUseDefDesc : ClassDefDesc {
    explicit InteractivePropAUseDefDesc(const EntityDefDesc& loader)
        : ClassDefDesc(loader, kType2_InteractivePropA, SecondWord{}) {}
    uint32_t sharedUse = 0;     // 0xE0 index of the "type" value (only one value is known)

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001DBC28 InteractivePropAUseDefDesc::Parse
void InteractivePropAUseDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                       const EntityDefDesc& loader, const EntityDef* parent)
{
    ActorDefDesc::Parse(root, elem, loader, parent);
    const auto* prop = SameClassParent2<InteractivePropAUseDef>(parent, kType2_InteractivePropA);
    sharedUse = prop ? prop->sharedUse : 0;
    // "type" names one of the known values; anything else earns a warning (stripped).
    static constexpr uint32_t kTypes[1] = {0x03D3E3B0u};
    uint32_t type = 0;
    if (elem->GetHash(0x0027873Au, &type, 0) == ape::xml::kQuery_Ok) {
        for (uint32_t i = 0; i < 1; ++i) {
            if (type == kTypes[i]) {
                sharedUse = i;
                break;
            }
        }
    }
}

// Rotation quaternion from Euler angles (radians), as the definition parsers
// build it with the engine's fast sine and cosine.
ape::Vec4 QuatFromEuler(const ape::Vec3& angles)
{
    const float hx = angles.x * 0.5f, hy = angles.y * 0.5f, hz = angles.z * 0.5f;
    const float cx = ape::FastCos(hx), cy = ape::FastCos(hy), cz = ape::FastCos(hz);
    const float sx = ape::FastSin(hx), sy = ape::FastSin(hy), sz = ape::FastSin(hz);
    ape::Vec4 q;
    q.x = sy * sz * cx + cy * cz * sx;
    q.y = cz * sy * cx + cy * sz * sx;
    q.z = cy * sz * cx - cz * sy * sx;
    q.w = cy * cz * cx - sy * sz * sx;
    return q;
}

// -- Searchlight -----------------------------------------------------------------
constexpr uint32_t kType_Searchlight = 0x20000000;

struct SearchlightDefDesc : ClassDefDesc {
    explicit SearchlightDefDesc(const EntityDefDesc& loader) : ClassDefDesc(loader, kType_Searchlight) {}
    ape::Vec3 cameraOffset{};                       // 0xF0 attribute 0x81785FF8
    ape::Vec4 rotation{0.0f, 0.0f, 0.0f, 1.0f};     // 0x100 from attribute 0x91B5F872
    uint32_t field_110[4] = {};                     // 0x110
    float field_120 = 1.0f;                         // 0x120

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001E9F48 SearchlightDefDesc::Parse
void SearchlightDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                               const EntityDef* parent)
{
    ActorDefDesc::Parse(root, elem, loader, parent);
    SearchlightDefDesc defaults(loader);
    if (const auto* light = SameClassParent<SearchlightDef>(parent, kType_Searchlight)) {
        defaults.cameraOffset = light->cameraOffset;
        for (int i = 0; i < 4; ++i)
            defaults.field_110[i] = light->field_160[i];
        defaults.field_120 = light->field_170;
    }
    elem->GetVec3(0x81785FF8u, &cameraOffset.x, &defaults.cameraOffset.x);
    const ape::Vec3 zero{};
    ape::Vec3 angles{};
    elem->GetVec3(0x91B5F872u, &angles.x, &zero.x);
    static constexpr uint32_t kHashAttrs[4] = {0xC8168A44u, 0x942EEC95u, 0x7D68F822u, 0x4C4DB3D0u};
    for (int i = 0; i < 4; ++i)
        elem->GetHash(kHashAttrs[i], &field_110[i], defaults.field_110[i]);
    elem->GetFloat(0x35F1E76Eu, &field_120, defaults.field_120);
    rotation = QuatFromEuler(angles);
}

// -- TweenActor ------------------------------------------------------------------
constexpr uint32_t kType2_Tween = 0x8;

struct TweenActorDefDesc : ClassDefDesc {
    explicit TweenActorDefDesc(const EntityDefDesc& loader) : ClassDefDesc(loader, kType2_Tween, SecondWord{})
    {
        // NOTE: the console clears these damage scales here; the actor
        // parser then overwrites them with its own defaults.
        damage[0] = damage[2] = damage[4] = damage[7] = damage[9] = 0.0f;
    }
    uint32_t field_E0 = 0;      // 0xE0
    uint32_t field_E4 = 0;      // 0xE4
    ape::Vec3 field_F0{};       // 0xF0
    uint32_t field_100 = 0;     // 0x100
    uint32_t field_104 = 0;     // 0x104
    ape::Vec3 field_110{};      // 0x110
    ape::Vec3 field_120{};      // 0x120
    ape::Vec3 field_130{};      // 0x130
    ape::Vec3 field_140{};      // 0x140
    uint32_t field_150 = 0;     // 0x150
    uint32_t field_154 = 0;     // 0x154
    float field_158 = 0.0f;     // 0x158

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001EC7F8 TweenActorDefDesc::Parse
// Only the "type" index comes from a parent; the other attributes keep the
// description's values when missing.
void TweenActorDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                              const EntityDef* parent)
{
    ActorDefDesc::Parse(root, elem, loader, parent);
    const auto* tween = SameClassParent2<TweenActorDef>(parent, kType2_Tween);
    field_E0 = tween ? tween->field_130 : 0;
    static constexpr uint32_t kTypes[1] = {0xFE345D36u};
    uint32_t type = 0;
    if (elem->GetHash(0x0027873Au, &type, 0) == ape::xml::kQuery_Ok) {
        for (uint32_t i = 0; i < 1; ++i) {
            if (type == kTypes[i]) {
                field_E0 = i;
                break;
            }
        }
    }
    elem->GetHash(0x161851F3u, &field_E4, field_E4);
    elem->GetVec3(0xB8B33A30u, &field_F0.x, &field_F0.x);
    elem->GetHash(0xEFE3F2BEu, &field_100, field_100);
    elem->GetHash(0x80F96352u, &field_104, field_104);
    elem->GetVec3(0x43DAD405u, &field_110.x, &field_110.x);
    elem->GetVec3(0xE7590BDAu, &field_120.x, &field_120.x);
    elem->GetVec3(0x02528C1Cu, &field_130.x, &field_130.x);
    elem->GetVec3(0x0252939Fu, &field_140.x, &field_140.x);
    constexpr float kDegToRad = 0.017453292f;
    field_140.x *= kDegToRad;
    field_140.y *= kDegToRad;
    field_140.z *= kDegToRad;
    elem->GetHash(0x6350BDB4u, &field_150, field_150);
    elem->GetHash(0x04623C7Cu, &field_154, field_154);
    elem->GetFloat(0x81B5B597u, &field_158, field_158);
}

// -- InteractivePropB ------------------------------------------------------------
constexpr uint32_t kType2_InteractivePropB = 0x10;

struct InteractivePropBDefDesc : ClassDefDesc {
    explicit InteractivePropBDefDesc(const EntityDefDesc& loader)
        : ClassDefDesc(loader, kType2_InteractivePropB, SecondWord{})
    {
        // NOTE: overwritten by the actor parser (see TweenActorDefDesc).
        damage[0] = damage[2] = damage[4] = damage[7] = damage[9] = 0.0f;
    }
    uint32_t field_E0 = 0;                          // 0xE0
    ape::Vec3 field_F0{};                           // 0xF0
    ape::Vec4 rotation{0.0f, 0.0f, 0.0f, 1.0f};     // 0x100
    float field_110[3] = {};                        // 0x110
    uint32_t field_11C = 0;                         // 0x11C

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001F5938 InteractivePropBDefDesc::Parse
// No parent defaults: missing attributes keep the description's values.
void InteractivePropBDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                    const EntityDefDesc& loader, const EntityDef* parent)
{
    ActorDefDesc::Parse(root, elem, loader, parent);
    elem->GetHash(0x2899600Fu, &field_E0, field_E0);
    elem->GetVec3(0xEF4DDD13u, &field_F0.x, &field_F0.x);
    const ape::Vec3 zero{};
    ape::Vec3 degrees{};
    elem->GetVec3(0xC7400937u, &degrees.x, &zero.x);
    constexpr float kDegToRad = 0.017453292f;
    rotation = QuatFromEuler({degrees.x * kDegToRad, degrees.y * kDegToRad, degrees.z * kDegToRad, 0.0f});
    elem->GetFloat(0xDD61F47Cu, &field_110[0], field_110[0]);
    elem->GetFloat(0x3CC6000Du, &field_110[1], field_110[1]);
    elem->GetFloat(0x6AA47C5Au, &field_110[2], field_110[2]);
    elem->GetHash(0x6AA4B8C7u, &field_11C, field_11C);
}


} // namespace

// -- SoundActor (and the base of the sound actor classes) --------------------------

struct SoundActorDefDesc : ClassDefDesc {
    explicit SoundActorDefDesc(const EntityDefDesc& loader) : ClassDefDesc(loader, kType_SoundActor) {}
    SoundActorDef::SlotSound slots[7];  // 0xE0 sound hash and duration per slot
    ape::Vec3 soundOffset{};            // 0x120 attribute 0x22167A42
    uint32_t field_130 = 0;             // 0x130 hash 0x3A15BFAB
    uint32_t field_134 = 0;             // 0x134 hash 0xC03F596F
    float durationScale = 1.0f;         // 0x138 attribute 0x3E1A4D9C (negative: 1)
    uint8_t soundFlags = 0;             // 0x13C bit 7: attribute 0xBD75D672

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
    // The class fields of the definition.
    void CopyTo(SoundActorDef& def) const
    {
        for (int i = 0; i < 7; ++i)
            def.slotSounds[i] = slots[i];
        def.soundOffset = soundOffset;
        def.field_180 = field_130;
        def.field_184 = field_134;
        def.durationScale = durationScale;
        def.soundFlags = uint8_t((def.soundFlags & 0x7F) | (soundFlags & 0x80));
    }
};

// @func 0x001AD1F8 SoundActorDefDesc::Parse
// A sound actor parent passes on its slot durations (not its sounds), its
// sound offset, the two hashes, the duration scale and the flag.
void SoundActorDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                              const EntityDef* parent)
{
    ActorDefDesc::Parse(root, elem, loader, parent);
    SoundActorDefDesc defaults(loader);
    if (const auto* sound = SameClassParent<SoundActorDef>(parent, kType_SoundActor)) {
        for (int i = 0; i < 7; ++i)
            defaults.slots[i].duration = sound->slotSounds[i].duration;
        defaults.soundOffset = sound->soundOffset;
        defaults.field_130 = sound->field_180;
        defaults.field_134 = sound->field_184;
        defaults.durationScale = sound->durationScale;
        defaults.soundFlags = sound->soundFlags & 0x80;
    }
    // The shared fields go back to the loader's.
    name = loader.name;
    geomName = loader.geomName;
    templateName = loader.templateName;
    loadScope = loader.loadScope;

    elem->GetVec3(0x22167A42u, &soundOffset.x, &defaults.soundOffset.x);
    elem->GetHash(0x3A15BFABu, &field_130, defaults.field_130);
    elem->GetHash(0xC03F596Fu, &field_134, defaults.field_134);
    elem->GetFloat(0x3E1A4D9Cu, &durationScale, defaults.durationScale);
    if (durationScale < 0.0f)
        durationScale = 1.0f;
    static constexpr uint32_t kDurationAttrs[7] = {0xD5F932F0u, 0xEABE8E36u, 0x800ACF3Au, 0xF9217A56u,
                                                   0x7BA17740u, 0xA9262FCFu, 0xA88C3581u};
    static constexpr uint32_t kSoundAttrs[7] = {0x099867CCu, 0x20B43DD4u, 0x26F1B3DCu, 0x2F1729F4u,
                                                0x22885BE2u, 0x095F2EEDu, 0x314C79A3u};
    for (int i = 0; i < 7; ++i)
        elem->GetFloat(kDurationAttrs[i], &slots[i].duration, defaults.slots[i].duration);
    for (int i = 0; i < 7; ++i)
        elem->GetHash(kSoundAttrs[i], &slots[i].name, defaults.slots[i].name);
    bool flag = false;
    elem->GetBool(0xBD75D672u, &flag, (defaults.soundFlags & 0x80) != 0);
    soundFlags = uint8_t((soundFlags & 0x7F) | (flag ? 0x80 : 0));
}

// -- SoundActorB17728 ------------------------------------------------------------
constexpr uint32_t kType_SoundActorB17728 = 0x4000000;

struct SoundActorB17728DefDesc : SoundActorDefDesc {
    explicit SoundActorB17728DefDesc(const EntityDefDesc& loader) : SoundActorDefDesc(loader)
    {
        typeBits |= kType_SoundActorB17728;
    }
    uint32_t field_140 = 0;             // 0x140 hash 0x53FFAFC2
    float field_144 = 0.0f;             // 0x144 attribute 0x667AB4F5
    uint32_t requiredCharacter = 0;     // 0x148 hash 0xCD839910

    // 0x001D3B80
    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001D3B80 SoundActorB17728DefDesc::Parse
void SoundActorB17728DefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                    const EntityDefDesc& loader, const EntityDef* parent)
{
    SoundActorDefDesc::Parse(root, elem, loader, parent);
    uint32_t defaultHash = 0, defaultCharacter = 0;
    float defaultValue = 0.0f;
    if (const auto* other = SameClassParent<SoundActorB17728Def>(parent, kType_SoundActorB17728)) {
        defaultHash = other->field_190;
        defaultValue = other->field_194;
        defaultCharacter = other->requiredCharacter;
    }
    elem->GetHash(0x53FFAFC2u, &field_140, defaultHash);
    elem->GetFloat(0x667AB4F5u, &field_144, defaultValue);
    elem->GetHash(0xCD839910u, &requiredCharacter, defaultCharacter);
}

// @func 0x001D3A24 SoundActorB17728Def::SoundActorB17728Def
SoundActorB17728Def::SoundActorB17728Def(const SoundActorB17728DefDesc& desc)
    : SoundActorDef(desc), field_190(desc.field_140), field_194(desc.field_144),
      requiredCharacter(desc.requiredCharacter)
{
    desc.CopyTo(*this);
}

// -- SoundActorB174C0 (sentry light) ---------------------------------------------
constexpr uint32_t kType_SoundActorB174C0 = 0x8000000;

struct SoundActorB174C0DefDesc : SoundActorDefDesc {
    explicit SoundActorB174C0DefDesc(const EntityDefDesc& loader) : SoundActorDefDesc(loader)
    {
        typeBits |= kType_SoundActorB174C0;
    }
    uint32_t field_140 = 0;     // 0x140 hash 0x503616D9
    uint32_t field_144 = 0;     // 0x144 hash 0x5B85769D

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001CFF58 SoundActorB174C0DefDesc::Parse
void SoundActorB174C0DefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                    const EntityDefDesc& loader, const EntityDef* parent)
{
    SoundActorDefDesc::Parse(root, elem, loader, parent);
    const auto* other = SameClassParent<SoundActorB174C0Def>(parent, kType_SoundActorB174C0);
    elem->GetHash(0x503616D9u, &field_140, other ? other->field_190 : 0);
    elem->GetHash(0x5B85769Du, &field_144, other ? other->field_194 : 0);
}

// @func 0x001CFE00 SoundActorB174C0Def::SoundActorB174C0Def
SoundActorB174C0Def::SoundActorB174C0Def(const SoundActorB174C0DefDesc& desc)
    : SoundActorDef(desc), field_190(desc.field_140), field_194(desc.field_144)
{
    desc.CopyTo(*this);
}

// -- SoundActorB16678 (two-way switch) -------------------------------------------
constexpr uint32_t kType_SoundActorB16678 = 0x100000;

struct SoundActorB16678DefDesc : SoundActorDefDesc {
    explicit SoundActorB16678DefDesc(const EntityDefDesc& loader) : SoundActorDefDesc(loader)
    {
        typeBits |= kType_SoundActorB16678;
    }
    uint32_t position = 0;      // 0x140
    float field_144 = -1.0f;    // 0x144
    bool field_148 = false;     // 0x148
    bool field_149 = false;     // 0x149

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001BEA40 SoundActorB16678DefDesc::Parse
// The starting position is "OFF" or "ON" (anything else: off).
void SoundActorB16678DefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                    const EntityDefDesc& loader, const EntityDef* parent)
{
    SoundActorDefDesc::Parse(root, elem, loader, parent);
    SoundActorB16678DefDesc defaults(loader);
    if (const auto* other = SameClassParent<SoundActorB16678Def>(parent, kType_SoundActorB16678)) {
        defaults.position = other->field_190;
        defaults.field_144 = other->field_194;
        defaults.field_148 = other->field_198;
        defaults.field_149 = other->field_199;
    }
    static constexpr uint32_t kPositions[2] = {0x0001314Fu /* "OFF" */, 0x000009DFu /* "ON" */};
    position = defaults.position;
    uint32_t name = 0;
    if (elem->GetHash(0xC03F596Fu, &name, 0) == ape::xml::kQuery_Ok) {
        position = 0;
        for (uint32_t i = 0; i < 2; ++i) {
            if (kPositions[i] == name) {
                position = i;
                break;
            }
        }
    }
    elem->GetFloat(0xA6710C18u, &field_144, defaults.field_144);
    elem->GetBool(0xD28465E5u, &field_148, defaults.field_148);
    elem->GetBool(0xF0169515u, &field_149, defaults.field_149);
}

// @func 0x001BE8CC SoundActorB16678Def::SoundActorB16678Def
SoundActorB16678Def::SoundActorB16678Def(const SoundActorB16678DefDesc& desc)
    : SoundActorDef(desc), field_190(desc.position), field_194(desc.field_144), field_198(desc.field_148),
      field_199(desc.field_149)
{
    desc.CopyTo(*this);
}

// -- SoundActorB168E0 ------------------------------------------------------------
constexpr uint32_t kType_SoundActorB168E0 = 0x200000;

struct SoundActorB168E0DefDesc : SoundActorDefDesc {
    explicit SoundActorB168E0DefDesc(const EntityDefDesc& loader) : SoundActorDefDesc(loader)
    {
        typeBits |= kType_SoundActorB168E0;
    }
    int32_t field_140 = -1;     // 0x140
    uint32_t field_144 = 0;     // 0x144
    float field_148 = 0.0f;     // 0x148
    uint32_t flags_14C = 0;     // 0x14C bits 30/31

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001C02A8 SoundActorB168E0DefDesc::Parse
void SoundActorB168E0DefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                    const EntityDefDesc& loader, const EntityDef* parent)
{
    SoundActorDefDesc::Parse(root, elem, loader, parent);
    int32_t defaultIndex = -1;
    uint32_t defaultHash = 0;
    float defaultValue = 0.0f;
    uint32_t defaultFlags = 0x40000000u;
    if (const auto* other = SameClassParent<SoundActorB168E0Def>(parent, kType_SoundActorB168E0)) {
        defaultIndex = other->field_190;
        defaultHash = other->field_194;
        defaultValue = other->field_198;
        defaultFlags = other->flags_19C & 0xC0000000u;
    }
    elem->GetHash(0x2DF2E2B5u, &field_144, defaultHash);
    elem->GetFloat(0x08C08CA8u, &field_148, defaultValue);
    ReadBitAttr(elem, 0xD28465E5u, flags_14C, defaultFlags, 31);
    ReadBitAttr(elem, 0xD02B978Au, flags_14C, defaultFlags, 30);
    if (elem->GetInt(0xCBC57F7Du, &field_140, defaultIndex) == ape::xml::kQuery_Ok)
        field_140 = std::max(field_140 - 1, 0);
}

// @func 0x001C011C SoundActorB168E0Def::SoundActorB168E0Def
SoundActorB168E0Def::SoundActorB168E0Def(const SoundActorB168E0DefDesc& desc)
    : SoundActorDef(desc), field_190(desc.field_140), field_194(desc.field_144), field_198(desc.field_148)
{
    desc.CopyTo(*this);
    flags_19C = (flags_19C & 0x3FFFFFFFu) | (desc.flags_14C & 0xC0000000u);
}

// -- SoundActorB17008 (six trigger volumes) --------------------------------------
constexpr uint32_t kType_SoundActorB17008 = 0x800000;

struct SoundActorB17008DefDesc : SoundActorDefDesc {
    explicit SoundActorB17008DefDesc(const EntityDefDesc& loader) : SoundActorDefDesc(loader)
    {
        typeBits |= kType_SoundActorB17008;
    }
    SoundActorB17008Def::Volume volumes[SoundActorB17008Def::kVolumeCount];   // 0x140

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001C8138 SoundActorB17008DefDesc::Parse
// The volumes start cleared (all zero, not the volume defaults) or as the
// parent's; each "Trigger" child updates the next one.
void SoundActorB17008DefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                    const EntityDefDesc& loader, const EntityDef* parent)
{
    SoundActorDefDesc::Parse(root, elem, loader, parent);
    const auto* other = SameClassParent<SoundActorB17008Def>(parent, kType_SoundActorB17008);
    for (int i = 0; i < SoundActorB17008Def::kVolumeCount; ++i) {
        if (other) {
            volumes[i] = other->volumes[i];
        } else {
            volumes[i] = SoundActorB17008Def::Volume();
            std::memset(static_cast<void*>(&volumes[i]), 0, sizeof(volumes[i]));
        }
    }
    constexpr uint32_t kElem_Trigger = 0xEB9EE198u;   // "Trigger"
    constexpr float kDegToRad = 0.017453292f;
    int index = 0;
    for (ape::xml::XmlElement* child = elem->FirstChildElement(kElem_Trigger); child;
         child = child->NextSiblingElement(kElem_Trigger), ++index) {
        if (index >= SoundActorB17008Def::kVolumeCount)
            continue;
        SoundActorB17008Def::Volume& volume = volumes[index];
        child->GetHash(0x75849CFEu, &volume.field_00, volume.field_00);
        child->GetVec3(0x2B7B3EB4u, &volume.field_10.x, &volume.field_10.x);
        child->GetVec3(0x313AB0C1u, &volume.field_20.x, &volume.field_20.x);
        const ape::Vec3 zero{};
        ape::Vec3 degrees{};
        if (child->GetVec3(0x313AB844u, &degrees.x, &zero.x) == ape::xml::kQuery_Ok)
            volume.rotation =
                QuatFromEuler({degrees.x * kDegToRad, degrees.y * kDegToRad, degrees.z * kDegToRad, 0.0f});
        child->GetBool(0x8141EC8Au, &volume.field_40, volume.field_40);
        child->GetBool(0xF8E455ACu, &volume.field_41, volume.field_41);
        // NOTE: the console parses the "WindType" attribute here into a
        // temporary and drops it.
        child->GetFloat(0x7C8F6049u, &volume.field_50, volume.field_50);
        child->GetFloat(0x8FA5922Du, &volume.field_48, volume.field_48);
        child->GetFloat(0x3C8207B4u, &volume.field_4C, volume.field_4C);
        child->GetFloat(0x8C014439u, &volume.field_54, volume.field_54);
    }
}

// @func 0x001C7F28 SoundActorB17008Def::SoundActorB17008Def
SoundActorB17008Def::SoundActorB17008Def(const SoundActorB17008DefDesc& desc) : SoundActorDef(desc)
{
    desc.CopyTo(*this);
    for (int i = 0; i < kVolumeCount; ++i)
        volumes[i] = desc.volumes[i];
}

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

// @func 0x001EB2B8 CreateObjectActorDef
EntityDef* CreateObjectActorDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                                const EntityDef* parent)
{
    ObjectActorDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new ObjectActorDef(desc);
    delete desc.parts;
    return def;
}

// @func 0x001DBE80 CreateInteractivePropAUseDef
EntityDef* CreateInteractivePropAUseDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                        const EntityDefDesc& loader, const EntityDef* parent)
{
    InteractivePropAUseDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new InteractivePropAUseDef(desc);
    def->sharedUse = desc.sharedUse;
    delete desc.parts;
    return def;
}

// @func 0x001EA3B8 CreateSearchlightDef
EntityDef* CreateSearchlightDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                                const EntityDef* parent)
{
    SearchlightDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new SearchlightDef(desc);
    def->cameraOffset = desc.cameraOffset;
    def->field_150 = desc.rotation;
    for (int i = 0; i < 4; ++i)
        def->field_160[i] = desc.field_110[i];
    def->field_170 = desc.field_120;
    delete desc.parts;
    return def;
}

// @func 0x001ECD08 CreateTweenActorDef
EntityDef* CreateTweenActorDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                               const EntityDef* parent)
{
    TweenActorDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new TweenActorDef(desc);
    def->field_130 = desc.field_E0;
    def->field_134 = desc.field_E4;
    def->field_140 = desc.field_F0;
    def->field_150 = desc.field_100;
    def->field_154 = desc.field_104;
    def->field_160 = desc.field_110;
    def->field_170 = desc.field_120;
    def->field_180 = desc.field_130;
    def->field_190 = desc.field_140;
    def->field_1A0 = desc.field_150;
    def->field_1A4 = desc.field_154;
    def->field_1A8 = desc.field_158;
    delete desc.parts;
    return def;
}

// @func 0x001F5BA8 CreateInteractivePropBDef
EntityDef* CreateInteractivePropBDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                     const EntityDefDesc& loader, const EntityDef* parent)
{
    InteractivePropBDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new InteractivePropBDef(desc);
    def->field_130 = desc.field_E0;
    def->field_140 = desc.field_F0;
    def->rotation = desc.rotation;
    for (int i = 0; i < 3; ++i)
        def->field_160[i] = desc.field_110[i];
    def->field_16C = desc.field_11C;
    delete desc.parts;
    return def;
}

// @func 0x001AD7F0 CreateSoundActorDef
EntityDef* CreateSoundActorDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                               const EntityDef* parent)
{
    SoundActorDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new SoundActorDef(desc);
    desc.CopyTo(*def);
    delete desc.parts;
    return def;
}

// @func 0x001D3E58 CreateSoundActorB17728Def
EntityDef* CreateSoundActorB17728Def(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                     const EntityDefDesc& loader, const EntityDef* parent)
{
    SoundActorB17728DefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new SoundActorB17728Def(desc);
    delete desc.parts;
    return def;
}

// @func 0x001D0208 CreateSoundActorB174C0Def
EntityDef* CreateSoundActorB174C0Def(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                     const EntityDefDesc& loader, const EntityDef* parent)
{
    SoundActorB174C0DefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new SoundActorB174C0Def(desc);
    delete desc.parts;
    return def;
}

// @func 0x001BED98 CreateSoundActorB16678Def
EntityDef* CreateSoundActorB16678Def(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                     const EntityDefDesc& loader, const EntityDef* parent)
{
    SoundActorB16678DefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new SoundActorB16678Def(desc);
    delete desc.parts;
    return def;
}

// @func 0x001C0658 CreateSoundActorB168E0Def
EntityDef* CreateSoundActorB168E0Def(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                     const EntityDefDesc& loader, const EntityDef* parent)
{
    SoundActorB168E0DefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new SoundActorB168E0Def(desc);
    delete desc.parts;
    return def;
}

// @func 0x001C8780 CreateSoundActorB17008Def
EntityDef* CreateSoundActorB17008Def(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                     const EntityDefDesc& loader, const EntityDef* parent)
{
    SoundActorB17008DefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new SoundActorB17008Def(desc);
    delete desc.parts;
    return def;
}

} // namespace entity
} // namespace sly
