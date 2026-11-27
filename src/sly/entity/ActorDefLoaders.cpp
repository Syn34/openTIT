// openTIT - Factories of the actor definition classes: each builds its class
// description on top of the actor description (ActorDefDesc), reads its own
// attributes (taking the defaults from a "from" parent of the same class) and
// constructs the definition. The loader (EntityDefLoader.cpp) calls them by
// element name hash.
#include "sly/entity/ActorDefLoaders.h"
#include "ape/core/ApeString.h"

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
    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001EB118 ObjectActorDefDesc::Parse
// Nothing of its own: only the actor attributes (a parent of another class
// earns a warning, stripped).
void ObjectActorDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                               const EntityDef* parent)
{
    ActorDefDesc::Parse(root, elem, loader, parent);
}

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

// -- Safe --------------------------------------------------------------------------
namespace {
constexpr uint32_t kType_Safe = 0x10000000;
} // namespace

struct SafeDefDesc : ClassDefDesc {
    explicit SafeDefDesc(const EntityDefDesc& loader) : ClassDefDesc(loader, kType_Safe) {}
    uint32_t hashes[10] = {};           // 0xE0..0x104
    float field_108 = 0.0f;             // 0x108
    ape::Vec3 field_110{};              // 0x110
    float floats[7] = {55.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};   // 0x120..0x138
    uint32_t hashes13C[10] = {};        // 0x13C..0x160
    ape::Vec3 field_170{};              // 0x170
    uint32_t hashes180[3] = {};         // 0x180..0x188
    float field_18C = 4.0f;             // 0x18C

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001E6300 SafeDefDesc::Parse
// A safe parent passes on everything but the two vectors.
void SafeDefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                        const EntityDef* parent)
{
    ActorDefDesc::Parse(root, elem, loader, parent);
    SafeDefDesc defaults(loader);
    if (const auto* safe = SameClassParent<SafeDef>(parent, kType_Safe)) {
        for (int i = 0; i < 3; ++i) {
            defaults.hashes[i] = safe->dialNames[i];
            defaults.hashes[3 + i] = safe->highlightNames[i];
            defaults.hashes[6 + i] = safe->solvedNames[i];
            defaults.floats[4 + i] = safe->dialSmoothing[i];
            defaults.hashes13C[1 + i] = safe->tickSounds[i];
            defaults.hashes13C[4 + i] = safe->solvedSounds[i];
        }
        defaults.hashes[9] = safe->openObjectName;
        defaults.field_108 = safe->tolerance;
        defaults.floats[0] = safe->cameraFov;
        defaults.floats[1] = safe->cameraBlend;
        defaults.floats[2] = safe->timeLimit;
        defaults.floats[3] = safe->openDelay;
        defaults.hashes13C[0] = safe->soundBank;
        defaults.hashes13C[7] = safe->failSound;
        defaults.hashes13C[8] = safe->openSound;
        defaults.hashes13C[9] = safe->rewardDefName;
        defaults.hashes180[0] = safe->openScript;
        defaults.hashes180[1] = safe->timerHudName;
        defaults.hashes180[2] = safe->promptHudName;
        defaults.field_18C = safe->turnSpeed;
    }
    static constexpr uint32_t kHashAttrs[10] = {0xCA9B378Eu, 0x07AB8180u, 0xCAF309B3u, 0x00F161A9u, 0xDA59999Bu,
                                                0x4B3B3ECEu, 0xA4C980A9u, 0xF86846F7u, 0xA3BB4824u, 0x1F114A8Au};
    static constexpr uint32_t kFloatAttrs[7] = {0x91B5CB68u, 0x462137C5u, 0xCA7EA426u, 0x4A3F3DB5u,
                                                0x19C09DDEu, 0xC55EF4EBu, 0x6AF36019u};
    static constexpr uint32_t kHash13CAttrs[10] = {0x3A15BFABu, 0xDB732E22u, 0x5977F5AFu, 0xDB1B5BFDu,
                                                   0xCD553C2Au, 0x3B15E5B7u, 0xCCFD6A05u, 0x098591A0u,
                                                   0x3A1BDFB9u, 0x06556689u};
    static constexpr uint32_t kHash180Attrs[3] = {0x675C8395u, 0x8DAC3405u, 0x07E04656u};
    for (int i = 0; i < 10; ++i)
        elem->GetHash(kHashAttrs[i], &hashes[i], defaults.hashes[i]);
    elem->GetFloat(0x16DD189Au, &field_108, defaults.field_108);
    elem->GetVec3(0x81785FF8u, &field_110.x, &defaults.field_110.x);
    for (int i = 0; i < 7; ++i)
        elem->GetFloat(kFloatAttrs[i], &floats[i], defaults.floats[i]);
    for (int i = 0; i < 10; ++i)
        elem->GetHash(kHash13CAttrs[i], &hashes13C[i], defaults.hashes13C[i]);
    elem->GetVec3(0xC88E4511u, &field_170.x, &defaults.field_170.x);
    for (int i = 0; i < 3; ++i)
        elem->GetHash(kHash180Attrs[i], &hashes180[i], defaults.hashes180[i]);
    elem->GetFloat(0x3EF33B23u, &field_18C, defaults.field_18C);
}

// @func 0x001E5170 SafeDef::SafeDef
SafeDef::SafeDef(const SafeDefDesc& desc) : ActorDef(desc)
{
    for (int i = 0; i < 3; ++i) {
        dialNames[i] = desc.hashes[i];
        highlightNames[i] = desc.hashes[3 + i];
        solvedNames[i] = desc.hashes[6 + i];
    }
    openObjectName = desc.hashes[9];
    tolerance = desc.field_108;
    cameraOffset = desc.field_110;
    cameraFov = desc.floats[0];
    cameraBlend = desc.floats[1];
    timeLimit = desc.floats[2];
    openDelay = desc.floats[3];
    for (int i = 0; i < 3; ++i)
        dialSmoothing[i] = desc.floats[4 + i];
    soundBank = desc.hashes13C[0];
    for (int i = 0; i < 3; ++i) {
        tickSounds[i] = desc.hashes13C[1 + i];
        solvedSounds[i] = desc.hashes13C[4 + i];
    }
    failSound = desc.hashes13C[7];
    openSound = desc.hashes13C[8];
    rewardDefName = desc.hashes13C[9];
    for (int i = 0; i < 3; ++i)
        rewardOffset[i] = (&desc.field_170.x)[i];
    openScript = desc.hashes180[0];
    timerHudName = desc.hashes180[1];
    promptHudName = desc.hashes180[2];
    turnSpeed = desc.field_18C;
}

