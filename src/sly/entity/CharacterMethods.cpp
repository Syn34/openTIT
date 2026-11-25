// openTIT - Character helpers and entity-slot overrides (0x0011F000-0x0012A000).
//
// Model variants, the weapon attachment, profile costumes, tinting, ability
// states and the small overrides of the entity interface. The character
// component and the game world's profile records are reached through
// CharacterHooks.h.
#include "sly/entity/Character.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/memory/ApeMem.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeObj.h"
#include "sly/components/ComponentDescs.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/EntityHooks.h"
#include "sly/entity/GameEntityHooks.h"
#include "sly/objects/CharacterMoveTuning.h"

#include <algorithm>
#include <cmath>

namespace sly {
namespace entity {

namespace {

// Child link part the weapon hangs from. NOTE: name of the hash unknown.
constexpr uint32_t kPart_Weapon = 0x03D69F9Cu;

constexpr uint32_t kChar_Sly = 0x00014120u;       // "Sly"
constexpr uint32_t kChar_Bentley = 0x1D8C2817u;   // "Bentley"
constexpr uint32_t kChar_Murray = 0x88397820u;    // "Murray"
// NOTE: the fourth playable character of the costume table; name unknown.
constexpr uint32_t kChar_DF0611F9 = 0xDF0611F9u;

// Default selections when the profile holds none.
constexpr uint32_t kDefaultWeapon = 0xFE11127Cu;      // NOTE: name unknown
constexpr uint32_t kDefaultOutfit = 0x86DF6221u;      // "default"

// Director script played when the model is swapped. NOTE: name unknown.
constexpr uint32_t kScript_CostumeSwap = 0x05220E90u;
// World flag that enlarges the swapped model's bounding radius. NOTE: name unknown.
constexpr uint32_t kFlag_LargeBounds = 0xF11D508Eu;

// Class bit 36: the character holds a non-default weapon.
constexpr uint64_t kClass_CustomWeapon = 1ull << 36;
// Class bit 33: the model is never tinted.
constexpr uint64_t kClass_NoTint = 1ull << 33;

// Profile record offsets of the selections (record-relative).
constexpr uint32_t kSel_Weapon = 0x124;
constexpr uint32_t kSel_Murray = 0x128;
constexpr uint32_t kSel_Bentley = 0x12C;
constexpr uint32_t kSel_DF0611F9 = 0x130;

// Costume table (0x00AC8C28): the costume flag of each playable character
// and its two models (the second is worn with the alternate costume flag).
struct CostumeEntry {
    uint32_t costume;
    uint32_t character;
    uint32_t model;
    uint32_t alternateModel;
};
constexpr CostumeEntry kCostumes[] = {
    { 2, kChar_Sly, 0x166A4BCFu, 0xC1E25120u },
    { 3, kChar_Bentley, 0x6E024346u, 0x8562E197u },
    { 4, kChar_Murray, 0x720BBB11u, 0xB57CEC13u },
    { 5, kChar_DF0611F9, 0xCB5C9C2Au, 0xDEA4ADB6u },
};

constexpr uint32_t kComp_Weapon = 0x10;
constexpr uint32_t kComp_Props = 0x15;

ape::world::ApeGeomDef* FindGeomDef(uint32_t name)
{
    ape::world::ApeDef** def = ape::world::ApeTDefList<ape::world::ApeGeomDef>::Map().Find(name);
    return def ? static_cast<ape::world::ApeGeomDef*>(*def) : nullptr;
}

// Index of an ability name in the ability table, -1 when it is not there.
int AbilityIndex(uint32_t name)
{
    for (int i = 0; i < kAbilityCount; ++i) {
        if (AbilityHash(i) == name)
            return i;
    }
    return -1;
}

const CharacterDef* DefOf(const Character* character)
{
    return static_cast<const CharacterDef*>(character->def);
}

// Swaps the counted reference in `slot`.
void SetCountedReference(Entity*& slot, Entity* entity)
{
    if (slot)
        slot->Release(false);
    slot = entity;
    if (entity)
        entity->AddRef();
}

const ape::Matrix34 kIdentity = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 0 } };
// Entity flag 13: the entity follows its root animation.
constexpr uint32_t kEnt_RootMotion = 0x2000;

float ByteToUnit(uint8_t value) { return float(value) * (1.0f / 255.0f); }

} // namespace

// -- entity base ------------------------------------------------------------------

// Base version of slot 30: tints the world object unless the entity is never
// tinted (class bit 33).
// @func 0x00743658 Entity::SetTint
void Entity::SetTint(float amount, const uint8_t color[4], bool immediate)
{
    (void)immediate;
    if (classBits & (1ull << 33)) {
        const uint8_t none[4] = {};
        ObjectSetTint(0.0f, object, none);
    } else {
        ObjectSetTint(amount, object, color);
    }
}

// -- camera settings block --------------------------------------------------------

// @func 0x00119C00 CameraSettingsBlock::LoadDefaults
void CameraSettingsBlock::LoadDefaults()
{
    const float fovInterp = settings.fovInterp;
    const int32_t lookAheadVelMode = settings.lookAheadVelMode;
    const float headingMaxRate = settings.headingMaxRate;
    settings.LoadDefaults();
    settings.fovInterp = fovInterp;
    settings.lookAheadVelMode = lookAheadVelMode;
    settings.headingMaxRate = headingMaxRate;
}

// -- definition -------------------------------------------------------------------

// @func 0x0011F010 CharacterDef::GetBounds
void CharacterDef::GetBounds(ape::Aabb& out) const
{
    const auto* physics = static_cast<const components::PhysicsComponentDesc*>(
        FindComponentDesc(components::kCompDesc_Physics));
    if (!physics) {
        EntityDefBaseBounds(this, out);
        return;
    }
    // The capsule stands on the offset point: half extents in x/z, full
    // height (twice the half height) in y.
    const ape::Vec3& size = physics->size;
    const ape::Vec3& offset = physics->offset;
    out.min = { offset.x - size.x, offset.y, offset.z - size.z, out.min.pad };
    out.max = { offset.x + size.x, offset.y + size.y * 2.0f, offset.z + size.z, out.max.pad };
}

