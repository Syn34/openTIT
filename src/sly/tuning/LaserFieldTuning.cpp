// openTIT - Laser and laser field entity tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/tuning/LaserFieldTuning.h"

namespace sly {

// @func 0x003A2948 StaticInit_LaserFieldTuning
// @func 0x003A23D4 StaticDestroy_LaserFieldTuning
// Compiler-generated initialiser for the globals of this file. (It also
// constructs a file-local zero vector pulled in from a shared header.)

ape::TuningBool g_gadgetLaserForceElectricDamage("Entity.Gadget.Laser.ForceElectricDamage", true);
ape::TuningBool g_debugDisplayLaser("Debug.Display.Laser", false);
ape::TuningBool g_laserAlwaysUpdate("Entity.Laser.AlwaysUpdate", false);
ape::TuningString g_laserDefaultGeom("Entity.Laser.DefaultGeom", "Static_LaserA");
ape::TuningString g_laserDefaultDirScriptPt1("Entity.Laser.DefaultDirScriptPt1", "LaserAPoint");
ape::TuningString g_laserDefaultDirScriptPt2("Entity.Laser.DefaultDirScriptPt2", "LaserAPoint");
ape::TuningString g_laserFieldDefaultOutlineGeom("Entity.LaserField.DefaultOutlineGeom", "Static_LaserA");
ape::TuningString g_laserFieldDefaultJointEntityDef("Entity.LaserField.DefaultJointEntityDef", "FloorLaserA");
ape::TuningFloat g_laserFieldDefaultJointYOffset("Entity.LaserField.DefaultJointYOffset", -0.65f);
ape::TuningString g_laserFieldDefaultTrigFXDirScript("Entity.LaserField.DefaultTrigFXDirScript", "BallRollInLaserField");
ape::TuningFloat g_laserFlickerDelayTimeMax("Entity.Laser.FlickerDelayTimeMax", 0.67f);
ape::TuningFloat g_laserFlickerTimeMax("Entity.Laser.FlickerTimeMax", 2.0f);
ape::TuningFloat g_laserFlickerMinToggleTime("Entity.Laser.FlickerMinToggleTime", 0.2f);
ape::TuningFloat g_laserFlickerMaxToggleTime("Entity.Laser.FlickerMaxToggleTime", 0.5f);
ape::TuningVec2 g_laserPlayerCylinderScale("Entity.Laser.PlayerCylinderScale", 0.65f, 1.0f);
ape::TuningVec2 g_laserVehicleCylinderScale("Entity.Laser.VehicleCylinderScale", 0.35f, 0.4f);
ape::TuningString g_laserFieldDefaultMaterial1("Entity.LaserField.DefaultMaterial1", "Mat_LaserAField");
ape::TuningString g_laserFieldDefaultMaterial1Child1("Entity.LaserField.DefaultMaterial1Child1", "");
ape::TuningString g_laserFieldDefaultMaterial1Child2("Entity.LaserField.DefaultMaterial1Child2", "");
ape::TuningString g_laserFieldDefaultMaterial1Child3("Entity.LaserField.DefaultMaterial1Child3", "");
ape::TuningString g_laserFieldDefaultMaterial1Child4("Entity.LaserField.DefaultMaterial1Child4", "");
ape::TuningString g_laserFieldDefaultMaterial1Child5("Entity.LaserField.DefaultMaterial1Child5", "");
ape::TuningString g_laserFieldDefaultMaterial1Child6("Entity.LaserField.DefaultMaterial1Child6", "");
ape::TuningString g_laserFieldDefaultMaterial1Child7("Entity.LaserField.DefaultMaterial1Child7", "");
ape::TuningString g_laserFieldDefaultMaterial1Child8("Entity.LaserField.DefaultMaterial1Child8", "");
ape::TuningString g_laserFieldDefaultMaterial1Child9("Entity.LaserField.DefaultMaterial1Child9", "");
ape::TuningString g_laserFieldDefaultMaterial1Child10("Entity.LaserField.DefaultMaterial1Child10", "");
ape::TuningString g_laserFieldDefaultMaterial1Child11("Entity.LaserField.DefaultMaterial1Child11", "");
ape::TuningString g_laserFieldDefaultMaterial1Child12("Entity.LaserField.DefaultMaterial1Child12", "");
ape::TuningString g_laserFieldDefaultMaterial1Child13("Entity.LaserField.DefaultMaterial1Child13", "");
ape::TuningString g_laserFieldDefaultMaterial1Child14("Entity.LaserField.DefaultMaterial1Child14", "");
ape::TuningString g_laserFieldDefaultMaterial1Child15("Entity.LaserField.DefaultMaterial1Child15", "");
ape::TuningString g_laserFieldDefaultMaterial2("Entity.LaserField.DefaultMaterial2", "Mat_LaserAGrid");
ape::TuningString g_laserFieldDefaultMaterial2Child1("Entity.LaserField.DefaultMaterial2Child1", "Mat_LaserAGrid01");
ape::TuningString g_laserFieldDefaultMaterial2Child2("Entity.LaserField.DefaultMaterial2Child2", "Mat_LaserAGrid02");
ape::TuningString g_laserFieldDefaultMaterial2Child3("Entity.LaserField.DefaultMaterial2Child3", "Mat_LaserAGrid03");
ape::TuningString g_laserFieldDefaultMaterial2Child4("Entity.LaserField.DefaultMaterial2Child4", "Mat_LaserAGrid04");
ape::TuningString g_laserFieldDefaultMaterial2Child5("Entity.LaserField.DefaultMaterial2Child5", "Mat_LaserAGrid05");
ape::TuningString g_laserFieldDefaultMaterial2Child6("Entity.LaserField.DefaultMaterial2Child6", "Mat_LaserAGrid06");
ape::TuningString g_laserFieldDefaultMaterial2Child7("Entity.LaserField.DefaultMaterial2Child7", "Mat_LaserAGrid07");
ape::TuningString g_laserFieldDefaultMaterial2Child8("Entity.LaserField.DefaultMaterial2Child8", "Mat_LaserAGrid08");
ape::TuningString g_laserFieldDefaultMaterial2Child9("Entity.LaserField.DefaultMaterial2Child9", "Mat_LaserAGrid09");
ape::TuningString g_laserFieldDefaultMaterial2Child10("Entity.LaserField.DefaultMaterial2Child10", "Mat_LaserAGrid10");
ape::TuningString g_laserFieldDefaultMaterial2Child11("Entity.LaserField.DefaultMaterial2Child11", "Mat_LaserAGrid11");
ape::TuningString g_laserFieldDefaultMaterial2Child12("Entity.LaserField.DefaultMaterial2Child12", "Mat_LaserAGrid12");
ape::TuningString g_laserFieldDefaultMaterial2Child13("Entity.LaserField.DefaultMaterial2Child13", "Mat_LaserAGrid13");
ape::TuningString g_laserFieldDefaultMaterial2Child14("Entity.LaserField.DefaultMaterial2Child14", "Mat_LaserAGrid14");
ape::TuningString g_laserFieldDefaultMaterial2Child15("Entity.LaserField.DefaultMaterial2Child15", "Mat_LaserAGrid15");
ape::TuningString g_laserFieldDefaultMaterial3("Entity.LaserField.DefaultMaterial3", "Mat_LaserAPulse");
ape::TuningString g_laserFieldDefaultMaterial3Child1("Entity.LaserField.DefaultMaterial3Child1", "");
ape::TuningString g_laserFieldDefaultMaterial3Child2("Entity.LaserField.DefaultMaterial3Child2", "");
ape::TuningString g_laserFieldDefaultMaterial3Child3("Entity.LaserField.DefaultMaterial3Child3", "");
ape::TuningString g_laserFieldDefaultMaterial3Child4("Entity.LaserField.DefaultMaterial3Child4", "");
ape::TuningString g_laserFieldDefaultMaterial3Child5("Entity.LaserField.DefaultMaterial3Child5", "");
ape::TuningString g_laserFieldDefaultMaterial3Child6("Entity.LaserField.DefaultMaterial3Child6", "");
ape::TuningString g_laserFieldDefaultMaterial3Child7("Entity.LaserField.DefaultMaterial3Child7", "");
ape::TuningString g_laserFieldDefaultMaterial3Child8("Entity.LaserField.DefaultMaterial3Child8", "");
ape::TuningString g_laserFieldDefaultMaterial3Child9("Entity.LaserField.DefaultMaterial3Child9", "");
ape::TuningString g_laserFieldDefaultMaterial3Child10("Entity.LaserField.DefaultMaterial3Child10", "");
ape::TuningString g_laserFieldDefaultMaterial3Child11("Entity.LaserField.DefaultMaterial3Child11", "");
ape::TuningString g_laserFieldDefaultMaterial3Child12("Entity.LaserField.DefaultMaterial3Child12", "");
ape::TuningString g_laserFieldDefaultMaterial3Child13("Entity.LaserField.DefaultMaterial3Child13", "");
ape::TuningString g_laserFieldDefaultMaterial3Child14("Entity.LaserField.DefaultMaterial3Child14", "");
ape::TuningString g_laserFieldDefaultMaterial3Child15("Entity.LaserField.DefaultMaterial3Child15", "");
ape::TuningColor g_laserFieldDefaultVertColor("Entity.LaserField.DefaultVertColor", { 255, 255, 255, 255 });

// Lookup table of the string variables above (0x0106FD80), indexed by
// effect slot; it points at each variable's value.
const char* const g_laserFieldStringTable[44] = {
    g_laserFieldDefaultMaterial1Child4.Get(),
    g_laserFieldDefaultMaterial1Child5.Get(),
    g_laserFieldDefaultMaterial1Child6.Get(),
    g_laserFieldDefaultMaterial1Child7.Get(),
    g_laserFieldDefaultMaterial1Child8.Get(),
    g_laserFieldDefaultMaterial1Child9.Get(),
    g_laserFieldDefaultMaterial1Child10.Get(),
    g_laserFieldDefaultMaterial1Child11.Get(),
    g_laserFieldDefaultMaterial1Child12.Get(),
    g_laserFieldDefaultMaterial1Child13.Get(),
    g_laserFieldDefaultMaterial1Child14.Get(),
    g_laserFieldDefaultMaterial1Child15.Get(),
    g_laserFieldDefaultMaterial2.Get(),
    g_laserFieldDefaultMaterial2Child1.Get(),
    g_laserFieldDefaultMaterial2Child2.Get(),
    g_laserFieldDefaultMaterial2Child3.Get(),
    g_laserFieldDefaultMaterial2Child4.Get(),
    g_laserFieldDefaultMaterial2Child5.Get(),
    g_laserFieldDefaultMaterial2Child6.Get(),
    g_laserFieldDefaultMaterial2Child7.Get(),
    g_laserFieldDefaultMaterial2Child8.Get(),
    g_laserFieldDefaultMaterial2Child9.Get(),
    g_laserFieldDefaultMaterial2Child10.Get(),
    g_laserFieldDefaultMaterial2Child11.Get(),
    g_laserFieldDefaultMaterial2Child12.Get(),
    g_laserFieldDefaultMaterial2Child13.Get(),
    g_laserFieldDefaultMaterial2Child14.Get(),
    g_laserFieldDefaultMaterial2Child15.Get(),
    g_laserFieldDefaultMaterial3.Get(),
    g_laserFieldDefaultMaterial3Child1.Get(),
    g_laserFieldDefaultMaterial3Child2.Get(),
    g_laserFieldDefaultMaterial3Child3.Get(),
    g_laserFieldDefaultMaterial3Child4.Get(),
    g_laserFieldDefaultMaterial3Child5.Get(),
    g_laserFieldDefaultMaterial3Child6.Get(),
    g_laserFieldDefaultMaterial3Child7.Get(),
    g_laserFieldDefaultMaterial3Child8.Get(),
    g_laserFieldDefaultMaterial3Child9.Get(),
    g_laserFieldDefaultMaterial3Child10.Get(),
    g_laserFieldDefaultMaterial3Child11.Get(),
    g_laserFieldDefaultMaterial3Child12.Get(),
    g_laserFieldDefaultMaterial3Child13.Get(),
    g_laserFieldDefaultMaterial3Child14.Get(),
    g_laserFieldDefaultMaterial3Child15.Get(),
};

} // namespace sly
