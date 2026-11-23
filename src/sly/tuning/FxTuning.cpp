// openTIT - Effects (object shake, trails) tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/FxTuning.h"

#include "ape/memory/ApeTArray.h"

namespace sly {

// @func 0x002F5E00 StaticInit_FxTuning
// @func 0x002F5C08 StaticDestroy_FxTuning
// Compiler-generated initialiser for the globals of this file. (It also
// constructs a file-local zero vector pulled in from a shared header.)

namespace {
class FxObjShake;
// NOTE: 0x60-byte trail records; the trail system is not reconstructed yet.
struct FxTrailRecord {
    uint8_t data[0x60];
};
}

ape::ApeTArray<FxObjShake*> g_fxObjShakes;        // 0x01068818
ape::ApeTArray<FxTrailRecord> g_fxTrails;         // 0x0106882C
void* g_fxActiveTrailOwner = nullptr;             // 0x01068840

ape::TuningBool g_fXDisableAll("FX.DisableAll", false);
ape::TuningFloat g_fXShakeSpeedInterp("FX.ShakeSpeedInterp", 0.5f);
ape::TuningFloat g_fXObjShakeDefaultCullDist("FX.ObjShake.DefaultCullDist", 7e+01f);
ape::TuningBool g_fXObjShakeDisableIfNotRendered("FX.ObjShake.DisableIfNotRendered", true);
ape::TuningFloat g_fXTrailFrameRate("FX.Trail.FrameRate", 1.8e+02f);
ape::TuningInt g_fXTrailNoTexture("FX.Trail.NoTexture", 0);
ape::TuningBool g_fXTrailOutlineEnable("FX.Trail.OutlineEnable", false);
ape::TuningColor g_fXTrailOutlineColor("FX.Trail.OutlineColor", { 0, 0, 0, 255 });
ape::TuningFloat g_fXTrailPointAccelMax("FX.Trail.PointAccelMax", 1e+01f);
ape::TuningFloat g_fXTrailHintAngleMin("FX.Trail.HintAngleMin", 45.0f);

} // namespace sly