uint32_t CharacterDef::SkinModel(int mode) const
{
    switch (mode) {
    case Character::kSkin_Variant3: return skinModels[0];
    case Character::kSkin_Variant2: return skinModels[1];
    case Character::kSkin_Variant1: return skinModels[2];
    case Character::kSkin_Variant4: return skinModels[3];
    case Character::kSkin_Variant5: return skinModels[4];
    default: return 0;
    }
}

// -- models and costumes ----------------------------------------------------------

// @func 0x0011F0E4 Character::UpdateSkin
void Character::UpdateSkin(bool resetPose)
{
    // Class bits that force a variant, in priority order. Bit 26 forces the
    // default model; without any of them variants 3-5 fall back to the
    // default while variants 1 and 2 stay.
    if (classBits & 0x8000)
        skinMode = kSkin_Variant4;
    else if (classBits & 0x80000)
        skinMode = kSkin_Variant5;
    else if (classBits & 0x4000000)
        skinMode = kSkin_Default;
    else if (classBits & 0x4)
        skinMode = kSkin_Variant2;
    else if (classBits & 0x8)
        skinMode = kSkin_Variant3;
    else if (skinMode >= kSkin_Variant3)
        skinMode = kSkin_Default;

    EntityComponent* component = GetCharacterComponent();
    const uint32_t model = skinMode == kSkin_Default ? CharacterCompDefaultModel(component)
                                                     : DefOf(this)->SkinModel(skinMode);
    if (model == CharacterCompModel(component))
        return;
    if (resetPose)
        CharacterCompResetPose(component);
    CharacterCompSetModel(component, model);
}

// @func 0x0011F2C8 Character::WeaponEntity
Entity* Character::WeaponEntity() const
{
    if (!children)
        return nullptr;
    for (const EntityChildLink& link : *children) {
        if (link.partName == kPart_Weapon)
            return link.child;
    }
    return nullptr;
}

// @func 0x0011F32C Character::AttachWeapon
void Character::AttachWeapon(uint32_t defName)
{
    if (Entity* old = WeaponEntity()) {
        DetachChild(old);
        old->Release(true);
    }

    EntityDef* weaponDef = FindEntityDef(defName);
    if (!weaponDef)
        return;

    SpawnParams params;
    ape::StrCopyBounded(params.name, ape::TempFormat("%s_Weapon", object ? object->GetName() : "<unknown>"), sizeof(params.name));
    if (weaponDef->attachments && weaponDef->attachments->Count() != 0) {
        for (const SpawnAttachmentDesc& desc : *weaponDef->attachments) {
            if (!params.attachments)
                params.attachments = new ape::ApeTArray<SpawnAttachmentDesc>(4);
            params.attachments->PushBack(desc);
        }
    }
    if (Entity* weapon = weaponDef->Spawn(params))
        EntityAttachSpawned(this, params, weapon);
    delete params.attachments;
}

// @func 0x0011F7B8 Character::ApplyProfileCostume
void Character::ApplyProfileCostume(bool force)
{
    const CharacterDef* charDef = DefOf(this);
    const uint32_t name = charDef->characterName;
    if (name == 0)
        return;
    const uint8_t profile = PlayerProfileIndex(0);

    for (const CostumeEntry& entry : kCostumes) {
        if (entry.character != name)
            continue;
        const bool alternate = ProfileAlternateCostume(profile, entry.costume);
        const uint32_t wanted = alternate ? entry.alternateModel : entry.model;
        const uint32_t other = alternate ? entry.model : entry.alternateModel;
        if (!object || !object->GetDef() || object->GetDef()->GetAnimHash() != other)
            break;

        // Swapping in the middle of something would pop: only an inactive
        // character in its idle states may change unless forced.
        if (!force) {
            if (flags & kEnt_Active)
                break;
            if (EntityGetState(this, 0) != 0)
                break;
            const int layer1 = EntityGetState(this, 1);
            if (layer1 != 0 && layer1 != 2)
                break;
            if (carried)
                break;
            if (EntityComponent* holder = EntityComponent03(this); holder && Component03Busy(holder))
                break;
        }

        ape::world::ApeGeomDef* geom = FindGeomDef(wanted);
        if (!geom)
            break;
        if (!force && !(flags & kEnt_Active))
            EntityPlayDirectorScript(this, kScript_CostumeSwap);
        object->SetDef(geom);
        for (EntityComponent* weapon = GetComponent(kComp_Weapon); weapon;
             weapon = GetComponent(kComp_Weapon, weapon))
            Component10SetAlternate(weapon, alternate);
        if (WorldFlag(kFlag_LargeBounds))
            object->SetBoundingRadius(20.0f);
        break;
    }

    if (flags3C8 & 0x01)
        return;

    if (name == kChar_Murray) {
        // Murray's outfit is an anim script state.
        uint32_t outfit = ProfileSelection(profile, kSel_Murray);
        if (outfit == 0)
            outfit = kDefaultOutfit;
        if (animScript) {
            ape::ApeAnimScriptState* state = animScript->GetState();
            if (!state || state->GetNameHash() != outfit)
                animScript->SetState(outfit);
        }
    } else if (name == kChar_Sly) {
        // Sly's cane is a weapon entity.
        Entity* weapon = WeaponEntity();
        const uint32_t current = (weapon && weapon->def) ? weapon->def->nameHash : 0;
        uint32_t selected = ProfileSelection(profile, kSel_Weapon);
        if (selected == 0)
            selected = kDefaultWeapon;
        if (current != selected)
            AttachWeapon(selected);
        if (selected == kDefaultWeapon) {
            classBits &= ~kClass_CustomWeapon;
        } else {
            classBits |= kClass_CustomWeapon;
            if (skinMode == kSkin_Variant1) {
                skinMode = kSkin_Default;
                UpdateSkin(true);
            }
        }
    }
}

