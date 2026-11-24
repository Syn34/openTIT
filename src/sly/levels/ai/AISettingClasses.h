// openTIT - Setting classes of the AI settings level objects.
//
// Level objects carry their values in property blocks described by a
// setting class (the engine's property/DTD system; base at 0x004F7128 adds
// a property, 0x004F77F8 constructs an instance). The level AI defines four
// classes: critter, buddy, enemy and sentry settings. Each registers its
// properties (descriptor globals listed in AIHooks.h, AIProp) and creates
// its instances (0x24 or 0x28 bytes).
#pragma once

#include "sly/levels/ai/AIHooks.h"

#include <cstdint>

namespace sly {
namespace levels {
namespace ai {

// Instance of a setting class (one per settings object in the level).
class AISettingInstance {
public:
    AISettingInstance(const void* source, const class AISettingClass* settingClass)
        : source(source), settingClass(settingClass) {}
    virtual ~AISettingInstance() = default;

    const void* source = nullptr;                    // level data the instance reads
    const class AISettingClass* settingClass = nullptr;
};

class AISettingClass {
public:
    virtual ~AISettingClass() = default;
    // Slot 5: registers the class's properties.
    virtual void RegisterProperties() = 0;
    // Slot 8: new instance for a settings object.
    virtual AISettingInstance* CreateInstance(const void* source, const void* owner) = 0;

protected:
    // 0x004F7128 (engine; forwarded to the property system).
    void AddProperty(AIProp prop) { SettingClassAddProperty(this, prop); }
};

// Setting class of the critter settings objects (vtable 0x00B0DFD0).
class CritterSettingClass : public AISettingClass {
public:
    void RegisterProperties() override;
    AISettingInstance* CreateInstance(const void* source, const void* owner) override;
};

// Setting class of the buddy settings objects (vtable 0x00B0E160).
class BuddySettingClass : public AISettingClass {
public:
    void RegisterProperties() override;
    AISettingInstance* CreateInstance(const void* source, const void* owner) override;
};

// Setting class of the enemy settings objects (vtable 0x00B0E560).
class EnemySettingClass : public AISettingClass {
public:
    void RegisterProperties() override;
    AISettingInstance* CreateInstance(const void* source, const void* owner) override;
};

// Setting class of the sentry settings objects (vtable 0x00B0E6F0).
class SentrySettingClass : public AISettingClass {
public:
    void RegisterProperties() override;
    AISettingInstance* CreateInstance(const void* source, const void* owner) override;
};

} // namespace ai
} // namespace levels
} // namespace sly
