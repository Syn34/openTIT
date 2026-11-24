// openTIT - Bentley vs Penelope boss fight: the two fighters and the battle
// camera settings.
//
// Each fighter is a layered state machine (see StateMachine.h) driving an
// actor pair: the fighter's own actor (+0x60) and a second actor that plays
// matching clips (+0x64, e.g. the mech's pilot). Clip names are built from
// the actors' animation prefixes: "Clip_<prefixA>_<move>" and
// "<prefixB>_<move>".
//
// Vtables: BvpFighter 0x00B202F0, BvpBentley 0x00B20398, BvpPenelope
// 0x00B203E0. The big per-state handlers of both fighters (0x003AD270,
// 0x003AECB0) are not reconstructed yet.
#pragma once

#include "sly/game/Tween.h"
#include "sly/gameplay/StateMachine.h"

#include "ape/math/ApeMath.h"

#include <cstdint>
#include <vector>

namespace sly {

class WorldObject;

namespace gameplay {

// The battle object both fighters belong to (fighter +0x5C, vtable
// 0x00B20338, 0x1E0 bytes). NOTE: its real base is the game's goal class
// (constructor 0x0047C5D8, destructor 0x00490B34), modelled here by the
// StateMachine stand-in.
class BvpBattle : public StateMachine {
public:
    // Number of camera parameters blended between the default and the
    // super attack camera.
    static constexpr int kCameraParamCount = 9;
    enum CameraMode : int32_t { kCamera_None = 0, kCamera_Default = 1, kCamera_Super = 2 };
    // Battle phases (state layer 0).
    enum Phase : int32_t { kPhase_Intro = 0, kPhase_Fight = 1, kPhase_HockeyFight = 2 };

    explicit BvpBattle(WorldObject* owner);
    ~BvpBattle() override;

    // Slot 9: routes messages to the handler of the current phase while the
    // owner is not finished (owner byte +0x12 == 6) and the battle is not
    // stopped.
    bool OnMessage(const Message& message) override;
    // Slot 20: back to the intro.
    void Reset();

    // Blends the camera to the default framing (snap: jump straight there).
    void SetDefaultCamera(bool snap);
    // Blends the camera to the super attack framing.
    void SetSuperCamera(bool snap);

    struct EnemySlot {
        WorldObject* actor = nullptr;   // +0x00
        uint32_t field_04 = 0;          // +0x04
    };

    WorldObject* owner = nullptr;                   // +0x38
    uint32_t field_3C = 0;                          // +0x3C
    uint32_t field_40 = 0;                          // +0x40
    uint32_t field_44 = 0;                          // +0x44
    ape::Vec3 field_50 = {0.0f, 0.0f, 0.0f, 0.0f};  // +0x50
    uint32_t field_60 = 0;                          // +0x60
    uint32_t field_64 = 0;                          // +0x64
    uint32_t field_68 = 0;                          // +0x68
    uint32_t field_6C = 0;                          // +0x6C
    std::vector<EnemySlot> enemies;                 // +0x70 (count +0x78)
    // Camera distances above/from the player, focal distances and FOV, in
    // that order (+0x84, 0x24 bytes each).
    FloatTween cameraParams[kCameraParamCount];
    uint32_t field_1C8 = 0;                         // +0x1C8
    int32_t cameraMode = kCamera_None;              // +0x1CC
    bool field_1D0 = false;                         // +0x1D0
    bool field_1D1 = false;                         // +0x1D1
    bool stopped = false;                           // +0x1D2
    bool controlPenelope = false;                   // +0x1D3
    bool field_1D4 = false;                         // +0x1D4

private:
    void BlendCamera(const float targets[kCameraParamCount], float time, bool snap);
    // Per-phase handlers (0x003B0BA8, 0x003B2718, 0x003B1E88).
    void UpdateIntro();
    bool HandleFight(const Message& message);
    bool HandleHockeyFight(const Message& message);
};

class BvpFighter : public StateMachine {
public:
    // Takes a reference on each actor.
    BvpFighter(BvpBattle* battle, WorldObject* actor, WorldObject* partnerActor,
               WorldObject* victim);
    ~BvpFighter() override;