// @func 0x0011FCC0 Character::FindAimUiObjects
void Character::FindAimUiObjects()
{
    const CharacterDef* charDef = DefOf(this);
    if (!aimArcObject) {
        ape::world::ApeGeomDef* arcDef = FindGeomDef(charDef->aimArcName);
        ape::world::ApeObj* arc = arcDef ? CreateGeomInstance(arcDef, true) : nullptr;
        if (!arc) {
            if (charDef->aimArcName != 0)
                ape::TempFormat("couldn't find aim UI arc object '%s'.", ape::TempHexString(charDef->aimArcName));
        } else {
            aimArcObject = arc;
            arc->SetVisible(false, false);
            uint8_t color[4];
            CharacterHudColor(def, color);
            SetGeomMaterialColor(arc, color);
        }
    }
    if (!aimEndObject) {
        ape::world::ApeGeomDef* endDef = FindGeomDef(charDef->aimEndName);
        ape::world::ApeObj* end = endDef ? CreateGeomInstance(endDef, false) : nullptr;
        if (!end) {
            if (charDef->aimEndName != 0)
                ape::TempFormat("couldn't find aim UI endpoint object '%s'.", ape::TempHexString(charDef->aimEndName));
        } else {
            aimEndObject = end;
            end->SetVisible(false, false);
        }
    }
}

// -- tint ---------------------------------------------------------------------------

// @func 0x0011FFF0 Character::UpdateTint
void Character::UpdateTint(float t, bool force)
{
    // NOTE: the console reshapes `t` through an easing curve (0x004A8320)
    // before blending; EaseTint stands for it.
    const float blend = 1.0f - EaseTint(t);
    uint8_t color[4];
    for (int i = 0; i < 4; ++i) {
        const float from = ByteToUnit(tintColor[i]);
        const float to = ByteToUnit(tintTargetColor[i]);
        color[i] = uint8_t(int(((to - from) * blend + from) * 255.0f));
    }
    const float amount = (tintTargetAmount - tintAmount) * blend + tintAmount;

    const bool changed = !std::equal(color, color + 4, tintColor) || amount != tintAmount;
    if (!force && !changed)
        return;

    for (EntityComponent* props = GetComponent(kComp_Props); props; props = GetComponent(kComp_Props, props)) {
        for (uint32_t i = 0; i < Component15ObjectCount(props); ++i) {
            if (ape::world::ApeObj* prop = Component15Object(props, i))
                ObjectSetTint(amount, prop, color);
        }
    }
    if (classBits & kClass_NoTint) {
        const uint8_t none[4] = {};
        ObjectSetTint(0.0f, object, none);
    } else {
        ObjectSetTint(amount, object, color);
    }
    std::copy(color, color + 4, tintColor);
    tintAmount = amount;
}

// @func 0x00120338 Character::SetTint
void Character::SetTint(float amount, const uint8_t color[4], bool immediate)
{
    std::copy(color, color + 4, tintTargetColor);
    tintTargetAmount = amount;
    if (immediate) {
        std::copy(color, color + 4, tintColor);
        tintAmount = amount;
        UpdateTint(0.0f, true);
    }
    if (!children)
        return;
    for (EntityChildLink& link : *children)
        link.child->SetTint(amount, color, immediate);
}

// -- setup ----------------------------------------------------------------------------

// @func 0x00120490 Character::InitAnimation
void Character::InitAnimation()
{
    if (animController) {
        AnimControllerSetFlag(animController, 0x200, DefOf(this)->forwardRootMotion);
        if (classBits & 1)
            AnimControllerSetFlag(animController, 0x400, false);
        AnimControllerSetEventSink(animController, this);
        if (EntityComponent* component = GetCharacterComponent())
            CharacterCompSetAnimController(component, animController);
    }
    UpdateSkin(false);
    if (EntityComponent* component = EntityComponent11(this))
        Component11Refresh(component);
}

// @func 0x001205FC Character::RefreshAbilities
void Character::RefreshAbilities()
{
    const CharacterDef* charDef = DefOf(this);
    abilities = AbilitySet{};
    for (int i = 0; i < kAbilityCount; ++i) {
        if (charDef->abilityBits[i >> 5] & (1u << (i & 31)))
            abilities.state[i] |= kAbility_Available;
    }
    field_300 = -1;
    field_308 = -1;
    field_310 = -1;

    // Only players learn more than their definition gives them.
    if (!(classBits & 1))
        return;
    const uint32_t player = PlayerSlot() & 0xFF;
    for (int i = 0; i < kAbilityCount; ++i) {
        if (LevelPlayerHasAbility(player, i))
            abilities.state[i] |= kAbility_Available;
    }
    if (charDef->characterName == 0)
        return;

    const uint8_t profile = PlayerProfileIndex(player);
    uint32_t bought[kAbilityCount];
    const uint32_t count = PurchasedAbilities(charDef->characterName, bought, kAbilityCount);
    for (uint32_t i = 0; i < count; ++i) {
        if (bought[i] == 0)
            continue;
        const int index = AbilityIndex(bought[i]);
        if (index >= 0)
            abilities.state[index] |= kAbility_Available;
    }

    // The selected outfit of some characters grants one more ability.
    uint32_t selection = 0;
    switch (charDef->characterName) {
    case kChar_Bentley: selection = ProfileSelection(profile, kSel_Bentley); break;
    case kChar_DF0611F9: selection = ProfileSelection(profile, kSel_DF0611F9); break;
    case kChar_Murray: selection = ProfileSelection(profile, kSel_Murray); break;
    default: break;
    }
    if (selection != 0) {
        // NOTE: the console indexes with -1 when the name is not in the table.
        const int index = AbilityIndex(selection);
        if (index >= 0)
            abilities.state[index] |= kAbility_Available;
    }
}

// @func 0x00121E8C Character::SetReference1A4
void Character::SetReference1A4(Entity* entity)
{
    SetCountedReference(reference1A4, entity);
}

// @func 0x00121F10 Character::SetReference1AC
void Character::SetReference1AC(Entity* entity)
{
    SetCountedReference(reference1AC, entity);
}

// -- release and reset ------------------------------------------------------------------

