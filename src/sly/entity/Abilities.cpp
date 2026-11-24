// openTIT - Character abilities (see Abilities.h).
#include "sly/entity/Abilities.h"

namespace sly {
namespace entity {

namespace {

// Ability table (0x00AC9A18) merged with the control masks (0x00ACA980).
const AbilityInfo kAbilities[kAbilityCount] = {
    { "Turn", 0x0027787Du, 0x00000A00u, 3.5f, "", "", 0x01000000u, 0x00000000u },
    { "Walk", 0x002889C9u, 0x00000A00u, 3.5f, "", "", 0x01000000u, 0x00000000u },
    { "Run", 0x00013E6Bu, 0x00000A00u, 3.5f, "", "", 0x01000000u, 0x00000000u },
    { "Jump", 0x0022EC2Eu, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "Sprint", 0x922FF61Au, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "DoubleJump", 0x4B5F0B7Fu, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "TripleJump", 0x52CC8D8Cu, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "SuperJump", 0x924B3E09u, 0x00010A00u, 3.5f, "$BUTTON_RTRIG2 $UPGRADE_S_SUPERJUMP_TITLE", "$BUTTON_X $UPGRADE_S_SUPERJUMP_TITLE", 0x00000000u, 0x00000000u },
    { "Glide", 0x040C2B85u, 0x00011400u, 3.5f, "$BUTTON_RTRIG1 $STR_PARAGLIDER", "$BUTTON_RTRIG1 $STR_MAGICCARPET", 0x00000000u, 0x00000000u },
    { "Hover", 0x041BD11Cu, 0x01011400u, 3.5f, "$BUTTON_X $STR_HOVER", "", 0x00000000u, 0x00000000u },
    { "AirSprint", 0xC884EBC4u, 0x01010A00u, 3.5f, "$BUTTON_RTRIG1 $UPGRADE_B_SPRINT_AIR_TITLE", "", 0x00000000u, 0x00000000u },
    { "CarrySprint", 0xB8C6FED5u, 0x00010A00u, 3.5f, "$BUTTON_RTRIG1 $STR_CARRYSPRINT", "", 0x00000000u, 0x00000000u },
    { "QuietSprint", 0x6DAF452Eu, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "Sneak", 0x04B61EF4u, 0x00000A00u, 3.5f, "", "", 0x01000000u, 0x00000000u },
    { "Skid", 0x0026DD73u, 0x00000A00u, 3.5f, "", "", 0x01000000u, 0x00000000u },
    { "Combat", 0x76D1F2F4u, 0x00000A00u, 3.5f, "", "", 0x01000000u, 0x00000000u },
    { "CombatStrafe", 0xF7ED2103u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "ClimbRail", 0x4196BFA7u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000044u },
    { "ClimbPoint", 0xF12B63FBu, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000008u },
    { "ClimbPole", 0x41960BCDu, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000010u },
    { "ClimbChain", 0xF070E52Cu, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000020u },
    { "ClimbZipline", 0x5B052A40u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000080u },
    { "ClimbSwing", 0xF1594D69u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000100u },
    { "ClimbHang", 0x41923487u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000200u },
    { "ClimbGrind", 0xF0ADED7Fu, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00002400u },
    { "ClimbSpring", 0x3971C0E2u, 0x00000A00u, 3.5f, "$LSTICK_UD_ANIM $STR_PULLDOWN", "$BUTTON_X $STR_SPRING", 0x00000000u, 0x00000800u },
    { "ClimbSidle", 0xF152DD32u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00001000u },
    { "ClimbVehicle", 0x8026AAD7u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00004000u },
    { "ClimbStealth", 0xFADEE3AEu, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00008000u },
    { "SuperRail", 0x924E956Du, 0x00020A00u, 3.5f, "$BUTTON_RTRIG1 $UPGRADE_S_RAILRUN_SPRINT_TITLE", "", 0x00000000u, 0x00000044u },
    { "SuperPoint", 0xB76E46F5u, 0x00020A00u, 3.5f, "$BUTTON_R2_ANIM $JAPAN_RIOICHI_SPECIAL01", "$JAPAN_RIOICHI_SPECIAL02", 0x00000000u, 0x00000000u },
    { "SuperPole", 0x924DE193u, 0x00020A00u, 3.5f, "$BUTTON_RTRIG2 $STR_COBRA_CHARGE", "$LSTICK_UD_ANIM $STR_COBRA_CLIMB", 0x00000000u, 0x00000000u },
    { "SuperGrind", 0xB6F0D079u, 0x00020A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "SuperSpring", 0x3B8B3D28u, 0x0002FF00u, 999999.0f, "$BUTTON_RTRIG2 $STR_CHARGE", "$BUTTON_X $STR_CATAPULT", 0x00000000u, 0x00000000u },
    { "SuperTrack", 0xB7A7E2B0u, 0x00020A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "SpringJump", 0x6E1F13BBu, 0x00020A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "ClimbAttack", 0x1AF3AD1Du, 0x00000A00u, 3.5f, "", "", 0x01000000u, 0x00000000u },
    { "Detach", 0x77FD0B93u, 0x0000FF00u, 999999.0f, "$BUTTON_TRIANGLE $STR_DISCONNECT", "", 0x00000000u, 0x00000000u },
    { "EdgeGrab", 0x89EEC189u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "EdgePeer", 0x89F2A89Fu, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "LedgePerch", 0x3A302FB9u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CrawlView", 0x1B4B432Cu, 0x0000FF00u, 3.5f, "", "", 0x00000000u, 0x00010000u },
    { "Crawl", 0x03D66C07u, 0x00010A00u, 3.5f, "$BUTTON_CIRCLE $STR_CRAWL", "$LSTICK_CIRCLE_ANIM $STR_MOVE", 0x00000000u, 0x00010000u },
    { "Disguise", 0x3F2950FFu, 0x0000FF00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "Interact", 0x50A58636u, 0x0001FF00u, 3.5f, "$STR_PRESS_INTERACTBUTTON_TO_ACTIVATE", "", 0x01000000u, 0x00000000u },
    { "InteractTreasure", 0x754A02E5u, 0x0001FF00u, 3.5f, "$STR_PRESS_INTERACTBUTTON_TO_ACTIVATE", "", 0x00000000u, 0x00000000u },
    { "Pickpocket", 0xDAD5A4F7u, 0x00010A00u, 3.5f, "$BUTTON_CIRCLE $STR_PICKPOCKET", "$BUTTON_CIRCLE $STR_PICKPOCKET", 0x00000000u, 0x00000000u },
    { "PickpocketMoving", 0xB7146C05u, 0x00010A00u, 3.5f, "$BUTTON_CIRCLE $UPGRADE_B_PICKPOCKET_MOVING_TITLE", "", 0x00000000u, 0x00000000u },
    { "Carry", 0x03CEF0FBu, 0x00010A00u, 3.5f, "$STR_STOMP", "", 0x00000000u, 0x00000000u },
    { "CarryThrowAim", 0x640A569Au, 0x00010A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CarryThrow", 0x691922ABu, 0x00010A00u, 3.5f, "$BUTTON_SQUARE $STR_THROW", "", 0x00000000u, 0x00000000u },
    { "CarryShake", 0x690ACACBu, 0x00010A00u, 3.5f, "$BUTTON_CIRCLE $STR_SHAKE", "", 0x00000000u, 0x00000000u },
    { "CarrySlam", 0xB0CEDBC0u, 0x00010A00u, 3.5f, "$BUTTON_TRIANGLE $STR_SLAM", "", 0x00000000u, 0x00000000u },
    { "LootyShake", 0x4D86C6D5u, 0x00020A00u, 3.5f, "$BUTTON_CIRCLE $STR_SHAKE", "", 0x00000000u, 0x00000000u },
    { "Binocucom", 0x81A72067u, 0x00000A00u, 3.5f, "$BUTTON_LSTICK $STR_BINOCUCOM", "", 0x00000000u, 0x00000000u },
    { "BinocShoot", 0xB32303E4u, 0x00010A00u, 3.5f, "$BUTTON_RTRIG1 $STR_SHOOT", "", 0x00000000u, 0x00000000u },
    { "Cloak", 0x03D3E3B0u, 0x01010A00u, 3.5f, "$BUTTON_RTRIG2 $STR_CLOAK", "", 0x00000000u, 0x00000000u },
    { "SlowTime", 0xEE1B444Eu, 0x01010A00u, 3.5f, "$BUTTON_RTRIG2 $STR_SLOWTIME", "", 0x00000000u, 0x00000000u },
    { "Archery", 0xFE345D36u, 0x00010A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "BombPlant", 0xA68C04E9u, 0x00010A00u, 3.5f, "$BUTTON_TRIANGLE $STR_BOMB_ARM", "$BUTTON_TRIANGLE_ANIM $STR_BOMB_DROP", 0x00000000u, 0x00000000u },
    { "BombThrow", 0xA6C2D164u, 0x00010A00u, 3.5f, "$BUTTON_LTRIG1 $STR_BOMB_AIM", "$BUTTON_TRIANGLE $STR_THROW", 0x00000000u, 0x00000000u },
    { "BombTrigger", 0x11E1D4B6u, 0x00010A00u, 3.5f, "$BUTTON_RTRIG2 $STR_TRIGGER", "", 0x00000000u, 0x00000000u },
    { "WallClimb", 0xFD4BD30Bu, 0x0001FF00u, 3.5f, "$BUTTON_X$STR_SIGNPLUS$BUTTON_CIRCLE $STR_WALLATTACH", "", 0x00000000u, 0x00020000u },
    { "WallLeap", 0x41FE2C32u, 0x00010A00u, 3.5f, "$BUTTON_X $STR_LEAP", "", 0x00000000u, 0x00000000u },
    { "WallSlide", 0xFE2D4A07u, 0x00010A00u, 3.5f, "$BUTTON_RTRIG1 $STR_WALLSLIDE", "", 0x00000000u, 0x00000000u },
    { "WallPound", 0xFE048EE2u, 0x00010A00u, 3.5f, "$BUTTON_SQUARE $STR_WALLPOUND", "", 0x00000000u, 0x00000000u },
    { "ShootAim", 0x1A117CC6u, 0x00000A00u, 3.5f, "$BUTTON_LTRIG1 $STR_AIM", "$BUTTON_RTRIG1 $STR_SHOOT", 0x00000000u, 0x00000000u },
    { "SuperShot", 0x924F24D5u, 0x01010A00u, 3.5f, "$BUTTON_RTRIG2 $STR_AIM", "", 0x00000000u, 0x00000000u },
    { "Ball", 0x001EFDFFu, 0x00010A00u, 3.5f, "$BUTTON_RTRIG2 $STR_BALL", "", 0x00000000u, 0x00000000u },
    { "AutoClimb", 0x09522E46u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "PlayDead", 0x9EA8BD98u, 0x00010A00u, 3.5f, "$BUTTON_RTRIG2 $STR_PLAYDEAD", "", 0x00000000u, 0x00000000u },
    { "Pounce", 0x8D04C33Au, 0x01010A00u, 3.5f, "$BUTTON_X $STR_CYCLONE_POUNCE", "", 0x01000000u, 0x00000000u },
    { "Rewind", 0x8FE260BBu, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CustomTakedown", 0xABAF2C1Au, 0x00010A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "LoudTakedown", 0xC21FFAFBu, 0x00010A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "SprintTurbo1", 0x236CA48Du, 0x01010A00u, 3.5f, "$BUTTON_RTRIG1,$BUTTON_RTRIG1 $UPGRADE_B_SPRINT_BURST_01_TITLE", "", 0x00000000u, 0x00000000u },
    { "SprintTurbo2", 0x236CA48Eu, 0x01010A00u, 3.5f, "$BUTTON_RTRIG1,$BUTTON_RTRIG1 $UPGRADE_B_SPRINT_BURST_02_TITLE", "", 0x00000000u, 0x00000000u },
    { "SprintAttack1", 0x5B783F8Fu, 0x01010A00u, 3.5f, "$BUTTON_RTRIG1,$BUTTON_RTRIG1 $STR_SPRINT_ATTACK", "", 0x00000000u, 0x00000000u },
    { "SprintAttack2", 0x5B783F90u, 0x01010A00u, 3.5f, "$BUTTON_RTRIG1,$BUTTON_RTRIG1 $STR_SPRINT_ATTACK", "", 0x00000000u, 0x00000000u },
    { "SmokeBomb", 0x27DDC471u, 0x01010A00u, 3.5f, "$BUTTON_LTRIG1 $STR_SMOKEBOMB", "", 0x00000000u, 0x00000000u },
    { "FlashBomb", 0x61BCC272u, 0x01010A00u, 3.5f, "$BUTTON_LTRIG1 $STR_SMOKEBOMB", "", 0x00000000u, 0x00000000u },
    { "SpireAttack", 0xE07D2FC7u, 0x00010A00u, 3.5f, "$BUTTON_RTRIG1 $UPGRADE_S_PICKPOCKET_SPIRE_TITLE", "", 0x00000000u, 0x00000000u },
    { "GlideAttack", 0x664E960Du, 0x00010A00u, 3.5f, "$BUTTON_RTRIG1 $STR_GLIDEATTACK", "", 0x00000000u, 0x00000000u },
    { "HoverAttack", 0x771B6E64u, 0x00010A00u, 3.5f, "$BUTTON_X $UPGRADE_B_HOVERPACK_02_TITLE", "", 0x00000000u, 0x00000000u },
    { "FastRecharge", 0xC1812303u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "Roar", 0x0026772Eu, 0x01010A00u, 3.5f, "$BUTTON_RTRIG2 $STR_ROAR", "", 0x00000000u, 0x00000000u },
    { "ShootingAutoFire", 0xC32CDCC8u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "ShootingChargeShot", 0xF6FB6B51u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "ShootingTripleShot", 0xD84D6FFBu, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "ShootingUpgrade1", 0xC23E2218u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "ShootingUpgrade2", 0xC23E2219u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CombatUp1", 0x3A010182u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CombatUp2", 0x3A010183u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CombatUp3", 0x3A010184u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CombatUp4", 0x3A010185u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CombatUp5", 0x3A010186u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CombatUp6", 0x3A010187u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CombatUp7", 0x3A010188u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CombatUp8", 0x3A010189u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CombatUp9", 0x3A01018Au, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CombatUp10", 0x061F2EEEu, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CombatUp11", 0x061F2EEFu, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CombatUp12", 0x061F2EF0u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CombatUp13", 0x061F2EF1u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CombatUp14", 0x061F2EF2u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CombatUp15", 0x061F2EF3u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
    { "CombatUp16", 0x061F2EF4u, 0x00000A00u, 3.5f, "", "", 0x00000000u, 0x00000000u },
};

} // namespace

const AbilityInfo& GetAbility(int index)
{
    return kAbilities[index];
}

// @func 0x001205AC FindAbility
int FindAbility(uint32_t nameHash)
{
    if (nameHash == 0)
        return -1;
    for (int i = 0; i < kAbilityCount; ++i)
        if (kAbilities[i].nameHash == nameHash)
            return i;
    return -1;
}

// @func 0x0012B630 AbilityHash
uint32_t AbilityHash(int index)
{
    return kAbilities[index].nameHash;
}

// @func 0x0012A71C AbilityControlMask
uint32_t AbilityControlMask(const AbilitySet& set, int moveState)
{
    uint32_t mask = 0;
    for (int i = 0; i < kAbilityCount; ++i)
        if (set.IsUsable(i))
            mask |= kAbilities[i].controlMask;
    if (moveState == 1)
        mask &= ~0x10000u;
    else if (moveState == 7)
        mask &= ~0x20000u;
    return mask;
}

} // namespace entity
} // namespace sly
