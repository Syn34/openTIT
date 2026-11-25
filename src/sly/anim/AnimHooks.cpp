// openTIT - Havok Animation calls made by the animation layer. Havok is
// middleware and not ported: each stub stands for the call named in
// AnimHavok.h and does what the console would do with no Havok objects.
#include "sly/anim/AnimHavok.h"

namespace sly {
namespace anim {

// TODO(Havok hkaAnimatedSkeleton::stepDeltaTime)
void HkSkeletonStepDeltaTime(HkaAnimatedSkeleton*, float) {}
// TODO(0x00A45478 hkaAnimatedSkeleton::addAnimationControl)
void HkSkeletonAddControl(HkaAnimatedSkeleton*, HkaAnimationControl*) {}
// TODO(0x00A459B8 hkaAnimatedSkeleton::removeAnimationControl)
void HkSkeletonRemoveControl(HkaAnimatedSkeleton*, HkaAnimationControl*) {}
// TODO(Havok hkaAnimatedSkeleton::sampleAndCombineAnimations)
void HkSkeletonSampleIntoPose(HkaAnimatedSkeleton*, HkaPose*) {}
// TODO(Havok hkaAnimation::getAnnotations)
uint32_t HkAnimationGetAnnotations(HkaAnimation*, float, float, HkaAnnotation*, uint32_t) { return 0; }
// TODO(0x00A4C600 hkaDefaultAnimationControl::hkaDefaultAnimationControl)
HkaAnimationControl* HkNewDefaultControl(HkaAnimationBinding* binding, int32_t)
{
    auto* control = new HkaAnimationControl();
    control->binding = binding;
    return control;
}
// TODO(0x00A4CC64 hkaDefaultAnimationControl::addDefaultControlListener)
void HkControlAddListener(HkaAnimationControl*, void*) {}
// TODO(0x007CCD60 hkArray growth of m_defaultListeners)
void HkControlPushListener(HkaAnimationControl*, void*) {}
// TODO(0x007D4EA8 hkReferencedObject::removeReference)
void HkRemoveReference(void*) {}
// TODO(Havok hkaPose destructor and heap free)
void HkPoseDelete(HkaPose*) {}

} // namespace anim
} // namespace sly