namespace {

// World callback (0x00130430, queued on the game world's list 12 when a
// character leaves): characters with a fade in progress go back on the
// fade update list (entity list B) unless they are on it already.
void RequeueFadingCharacter(void* owner, Entity* entity)
{
    (void)owner;
    auto* character = static_cast<Character*>(entity);
    if (!character || !(character->flags3C8 & (Character::kFlag3C8_FadeA | Character::kFlag3C8_FadeB |
                                                Character::kFlag3C8_FadeBuffer)))
        return;
    for (uint32_t i = EntityCount(kList_B); i-- > 0;) {
        if (EntityAt(kList_B, i) == character)
            return;
    }
    RegisterEntity(kList_B, character);
}

} // namespace

// @func 0x00130430 Character::RequeueFading (world callback)
void Character::QueueFadeRequeue()
{
    GameWorldQueueCallback(12, &RequeueFadingCharacter, this, this);
}

// @func 0x00122518 Character::Release
bool Character::Release(bool kill)
{
    if (kill) {
        QueueFadeRequeue();
        UnregisterEntity(kList_B, this);
        EntityShutdown(this);
        SetCarried(nullptr, nullptr, nullptr, nullptr, true);
        SetCountedReference(reference1AC, nullptr);
        SetCountedReference(reference1A4, nullptr);
        field_17C = nullptr;
        field_18C[0] = field_18C[1] = field_18C[2] = 0;

        // The active controller may be one of the other two; delete each once.
        if (extController != controller && extController)
            delete extController;
        extController = nullptr;
        if (aiController && aiController != controller)
            delete aiController;
        aiController = nullptr;
        if (controller) {
            delete controller;
            controller = nullptr;
        }

        if (animScript198) {
            DetachAnimScript(animScript198);
            animScript198 = nullptr;
        }
        if (animScript19C) {
            DetachAnimScript(animScript19C);
            animScript19C = nullptr;
        }
        for (void*& script : animScripts) {
            if (script) {
                DetachAnimScript(script);
                script = nullptr;
            }
        }
        if (aimArcObject) {
            aimArcObject->Release();
            aimArcObject = nullptr;
        }
        if (aimEndObject) {
            aimEndObject->Release();
            aimEndObject = nullptr;
        }
        // Each watcher takes itself off the list.
        while (watchers530.Count() != 0)
            watchers530[0]->ForgetCharacter(this);
        watchers530.Clear();
    }
    return ScriptedEntity::Release(kill);
}

// @func 0x00122830 Character::ResetState
void Character::ResetState()
{
    EntitySetState(this, 0, 0);
    for (EntityComponent* physics = GetComponent(kComp_00); physics; physics = GetComponent(kComp_00, physics)) {
        PhysicsCompReset(physics);
        PhysicsCompResetAfterSwap(physics);
    }
    for (EntityComponent* props = GetComponent(kComp_Props); props; props = GetComponent(kComp_Props, props))
        Component15Reset(props);
    field_1B4 = 0;
    field_1B8 = 0;
    if (aimArcObject)
        aimArcObject->SetVisible(false, false);
    if (aimEndObject)
        aimEndObject->SetVisible(false, false);
}

// -- carrying ----------------------------------------------------------------------------

namespace {

constexpr uint64_t kClass_Carrying = 0x8000;           // bit 15 (also picks skin variant 4)
constexpr uint64_t kClass_HeavyCarry = 1ull << 39;     // the carried entity blocks two abilities
constexpr uint64_t kClass_Carried = 0x20000;           // bit 17, on the carried entity
// Abilities blocked while carrying something heavy. NOTE: names from the
// ability table order (indices 2 and 3).
constexpr int kAbility_BlockedByCarry[] = { 2, 3 };

// Message posted when something is picked up. NOTE: names unknown.
constexpr uint32_t kMsgKey_PickedUp = 0x600AE700u;
constexpr uint32_t kMsg_PickedUp = 0x35007D87u;

// Quaternion of Euler angles in degrees (x, y, z), composed as the console's
// carry setup does.
ape::Vec4 QuatFromEulerDegrees(const ape::Vec3& angles)
{
    constexpr float kDegToRad = 0.017453292f;
    const float hx = angles.x * kDegToRad * 0.5f;
    const float hy = angles.y * kDegToRad * 0.5f;
    const float hz = angles.z * kDegToRad * 0.5f;
    const float cx = ape::FastCos(hx), cy = ape::FastCos(hy), cz = ape::FastCos(hz);
    const float sx = ape::FastSin(hx), sy = ape::FastSin(hy), sz = ape::FastSin(hz);
    return { sy * sz * cx + cy * cz * sx,
             cz * sy * cx + cy * sz * sx,
             cy * sz * cx - cz * sy * sx,
             cy * cz * cx - sy * sz * sx };
}

} // namespace

// @func 0x00121FA8 Character::SetCarried
void Character::SetCarried(Entity* entity, const uint32_t* part, const ape::Vec3* offset,
                           const ape::Vec4* rotation, bool parentRotation)
{
    if (entity == carried)
        return;
    const CharacterDef* charDef = DefOf(this);

    if (carried) {
        if (classBits & kClass_HeavyCarry) {
            for (int index : kAbility_BlockedByCarry)
                abilities.state[index] &= ~kAbility_Blocked;
        }
        carried->SetFlag80(false);
        carried->SetAlpha(0xFF, true, true, false);
        DetachChild(carried);
        carried->Release(false);
        carried->classBits &= ~kClass_Carried;
        carried = nullptr;
    }

    if (entity) {
        carried = entity;
        EntityChildLink link;
        link.child = entity;
        link.partName = part ? *part : charDef->carryPart;
        const ape::Vec3& at = offset ? *offset : charDef->carryOffset;
        link.offset = { at.x, at.y, at.z, link.offset.pad };
        link.rotation = rotation ? *rotation : QuatFromEulerDegrees(charDef->carryAngles);
        link.flags = kChild_RestoreState | (parentRotation ? kChild_ParentRotation : 0u);
        EntityAttachChild(this, link);

        classBits |= kClass_Carrying;
        EntityPostScriptMessage(this, kMsgKey_PickedUp, kMsg_PickedUp, entity->nameHash);
        entity->SetFlag80(true);
        entity->AddRef();
        entity->classBits |= kClass_Carried;
        if (entity->listener && ListenerIsHeavy(entity->listener)) {
            for (int index : kAbility_BlockedByCarry)
                abilities.state[index] |= kAbility_Blocked;
            classBits |= kClass_HeavyCarry;
        }
    }

    if (!carried) {
        skinMode = kSkin_Default;
        classBits &= ~(kClass_Carrying | kClass_HeavyCarry);
    } else {
        classBits |= kClass_Carrying;
        if (carried->listener && ListenerIsHeavy(carried->listener))
            classBits |= kClass_HeavyCarry;
    }
}

