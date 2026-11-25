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

// The instance class of each setting class (console vtables 0x00B0E010,
// 0x00B0E1A0, 0x00B0E5A0, 0x00B0E734); they add no behaviour here.
#define SLY_AI_SETTING_INSTANCE(Name)                                            \
    class Name : public AISettingInstance {                                      \
    public:                                                                      \
        using AISettingInstance::AISettingInstance;                              \
        ~Name() override;                                                        \
    }
SLY_AI_SETTING_INSTANCE(CritterSettingInstance);
SLY_AI_SETTING_INSTANCE(BuddySettingInstance);
SLY_AI_SETTING_INSTANCE(EnemySettingInstance);
SLY_AI_SETTING_INSTANCE(SentrySettingInstance);
#undef SLY_AI_SETTING_INSTANCE

class AISettingClass {
public:
    virtual ~AISettingClass() = default;
    // Slots 2-4: name of the setting type (the XML element), its hash and
    // the group hash (0 by default, see ape::setting::SettingDef).
    virtual const char* GetName() const = 0;
    virtual uint32_t GetTypeHash() const = 0;
    virtual uint32_t GetGroupHash() const { return 0; }
    // Slot 5: registers the class's properties.
    virtual void RegisterProperties() = 0;
    // Slot 8: new instance for a settings object.
    virtual AISettingInstance* CreateInstance(const void* source, const void* owner) = 0;

protected:
    // 0x004F7128 (engine; forwarded to the property system).
    void AddProperty(AIProp prop) { SettingClassAddProperty(this, prop); }
};

// Setting class of the critter settings objects (vtable 0x00B0DFD0, "CritterSetting").
class CritterSettingClass : public AISettingClass {
public:
    ~CritterSettingClass() override;
    const char* GetName() const override;
    uint32_t GetTypeHash() const override;
    uint32_t GetGroupHash() const override;
    void RegisterProperties() override;
    AISettingInstance* CreateInstance(const void* source, const void* owner) override;
};

// Setting class of the buddy settings objects (vtable 0x00B0E160, "PlayerSetting").
class BuddySettingClass : public AISettingClass {
public:
    ~BuddySettingClass() override;
    const char* GetName() const override;
    uint32_t GetTypeHash() const override;
    void RegisterProperties() override;
    AISettingInstance* CreateInstance(const void* source, const void* owner) override;
};

// Setting class of the enemy settings objects (vtable 0x00B0E560, "EnemySetting").
class EnemySettingClass : public AISettingClass {
public:
    ~EnemySettingClass() override;
    const char* GetName() const override;
    uint32_t GetTypeHash() const override;
    uint32_t GetGroupHash() const override;
    void RegisterProperties() override;
    AISettingInstance* CreateInstance(const void* source, const void* owner) override;
};

// Setting class of the sentry settings objects (vtable 0x00B0E6F0,
// "ScriptedPathSetting").
class SentrySettingClass : public AISettingClass {
public:
    ~SentrySettingClass() override;
    const char* GetName() const override;
    uint32_t GetTypeHash() const override;
    uint32_t GetGroupHash() const override;
    void RegisterProperties() override;
    AISettingInstance* CreateInstance(const void* source, const void* owner) override;
};

} // namespace ai
} // namespace levels
} // namespace sly
