// openTIT - Animatable GUI properties.
//
// GUI animations address object properties by name ("position", "color",
// "visible", ...). Each property is a static descriptor with a value type and
// a setter that applies a value to an object. Properties shared by every
// object live in one table; labels and images add their own ("tracking",
// "string", "uv", "st"). FindProperty looks in the class table first.
#pragma once

#include "ape/container/ApeTSortedMap.h"
#include "ape/level/ApeGui.h"

namespace ape {

// Identity of a property value type: the address of a static tag
// (0x01164E00..0x01164E10 on the console).
using GuiPropertyType = const void*;

// Property descriptor (12 bytes, base vtable 0x00B22698).
class GuiProperty {
public:
    // The setter receives a pointer to a value of the property's type.
    using Setter = void (*)(ApeGuiObject* object, const void* value);

    GuiProperty(const char* name, Setter setter) : name(name), setter(setter) {}
    virtual ~GuiProperty();
    virtual GuiPropertyType GetType() const = 0;

    void Apply(ApeGuiObject* object, const void* value) const { setter(object, value); }

    const char* name;   // +0x04
    Setter setter;      // +0x08
};

// The value types (vtables 0x00B226AC, 0x00B226C0, 0x00B226D4, 0x00B226E8, 0x00B226FC).
class GuiUIntProperty : public GuiProperty {
public:
    using GuiProperty::GuiProperty;
    ~GuiUIntProperty() override;
    GuiPropertyType GetType() const override;
    static GuiPropertyType Type();
};
class GuiFloatProperty : public GuiProperty {
public:
    using GuiProperty::GuiProperty;
    ~GuiFloatProperty() override;
    GuiPropertyType GetType() const override;
    static GuiPropertyType Type();
};
class GuiBoolProperty : public GuiProperty {
public:
    using GuiProperty::GuiProperty;
    ~GuiBoolProperty() override;
    GuiPropertyType GetType() const override;
    static GuiPropertyType Type();
};
class GuiStringProperty : public GuiProperty {
public:
    using GuiProperty::GuiProperty;
    ~GuiStringProperty() override;
    GuiPropertyType GetType() const override;
    static GuiPropertyType Type();
};
class GuiVectorProperty : public GuiProperty {
public:
    using GuiProperty::GuiProperty;
    ~GuiVectorProperty() override;
    GuiPropertyType GetType() const override;
    static GuiPropertyType Type();
};

// The property tables (0x010C2AF0 common, 0x010C2B08 labels, 0x010C2B20 images).
ApeTSortedMap<const GuiProperty*>& CommonGuiProperties();
ApeTSortedMap<const GuiProperty*>& LabelGuiProperties();
ApeTSortedMap<const GuiProperty*>& ImageGuiProperties();

// Lookups used by the FindProperty overrides.
const GuiProperty* FindGuiProperty(ApeTSortedMap<const GuiProperty*>* classTable, const char* name);
const GuiProperty* FindGuiProperty(ApeTSortedMap<const GuiProperty*>* classTable, const char* name,
                                   GuiPropertyType type);

// GUI globals set up by the module's static initialiser.
struct GuiViewport {
    int16_t x, y, width, height;
};
extern GuiViewport g_guiViewport;        // 0x010C2AE8
extern Matrix34 g_guiViewTransform;      // 0x010C29E0 world matrix of the screen fade quad
extern xml::Color8 g_guiFadeColor;       // 0x010C2AD4 colour of the full screen fade (alpha 0: none)

} // namespace ape