// @func 0x001244A4 Character::DropCarried
void Character::DropCarried(bool release)
{
    Entity* held = carried;
    if (!held)
        return;
    EntityListener* listener = held->listener;
    if (listener)
        ListenerOnDropped(listener, DefOf(this)->dropSettings);
    SetCarried(nullptr, nullptr, nullptr, nullptr, true);
    if (!listener && release)
        held->Release(true);
}

// -- opacity -------------------------------------------------------------------------------

namespace {

// Tint amount the opacity path uses. NOTE: what the value means to the
// material is not known.
constexpr float kOpacityTintAmount = 7.25f;

// Children that fade with their parent have entity type bit 70 (+0xC8 bit 6).
bool FadesWithParent(const Entity* child)
{
    return child && (child->typeBits[2] & 0x40);
}

} // namespace

// @func 0x001241A0 Character::SetOpacity
void Character::SetOpacity(uint8_t alpha)
{
    opacity = alpha;
    flags3C8 &= ~kFlag3C8_FadeBuffer;
    if (fadeBuffer) {
        ape::MemFree(fadeBuffer);
        fadeBuffer = nullptr;
    }
    const uint8_t color[4] = { opacityColor[0], opacityColor[1], opacityColor[2], opacity };
    opacityColor[3] = opacity;
    SetTint(kOpacityTintAmount, color, true);
    SetAlpha(opacity, false, false, true);
    if (object)
        RenderObjectSetLodField(object, lodField);
    if (!children)
        return;
    for (EntityChildLink& link : *children) {
        Entity* child = link.child;
        if (FadesWithParent(child) && child->object) {
            EntitySetFadeAlpha(child, false, opacity, true);
            ObjectSetTint(kOpacityTintAmount, child->object, color);
        }
    }
}

// @func 0x00123F70 Character::ApplyFadeBuffer
void Character::ApplyFadeBuffer()
{
    ObjectApplyAlphaBuffer(object, fadeBuffer, opacity);
    if (!children)
        return;
    for (EntityChildLink& link : *children) {
        if (FadesWithParent(link.child))
            EntityApplyFadeBuffer(link.child);
    }
}

// @func 0x001243D0 Character::ResetFade
void Character::ResetFade()
{
    SetTint(kOpacityTintAmount, opacityColor, true);
    EntityComponent* driver = GetComponent(kComp_17);
    if (driver) {
        // Driver states 2-5 are the scripted fades.
        const uint32_t state = AnimDriverState(driver);
        if (state > 1 && state < 6) {
            AnimDriverStop(driver, false);
            SetOpacity(0xFF);
            flags3C8 |= kFlag3C8_Opaque;
        }
    }
    field_1B8 = 0;
    classBits &= ~uint64_t(0x40);
}

// -- activity --------------------------------------------------------------------------------

namespace {
constexpr uint32_t kComp_24 = 0x24;
// Anim event sent to the character component's controller when the
// character is deactivated. NOTE: name unknown.
constexpr uint32_t kAnimEvent_Deactivated = 0x001F3381u;
} // namespace

// @func 0x00124598 Character::SetActive
void Character::SetActive(bool active)
{
    if (EntityComponent* physics = GetComponent00()) {
        if (!(classBits & 1)) {
            PhysicsCompSetDisabled(physics, !active);
        } else {
            physics->SetOption(!active);
            if (!active)
                PhysicsCompSetDisabled(physics, true);
            else
                field_17C = controller;
        }
    }
    for (EntityComponent* weapon = GetComponent(kComp_Weapon); weapon; weapon = GetComponent(kComp_Weapon, weapon))
        Component10SetHidden(weapon, !active);

    if (active) {
        DropCarried(true);
        if (EntityComponent* holder = EntityComponent03(this))
            holder->OnOwnerSpawned(this);
        if (EntityComponent* comp25 = EntityComponent25(this))
            comp25->OnOwnerSpawned(this);
        if (EntityComponent* comp24 = GetComponent(kComp_24))
            Component24Reset(comp24);
        ResetFade();
        if (EntityComponent* character = GetCharacterComponent())
            CharacterCompSendAnimEvent(character, kAnimEvent_Deactivated);
        SetCountedReference(reference1A4, nullptr);
        classBits &= ~uint64_t(0x80004);
    }
    Entity::SetActive(active);
    ApplyProfileCostume(true);
}

// @func 0x00124A04 Character::SetRenderId
void Character::SetRenderId(uint32_t id)
{
    if (PhysicsWorldPresent()) {
        EntityComponent* physics = GetComponent00();
        if (void* body = physics ? PhysicsCompRigidBody(physics) : nullptr)
            RigidBodySetCollisionGroup(body, id);
    }
    Entity::SetRenderId(id);
}

// -- damage ---------------------------------------------------------------------------------

