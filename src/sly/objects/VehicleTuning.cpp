// openTIT - Vehicle tuning variables (defaults; Scripts/Tuning.xml may override them).
#include "sly/objects/VehicleTuning.h"

namespace sly {
namespace objects {

// @func 0x0024F034 StaticInit_VehicleTuning
// @func 0x0024EEDC StaticDestroy_VehicleTuning
// Compiler-generated initialiser/finaliser for the globals of this file. (They also
// construct/destroy a file-local sorted map with 16-byte entries at 0x01056C00 whose
// users are in the vehicle code; it is not modelled until that code is ported.)

ape::TuningBool g_vehicleDestroyMe("Entity.Vehicle.DestroyMe", false);
ape::TuningFloat g_vehicleSimpleWheeledCollisionDamage("Entity.Vehicle.SimpleWheeled.CollisionDamage", 25.0f);
ape::TuningFloat g_vehicleSimpleWheeledCollisionImpulse("Entity.Vehicle.SimpleWheeled.CollisionImpulse", 5.0f);
ape::TuningFloat g_vehicleSimpleWheeledCollisionRumbleStrength("Entity.Vehicle.SimpleWheeled.CollisionRumbleStrength", 0.7f);
ape::TuningFloat g_vehicleSimpleWheeledCollisionRumbleDuration("Entity.Vehicle.SimpleWheeled.CollisionRumbleDuration", 0.5f);
ape::TuningInt g_vehicleSimpleWheeledHelpTextDriveMax("Entity.Vehicle.SimpleWheeled.HelpText.DriveMax", 1);
ape::TuningInt g_vehicleSimpleWheeledHelpTextShootMax("Entity.Vehicle.SimpleWheeled.HelpText.ShootMax", 4);
ape::TuningFloat g_vehicleSimpleWheeledHelpTextDriveDuration("Entity.Vehicle.SimpleWheeled.HelpText.DriveDuration", 5.0f);
ape::TuningFloat g_vehicleSimpleWheeledHelpTextShootDuration("Entity.Vehicle.SimpleWheeled.HelpText.ShootDuration", 3.5f);

} // namespace objects
} // namespace sly
