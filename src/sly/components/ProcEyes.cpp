// openTIT - <ProcEyes> component (vtable 0x00B1C5D0).
#include "sly/components/ProcEyes.h"

#include "sly/anim/AnimController.h"
#include "sly/components/ComponentDescs.h"
#include "sly/components/ComponentHooks.h"

#include "agl/AglResources.h"
#include "ape/core/ApeString.h"
#include "ape/world/ApeBase.h"
#include "ape/world/ApeObj.h"
#include "ape/world/ApeSkeleton.h"

#include <cstring>

namespace sly {
namespace components {

void ProcEyesComponent::LoadMaterial(Eye& eye, uint32_t materialName)
{
    ape::world::ApeObj* object = owner ? owner->object : nullptr;
    if (object && materialName && object->GetType() == ape::world::kObjType_GeomInstance)
        eye.material = GeomFindMaterial(object, materialName);
    if (!eye.material)
        return;
    agl::AglMaterial* m = eye.material;
    for (int i = 0; i < 4; ++i)
        *m->GetConstant(28 + i) = eye.pivots[i];
    // Keep a copy of the material's constants.
    for (int i = 0; i < 32; ++i)
        eye.constants[i] = m->constants ? m->constants[i] : 0.0f;
}

// @func 0x0025EBF8 ProcEyesComponent::ProcEyesComponent
ProcEyesComponent::ProcEyesComponent(const ProcEyesComponentDesc* desc) : EntityComponent(desc)
{
    flag80 = true;
    flag40 = desc->flag11;
    if (!owner)
        return;
    ape::world::ApeObj* object = owner->object;
    ape::world::ApeSkeleton* skeleton = object ? object->GetSkeleton() : nullptr;
    if (skeleton && desc->eyes.Count()) {
        eyeCount = desc->eyes.Count();
        eyes = new Eye[eyeCount];
        for (uint32_t i = 0; i < eyeCount; ++i) {
            const ProcEye& from = desc->eyes[i];
            Eye& eye = eyes[i];
            eye.name = from.name;
            eye.materialNames[0] = from.material;
            eye.materialNames[1] = from.hash08;
            eye.pivots[0] = from.upperRotPivot[0];
            eye.pivots[1] = from.upperRotPivot[1];
            eye.pivots[2] = from.lowerRotPivot[0];
            eye.pivots[3] = from.lowerRotPivot[1];
            const uint32_t boneNames[4] = {from.bone, from.eyelidBone, from.hash14, from.hash18};
            for (int b = 0; b < 4; ++b) {
                if (boneNames[b])
                    eye.bones[b] = skeleton->FindBone(boneNames[b]);
            }
            if (from.enableBlink) {
                eye.blinking = 1.0f;
                eye.blink[0] = from.blinkMin;
                eye.blink[1] = from.blinkMax;
                eye.blink[2] = from.blink28;
            }
            LoadMaterial(eye, eye.materialNames[0]);
        }
    }
    if (auto* controller = static_cast<anim::AnimController*>(owner->animController))
        controller->AddListener(reinterpret_cast<void*>(&AnimEventCallback), this);
}

// @func 0x002869A0 ProcEyesComponent::~ProcEyesComponent
// @func 0x00286B0C ProcEyesComponent::~ProcEyesComponent (deleting)
ProcEyesComponent::~ProcEyesComponent()
{
    delete[] eyes;
    eyes = nullptr;
    eyeCount = 0;
    if (owner) {
        if (auto* controller = static_cast<anim::AnimController*>(owner->animController))
            controller->RemoveListener(reinterpret_cast<void*>(&AnimEventCallback), this);
    }
}

// @func 0x00286B54 ProcEyesComponent::Update
void ProcEyesComponent::Update(float)
{
    QueueWorldCallback(2, reinterpret_cast<void*>(&WorldCallback), this, owner);
}

// @func 0x002878AC ProcEyesComponent::Slot9
void ProcEyesComponent::Slot9()
{
    for (uint32_t i = 0; i < eyeCount; ++i) {
        if (!eyes[i].material)
            continue;
        for (uint32_t c = 0; c < 32; ++c)
            *eyes[i].material->GetConstant(c) = eyes[i].constants[c];
    }
}

// @func 0x00287260 ProcEyesComponent::SetConstant2
void ProcEyesComponent::SetConstant2(uint32_t value)
{
    for (uint32_t i = 0; i < eyeCount; ++i) {
        if (eyes[i].material)
            *eyes[i].material->GetConstant(2) = float(value);
    }
}

// @func 0x00287318 ProcEyesComponent::SwapMaterials
void ProcEyesComponent::SwapMaterials(uint8_t set)
{
    for (uint32_t i = 0; i < eyeCount; ++i)
        LoadMaterial(eyes[i], eyes[i].materialNames[set]);
}

// @func 0x002874E8 ProcEyesComponent::OnAnimEvent
void ProcEyesComponent::OnAnimEvent(float value, const char* event)
{
    const char* underscore = std::strchr(event, '_');
    if (!underscore)
        return;
    char eyeName[0x100];
    const size_t length = size_t(underscore - event) < sizeof eyeName - 1 ? size_t(underscore - event) : sizeof eyeName - 1;
    std::memcpy(eyeName, event, length);
    eyeName[length] = '\0';
    const uint32_t eyeHash = ape::HashNameNoCase(eyeName);
    for (uint32_t i = 0; i < eyeCount; ++i) {
        if (eyes[i].name != eyeHash)
            continue;
        Eye& eye = eyes[i];
        constexpr float kDegToRad = 0.017453292f;
        // NOTE: a debug switch (0x01066B98) also printed the owner's name.
        switch (ape::HashNameNoCase(underscore)) {
        case kChannel_A:
            if (!flag80 || eye.blinking == 0.0f)
                eye.value40 = value;
            break;
        case kChannel_B:
            eye.angle48 = value * kDegToRad;
            break;
        case kChannel_D:
            if (!flag80 || eye.blinking == 0.0f)
                eye.value3C = value;
            break;
        case kChannel_C:
            eye.angle44 = value * kDegToRad;
            break;
        default:
            break;
        }
        return;
    }
}

// TODO(0x00286BB8 ProcEyesComponent::UpdateEyes): bone driven eye and lid
// constants with blinking (vector code).
void ProcEyesComponent::UpdateEyes() {}

// @func 0x00287874 ProcEyesComponent::AnimEventCallback
void ProcEyesComponent::AnimEventCallback(float value, void*, ProcEyesComponent* eyes, const char* event)
{
    if (eyes)
        eyes->OnAnimEvent(value, event);
}

// @func 0x00287988 ProcEyesComponent::WorldCallback
void ProcEyesComponent::WorldCallback(void*, ProcEyesComponent* eyes)
{
    eyes->UpdateEyes();
}

} // namespace components
} // namespace sly