namespace {

// Damage types handed to ApplyDamage per hit category. NOTE: names unknown;
// the actor base uses the same values.
constexpr uint32_t kType_PhysicsImpact = 0x80F43146u;   // category 0
constexpr uint32_t kType_Melee = 0x045D9554u;           // category 1
constexpr uint32_t kType_Blast = 0x03C599D8u;           // category 2
constexpr uint32_t kType_Environment = 0x811B1D5Eu;     // category 3
constexpr uint32_t kType_Projectile = 0xA77EB5E9u;      // category 4
// Damage type that posts no "damaged" game event. NOTE: name unknown.
constexpr uint32_t kType_Silent = 0x20CE2C3Au;

// Hit reactions with their own environment scale. NOTE: names unknown.
constexpr uint32_t kReaction_Fire = 0x0020EE36u;
constexpr uint32_t kReaction_Electric = 0x2DA0A51Du;
constexpr uint32_t kReaction_Poison = 0x8CFF6308u;

// Entity type bits used to find who is behind a hit.
constexpr uint32_t kType_CharacterBit = 0x08;     // +0xC0 bit 3
constexpr uint32_t kType_ThrownBit = 0x20;        // +0xC0 bit 5 (owner at +0x1AC)
constexpr uint32_t kType_OwnedBit = 0x1000;       // +0xC0 bit 12 (owner at +0x188)
constexpr uint32_t kType_SpawnedBit = 0x04;       // +0xC0 bit 2 (owner at +0x190)
constexpr uint32_t kType_EnvironmentBit = 0x10000000;   // +0xC0 bit 28: no reaction scaling

// Class bits of the friendly-fire rule.
constexpr uint64_t kClass_Disguised = 0x20;          // bit 5: disguised (on the other team; full damage)
constexpr uint64_t kClass_HurtsFriends = 0x8000000;     // bit 27 on the attacker

bool IsCharacterEntity(const Entity* entity)
{
    return entity && (entity->typeBits[0] & kType_CharacterBit);
}

// Character responsible for a hit: the source itself, or the character that
// threw, owns or spawned it.
Entity* AttackingCharacter(Entity* source)
{
    if (!source)
        return nullptr;
    Entity* owner = source;
    const uint32_t type = source->typeBits[0];
    if (type & kType_ThrownBit)
        owner = HitSourceOwner(source, kHitOwner_Thrower);
    else if (type & kType_OwnedBit)
        owner = HitSourceOwner(source, kHitOwner_Owner);
    else if (type & kType_SpawnedBit)
        owner = HitSourceOwner(source, kHitOwner_Spawner);
    return IsCharacterEntity(owner) ? owner : nullptr;
}

uint32_t TeamOf(const Entity* entity) { return (entity->flags >> 22) & 3; }

} // namespace

// Friendly fire between characters of one team is scaled down (the
// Entity.Character.Damage.GlobalFriendly tuning), unless the victim always
// takes full damage or the attacker always hurts friends.
bool Character::IsFriendlyHit(const Entity* attacker) const
{
    return attacker && TeamOf(this) == TeamOf(attacker) && !(classBits & kClass_Disguised) &&
           !(attacker->classBits & kClass_HurtsFriends);
}

// @func 0x00126E04 Character::OnHitCategory0
// Physics impacts: the impact speed times the scalar of keyframed or of
// dynamic bodies (by the mass of the body that hit).
void Character::OnHitCategory0(EntityHitRecord& hit)
{
    if (!ReceiveHit(hit))
        return;
    void* otherBody = HitOtherBody(hit);
    if (!otherBody)
        return;
    Entity* attacker = AttackingCharacter(hit.source);
    if (attacker == this)
        return;

    float amount;
    if (RigidBodyIsKeyframed(otherBody))
        amount = hit.data.collision.speed * damage.impactScalarKey;
    else
        amount = hit.data.collision.speed * RigidBodyMass(otherBody) * damage.impactScalarDyn;
    if (attacker && IsFriendlyHit(attacker))
        amount *= objects::g_characterDamageGlobalFriendly;
    ApplyDamage(amount, attacker, hit, kType_PhysicsImpact);
}

// @func 0x00127048 Character::OnHitCategory1
void Character::OnHitCategory1(EntityHitRecord& hit)
{
    if (!ReceiveHit(hit))
        return;
    float amount = hit.data.impact.damage * damage.melee;
    float scale = 1.0f;
    if (hit.reaction == kReaction_Fire)
        scale = damage.environmentFire;
    else if (hit.reaction == kReaction_Electric)
        scale = damage.environmentElectric;

    Entity* source = hit.source;
    if (IsCharacterEntity(source)) {
        // Friendly melee: players hurt their own team only through their
        // own (non-player) hits, and the friendly scale caps the reaction scale.
        const bool fromPlayer = (hit.flags & EntityHitRecord::kHit_FromPlayer) != 0;
        if (TeamOf(this) == TeamOf(source) && (!fromPlayer || TeamOf(this) == 0) &&
            !(classBits & kClass_Disguised) && !(source->classBits & kClass_HurtsFriends) &&
            objects::g_characterDamageGlobalFriendly <= scale)
            scale = objects::g_characterDamageGlobalFriendly;
        amount *= scale;
    }
    if (scale != 0.0f) {
        ApplyDamage(amount, source, hit, kType_Melee);
        RecordMeleeHit(this, hit);
    }
}

// @func 0x001271D8 Character::OnHitCategory2
// Blasts: full damage inside the inner radius, falling off to the outer one.
void Character::OnHitCategory2(EntityHitRecord& hit)
{
    if (!ReceiveHit(hit))
        return;
    const auto& blast = hit.data.blast;
    float falloff = 1.0f;
    if (blast.innerRadius < blast.distance && blast.innerRadius < blast.outerRadius)
        falloff = (blast.outerRadius - blast.distance) / (blast.outerRadius - blast.innerRadius);
    float amount = blast.damage * falloff * damage.blast;

    Entity* source = hit.source;
    Entity* attacker = IsCharacterEntity(source) ? source : nullptr;
    // A character's own thrown explosive never hurts it.
    if (source && (source->typeBits[0] & kType_ThrownBit) && ThrownBySelf(source, this))
        amount = 0.0f;
    if (attacker && IsFriendlyHit(attacker))
        amount *= objects::g_characterDamageGlobalFriendly;
    ApplyDamage(amount, attacker, hit, kType_Blast);
    hit.flags |= EntityHitRecord::kHit_Fatal;
}

// @func 0x00127358 Character::OnHitCategory3
// Environment damage, scaled by the kind of reaction (fire, electricity,
// poison) unless the source is an environment entity.
void Character::OnHitCategory3(EntityHitRecord& hit)
{
    if (!ReceiveHit(hit))
        return;
    Entity* source = hit.source;
    float scale = 1.0f;
    if (!source || !(source->typeBits[0] & kType_EnvironmentBit)) {
        if (hit.reaction == kReaction_Electric)
            scale = damage.environmentElectric;
        else if (hit.reaction == kReaction_Fire)
            scale = damage.environmentFire;
        else if (hit.reaction == kReaction_Poison)
            scale = damage.environment * objects::g_characterPoisonDamageRatio;
        else
            scale = damage.environment;
    }
    float amount = hit.data.knock.amount * scale;
    if (IsCharacterEntity(source) && TeamOf(this) == TeamOf(source) &&
        (!(classBits & kClass_Disguised) || scale <= 0.0f) && !(source->classBits & kClass_HurtsFriends))
        amount *= objects::g_characterDamageGlobalFriendly;
    ApplyDamage(amount, source, hit, kType_Environment);
}

