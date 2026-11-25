// openTIT - Knowledge cause/result callbacks (see Knowledge.h for the table
// layout). Addresses are given in KnowledgeCauses.cpp / KnowledgeResults.cpp.
#pragma once

#include "sly/knowledge/Knowledge.h"

namespace sly {
namespace knowledge {

// Comparison operators of CIsVar / CIsTimer (HashNameNoCase of the names).
enum KOperator : uint32_t {
    kOp_Equal       = 0x03F26F14u,
    kOp_Larger      = 0x856AA097u,
    kOp_LargerEqual = 0x59D794DDu,
    kOp_Less        = 0x00239999u,
    kOp_LessEqual   = 0xBC3BED1Bu,
    kOp_NotEqual    = 0x8CF17841u,
};

// Causes
bool CAlwaysEvaluate(KRef* ref, KnowledgeData* list);
bool CNeverEvaluate(KRef* ref, KnowledgeData* list);
bool COnEnterStateEvaluate(KRef* ref, KnowledgeData* list);
bool COnExitStateEvaluate(KRef* ref, KnowledgeData* list);
bool CEveryXSecondEvaluate(KRef* ref, KnowledgeData* list);
void CIsVarLink(KRef* ref, KnowledgeData* list);
bool CIsVarEvaluate(KRef* ref, KnowledgeData* list);
void CIsTimerLink(KRef* ref, KnowledgeData* list);
bool CIsTimerEvaluate(KRef* ref, KnowledgeData* list);

// Results
bool RDoNothingExecute(KRef*, KnowledgeData*);
bool RNachiDebugExecute(KRef*, KnowledgeData*);
bool RBreakPointExecute(KRef*, KnowledgeData*);
void RChangeStateLink(KRef*, KnowledgeData*);
bool RChangeStateExecute(KRef*, KnowledgeData*);
void RSetVarDataIntLink(KRef*, KnowledgeData*);
bool RSetVarDataIntExecute(KRef*, KnowledgeData*);
void RSetVarDataFloatLink(KRef*, KnowledgeData*);
bool RSetVarDataFloatExecute(KRef*, KnowledgeData*);
void RSetVarDataBoolLink(KRef*, KnowledgeData*);
bool RSetVarDataBoolExecute(KRef*, KnowledgeData*);
void RSetVarDataHashLink(KRef*, KnowledgeData*);
bool RSetVarDataHashExecute(KRef*, KnowledgeData*);
void RIncrementVarDataLink(KRef*, KnowledgeData*);
bool RIncrementVarDataExecute(KRef*, KnowledgeData*);
bool RDisplayDebugTextExecute(KRef*, KnowledgeData*);
void RPlaySndSFXRelease(KRef*, KnowledgeData*);
void RPlaySndSFXLink(KRef*, KnowledgeData*);
void RPlaySndSFXClone(KRef*, KnowledgeData*);
bool RPlaySndSFXExecute(KRef*, KnowledgeData*);
bool RStopSndSFXExecute(KRef*, KnowledgeData*);
void RPlayDirScriptLink(KRef*, KnowledgeData*);
bool RPlayDirScriptExecute(KRef*, KnowledgeData*);
bool RTurnOnHighlightExecute(KRef*, KnowledgeData*);
void RResetTimerClone(KRef*, KnowledgeData*);
bool RResetTimerExecute(KRef*, KnowledgeData*);
bool RSendEventExecute(KRef*, KnowledgeData*);
bool RBroadcastEventExecute(KRef*, KnowledgeData*);
bool RChangeDirectorStateExecute(KRef*, KnowledgeData*);
void RStartMeleeAttackLink(KRef*, KnowledgeData*);
void RStartMeleeAttackClone(KRef*, KnowledgeData*);
bool RStartMeleeAttackExecute(KRef*, KnowledgeData*);
void REndMeleeAttackLink(KRef*, KnowledgeData*);
bool REndMeleeAttackExecute(KRef*, KnowledgeData*);
void RFireProjectileLink(KRef*, KnowledgeData*);
void RFireProjectileClone(KRef*, KnowledgeData*);
bool RFireProjectileExecute(KRef*, KnowledgeData*);
void RFireWeaponLink(KRef*, KnowledgeData*);
bool RFireWeaponExecute(KRef*, KnowledgeData*);
bool RSetAnimSpeedExecute(KRef*, KnowledgeData*);
bool RCreateSoundAlertExecute(KRef*, KnowledgeData*);
bool RPlayAnimDirectExecute(KRef*, KnowledgeData*);
bool RSetAttackInvincibilityExecute(KRef*, KnowledgeData*);
bool RSetMaxRotationRateExecute(KRef*, KnowledgeData*);
bool RSetAdditiveAlphaExecute(KRef*, KnowledgeData*);
bool RMotionExTuningExecute(KRef*, KnowledgeData*);
bool RSetGravityExecute(KRef*, KnowledgeData*);
bool RCameraShakeExecute(KRef*, KnowledgeData*);
bool RControllerRumbleExecute(KRef*, KnowledgeData*);
bool RPlayFootstepExecute(KRef*, KnowledgeData*);
bool RFootIKExecute(KRef*, KnowledgeData*);
bool REnableComponentExecute(KRef*, KnowledgeData*);
bool RSetProjectileParamExecute(KRef*, KnowledgeData*);
bool RStopDirScriptExecute(KRef*, KnowledgeData*);
bool RSetPositionExecute(KRef*, KnowledgeData*);

} // namespace knowledge
} // namespace sly