// -- InteractivePropC (the player turret prop; definition "InteractivePropADef") ----
namespace {
constexpr uint32_t kType_TurretProp = 0x40000000;
} // namespace

struct InteractivePropADefDesc : ClassDefDesc {
    explicit InteractivePropADefDesc(const EntityDefDesc& loader) : ClassDefDesc(loader, kType_TurretProp) {}
    ~InteractivePropADefDesc() { ape::FreeIfNotNull(buffer); }
    InteractivePropADefDesc(const InteractivePropADefDesc&) = delete;
    InteractivePropADefDesc& operator=(const InteractivePropADefDesc&) = delete;
    uint32_t field_E0 = 0;                          // 0xE0
    ape::Vec3 field_F0{};                           // 0xF0
    uint32_t field_100 = 0;                         // 0x100
    ape::Vec3 aimOffset{};                          // 0x110
    uint32_t field_128 = 0;                         // 0x128
    uint32_t field_12C = 0;                         // 0x12C
    ape::Vec3 field_130{};                          // 0x130
    ape::Vec4 rotation140{0.0f, 0.0f, 0.0f, 1.0f};  // 0x140
    bool field_150 = false;                         // 0x150
    bool field_151 = false;                         // 0x151
    uint32_t field_154 = 0;                         // 0x154
    ape::Vec3 field_160{};                          // 0x160
    ape::Vec4 rotation170{0.0f, 0.0f, 0.0f, 1.0f};  // 0x170
    float floats180[4] = {};                        // 0x180
    uint32_t field_190 = 0;                         // 0x190
    float floats194[7] = {};                        // 0x194
    bool field_1B0 = true;                          // 0x1B0
    bool field_1B1 = false;                         // 0x1B1
    // 0x1B4..0x1FC; the last four are angles kept in radians (0.25 and 2
    // degrees by default).
    float floats1B4[19] = {0.0f, -90.0f, 90.0f, -180.0f, 180.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                           0.0f, 0.0f, -1.0f, -1.0f, -1.0f, 0.004363323f, 0.034906585f, 0.004363323f,
                           0.034906585f};
    uint32_t mode = 0;                              // 0x200
    uint32_t bufferSize = 0;                        // 0x204
    char* buffer = nullptr;                         // 0x208
    uint32_t hudNames[3] = {};                      // 0x20C
    bool bools218[3] = {};                          // 0x218
    float floats21C[3] = {-1.0f, -1.0f, -1.0f};     // 0x21C
    float field_228[2] = {};                        // 0x228 radians

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001DD3F8 InteractivePropADefDesc::Parse
// A turret prop parent passes on everything but the vectors, rotations,
// mode and placement angles (its player name is not inherited either).
// Seven of the floats are kept in [0, 1].
void InteractivePropADefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                    const EntityDefDesc& loader, const EntityDef* parent)
{
    ActorDefDesc::Parse(root, elem, loader, parent);
    InteractivePropADefDesc defaults(loader);
    if (const auto* prop = SameClassParent<InteractivePropADef>(parent, kType_TurretProp)) {
        defaults.field_E0 = prop->frameObjectName;
        defaults.field_100 = prop->field_150;
        defaults.field_128 = prop->field_178;
        defaults.field_12C = prop->field_17C;
        defaults.field_150 = prop->field_1A0;
        defaults.field_151 = prop->field_1A1;
        defaults.field_154 = prop->field_1A4;
        for (int i = 0; i < 4; ++i)
            defaults.floats180[i] = prop->floats1D0[i];
        defaults.field_190 = prop->field_1E0;
        for (int i = 0; i < 7; ++i)
            defaults.floats194[i] = prop->floats1E4[i];
        defaults.field_1B0 = prop->field_200;
        defaults.field_1B1 = prop->field_201;
        for (int i = 0; i < 19; ++i)
            defaults.floats1B4[i] = prop->floats204[i];
        for (int i = 0; i < 3; ++i) {
            defaults.hudNames[i] = prop->hudNames[i];
            defaults.bools218[i] = prop->bools268[i];
            defaults.floats21C[i] = prop->floats26C[i];
        }
    }
    constexpr float kDegToRad = 0.017453292f;
    const ape::Vec3 zero{};
    elem->GetHash(0x0A2E921Eu, &field_E0, defaults.field_E0);
    elem->GetVec3(0x81785FF8u, &field_F0.x, &defaults.field_F0.x);
    elem->GetHash(0x7242FC8Fu, &field_100, defaults.field_100);
    elem->GetVec3(0xCC0D6864u, &aimOffset.x, &defaults.aimOffset.x);
    elem->GetHash(0xFC8AFCD5u, &field_128, defaults.field_128);
    elem->GetHash(0xA7E293B8u, &field_12C, defaults.field_12C);
    elem->GetVec3(0x3818B318u, &field_130.x, &defaults.field_130.x);
    ape::Vec3 degrees{};
    elem->GetVec3(0x898B7952u, &degrees.x, &zero.x);
    rotation140 = QuatFromEuler({degrees.x * kDegToRad, degrees.y * kDegToRad, degrees.z * kDegToRad, 0.0f});
    elem->GetBool(0x2A654FC3u, &field_150, defaults.field_150);
    elem->GetBool(0xDA165A53u, &field_151, defaults.field_151);
    elem->GetHash(0x753EE781u, &field_154, defaults.field_154);
    elem->GetVec3(0x1FAF38A1u, &field_160.x, &defaults.field_160.x);
    degrees = {};
    elem->GetVec3(0x66E10869u, &degrees.x, &zero.x);
    rotation170 = QuatFromEuler({degrees.x * kDegToRad, degrees.y * kDegToRad, degrees.z * kDegToRad, 0.0f});
    elem->GetFloat(0x2C50D8F5u, &floats180[0], defaults.floats180[0]);
    floats180[0] = std::max(floats180[0], 0.0f);
    elem->GetFloat(0xA35DB66Eu, &floats180[1], defaults.floats180[1]);
    floats180[1] = std::max(floats180[1], floats180[0]);
    elem->GetFloat(0xA21AA8A9u, &floats180[2], defaults.floats180[2]);
    floats180[2] = std::max(floats180[2], 0.0f);
    elem->GetFloat(0x295E99EEu, &floats180[3], defaults.floats180[3]);
    floats180[3] = std::max(floats180[3], 0.0f);
    elem->GetHash(0x4205E7F3u, &field_190, defaults.field_190);
    static constexpr uint32_t kAttrs194[7] = {0xA771B991u, 0xD12B2C33u, 0xA5972CE6u, 0xB09C9890u,
                                              0x4314A942u, 0x4CEF43B4u, 0x471C7A06u};
    for (int i = 0; i < 7; ++i)
        elem->GetFloat(kAttrs194[i], &floats194[i], defaults.floats194[i]);
    elem->GetBool(0xEC4A7369u, &field_1B0, defaults.field_1B0);
    elem->GetBool(0x6661DA37u, &field_1B1, defaults.field_1B1);
    static constexpr uint32_t kAttrs1B4[19] = {
        0xEADC4688u, 0xDE05302Eu, 0x47E3CC5Cu, 0x878EA6DDu, 0x8722768Bu, 0x4CB13E10u, 0x824119DCu,
        0x2CA6B8AFu, 0x92AC6682u, 0x24EC3F49u, 0x093E0E77u, 0x183B44B4u, 0x99479EADu, 0x172CC986u,
        0x810B65B4u, 0x64AB749Eu, 0xD617D519u, 0xC209726Fu, 0x99F8C3AAu};
    for (int i = 0; i < 19; ++i) {
        const bool read = elem->GetFloat(kAttrs1B4[i], &floats1B4[i], defaults.floats1B4[i]) == ape::xml::kQuery_Ok;
        if (read && i >= 15)
            floats1B4[i] *= kDegToRad;
    }
    uint32_t modeName = 0;
    if (elem->GetHash(0x79545A55u, &modeName, 0) == ape::xml::kQuery_Ok) {
        static constexpr uint32_t kModes[2] = {0x0024A738u, 0xE01ED3CBu};   // "None", ?
        for (uint32_t i = 0; i < 2; ++i) {
            if (kModes[i] == modeName) {
                mode = i;
                break;
            }
        }
    }
    if (const char* playerName = elem->GetString("playerName")) {
        ape::FreeIfNotNull(buffer);
        bufferSize = uint32_t(std::strlen(playerName)) + 1;
        buffer = static_cast<char*>(ape::Malloc(bufferSize));
        ape::StrCopyBounded(buffer, playerName, bufferSize);
    }
    static constexpr uint32_t kHudAttrs[3] = {0x6350BDB4u, 0xA2E0E877u, 0x6A024DD2u};
    static constexpr uint32_t kBoolAttrs[3] = {0x8F16DBE7u, 0x3FF0E768u, 0xF29E7002u};
    static constexpr uint32_t kSizeAttrs[3] = {0x1F8A5F77u, 0x17C27FD0u, 0x81A11BFEu};
    for (int i = 0; i < 3; ++i)
        elem->GetHash(kHudAttrs[i], &hudNames[i], defaults.hudNames[i]);
    for (int i = 0; i < 3; ++i)
        elem->GetBool(kBoolAttrs[i], &bools218[i], defaults.bools218[i]);
    for (int i = 0; i < 3; ++i)
        elem->GetFloat(kSizeAttrs[i], &floats21C[i], defaults.floats21C[i]);
    if (elem->GetVec2(0xBF3F0DB1u, field_228, defaults.field_228) == ape::xml::kQuery_Ok) {
        field_228[0] *= kDegToRad;
        field_228[1] *= kDegToRad;
    }
    for (int i = 5; i <= 11; ++i)
        floats1B4[i] = std::clamp(floats1B4[i], 0.0f, 1.0f);
}

