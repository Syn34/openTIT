// openTIT - Vehicle tuning variables.
#pragma once

#include "ape/tuning/ApeTuning.h"

namespace sly {
namespace objects {

extern ape::TuningBool g_vehicleDestroyMe;   // 0x01056E7C
extern ape::TuningFloat g_vehicleSimpleWheeledCollisionDamage;   // 0x01056C1C
extern ape::TuningFloat g_vehicleSimpleWheeledCollisionImpulse;   // 0x01056C68
extern ape::TuningFloat g_vehicleSimpleWheeledCollisionRumbleStrength;   // 0x01056CB4
extern ape::TuningFloat g_vehicleSimpleWheeledCollisionRumbleDuration;   // 0x01056D00
extern ape::TuningInt g_vehicleSimpleWheeledHelpTextDriveMax;   // 0x01056D4C
extern ape::TuningInt g_vehicleSimpleWheeledHelpTextShootMax;   // 0x01056D98
extern ape::TuningFloat g_vehicleSimpleWheeledHelpTextDriveDuration;   // 0x01056DE4
extern ape::TuningFloat g_vehicleSimpleWheeledHelpTextShootDuration;   // 0x01056E30

} // namespace objects
} // namespace sly
