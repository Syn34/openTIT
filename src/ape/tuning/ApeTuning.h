// openTIT - Tuning variables.
//
// Designers tweak gameplay constants through named, typed globals such as
//
//     static TuningFloat s_stuckCamInterp("Entity.WestBoss.StuckCamera.Interp", 0.1f);
//
// Every variable registers itself (by name hash) in a global table when it
// is constructed. At start-up Scripts/Tuning.xml is applied on top of the
// compiled-in defaults: each <Var name="..." value="..."/> overrides the
// variable with that name.
#pragma once

#include <cstdint>

#include "ape/xml/ApeXml.h"

namespace ape {

class TuningVar {
public:
    enum Type : int32_t {
        kBool = 0, kInt = 1, kFloat = 2, kVec2 = 3, kVec3 = 4, kColor = 5, kString = 6, kHash = 7,
    };

    explicit TuningVar(const char* name);
    virtual ~TuningVar();

    virtual Type GetType() const = 0;
    // Setters used by the loader; each type accepts the inputs it can convert.
    virtual void SetBool(bool value);
    virtual void SetInt(int32_t value);
    virtual void SetFloat(float value);
    virtual void SetVec2(const float value[2]);
    virtual void SetVec3(const float value[3]);
    virtual void SetColor(const xml::Color8* value);
    virtual void SetString(const char* value);
    virtual void SetHash(uint32_t value);

    const char* GetName() const { return m_name; }
    uint32_t GetNameHash() const { return m_nameHash; }

    TuningVar(const TuningVar&) = delete;
    TuningVar& operator=(const TuningVar&) = delete;

protected:
    char m_name[0x40];
    uint32_t m_nameHash;
};

class TuningBool final : public TuningVar {
public:
    // @func 0x00735FB0 TuningBool::TuningBool
    TuningBool(const char* name, bool value) : TuningVar(name), m_value(value) {}
    ~TuningBool() override;
    Type GetType() const override;
    void SetBool(bool value) override;
    void SetInt(int32_t value) override;
    void SetFloat(float value) override;
    bool Get() const { return m_value; }
    operator bool() const { return m_value; }
private:
    bool m_value;
};

class TuningInt final : public TuningVar {
public:
    // @func 0x007360A8 TuningInt::TuningInt
    TuningInt(const char* name, int32_t value) : TuningVar(name), m_value(value) {}
    ~TuningInt() override;
    Type GetType() const override;
    void SetInt(int32_t value) override;
    void SetFloat(float value) override;
    int32_t Get() const { return m_value; }
    operator int32_t() const { return m_value; }
private:
    int32_t m_value;
};

class TuningFloat final : public TuningVar {
public:
    // @func 0x0073616C TuningFloat::TuningFloat
    TuningFloat(const char* name, float value) : TuningVar(name), m_value(value) {}
    ~TuningFloat() override;
    Type GetType() const override;
    void SetInt(int32_t value) override;
    void SetFloat(float value) override;
    float Get() const { return m_value; }
    operator float() const { return m_value; }
private:
    float m_value;
};

class TuningVec2 final : public TuningVar {
public:
    // @func 0x0074E990 TuningVec2::TuningVec2
    TuningVec2(const char* name, float x, float y) : TuningVar(name), m_value{ x, y } {}
    ~TuningVec2() override;
    Type GetType() const override;
    void SetVec2(const float value[2]) override;
    const float* Get() const { return m_value; }
private:
    float m_value[2];
};

class TuningVec3 final : public TuningVar {
public:
    // @func 0x00736244 TuningVec3::TuningVec3
    TuningVec3(const char* name, float x, float y, float z) : TuningVar(name), m_value{ x, y, z, 0.0f } {}
    ~TuningVec3() override;
    Type GetType() const override;
    void SetVec3(const float value[3]) override;
    const float* Get() const { return m_value; }
private:
    alignas(16) float m_value[4];
};

class TuningColor final : public TuningVar {
public:
    // @func 0x00736320 TuningColor::TuningColor
    TuningColor(const char* name, xml::Color8 value) : TuningVar(name), m_value(value) {}
    ~TuningColor() override;
    Type GetType() const override;
    void SetColor(const xml::Color8* value) override;
    xml::Color8 Get() const { return m_value; }
private:
    xml::Color8 m_value;
};

class TuningString final : public TuningVar {
public:
    TuningString(const char* name, const char* value);
    ~TuningString() override;
    Type GetType() const override;
    void SetString(const char* value) override;
    const char* Get() const { return m_value; }
private:
    char m_value[0x80];
};

class TuningHash final : public TuningVar {
public:
    // @func 0x0073650C TuningHash::TuningHash
    TuningHash(const char* name, uint32_t value) : TuningVar(name), m_value(value) {}
    ~TuningHash() override;
    Type GetType() const override;
    void SetString(const char* value) override;
    void SetHash(uint32_t value) override;
    uint32_t Get() const { return m_value; }
private:
    uint32_t m_value;
};

// Registry access (mostly for tools/debug menus).
int TuningVarCount();
TuningVar* TuningVarAt(int index);
TuningVar* FindTuningVar(uint32_t nameHash);

// Applies a tuning file. `compareStrings` mirrors a developer option that
// diffed string values while loading.
bool LoadTuningFile(const char* path, bool compareStrings);

} // namespace ape