// @func 0x001DC5F8 InteractivePropADef::InteractivePropADef
InteractivePropADef::InteractivePropADef(const InteractivePropADefDesc& desc)
    : ActorDef(desc), frameObjectName(desc.field_E0), field_140(desc.field_F0), field_150(desc.field_100),
      aimOffset(desc.aimOffset), field_178(desc.field_128), field_17C(desc.field_12C), field_180(desc.field_130),
      rotation190(desc.rotation140), field_1A0(desc.field_150), field_1A1(desc.field_151),
      field_1A4(desc.field_154), field_1B0(desc.field_160), rotation1C0(desc.rotation170),
      field_1E0(desc.field_190), field_200(desc.field_1B0), field_201(desc.field_1B1), mode(desc.mode),
      bufferSize(desc.bufferSize)
{
    for (int i = 0; i < 4; ++i)
        floats1D0[i] = desc.floats180[i];
    for (int i = 0; i < 7; ++i)
        floats1E4[i] = desc.floats194[i];
    for (int i = 0; i < 19; ++i)
        floats204[i] = desc.floats1B4[i];
    for (int i = 0; i < 3; ++i) {
        hudNames[i] = desc.hudNames[i];
        bools268[i] = desc.bools218[i];
        floats26C[i] = desc.floats21C[i];
    }
    field_278[0] = desc.field_228[0];
    field_278[1] = desc.field_228[1];
    if (bufferSize) {
        buffer = static_cast<char*>(ape::Malloc(bufferSize));
        ape::StrCopyBounded(buffer, desc.buffer, bufferSize);
    }
}

