// openTIT - The parts of Havok Animation (2011.3) that the game's animation
// layer reads or writes directly, and the Havok calls it makes.
//
// The game keeps one hkaAnimatedSkeleton per animation layer and drives it
// through hkaDefaultAnimationControl objects (a main control and optionally
// a secondary one blended on top). Havok itself is middleware and is not
// ported; these stand-ins only name the members the game touches, at their
// console offsets, and AnimHooks.cpp holds TODO stubs for the calls.
#pragma once

#include <cstdint>

namespace sly {
namespace anim {

// hkaAnimation (the clip data).
struct HkaAnimation {
    enum Type : int32_t {
        // NOTE: the game treats type 4 controls specially when it prunes
        // eased-out controls (read as the mirrored/additive kind).
        kType_Special = 4,
    };
    uint8_t referencedObject[8] = {};   // +0x00 vtable, memory size, reference count
    int32_t type = 0;                   // +0x08 m_type
    float duration = 0.0f;              // +0x0C m_duration
    uint8_t field_10[0x0C] = {};        // +0x10 track counts, extracted motion
    void* annotationTracks = nullptr;   // +0x1C m_annotationTracks (data)
    int32_t annotationTrackCount = 0;   // +0x20 m_annotationTracks (size)
};

// hkaAnimationBinding.
struct HkaAnimationBinding {
    uint8_t referencedObject[8] = {};
    const char* skeletonName = nullptr; // +0x08 m_originalSkeletonName
    HkaAnimation* animation = nullptr;  // +0x0C m_animation
};

// One annotation returned by hkaAnimation::getAnnotations (12 bytes).
struct HkaAnnotation {
    uint32_t field_00 = 0;              // +0x00 bone / track index
    float time = 0.0f;                  // +0x04
    const char* text = nullptr;         // +0x08 (the game passes the pointer with bit 0 cleared)
};

// hkaDefaultAnimationControl.
struct HkaAnimationControl {
    enum EaseStatus : int32_t {
        kEase_EasingIn  = 0,
        kEase_EasedIn   = 1,
        kEase_EasingOut = 2,
        kEase_EasedOut  = 3,
    };
    uint8_t referencedObject[8] = {};
    float localTime = 0.0f;             // +0x08 m_localTime
    float weight = 0.0f;                // +0x0C m_weight
    uint8_t* transformTrackWeights = nullptr;   // +0x10 m_transformTrackWeights (data, one byte per bone)
    uint8_t field_14[0x14] = {};        // +0x14 sizes, float track weights
    HkaAnimationBinding* binding = nullptr;     // +0x28 m_binding
    uint8_t field_2C[0x10] = {};        // +0x2C motion track weight, listeners
    float masterWeight = 1.0f;          // +0x3C m_masterWeight
    float playbackSpeed = 1.0f;         // +0x40 m_playbackSpeed
    uint32_t overflowCount = 0;         // +0x44 m_overflowCount (completed forward loops)
    uint32_t underflowCount = 0;        // +0x48 m_underflowCount
    uint8_t field_4C[0x24] = {};        // +0x4C max cycles, ease curves
    float easeInvDuration = 0.0f;       // +0x70 m_easeInvDuration
    float easeT = 0.0f;                 // +0x74 m_easeT
    int32_t easeStatus = kEase_EasedIn; // +0x78 m_easeStatus (EaseStatus)
    uint8_t field_7C[8] = {};           // +0x7C crop amounts
    void** controlListeners = nullptr;  // +0x84 m_defaultListeners (data)
    int32_t controlListenerCount = 0;   // +0x88 (size)
    uint32_t controlListenerCapacity = 0; // +0x8C (capacity | flags)

    // Duration of the bound clip (0 without a binding).
    float Duration() const { return binding ? binding->animation->duration : 0.0f; }
};

// hkaSkeleton: only the bone table is used (bone names for weight lists).
struct HkaBone {
    const char* name = nullptr;         // +0x00 (the game masks bit 0 of the pointer)
    uint32_t lockTranslation = 0;       // +0x04
};
struct HkaSkeleton {
    uint8_t field_00[0x18] = {};
    HkaBone* bones = nullptr;           // +0x18 m_bones (data)
    int32_t boneCount = 0;              // +0x1C m_bones (size)
};

// hkaAnimatedSkeleton: the controls blended into one pose.
struct HkaAnimatedSkeleton {
    uint8_t field_00[0x10] = {};        // +0x00 vtable, reference count, ...
    HkaAnimationControl** controls = nullptr;  // +0x10 m_animationControls (data)
    int32_t controlCount = 0;           // +0x14 m_animationControls (size)
    uint32_t field_18 = 0;
    const HkaSkeleton* skeleton = nullptr;     // +0x1C m_skeleton
    uint32_t field_20 = 0;
    // +0x24: written with the control count when a special (type 4) control
    // is pruned and cleared otherwise. NOTE: read as the reference pose
    // weight threshold bookkeeping; meaning not confirmed.
    int32_t field_24 = 0;
};

// hkaPose (0x38 bytes, allocated from the Havok heap).
struct HkaPose;

// -- Havok calls (TODO stubs in AnimHooks.cpp) ----------------------------------

// hkaAnimatedSkeleton virtual slot 4: advances every control by `dt`.
void HkSkeletonStepDeltaTime(HkaAnimatedSkeleton* skeleton, float dt);
// hkaAnimatedSkeleton::addAnimationControl (0x00A45478) / removeAnimationControl (0x00A459B8).
void HkSkeletonAddControl(HkaAnimatedSkeleton* skeleton, HkaAnimationControl* control);
void HkSkeletonRemoveControl(HkaAnimatedSkeleton* skeleton, HkaAnimationControl* control);
// hkaAnimatedSkeleton virtual slot 5 into the local space pose of `pose`
// (hkaPose::accessUnsyncedPoseLocalSpace 0x00A56CC8 and the float slots +0x2C).
void HkSkeletonSampleIntoPose(HkaAnimatedSkeleton* skeleton, HkaPose* pose);
// hkaAnimation virtual slot 16: annotations crossed from `time` over `dt`;
// returns how many were written (at most `max`).
uint32_t HkAnimationGetAnnotations(HkaAnimation* animation, float time, float dt, HkaAnnotation* out,
                                   uint32_t max);
// new hkaDefaultAnimationControl (0x00A4C600) on `binding`, ease curves on,
// `maxCycles` loops (-1 for endless).
HkaAnimationControl* HkNewDefaultControl(HkaAnimationBinding* binding, int32_t maxCycles);
// hkaDefaultAnimationControl::addDefaultControlListener (0x00A4CC64).
void HkControlAddListener(HkaAnimationControl* control, void* listener);
// Appends to m_defaultListeners (hkArray growth through 0x007CCD60).
void HkControlPushListener(HkaAnimationControl* control, void* listener);
// hkReferencedObject::removeReference (0x007D4EA8).
void HkRemoveReference(void* object);
// Destroys a pose (its four hkArrays) and returns the 0x38 bytes to the
// Havok heap of the thread's memory router.
void HkPoseDelete(HkaPose* pose);

} // namespace anim
} // namespace sly
