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
class HkStreamReader;

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
    enum BlendHint : int8_t { kBlend_Normal = 0, kBlend_Additive = 1 };
    uint8_t referencedObject[8] = {};
    const char* skeletonName = nullptr; // +0x08 m_originalSkeletonName
    HkaAnimation* animation = nullptr;  // +0x0C m_animation
    uint8_t trackMaps[0x18] = {};       // +0x10 transform/float track index arrays
    int8_t blendHint = kBlend_Normal;   // +0x28 m_blendHint
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
    float referencePoseWeightThreshold = 0.0f;  // +0x20 m_referencePoseWeightThreshold
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
// new hkaAnimatedSkeleton (0x00A45750) for `skeleton`.
HkaAnimatedSkeleton* HkNewAnimatedSkeleton(const HkaSkeleton* skeleton);
// new hkaPose for `skeleton` (arrays sized to its bones and float slots),
// set to the reference pose (0x00A56E68).
HkaPose* HkNewReferencePose(const HkaSkeleton* skeleton);
// hkReferencedObject::removeReference (0x007D4EA8) / addReference (0x007D48C4).
void HkRemoveReference(void* object);
void HkAddReference(void* object);

// hkaAnimationContainer: the skeletons and animations of a loaded file.
struct HkaAnimationContainer {
    uint8_t field_00[8];
    HkaSkeleton** skeletons = nullptr;   // +0x08
    int32_t numSkeletons = 0;            // +0x0C
    uint32_t field_10 = 0;
    void** animations = nullptr;         // +0x14 hkaAnimation*
    int32_t numAnimations = 0;           // +0x18
};
// Loads a Havok resource (packfile or tagfile) from a stream
// (hkSerializeUtil::load 0x00802E08 with its error details 0x007CE648 /
// 0x007CEA58); null on failure. The caller owns a reference.
void* HkLoadResource(HkStreamReader* stream);
// Contents of a resource (its slot 7) as a root level container (class
// from 0x007FECB8) or directly as an animation container (class from
// 0x00A39858); null when it holds no such object.
void* HkResourceRootContainer(void* resource);
HkaAnimationContainer* HkResourceAnimationContainer(void* resource);
// hkRootLevelContainer::findObjectByType (0x00802B08) for the animation
// container class (0x011F7EA4).
HkaAnimationContainer* HkRootFindAnimationContainer(void* root);
// Destroys a pose (its four hkArrays) and returns the 0x38 bytes to the
// Havok heap of the thread's memory router.
void HkPoseDelete(HkaPose* pose);

} // namespace anim
} // namespace sly