// -- ActorB17990 (the input mover) -------------------------------------------------
namespace {
constexpr uint32_t kType2_InputMover = 0x1;
} // namespace

struct ActorB17990DefDesc : ClassDefDesc {
    explicit ActorB17990DefDesc(const EntityDefDesc& loader)
        : ClassDefDesc(loader, kType2_InputMover, SecondWord{})
    {
    }
    char hudPrompt[0x20] = {};          // 0xDE
    ape::Vec3 field_100{};              // 0x100
    ape::Vec3 field_110{};              // 0x110
    ape::Vec3 field_120{};              // 0x120
    ape::Vec3 field_130{};              // 0x130
    float floats140[5] = {50.0f, 0.0f, 0.5f, 0.5f, -1.0f};   // 0x140..0x150
    InputMoverControl control;          // 0x154
    uint32_t field_198 = 0;             // 0x198
    uint32_t moverFlags = 0x06000000u;  // 0x19C

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
    // One control child element (buttons, stick or the third mode).
    void ParseControl(ape::xml::XmlElement* child, const InputMoverControl& defaults);
};

// @func 0x001D6164 InputButtonMask
uint32_t InputButtonMask(uint32_t buttonName)
{
    switch (buttonName) {
    case 0x788F788Au: return 0x0001;
    case 0x928BCAD1u: return 0x0002;
    case 0x928F4636u: return 0x0004;
    case 0xBFAFE1ADu: return 0x0008;
    case 0x87EBFA08u: return 0x0010;
    case 0x923F4D1Du: return 0x0020;
    case 0x767FB0D0u: return 0x0040;
    case 0x03D6A020u: return 0x0080;
    case 0x00000965u: return 0x0100;
    case 0x00000A1Fu: return 0x0200;
    case 0x00000966u: return 0x0400;
    case 0x00000A20u: return 0x0800;
    case 0x00000967u: return 0x1000;
    case 0x00000A21u: return 0x2000;
    default:          return 0x0040;
    }
}

void ActorB17990DefDesc::ParseControl(ape::xml::XmlElement* child, const InputMoverControl& defaults)
{
    auto setBit = [](uint32_t& word, uint32_t bit, bool on) { word = on ? (word | bit) : (word & ~bit); };
    switch (child->GetNameHash()) {
    case 0x3257217Bu: {   // buttons
        control.press.buttons = defaults.press.buttons;
        uint32_t button = 0;
        if (child->GetHash(0x3257B96Cu, &button, 0) == ape::xml::kQuery_Ok)
            control.press.buttons = InputButtonMask(button);
        control.mode = InputMoverControl::kMode_Buttons;
        child->GetFloat(0xE1555935u, &control.press.field_14, defaults.press.field_14);
        child->GetFloat(0x636A522Bu, &control.press.field_18, defaults.press.field_18);
        child->GetFloat(0xE18D39E5u, &control.press.field_1C, defaults.press.field_1C);
        bool flag = false;
        child->GetBool(0x0E0CA78Cu, &flag, (defaults.press.flags24 & 0x80000000u) != 0);
        setBit(control.press.flags24, 0x80000000u, flag);
        control.press.field_18 = std::max(control.press.field_18, 0.0f);
        child->GetFloat(0x0C28FD40u, &control.press.field_20, defaults.press.field_20);
        break;
    }
    case 0x89F23F47u: {   // the third mode
        control.mode = InputMoverControl::kMode_Mode3;
        float range = 0.0f;
        child->GetFloat(0x82E789DBu, &range, 0.0f);
        control.mode3.rangeSq = range * range;
        child->GetFloat(0x02D738DFu, &control.mode3.field_14, defaults.mode3.field_14);
        setBit(control.mode3.flags18, 0x80000000u, (defaults.mode3.flags18 & 0x80000000u) != 0);
        uint32_t value = 0;
        if (child->GetHash(0x04B8E870u, &value, 0) == ape::xml::kQuery_Ok) {
            if (value == 0x00239807u)
                control.mode3.flags18 &= ~0x80000000u;
            else if (value == 0x04A5C9FCu)
                control.mode3.flags18 |= 0x80000000u;
        }
        setBit(control.mode3.flags18, 0x40000000u, (defaults.mode3.flags18 & 0x40000000u) != 0);
        if (child->GetHash(0x906DDB56u, &value, 0) == ape::xml::kQuery_Ok) {
            if (value == 0x00000874u)
                control.mode3.flags18 |= 0x40000000u;
            else if (value == 0x000103F7u)
                control.mode3.flags18 &= ~0x40000000u;
        }
        break;
    }
    case 0x89F1FF19u: {   // stick
        control.stick.stick = defaults.stick.stick;
        uint32_t stickName = 0;
        if (child->GetHash(0xB456B7AAu, &stickName, 0) == ape::xml::kQuery_Ok) {
            switch (stickName) {
            case 0x46BDBD35u: control.stick.stick = 1; break;
            case 0x842320BBu: control.stick.stick = 3; break;
            case 0x842320BAu: control.stick.stick = 2; break;
            default:          control.stick.stick = 0; break;
            }
        }
        control.mode = InputMoverControl::kMode_Stick;
        static constexpr uint32_t kValueAttrs[11] = {0x82E789DBu, 0x0BF5ADB7u, 0x96CB0082u, 0x3CE11DA0u,
                                                     0x46FFF512u, 0x31D7B193u, 0x036A8EAFu, 0x4C005DE1u,
                                                     0xA4D3EB03u, 0x57557B45u, 0x8C06F037u};
        for (int i = 0; i < 11; ++i)
            child->GetFloat(kValueAttrs[i], &control.stick.values[i], defaults.stick.values[i]);
        static constexpr uint32_t kFlagAttrs[3] = {0xF6866423u, 0x2443C7A5u, 0x153912A7u};
        for (int i = 0; i < 3; ++i) {
            const uint32_t bit = 0x80000000u >> i;
            bool flag = false;
            child->GetBool(kFlagAttrs[i], &flag, (defaults.stick.flags40 & bit) != 0);
            setBit(control.stick.flags40, bit, flag);
        }
        control.stick.values[0] = std::max(control.stick.values[0], 0.0f);
        control.stick.values[1] = std::max(control.stick.values[1], 0.0f);
        break;
    }
    default:
        break;
    }
}

