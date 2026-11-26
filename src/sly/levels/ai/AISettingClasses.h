// openTIT - Setting classes of the AI settings level objects.
//
// The AI settings objects are settings (ape/setting) of four definitions:
// critter, buddy, enemy and sentry settings. Each registers its properties
// (descriptor globals listed in AIHooks.h, AIProp) and creates its settings.
#pragma once

#include "sly/levels/ai/AIHooks.h"
#include "ape/setting/ApeSetting.h"

#include <cstdint>

namespace sly {
namespace levels {
namespace ai {

// A setting of one of the AI setting classes (one per settings object in
// the level). The four instance classes (console vtables 0x00B0E010,
// 0x00B0E1A0, 0x00B0E5A0, 0x00B0E734; 0x24 or 0x28 bytes) add no behaviour.
class AISettingInstance : public ape::setting::Setting {
public:
    using Setting::Setting;
};

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

// Base of the AI setting definitions: setting definitions whose parameters
// are the AI property descriptors (AISettingParams.h).
class AISettingClass : public ape::setting::SettingDef {
protected:
    // Registers the parameter of an AI property (0x004F7128).
    void AddProperty(AIProp prop);
};

// Setting class of the critter settings objects (vtable 0x00B0DFD0, "CritterSetting").
class CritterSettingClass : public AISettingClass {
public:
    ~CritterSettingClass() override;
    const char* GetName() const override;
    uint32_t GetTypeHash() const override;
    uint32_t GetGroupHash() const override;
    void InitParams() override;
    ape::setting::Setting* CreateSetting(const char* name) override;
};

// Setting class of the buddy settings objects (vtable 0x00B0E160, "PlayerSetting").
class BuddySettingClass : public AISettingClass {
public:
    ~BuddySettingClass() override;
    const char* GetName() const override;
    uint32_t GetTypeHash() const override;
    void InitParams() override;
    ape::setting::Setting* CreateSetting(const char* name) override;
};

// Setting class of the enemy settings objects (vtable 0x00B0E560, "EnemySetting").
class EnemySettingClass : public AISettingClass {
public:
    ~EnemySettingClass() override;
    const char* GetName() const override;
    uint32_t GetTypeHash() const override;
    uint32_t GetGroupHash() const override;
    void InitParams() override;
    ape::setting::Setting* CreateSetting(const char* name) override;
};

// Setting class of the sentry settings objects (vtable 0x00B0E6F0,
// "ScriptedPathSetting").
class SentrySettingClass : public AISettingClass {
public:
    ~SentrySettingClass() override;
    const char* GetName() const override;
    uint32_t GetTypeHash() const override;
    uint32_t GetGroupHash() const override;
    void InitParams() override;
    ape::setting::Setting* CreateSetting(const char* name) override;
};

} // namespace ai
} // namespace levels
} // namespace sly