// @func 0x001274D4 Character::OnHitCategory4
void Character::OnHitCategory4(EntityHitRecord& hit)
{
    if (!ReceiveHit(hit))
        return;
    Entity* source = hit.source;
    float amount = hit.data.projectile.damage * damage.projectile;
    if (IsCharacterEntity(source) && IsFriendlyHit(source))
        amount *= objects::g_characterDamageGlobalFriendly;
    ApplyDamage(amount, source, hit, kType_Projectile);
}

// @func 0x001275B8 Character::Damage
bool Character::Damage(float amount, Entity* source, uint32_t type)
{
    if (health <= 0.0f)
        return false;
    const bool isPlayer = (classBits & 1) != 0;
    if (isPlayer && objects::g_cheatInvulnerable) {
        amount = 0.0f;
    } else if (!isPlayer && listener) {
        // AI characters that ignore damage, and neutral ones, take none.
        if (ListenerIgnoresDamage(listener) || (TeamOf(this) == 0 && !(classBits & kClass_HurtsFriends)))
            amount = 0.0f;
    }
    const bool alive = ScriptedEntity::Damage(amount, source, type);
    if (amount > 0.0f) {
        DropCarried(false);
        if (isPlayer)
            GameWorldPlayerDamaged(PlayerSlot() & 0xFF);
        else if (classBits & kClass_400000)
            GameWorldNpcDamaged(this);
        if (type != kType_Silent)
            GameWorldPostEntityEvent(uint8_t(PlayerSlot()), 0x10, this);
    }
    return alive;
}

// TODO(0x00125A80 Character::ApplyDamage)
void Character::ApplyDamage(float amount, Entity* attacker, EntityHitRecord& hit, uint32_t type)
{
    (void)amount; (void)attacker; (void)hit; (void)type;
}

// -- animation ----------------------------------------------------------------------------------

// @func 0x0012852C Character::ApplyRootMotion
void Character::ApplyRootMotion()
{
    if (animController) {
        ape::Matrix34 delta;
        EntityRootMotionDelta(this, delta, false);
        const bool moved = !ape::Matrix34Equal(0.0f, delta, kIdentity);
        if (moved && (flags & kEnt_RootMotion)) {
            // NOTE: the console multiplies in place with an AltiVec helper;
            // the delta is taken as local to the current transform.
            ape::Matrix34 world;
            ape::world::Matrix34Mul(world, delta, transform);
            EntitySetWorldTransform(this, world);
            SetLocalTransform(world);
        }
    }
    // The root animation ended: stop following it.
    if (StopAnim(0, false) && !IsAnimFinished(0))
        flags &= ~kEnt_RootMotion;
}

// @func 0x001287F8 Character::FindAnimClip
void* Character::FindAnimClip(uint32_t nameHash)
{
    EntityComponent* animMap = EntityComponent0E(this);
    void* clip = animMap ? AnimMapFindClip(animMap, nameHash) : nullptr;
    if (!clip)
        clip = Entity::FindAnimClip(nameHash);
    if (!clip && animMap)
        ape::TempFormat("%s: no animation clip %s", object ? object->GetName() : "<unknown>",
                        ape::TempHexString(nameHash));
    return clip;
}

// @func 0x001288C8 Character::FindAnimClipByName
// Without an animation map clips are named "<prefix>_<name>".
void* Character::FindAnimClipByName(const char* name)
{
    EntityComponent* animMap = EntityComponent0E(this);
    if (!animMap)
        return Entity::FindAnimClipByName(ape::TempFormat("%s_%s", DefOf(this)->animPrefix, name));
    void* clip = AnimMapFindClip(animMap, ape::HashNameNoCase(name));
    if (!clip)
        ape::TempFormat("%s: no animation clip %s", object ? object->GetName() : "<unknown>", name);
    return clip;
}

// @func 0x00127AB4 Character::GetUpVector
ape::Vec3 Character::GetUpVector()
{
    EntityComponent* physics = GetComponent00();
    return physics ? PhysicsCompUpVector(physics) : ape::Vec3{ 0.0f, 0.0f, 0.0f, 0.0f };
}

// @func 0x00127B3C Character::GetSideVector
ape::Vec3 Character::GetSideVector()
{
    EntityComponent* physics = GetComponent00();
    return physics ? PhysicsCompSideVector(physics) : ape::Vec3{ 0.0f, 0.0f, 0.0f, 0.0f };
}

// -- interaction ----------------------------------------------------------------------------------

// @func 0x00127E20 Character::InteractionOffer
// Players offer kind 0; anyone offers kind 3 to things they threw, own or spawned.
uint32_t Character::InteractionOffer(int kind, Entity* other)
{
    (void)kind;
    uint32_t offer = (classBits & 1) ? 1u : 0u;
    if (other) {
        const uint32_t type = other->typeBits[0];
        if (((type & kType_ThrownBit) && HitSourceOwner(other, kHitOwner_Thrower) == this) ||
            ((type & kType_OwnedBit) && HitSourceOwner(other, kHitOwner_Owner) == this) ||
            ((type & kType_SpawnedBit) && HitSourceOwner(other, kHitOwner_Spawner) == this))
            offer |= 8;
    }
    return offer;
}