// @func 0x001D62A8 ActorB17990DefDesc::Parse
// An input mover parent passes on its floats, control block, hash and flag
// bits 26-31 (not its vectors or prompt). The control comes from the last
// control child element.
void ActorB17990DefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                               const EntityDef* parent)
{
    ActorDefDesc::Parse(root, elem, loader, parent);
    ActorB17990DefDesc defaults(loader);
    if (const auto* mover = SameClassParent2<ActorB17990Def>(parent, kType2_InputMover)) {
        defaults.floats140[0] = mover->field_190;
        defaults.floats140[1] = mover->field_194;
        defaults.floats140[2] = mover->field_198;
        defaults.floats140[3] = mover->field_19C;
        defaults.floats140[4] = mover->field_1A0;
        defaults.field_198 = mover->field_1E8;
        defaults.control = mover->control;
        defaults.moverFlags = (defaults.moverFlags & 0x03FFFFFFu) | (mover->moverFlags & 0xFC000000u);
    }
    auto readBit = [&](uint32_t attr, uint32_t bit) {
        bool on = false;
        elem->GetBool(attr, &on, (defaults.moverFlags & bit) != 0);
        moverFlags = on ? (moverFlags | bit) : (moverFlags & ~bit);
    };
    constexpr float kDegToRad = 0.017453292f;
    elem->GetVec3(0x3665776Au, &field_100.x, &defaults.field_100.x);
    elem->GetVec3(0xA21F9D02u, &field_120.x, &defaults.field_120.x);
    static constexpr uint32_t kFloatAttrs[5] = {0x760CBA9Eu, 0x176EDFCFu, 0x440242A6u, 0x7D51873Bu, 0x36EF44EBu};
    for (int i = 0; i < 5; ++i)
        elem->GetFloat(kFloatAttrs[i], &floats140[i], defaults.floats140[i]);
    elem->GetFloat(0x11AA6429u, &control.field_00, defaults.control.field_00);
    elem->GetFloat(0x26A56A39u, &control.field_04, defaults.control.field_04);
    elem->GetFloat(0xCB13F34Du, &control.field_08, defaults.control.field_08);
    control.field_08 = std::max(control.field_08, 0.0f);
    elem->GetHash(0xCC4EF294u, &field_198, defaults.field_198);
    readBit(0x65AFACDFu, 0x80000000u);
    readBit(0x59C5087Eu, 0x40000000u);
    readBit(0xB9AB0DC3u, 0x20000000u);
    moverFlags = (moverFlags & ~0x18000000u) | (defaults.moverFlags & 0x18000000u);
    uint32_t kind = 0;
    if (elem->GetHash(0xA50A23DFu, &kind, 0) == ape::xml::kQuery_Ok) {
        const uint32_t value = kind == 0x03FCEF54u ? 1u : kind == 0xFF4019A3u ? 2u : 0u;
        moverFlags = (moverFlags & ~0x18000000u) | (value << 27);
    }
    readBit(0x73D6C6F2u, 0x04000000u);
    const char* prompt = elem->GetString("hudPromptMsg");
    if (prompt && prompt[0])
        ape::StrCopyBounded(hudPrompt, prompt, sizeof hudPrompt);
    if (elem->GetVec3(0xA5936640u, &field_110.x, &defaults.field_110.x) == ape::xml::kQuery_Ok) {
        field_110.x *= kDegToRad;
        field_110.y *= kDegToRad;
        field_110.z *= kDegToRad;
    }
    if (elem->GetVec3(0x760CE7A8u, &field_130.x, &defaults.field_130.x) == ape::xml::kQuery_Ok) {
        field_130.x *= kDegToRad;
        field_130.y *= kDegToRad;
        field_130.z *= kDegToRad;
    }
    readBit(0x57DA11C3u, 0x02000000u);
    control.field_00 = std::max(control.field_00, 0.0f);
    for (ape::xml::XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
        ParseControl(child, defaults.control);
}

// @func 0x001D5A40 ActorB17990Def::ActorB17990Def
ActorB17990Def::ActorB17990Def(const ActorB17990DefDesc& desc)
    : ActorDef(desc), field_150(desc.field_100), field_160(desc.field_110), field_170(desc.field_120),
      field_180(desc.field_130), field_190(desc.floats140[0]), field_194(desc.floats140[1]),
      field_198(desc.floats140[2]), field_19C(desc.floats140[3]), field_1A0(desc.floats140[4]),
      control(desc.control), field_1E8(desc.field_198)
{
    moverFlags = (moverFlags & 0x01FFFFFFu) | (desc.moverFlags & 0xFE000000u);
    if (desc.hudPrompt[0])
        ape::StrCopyBounded(hudPrompt, desc.hudPrompt, sizeof hudPrompt);
}

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
        def.soundBank = field_130;
        def.startSlot = field_134;
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
        defaults.field_130 = sound->soundBank;
        defaults.field_134 = sound->startSlot;
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

// -- SoundActorB17270 (the gadget turret) ----------------------------------------
constexpr uint32_t kType_SoundActorB17270 = 0x2000000;

struct SoundActorB17270DefDesc : SoundActorDefDesc {
    explicit SoundActorB17270DefDesc(const EntityDefDesc& loader) : SoundActorDefDesc(loader)
    {
        typeBits |= kType_SoundActorB17270;
    }
    uint32_t aimPartName = 0;           // 0x140
    uint32_t field_144 = 0;             // 0x144
    uint32_t field_148 = 0;             // 0x148
    ape::Vec3 aimOffset{};              // 0x150
    uint32_t field_160 = 0;             // 0x160
    uint32_t field_164 = 0;             // 0x164
    float floats168[7] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f};   // 0x168..0x180
    float field_184 = 0.34906584f;      // 0x184 20 degrees
    float field_188 = 0.08726646f;      // 0x188 5 degrees
    uint32_t mode = 0;                  // 0x18C
    int32_t field_190 = -1;             // 0x190
    bool bools194[4] = {true, true, true, false};   // 0x194..0x197

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001CA080 SoundActorB17270DefDesc::Parse
// A turret parent passes on everything but the aim offset and the mode;
// three of the floats may not be negative and two angles are read in degrees.
void SoundActorB17270DefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                    const EntityDefDesc& loader, const EntityDef* parent)
{
    SoundActorDefDesc::Parse(root, elem, loader, parent);
    SoundActorB17270DefDesc defaults(loader);
    if (const auto* turret = SameClassParent<SoundActorB17270Def>(parent, kType_SoundActorB17270)) {
        defaults.aimPartName = turret->aimPartName;
        defaults.field_144 = turret->field_194;
        defaults.field_148 = turret->field_198;
        defaults.field_160 = turret->field_1B0;
        defaults.field_164 = turret->field_1B4;
        for (int i = 0; i < 7; ++i)
            defaults.floats168[i] = turret->field_1B8[i];
        defaults.field_184 = turret->field_1D4;
        defaults.field_188 = turret->field_1D8;
        defaults.field_190 = turret->field_1E0;
        defaults.bools194[0] = turret->field_1E4;
        defaults.bools194[1] = turret->field_1E5;
        defaults.bools194[2] = turret->field_1E6;
        defaults.bools194[3] = turret->field_1E7;
    }
    elem->GetHash(0x4156E113u, &aimPartName, defaults.aimPartName);
    elem->GetHash(0x7997D9A2u, &field_144, defaults.field_144);
    elem->GetHash(0xC72945F3u, &field_148, defaults.field_148);
    elem->GetVec3(0xCC0D6864u, &aimOffset.x, &defaults.aimOffset.x);
    elem->GetHash(0x7242FC8Fu, &field_160, defaults.field_160);
    elem->GetHash(0x53FFAFC2u, &field_164, defaults.field_164);
    static constexpr uint32_t kFloatAttrs[7] = {0x09198A77u, 0x58E70C1Bu, 0x0B109E97u, 0x0B10795Bu,
                                                0x0B13D227u, 0x667AB4F5u, 0xDBA26CDAu};
    for (int i = 0; i < 7; ++i) {
        const bool read = elem->GetFloat(kFloatAttrs[i], &floats168[i], defaults.floats168[i]) == ape::xml::kQuery_Ok;
        if (read && i >= 2 && i <= 4)
            floats168[i] = std::max(floats168[i], 0.0f);
    }
    constexpr float kDegToRad = 0.017453292f;
    if (elem->GetFloat(0x2D3AC4D0u, &field_184, defaults.field_184) == ape::xml::kQuery_Ok)
        field_184 *= kDegToRad;
    if (elem->GetFloat(0x2C30B0AFu, &field_188, defaults.field_188) == ape::xml::kQuery_Ok)
        field_188 *= kDegToRad;
    uint32_t modeName = 0;
    if (elem->GetHash(0x04F3FC3Du, &modeName, 0) == ape::xml::kQuery_Ok) {
        static constexpr uint32_t kModes[2] = {0x13234CFFu, 0x8FA5D8C3u};
        for (uint32_t i = 0; i < 2; ++i) {
            if (kModes[i] == modeName) {
                mode = i;
                break;
            }
        }
    }
    elem->GetInt(0x9F5A4080u, &field_190, defaults.field_190);
    static constexpr uint32_t kBoolAttrs[4] = {0x3EFBF433u, 0x4128CDCDu, 0xAD4866C3u, 0xC4F817C0u};
    for (int i = 0; i < 4; ++i)
        elem->GetBool(kBoolAttrs[i], &bools194[i], defaults.bools194[i]);
}