    bool OnMessage(const Message& message) override;
    // Slot 13: per-frame hook of the derived fighters (none in the base).
    virtual void UpdateExtra();

    // Damages the current victim; a non-lethal fighter never takes it below
    // 5% of its maximum health.
    void DamageVictim(float amount);
    // Plays a clip pair; returns false if clip A is already playing and
    // 'force' is not set.
    bool PlayClips(float speed, uint32_t clipA, uint32_t clipB, bool loop, bool force);
    void PlayIdle();
    // Walk clip in the direction of the left stick, at the stick's speed.
    void PlayWalk();
    void PlayDodge();
    void PlayAttack(uint32_t index);
    void PlaySuperAttack();
    void PlayHitReact();
    void PlayHockeyFightShot();
    void PlayHockeyFightSuccess();
    void PlayHockeyFightFailure();
    // Latches m_clipDone once the actor's clip has finished.
    void UpdateClipDone();

    float DistanceToOpponent() const;
    // Progress (0..1) of the actor's current animation.
    float ClipProgress() const;
    // Health fraction of the current victim (1 without one).
    float VictimHealthFraction() const;
    // Kills every battle enemy within sqrt(radiusSq) of the actor.
    void DamageNearbyEnemies(float radiusSq);
    // True if 'point' lies within range (plus the opponent's reach) and the
    // opponent is within maxAngleDeg of the actor's facing.
    bool CanHit(float range, float maxAngleDeg, const ape::Vec3& point) const;
    bool CanHitOpponent(float range, float maxAngleDeg) const;

protected:
    void PlayMove(const char* move, float speed, bool loop, bool force);

public:
    char actorPrefixA[0x20] = {};      // +0x1C
    char actorPrefixB[0x20] = {};      // +0x3C
    BvpBattle* battle = nullptr;       // +0x5C
    WorldObject* actor = nullptr;      // +0x60
    WorldObject* partnerActor = nullptr; // +0x64
    WorldObject* victim = nullptr;     // +0x68
    BvpFighter* opponent = nullptr;    // +0x6C
    uint32_t currentClip = 0;          // +0x70
    int32_t hitReact = 3;              // +0x74 index into the hit react clips
    uint32_t field_78 = 0;             // +0x78
    uint32_t field_7C = 0;             // +0x7C
    float reactTime = 0.0f;            // +0x80
    float reach = 0.0f;                // +0x84
    float stick[4] = {};               // +0x88 left X/Y, right X/Y
    bool field_98 = false;             // +0x98
    bool field_99 = false;             // +0x99
    bool dodgePressed = false;         // +0x9A
    bool field_9B = false;             // +0x9B
    bool superPressed = false;         // +0x9C
    bool field_9D = false;             // +0x9D
    bool field_9E = false;             // +0x9E
    bool heavyHit = false;             // +0x9F
    bool field_A1 = true;              // +0xA1
    bool healthCapped = false;         // +0xA2
    bool nonLethal = false;            // +0xA3
    bool clipDone = false;             // +0xA4
    float power = 0.0f;                // +0xA8
    float stateTimer = 0.0f;           // +0xAC
    bool dodged = false;               // +0xB5
};

class BvpBentley : public BvpFighter {
public:
    using BvpFighter::BvpFighter;
    ~BvpBentley() override;

    // Slot 10.
    void Start();
    // Slot 11: returns false while invulnerable (in state 7, or late in a
    // dodge).
    bool TakeHit(float reactTime, bool heavy, int32_t hitReact);
    // Slot 12: idle without stick input, walking otherwise.
    void ChooseMove();
    // Slot 14: samples the pad.
    void ReadInput();

    bool HasSuperMovePower();
    bool HasDodgePower();
};

class BvpPenelope : public BvpFighter {
public:
    using BvpFighter::BvpFighter;
    ~BvpPenelope() override;

    void StartChase();
    // Slot 10.
    void Start();
    // Slot 11.
    bool TakeHit(float reactTime, bool heavy, int32_t hitReact);
    // Slot 12: chases while far from Bentley.
    void ChooseMove();
    // Slot 13: dodges after a light hit, or after a health-dependent delay.
    void UpdateExtra() override;
    // Slot 14.
    void ReadInput();
};

} // namespace gameplay
} // namespace sly
