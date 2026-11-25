// openTIT - Calls from the goal module into systems that are not ported yet
// (job manager internals, sequence streaming, HUD objects, lasers, entity
// virtual slots, the game world's minigame slot).
//
// GoalHooks.cpp holds TODO stubs with safe defaults. Each declaration names
// the console function or member it stands for.
#pragma once

#include <cstddef>
#include <cstdint>

namespace ape { class ApeAnimScriptInstance; }

namespace sly {

class SlyGame;
namespace gameplay { class Minigame; }

namespace goals {

class Goal;
class Job;
struct GoalScriptBinding;

// ---------------------------------------------------------------------------
// Jobs (Goal::job) and the job manager (0x00F64C20)
// ---------------------------------------------------------------------------
// Goal list of a job (ApeTArray<Goal*> at +0x50).
uint32_t JobGoalCount(const Job* job);
Goal* JobGoalAt(const Job* job, uint32_t index);
// Per-goal save word (+0x20074 + slot*4): status in bits 28-31, bit 26 dirty.
uint32_t JobGoalSaveWord(const Job* job, int slot);
void JobSetGoalSaveWord(Job* job, int slot, uint32_t value);
// Next free goal save slot of a job (counter in the top 12 bits of Job +0x04;
// each call takes one).
int JobTakeGoalSaveSlot(Job* job);
// Byte +0xAC of the job's owner object (+0x08). NOTE: meaning unknown.
uint8_t JobOwnerFlag(const Job* job);

// 0x002E2104: the goal belongs to the job that is running right now.
bool IsGoalInLiveJob(const Goal* goal);
// 0x002E5DD8: the manager's reaction to a failed goal.
void JobManagerGoalFailed(Goal* goal, uint8_t ownerFlag);
// 0x002E2FD4: called before a failed goal's sequence starts.
void JobManagerBeforeFailSequence();
// Job manager +0x48: the goal being tracked, and 0x002E2214, the goal after it.
Goal* JobManagerTrackedGoal();
Goal* JobManagerGoalAfterTracked();
// Bindings of the current job (+0x28, array at +0x1AC).
const GoalScriptBinding* JobManagerJobBindings(size_t* count);
// 0x002E4A0C(manager, 1): a sequential group moved to its next child.
void JobManagerGroupAdvanced();
// Game world +0x128214 (16 bits). NOTE: non-zero while the HUD scripts
// must not be touched (loading / cut scene).
uint16_t GameHudLockCount(SlyGame* game);
// Player character position (game world +0x1078, +0x90).
bool PlayerPosition(float out[3]);
// World position of an entity (+0x90).
void EntityPosition(const void* entity, float out[3]);
// Job manager +0x50: the goal that owns the on-screen countdown.
void JobManagerSetTimerGoal(Goal* goal);

// ---------------------------------------------------------------------------
// Sequences and scripts
// ---------------------------------------------------------------------------
// 0x002E9B08 / 0x002E9B78 / 0x002E9BE4: stream requests of a sequence's
// data. Release returns true when the stream set changed.
void SequenceStreamRequest(uint32_t name);
bool SequenceStreamRelease(uint32_t name);
void SequenceStreamLoadNow(uint32_t name);
// 0x0046C750: the game world has queued or started sequence `name`.
bool GameIsSequenceQueued(SlyGame* game, uint32_t name);
// 0x0046C3B8: queues sequence `name` of library `library` with variable
// bindings. NOTE: `flag` comes from the "last goal of the job" test.
void GameQueueSequence(SlyGame* game, uint32_t name, uint32_t library,
                       const GoalScriptBinding* bindings, size_t count, bool flag);
// Game world +0x2D bit 1: the set of streamed sequences changed.
void GameMarkStreamsChanged(SlyGame* game);
// 0x0049FCC0: binds script variables to a new anim script instance.
void BindScriptVariables(ape::ApeAnimScriptInstance* instance,
                         const GoalScriptBinding* bindings, size_t count);

// ---------------------------------------------------------------------------
// Hint targets (GoalHint)
// ---------------------------------------------------------------------------
void* HintFindEntity(uint32_t name);                 // 0x000D6A28
bool HintEntityTargetable(const void* entity);       // field_C0 bit 22
bool HintEntityNeedsRefresh(const void* entity);     // field_C0 bit 31
void HintEntityRefresh(void* entity);                // 0x002202BC
bool HintEntityInteractable(const void* entity);     // field_C8 bit 1
void* HintEntityInteraction(void* entity);           // 0x000D951C
void* HintFindLaserGroup(uint32_t name);             // laser group pool 0x00F657FC (+0x0C)
void* HintFindLaser(uint32_t name);                  // laser pool (+0x1C, by group)
void* HintFindLaserField(uint32_t name);             // laser field pool (+0x2C, 0x0039FEBC)
void HintEntitySetEnabled(void* entity, bool enable);       // entity virtual slot 76
void HintInteractableSetEnabled(void* entity, bool enable); // 0x001EB814
int HintInteractionState(const void* interaction);          // +0x44
void HintInteractionSetState(void* interaction, int state); // 0x00266E6C
void HintLaserGroupEnable(void* group, bool enable);        // 0x003992AC
void HintLaserEnable(void* laser, bool enable);             // 0x0039D32C
void HintLaserFieldEnable(void* field, bool enable);        // 0x00399758

// ---------------------------------------------------------------------------
// Players
// ---------------------------------------------------------------------------
// Game world +0x1078 + player*0x20C80: the player's character entity.
void* PlayerCharacter(int player);
// Signed byte +0xAC of a world object: the player it belongs to.
int ObjectPlayerIndex(const void* object);
// field_C0 bit 3 of an entity: it is a character.
bool EntityIsCharacter(const void* entity);
// Checkpoint group of an entity (+0xD8 -> +0x34), 0 if none; and the
// group's own field_C0.
uint32_t EntityGroupName(const void* entity);
uint32_t EntityGroupField_C0(const void* entity);
// Class bit 0x20 (+0xB8): NOTE: the Sly character class.
bool EntityIsSlyClass(const void* entity);
// Byte +0x290+ability of a character: ability state bits (7 owned, 6 locked).
uint8_t CharacterAbilityBits(const void* character, int ability);

// Character virtual slot 7 (+0x1C): sets the state of animation layer `layer`.
void CharacterSetState(void* character, int layer, int state);
// Slot 16 (+0x40) of the character's input map (+0x17C): button bits bound
// to `action` (0 if unbound or no map).
uint32_t CharacterActionButtons(void* character, uint32_t action);
// Pad of a player (game world +0x1071 byte -> pad table 0x010B1A90, stride
// 0x2EC0) and its buttons that are both pressed (+0x08) and held (+0x0C).
const void* PlayerPad(int player);
uint32_t PadButtonsDown(const void* pad);
// 0x0045B160: stick axis value (0-3) of a pad, -1..1.
float PadStickAxis(const void* pad, int axis);
// Binocucom power level (global 0x010592EC).
float BinocucomPowerLevel();
// Builds a waypoint marker for an entity and appends it (0x0075EBFC).
// NOTE: the marker contents are built on the stack and not decoded.
void GoalAddEntityMarker(struct GoalMarkerList* list, void* entity);
// 0x000D6A28 for goal targets.
void* GoalFindEntity(uint32_t name);
// 0x000DB290: descendant of an entity by name (null if none).
void* EntityFindDescendant(void* entity, uint32_t name);

// Entity field_AC bit 18: alive / registered.
bool GoalEntityAlive(const void* entity);
// Entity health (+0x15C) and maximum health (+0x158).
float GoalEntityHealth(const void* entity);
float GoalEntityMaxHealth(const void* entity);
// field_C0 bit 1: an actor (has health).
bool GoalEntityIsActor(const void* entity);
// Targeting component of the job owner (Job +0x08, 0x000D949C): drops
// `entityName` from its targets (0x0029F510).
void JobOwnerUntarget(const Job* job, uint32_t entityName);
// Targeting component of the job owner: adds a highlighted target for an
// entity (0x0029F760, then the record is filled: type 1, radius 62.99)
// or removes it (0x0029F7A0).
void JobOwnerAddTarget(const Job* job, void* entity);
void JobOwnerRemoveTarget(const Job* job, void* entity);
// 0x00470660: stat counter `stat` of `category` += amount for an object.
void GoalReportStat(void* object, uint32_t category, uint32_t stat, int amount);
// Event listener groups of a goal (0x0048FA54 add, 0x0048F298 remove).
void GoalListenTo(Goal* goal, uint32_t group);
void GoalStopListening(Goal* goal);
// Spawns the Operation Mousetrap helper entity (definition 0x30667928, then
// placed through its slot 21 from a level locator); returns it, or null.
void* SpawnMousetrapHelper();
// field_150 of an entity: the object it is attached to.
void* GoalEntityParent(const void* entity);
// Marker filters: field_C0 bit 22 with +0x19C >= 0 (the entity hides its
// marker while targeted), and class bit 35 (+0xB8): markers allowed.
bool GoalEntityMarkerSuppressed(const void* entity);
bool GoalEntityAllowsMarker(const void* entity);
// Class bit 34 (+0xB8): the entity can be photographed.
bool GoalEntityPhotographable(const void* entity);
// The "too far" warning on the follow HUD (0x00368A48 on 0x00F64D3C, then 0x00384750).
void HudSetFollowWarning(bool on);
// Entity reference count (+0xB2) and virtual slot 10 (release).
void GoalEntityAddRef(void* entity);
void GoalEntityRelease(void* entity);
// Name hash of an entity (+0xA8).
uint32_t GoalEntityName(const void* entity);
// The player's binocucom gadget (0x000D94CC) is in mode 3 (field_F4 bits 20-27).
bool PlayerBinocucomActive();
// The gameplay camera (game world +0x1281E0): position (+0x90) and forward
// axis (+0x80). False if there is none.
bool GameCameraFrame(float position[3], float forward[3]);
// 0x00522750 on the main render target (+0x128168): screen position (0..1).
bool GameWorldToScreen(const float position[3], float screen[2]);
// Interaction component: state (+0x44), field_7C bit 22, owner (+0x08) and
// 0x002B719C.
bool InteractionFlag22(const void* interaction);
void* InteractionOwner(const void* interaction);
void InteractionTouch(void* interaction);

// ---------------------------------------------------------------------------
// HUD (0x003568E0 HudFind and friends, sly/game HUD module, not ported)
// ---------------------------------------------------------------------------
void* HudFind(uint32_t name);                         // 0x003568E0
bool HudIsShown(const void* hudObject);               // +0x18C bit 30
void HudHide(void* hudObject, bool a, bool b, bool c);  // 0x0036325C
void HudSetText(void* hudObject, const char* text, bool flag); // 0x00367530
void HudShowText(void* hudObject, const char* text, bool flag); // 0x00363A3C
// 0x00365E30 on 0x00F64CFC: the countdown is paused by the pause screen.
bool HudCountdownPaused();
// HUD object 0x0001447B is busy (+0x18 -> +0xA4 bit 15).
bool HudTimerSlotBusy();
// Name of the countdown HUD object (0x00F64CD0).
uint32_t HudCountdownName();

// ---------------------------------------------------------------------------
// Game world minigame slot (+0x1281F4)
// ---------------------------------------------------------------------------
gameplay::Minigame* GameActiveMinigame(SlyGame* game);
void GameInstallMinigame(SlyGame* game, gameplay::Minigame* minigame);  // 0x0047C530
void GameEndActiveMinigame(SlyGame* game);                              // 0x0047C490
void MinigameStart(gameplay::Minigame* minigame);                       // virtual slot 2
void MinigameCollectMarkers(gameplay::Minigame* minigame, struct GoalMarkerList* list); // slot 13
// Minigame classes built by the minigame goals (their constructors take the
// game, the goal's configuration block, the goal script library and the goal).
enum class MinigameKind {
    Bartender,           // 0x003A3B30 (also gets the goal's bartender parameters)
    BentleyVsPenelope,   // 0x003B04F8 BvpBattle
    PenguinBalance,      // 0x003F834C
    PenguinDodge,        // 0x004015D0
    PenguinPunch,        // 0x0040773C
    PenguinSlingshot,    // 0x0040F244
    PenguinSumo,         // 0x00416FA4
    PenguinWhack,        // 0x0041BB54
    Fencing,             // 0x003D8B34
    Fishing,             // 0x003B3148
    Qte,                 // 0x003D3704
    PenguinMontage,      // 0x0041F1FC
};
// 0x000B27B4: awards the trophy registered under `name`.
void GoalAwardTrophy(uint32_t name);
gameplay::Minigame* CreateMinigame(MinigameKind kind, SlyGame* game, const void* config,
                                   Goal* owner);

} // namespace goals
} // namespace sly
