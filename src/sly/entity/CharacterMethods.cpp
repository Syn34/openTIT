// openTIT - Character helpers and entity-slot overrides (0x0011F000-0x0012A000).
//
// Model variants, the weapon attachment, profile costumes, tinting, ability
// states and the small overrides of the entity interface. The character
// component and the game world's profile records are reached through
// CharacterHooks.h.
#include "sly/entity/Character.h"

#include "ape/anim/ApeAnimScript.h"
#include "ape/core/ApeString.h"
#include "ape/world/ApeDef.h"
#include "ape/world/ApeDefTypes.h"
#include "ape/world/ApeObj.h"
#include "sly/components/ComponentDescs.h"
#include "sly/entity/CharacterHooks.h"
#include "sly/entity/GameEntityHooks.h"

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
            if (field_1A8 != 0)
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

// -- entity slot overrides ----------------------------------------------------------------

// @func 0x00124954 Character::GetRenderId
uint32_t Character::GetRenderId()
{
    if (!RenderIdOverrideActive())
        return 0;
    EntityComponent* component = GetComponent00();
    return component ? CharacterCompRenderId(component) : 0;
}

// @func 0x00124BC0 Character::GetAttachBlend
float Character::GetAttachBlend()
{
    if (attachParent) {
        const EntityChildLink* link = attachParent->FindChildLink(this);
        return link ? link->restoreBlend : 0.0f;
    }
    EntityComponent* component = GetComponent00();
    if (!component)
        return 0.0f;
    ape::world::ApeObj* render = ComponentRenderObject(component);
    return render ? RenderObjectPackedFade(render) : 0.0f;
}

// @func 0x00124C94 Character::Detached
void Character::Detached(float blend)
{
    if (EntityComponent* component = GetComponent00()) {
        if (ape::world::ApeObj* render = ComponentRenderObject(component))
            RenderObjectSetPackedFade(render, blend);
    }
    Entity::Detached(blend);
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