// @func 0x001C9E34 SoundActorB17270Def::SoundActorB17270Def
SoundActorB17270Def::SoundActorB17270Def(const SoundActorB17270DefDesc& desc)
    : SoundActorDef(desc), aimPartName(desc.aimPartName), field_194(desc.field_144), field_198(desc.field_148),
      aimOffset(desc.aimOffset), field_1B0(desc.field_160), field_1B4(desc.field_164), field_1D4(desc.field_184),
      field_1D8(desc.field_188), mode(desc.mode), field_1E0(desc.field_190), field_1E4(desc.bools194[0]),
      field_1E5(desc.bools194[1]), field_1E6(desc.bools194[2]), field_1E7(desc.bools194[3])
{
    desc.CopyTo(*this);
    for (int i = 0; i < 7; ++i)
        field_1B8[i] = desc.floats168[i];
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

// -- SoundActorB16B48 (light) ----------------------------------------------------
constexpr uint32_t kType_SoundActorB16B48 = 0x1000000;

struct SoundActorB16B48DefDesc : SoundActorDefDesc {
    explicit SoundActorB16B48DefDesc(const EntityDefDesc& loader) : SoundActorDefDesc(loader)
    {
        typeBits |= kType_SoundActorB16B48;
    }
    ape::ApeTArray<uint32_t> lightNames{4, nullptr, true};   // 0x140
    ape::ApeTArray<int32_t> bodyParts{4, nullptr, true};     // 0x154
    float field_168 = -1.0f;            // 0x168
    bool options[4] = {};               // 0x16C

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001C2058 SoundActorB16B48DefDesc::Parse
// The lists start as the parent's and grow by the light names (attribute
// 0x2DF2E2B5 and its three numbered companions 0x9069741D..F) and the body
// part numbers (0xCBC57F7D and 0xACEA7055..7; one-based in the file). A
// missing numbered attribute repeats the value read before it.
void SoundActorB16B48DefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                    const EntityDefDesc& loader, const EntityDef* parent)
{
    SoundActorDefDesc::Parse(root, elem, loader, parent);
    float defaultValue = -1.0f;
    bool defaultOptions[4] = {false, true, false, true};
    if (const auto* other = SameClassParent<SoundActorB16B48Def>(parent, kType_SoundActorB16B48)) {
        for (uint32_t name : other->lightNames)
            lightNames.PushBack(name);
        for (int32_t part : other->bodyParts)
            bodyParts.PushBack(part);
        defaultValue = other->field_1B8;
        defaultOptions[0] = other->restartWhenWaiting;
        defaultOptions[1] = other->unlightOnSlot3;
        defaultOptions[2] = other->unlightOnSlot4;
        defaultOptions[3] = other->lightHasBodies;
    }

    uint32_t name = 0;
    elem->GetHash(0x2DF2E2B5u, &name, 0);
    if (name != 0)
        lightNames.PushBack(name);
    for (uint32_t i = 1; i < 4; ++i) {
        elem->GetHash(0x9069741Cu + i, &name, name);
        if (name != 0)
            lightNames.PushBack(name);
    }
    int32_t part = -1;
    if (elem->GetInt(0xCBC57F7Du, &part, -1) == ape::xml::kQuery_Ok) {
        part = std::max(part - 1, 0);
        bodyParts.PushBack(part);
    }
    for (uint32_t i = 1; i < 4; ++i) {
        if (elem->GetInt(0xACEA7054u + i, &part, part) == ape::xml::kQuery_Ok) {
            part = std::max(part - 1, 0);
            bodyParts.PushBack(part);
        }
    }

    elem->GetFloat(0x2DF33CE3u, &field_168, defaultValue);
    static constexpr uint32_t kOptionAttrs[4] = {0x45703A61u, 0xB10A3CDEu, 0x89D243B8u, 0x428E4540u};
    for (int i = 0; i < 4; ++i)
        elem->GetBool(kOptionAttrs[i], &options[i], defaultOptions[i]);
}

// @func 0x001C1D4C SoundActorB16B48Def::SoundActorB16B48Def
SoundActorB16B48Def::SoundActorB16B48Def(const SoundActorB16B48DefDesc& desc)
    : SoundActorDef(desc), field_1B8(desc.field_168), restartWhenWaiting(desc.options[0]),
      unlightOnSlot3(desc.options[1]), unlightOnSlot4(desc.options[2]), lightHasBodies(desc.options[3])
{
    desc.CopyTo(*this);
    lightNames.Init(desc.lightNames.Count(), nullptr, true);
    bodyParts.Init(desc.bodyParts.Count(), nullptr, true);
    for (uint32_t name : desc.lightNames)
        lightNames.PushBack(name);
    for (int32_t part : desc.bodyParts)
        bodyParts.PushBack(part);
}

// -- SoundActorB16DB0 (six tracked volumes) --------------------------------------
constexpr uint32_t kType_SoundActorB16DB0 = 0x400000;

struct SoundActorB16DB0DefDesc : SoundActorDefDesc {
    explicit SoundActorB16DB0DefDesc(const EntityDefDesc& loader) : SoundActorDefDesc(loader)
    {
        typeBits |= kType_SoundActorB16DB0;
    }
    ape::Vec3 angles{};                 // 0x140
    float field_150 = 0.0f;             // 0x150
    float field_154 = -1.0f;            // 0x154
    float field_158 = 0.0f;             // 0x158
    uint32_t field_15C = 0;             // 0x15C
    uint32_t field_160 = 0;             // 0x160
    float field_164 = -1.0f;            // 0x164
    float field_168 = -1.0f;            // 0x168
    bool flag16C = false;               // 0x16C
    bool flag16D = false;               // 0x16D
    bool options[9] = {};               // 0x16E bits 30..22 of the definition's options
    SoundActorB16DB0Def::Volume volumes[SoundActorB16DB0Def::kVolumeCount];   // 0x180

    void Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
               const EntityDef* parent);
};