// @func 0x00127BC8 Character::CanInteractWith
bool Character::CanInteractWith(Entity* other, int kind)
{
    const bool isPlayer = (classBits & 1) != 0;
    if (kind == 5)
        return false;
    if (kind == kInteract_Player && !DefOf(this)->allowPlayerInteraction)
        return false;
    if (health <= 0.0f)
        return false;
    // Characters of one team do not interact (except kind 4 and friendly-fire characters).
    if (kind != 4 && IsCharacterEntity(other) && TeamOf(this) == TeamOf(other) && !(classBits & kClass_Disguised))
        return false;
    if (isPlayer) {
        if (kind == 0) {
            EntityComponent* driver = GetComponent(kComp_17);
            if (driver && AnimDriverState(driver) == 1)
                return false;
        }
        if (classBits & (0x10 | 0x1000000))
            return false;
    }
    if (kind == 1) {
        // Only from the idle states, and only without ability 12.
        if (EntityGetState(this, 0) != 0 || EntityGetState(this, 1) != 0 || EntityGetState(this, 2) != 3)
            return false;
        if (abilities.IsUsable(12))
            return false;
    }
    if (kind == 4 && listener && ListenerIsAlly(listener))
        return false;
    return Entity::CanInteractWith(other, kind);
}

// -- scripts and character swaps -----------------------------------------------------------------

namespace {
constexpr uint32_t kState_End = 0x00010CBBu;   // "End"
}

// @func 0x00128C74 Character::PlayCharacterScript
ape::ApeAnimScriptInstance* Character::PlayCharacterScript(uint32_t scriptName, bool stopCurrent)
{
    ape::ApeAnimScriptLibrary* library = ape::ApeAnimScriptLibrary::Get(0);
    if (!library)
        return nullptr;
    ape::ApeAnimScript* script = library->FindScript(scriptName);
    ape::ApeAnimScriptInstance* instance = nullptr;
    if (animScript198 && (script || stopCurrent)) {
        auto* current = static_cast<ape::ApeAnimScriptInstance*>(animScript198);
        current->SetState(kState_End);
        DetachAnimScript(current);
        animScript198 = nullptr;
    }
    if (script) {
        instance = EntityStartScript(this, script, false, true);
        animScript198 = instance;
        if (instance)
            AttachAnimScript(instance);
    }
    return instance;
}

// @func 0x00125950 Character::BecomeSly
void Character::BecomeSly()
{
    SwapCharacter(FindEntityDef(kChar_Sly), false);
}

// @func 0x00125998 Character::EndDisguise
// A disguised character that is hit turns back into Sly; the player block
// forgets the disguise.
void Character::EndDisguise()
{
    if (EntityGetState(this, 0) == 5 || !(classBits & kClass_Disguised))
        return;
    SwapCharacter(FindEntityDef(kChar_Sly), false);
    PlayerBlockDisguiseEnded(PlayerSlot() & 0xFF);
}

// @func 0x00129C18 Character::EndScript19C
void Character::EndScript19C()
{
    auto* script = static_cast<ape::ApeAnimScriptInstance*>(animScript19C);
    if (!script)
        return;
    ape::ApeAnimScriptState* state = script->GetState();
    if (state && state->GetNameHash() != kState_End) {
        script->SetState(kState_End);
        script->Update(1.0f / 60.0f);
    }
    DetachAnimScript(script);
    animScript19C = nullptr;
}

// @func 0x00129CB8 Character::PlayScript19C
void Character::PlayScript19C(uint32_t scriptName)
{
    EndScript19C();
    void* script = EntityPlayDirectorScriptInstance(this, scriptName);
    animScript19C = script;
    if (script)
        AttachAnimScript(static_cast<ape::ApeAnimScriptInstance*>(script));
}

// @func 0x00129B58 Character::ReleaseCullHandle
void Character::ReleaseCullHandle()
{
    if (cullHandleUsed != 0) {
        CullManagerRelease(cullHandle);
        cullHandleUsed = 0;
    }
}

// @func 0x0012BC20 Character::SetPhysicsTarget
void Character::SetPhysicsTarget(const ape::Vec3& point)
{
    if (EntityComponent* physics = GetComponent00())
        PhysicsCompSetTarget(physics, point);
}

// TODO(0x00124D58 Character::SwapCharacter)
void Character::SwapCharacter(EntityDef* newDef, bool keepState)
{
    (void)newDef; (void)keepState;
}

// @func 0x00128E2C Character::SetVisible
void Character::SetVisible(bool visible, bool immediate, bool includeChildren, bool components)
{
    if (carried)
        carried->SetVisible(visible, immediate, includeChildren, components);
    Entity::SetVisible(visible, immediate, includeChildren, components);
}

// -- entity slot overrides ----------------------------------------------------------------

// @func 0x00124954 Character::GetRenderId
// NOTE: slots 25/26 are named "render id" on Entity; for characters the value
// is the collision group of the physics body.
uint32_t Character::GetRenderId()
{
    if (!PhysicsWorldPresent())
        return 0;
    EntityComponent* physics = GetComponent00();
    return physics ? PhysicsCompCollisionGroup(physics) : 0;
}

// @func 0x00124BC0 Character::GetGravityFactor
float Character::GetGravityFactor()
{
    // While attached the parent's link keeps the factor to restore.
    if (attachParent) {
        const EntityChildLink* link = attachParent->FindChildLink(this);
        return link ? link->restoreBlend : 0.0f;
    }
    EntityComponent* physics = GetComponent00();
    void* body = physics ? PhysicsCompRigidBody(physics) : nullptr;
    return body ? RigidBodyGravityFactor(body) : 0.0f;
}

// @func 0x00124C94 Character::Detached
void Character::Detached(float gravityFactor)
{
    EntityComponent* physics = GetComponent00();
    if (void* body = physics ? PhysicsCompRigidBody(physics) : nullptr)
        RigidBodySetGravityFactor(body, gravityFactor);
    Entity::Detached(gravityFactor);
}

// @func 0x0012905C Character::GetAimPosition
ape::Vec3 Character::GetAimPosition()
{
    EntityComponent* component = GetComponent00();
    return component ? CharacterCompAimPosition(component) : ape::Vec3{ 0.0f, 0.0f, 0.0f, 0.0f };
}

// @func 0x001290F0 Character::GetTurnRate
float Character::GetTurnRate()
{
    EntityComponent* component = GetComponent00();
    return component ? CharacterCompTurnRate(component) : 0.0f;
}

} // namespace entity
} // namespace sly