// @func 0x001C49E8 SoundActorB16DB0DefDesc::Parse
// Option 27 (the fourth) keeps the description's volumes; otherwise they
// start as the parent's (or the defaults). Each "Trigger" child then
// updates the next volume.
void SoundActorB16DB0DefDesc::Parse(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                    const EntityDefDesc& loader, const EntityDef* parent)
{
    SoundActorDefDesc::Parse(root, elem, loader, parent);
    float defaultFloats[3] = {0.0f, -1.0f, 0.0f};
    uint32_t defaultHashes[2] = {};
    float default164 = -1.0f, default168 = -1.0f;
    bool defaultOptions[9] = {};
    SoundActorB16DB0Def::Volume defaultVolumes[SoundActorB16DB0Def::kVolumeCount];
    if (const auto* other = SameClassParent<SoundActorB16DB0Def>(parent, kType_SoundActorB16DB0)) {
        defaultFloats[0] = other->field_1A0;
        defaultFloats[1] = other->triggerDelay;
        defaultFloats[2] = other->field_1A8;
        defaultHashes[0] = other->field_1AC;
        defaultHashes[1] = other->field_1B0;
        default164 = other->field_1B4;
        default168 = other->field_1B8;
        for (int i = 0; i < 9; ++i)
            defaultOptions[i] = (other->options >> (30 - i)) & 1;
        for (int i = 0; i < SoundActorB16DB0Def::kVolumeCount; ++i)
            defaultVolumes[i] = other->volumes[i];
    }
    constexpr float kDegToRad = 0.017453292f;
    const ape::Vec3 zero{};
    if (elem->GetVec3(0x3C6AE50Bu, &angles.x, &zero.x) == ape::xml::kQuery_Ok) {
        angles.x *= kDegToRad;
        angles.y *= kDegToRad;
        angles.z *= kDegToRad;
    }
    elem->GetFloat(0x7D873487u, &field_150, defaultFloats[0]);
    elem->GetFloat(0x7BC2B446u, &field_154, defaultFloats[1]);
    elem->GetFloat(0x1B75F0DAu, &field_158, defaultFloats[2]);
    elem->GetHash(0x2DF2E2B5u, &field_15C, defaultHashes[0]);
    elem->GetHash(0x53FFAFC2u, &field_160, defaultHashes[1]);
    elem->GetFloat(0x667AB4F5u, &field_164, default164);
    elem->GetFloat(0x7A392528u, &field_168, default168);
    elem->GetBool(0xBC50985Bu, &flag16C, false);
    elem->GetBool(0xCD7DD617u, &flag16D, false);
    static constexpr uint32_t kOptionAttrs[9] = {0xA4AC4BB2u, 0xC56C09B3u, 0x9698E4BFu, 0xA8F641D7u, 0xC53B9DE4u,
                                                 0xF7C51457u, 0x013BDD92u, 0x23872185u, 0xA8DB95B1u};
    for (int i = 0; i < 9; ++i)
        elem->GetBool(kOptionAttrs[i], &options[i], defaultOptions[i]);
    if (!options[3]) {
        for (int i = 0; i < SoundActorB16DB0Def::kVolumeCount; ++i)
            volumes[i] = defaultVolumes[i];
    }

    constexpr uint32_t kElem_Trigger = 0xEB9EE198u;   // "Trigger"
    int index = 0;
    for (ape::xml::XmlElement* child = elem->FirstChildElement(kElem_Trigger); child;
         child = child->NextSiblingElement(kElem_Trigger), ++index) {
        if (index >= SoundActorB16DB0Def::kVolumeCount)
            continue;
        SoundActorB16DB0Def::Volume& volume = volumes[index];
        child->GetHash(0x75849CFEu, &volume.field_00, volume.field_00);
        child->GetVec3(0x2B7B3EB4u, &volume.field_10.x, &volume.field_10.x);
        child->GetVec3(0x313AB0C1u, &volume.field_20.x, &volume.field_20.x);
        ape::Vec3 degrees{};
        if (child->GetVec3(0x313AB844u, &degrees.x, &zero.x) == ape::xml::kQuery_Ok)
            volume.rotation =
                QuatFromEuler({degrees.x * kDegToRad, degrees.y * kDegToRad, degrees.z * kDegToRad, 0.0f});
        child->GetHash(0x632DA119u, &volume.field_40, volume.field_40);
        child->GetBool(0x8141EC8Au, &volume.field_44, volume.field_44);
        child->GetBool(0xF8E455ACu, &volume.field_45, volume.field_45);
    }

    field_150 = std::max(field_150, 0.0f);
    field_158 = std::max(field_158, 0.0f);
    // The first two options exclude each other.
    if (options[0] && options[1])
        options[0] = options[1] = false;
}

// @func 0x001C463C SoundActorB16DB0Def::SoundActorB16DB0Def
SoundActorB16DB0Def::SoundActorB16DB0Def(const SoundActorB16DB0DefDesc& desc)
    : SoundActorDef(desc), angles(desc.angles), field_1A0(desc.field_150), triggerDelay(desc.field_154),
      field_1A8(desc.field_158), field_1AC(desc.field_15C), field_1B0(desc.field_160), field_1B4(desc.field_164),
      field_1B8(desc.field_168)
{
    desc.CopyTo(*this);
    options &= 0x8000FFFFu;
    for (int i = 0; i < 9; ++i)
        options |= uint32_t(desc.options[i]) << (30 - i);
    for (int i = 0; i < kVolumeCount; ++i)
        volumes[i] = desc.volumes[i];
    if (desc.flag16D)
        options = (options & 0xFFC0FFFFu) | 0x00050000u;
    else if (desc.flag16C)
        options = (options & 0xFFC0FFFFu) | 0x00030000u;
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

// @func 0x001CA7C0 CreateSoundActorB17270Def
EntityDef* CreateSoundActorB17270Def(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                     const EntityDefDesc& loader, const EntityDef* parent)
{
    SoundActorB17270DefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new SoundActorB17270Def(desc);
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

// @func 0x001C28E8 CreateSoundActorB16B48Def
EntityDef* CreateSoundActorB16B48Def(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                     const EntityDefDesc& loader, const EntityDef* parent)
{
    SoundActorB16B48DefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new SoundActorB16B48Def(desc);
    delete desc.parts;
    return def;
}

// @func 0x001BD668 CreateScriptObjectActorDef
// The class attributes are read in place: a float (the parent's as default)
// and a vector (no parent default).
EntityDef* CreateScriptObjectActorDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                      const EntityDefDesc& loader, const EntityDef* parent)
{
    constexpr uint32_t kType_ScriptObject = 0x200;
    ClassDefDesc desc(loader, kType_ScriptObject);
    desc.Parse(root, elem, loader, parent);
    const auto* other = SameClassParent<ScriptObjectActorDef>(parent, kType_ScriptObject);
    float value = 0.0f;
    ape::Vec3 vector{};
    elem->GetFloat(0x7DDB32EAu, &value, other ? other->field_130 : 0.0f);
    elem->GetVec3(0x7DDB2524u, &vector.x, &vector.x);
    auto* def = new ScriptObjectActorDef(desc);
    def->field_130 = value;
    def->field_140 = vector;
    delete desc.parts;
    return def;
}

// @func 0x001C5408 CreateSoundActorB16DB0Def
EntityDef* CreateSoundActorB16DB0Def(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                     const EntityDefDesc& loader, const EntityDef* parent)
{
    SoundActorB16DB0DefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new SoundActorB16DB0Def(desc);
    delete desc.parts;
    return def;
}

// @func 0x001DE610 CreateInteractivePropADef
EntityDef* CreateInteractivePropADef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                     const EntityDefDesc& loader, const EntityDef* parent)
{
    InteractivePropADefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new InteractivePropADef(desc);
    delete desc.parts;
    return def;
}

// @func 0x001D71D8 CreateActorB17990Def
EntityDef* CreateActorB17990Def(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                                const EntityDef* parent)
{
    ActorB17990DefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new ActorB17990Def(desc);
    delete desc.parts;
    return def;
}

// @func 0x001E6B18 CreateSafeDef
EntityDef* CreateSafeDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, const EntityDefDesc& loader,
                         const EntityDef* parent)
{
    SafeDefDesc desc(loader);
    desc.Parse(root, elem, loader, parent);
    auto* def = new SafeDef(desc);
    delete desc.parts;
    return def;
}

} // namespace entity
} // namespace sly
