// openTIT - Animatable GUI properties (0x004E9F78 / 0x004E9DB4 and the setters at 0x00795A34...).
#include "ape/level/ApeGuiProperties.h"

#include "ape/core/ApeString.h"
#include "ape/level/ApeGuiTypes.h"

namespace ape {

// ---------------------------------------------------------------------------
// Descriptor classes
// ---------------------------------------------------------------------------

namespace {
// Type tags (0x01164E00..0x01164E10): only their addresses matter.
const char s_uintTag = 0;
const char s_floatTag = 0;
const char s_boolTag = 0;
const char s_stringTag = 0;
const char s_vectorTag = 0;
} // namespace

// @func 0x00794EC0 GuiProperty::~GuiProperty
// @func 0x00795830 GuiProperty::~GuiProperty (deleting)
GuiProperty::~GuiProperty() = default;

// @func 0x00794ED0 GuiUIntProperty::~GuiUIntProperty
// @func 0x007958A4 GuiUIntProperty::~GuiUIntProperty (deleting)
GuiUIntProperty::~GuiUIntProperty() = default;
// @func 0x007958E8 GuiUIntProperty::GetType
GuiPropertyType GuiUIntProperty::GetType() const { return Type(); }
GuiPropertyType GuiUIntProperty::Type() { return &s_uintTag; }

// @func 0x00794F00 GuiFloatProperty::~GuiFloatProperty
// @func 0x007958F4 GuiFloatProperty::~GuiFloatProperty (deleting)
GuiFloatProperty::~GuiFloatProperty() = default;
// @func 0x00795938 GuiFloatProperty::GetType
GuiPropertyType GuiFloatProperty::GetType() const { return Type(); }
GuiPropertyType GuiFloatProperty::Type() { return &s_floatTag; }

// @func 0x00794EE0 GuiBoolProperty::~GuiBoolProperty
// @func 0x00795944 GuiBoolProperty::~GuiBoolProperty (deleting)
GuiBoolProperty::~GuiBoolProperty() = default;
// @func 0x00795988 GuiBoolProperty::GetType
GuiPropertyType GuiBoolProperty::GetType() const { return Type(); }
GuiPropertyType GuiBoolProperty::Type() { return &s_boolTag; }

// @func 0x00794F10 GuiStringProperty::~GuiStringProperty
// @func 0x00795994 GuiStringProperty::~GuiStringProperty (deleting)
GuiStringProperty::~GuiStringProperty() = default;
// @func 0x007959D8 GuiStringProperty::GetType
GuiPropertyType GuiStringProperty::GetType() const { return Type(); }
GuiPropertyType GuiStringProperty::Type() { return &s_stringTag; }

// @func 0x00794EF0 GuiVectorProperty::~GuiVectorProperty
// @func 0x007959E4 GuiVectorProperty::~GuiVectorProperty (deleting)
GuiVectorProperty::~GuiVectorProperty() = default;
// @func 0x00795A28 GuiVectorProperty::GetType
GuiPropertyType GuiVectorProperty::GetType() const { return Type(); }
GuiPropertyType GuiVectorProperty::Type() { return &s_vectorTag; }

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------

namespace {

template <class T>
const T& ValueAs(const void* value)
{
    return *static_cast<const T*>(value);
}

// @func 0x00795A34 SetGuiAlignProperty
void SetAlignProperty(ApeGuiObject* object, const void* value)
{
    object->SetAlign(ValueAs<uint32_t>(value));
}

// @func 0x00795A74 SetGuiAnchorProperty
void SetAnchorProperty(ApeGuiObject* object, const void* value)
{
    object->SetAnchor(ValueAs<uint32_t>(value));
}

// @func 0x00795AB4 SetGuiColorProperty
// Sets red, green and blue; the alpha is kept.
void SetColorProperty(ApeGuiObject* object, const void* value)
{
    const xml::Color8 rgb = ValueAs<xml::Color8>(value);
    xml::Color8 color = object->GetColor();
    color.r = rgb.r;
    color.g = rgb.g;
    color.b = rgb.b;
    object->SetColor(color);
}

// @func 0x00795B68 SetGuiColorAlphaProperty
void SetColorAlphaProperty(ApeGuiObject* object, const void* value)
{
    xml::Color8 color = object->GetColor();
    color.a = ValueAs<uint8_t>(value);
    object->SetColor(color);
}

// @func 0x00795C30 SetGuiExpandProperty
void SetExpandProperty(ApeGuiObject* object, const void* value)
{
    object->SetExpand(ValueAs<bool>(value));
}

// @func 0x00795C70 SetGuiScaleProperty
void SetScaleProperty(ApeGuiObject* object, const void* value)
{
    const Vec3 scale = ValueAs<Vec3>(value);
    object->SetScale(scale);
}

// @func 0x00795CC8 SetGuiSizeProperty
void SetSizeProperty(ApeGuiObject* object, const void* value)
{
    const float* v = static_cast<const float*>(value);
    const float size[2] = { v[0], v[1] };
    object->SetSize(size);
}

// @func 0x00795D18 SetGuiPositionProperty
void SetPositionProperty(ApeGuiObject* object, const void* value)
{
    const Vec3 position = ValueAs<Vec3>(value);
    object->SetPosition(position);
}

// @func 0x00795D70 SetGuiRotationProperty
void SetRotationProperty(ApeGuiObject* object, const void* value)
{
    object->SetRotation(ValueAs<float>(value));
}

// @func 0x00795DAC SetGuiMovePosition2Property
// NOTE: identical to "position" apart from the name.
void SetMovePosition2Property(ApeGuiObject* object, const void* value)
{
    const Vec3 position = ValueAs<Vec3>(value);
    object->SetPosition(position);
}

// @func 0x00795E2C SetGuiVisibleProperty
void SetVisibleProperty(ApeGuiObject* object, const void* value)
{
    object->SetVisible(ValueAs<bool>(value));
}

// @func 0x00796354 SetGuiUVProperty
void SetUVProperty(ApeGuiObject* object, const void* value)
{
    ApeGuiImage* image = static_cast<ApeGuiImage*>(object);
    const float* v = static_cast<const float*>(value);
    image->uv[0] = v[0];
    image->uv[1] = v[1];
    image->Invalidate();
}

// @func 0x007963A0 SetGuiSTProperty
void SetSTProperty(ApeGuiObject* object, const void* value)
{
    ApeGuiImage* image = static_cast<ApeGuiImage*>(object);
    const float* v = static_cast<const float*>(value);
    image->st[0] = v[0];
    image->st[1] = v[1];
    image->Invalidate();
}

// @func 0x00795E74 SetGuiTrackingProperty
void SetTrackingProperty(ApeGuiObject* object, const void* value)
{
    static_cast<ApeGuiLabel*>(object)->tracking = ValueAs<float>(value);
}

// @func 0x00795E7C SetGuiStringProperty
void SetStringProperty(ApeGuiObject* object, const void* value)
{
    object->SetText(ValueAs<const char*>(value), false);
}

} // namespace

// ---------------------------------------------------------------------------
// Tables
// ---------------------------------------------------------------------------

// The descriptors, the three tables and the GUI globals are built by the
// module's static initialiser and torn down by its static destructor; here
// they are the C++ globals below.
// @func 0x004E9F78 StaticInit_GuiProperties
// @func 0x004E9DB4 StaticDestroy_GuiProperties

GuiViewport g_guiViewport = { 0, 0, 0, 0 };
Matrix34 g_guiViewTransform = {
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, -1.0f, 0.0f },
};
uint8_t g_guiUnknown2AD4[4] = {};   // 0x010C2AD4 NOTE: zeroed, use not identified

namespace {

GuiUIntProperty s_alignProperty("align", &SetAlignProperty);                       // 0x010C2A20
GuiUIntProperty s_anchorProperty("anchor", &SetAnchorProperty);                    // 0x010C2A2C
GuiUIntProperty s_colorProperty("color", &SetColorProperty);                       // 0x010C2A38
GuiUIntProperty s_colorAlphaProperty("coloralpha", &SetColorAlphaProperty);        // 0x010C2A44
GuiBoolProperty s_expandProperty("expand", &SetExpandProperty);                    // 0x010C2A50
GuiVectorProperty s_scaleProperty("scale", &SetScaleProperty);                     // 0x010C2A5C
GuiVectorProperty s_sizeProperty("size", &SetSizeProperty);                        // 0x010C2A68
GuiVectorProperty s_positionProperty("position", &SetPositionProperty);            // 0x010C2A74
GuiFloatProperty s_rotationProperty("rotation", &SetRotationProperty);             // 0x010C2A80
GuiVectorProperty s_movePosition2Property("movePosition2", &SetMovePosition2Property); // 0x010C2A8C
GuiBoolProperty s_visibleProperty("visible", &SetVisibleProperty);                 // 0x010C2A98
GuiVectorProperty s_uvProperty("uv", &SetUVProperty);                              // 0x010C2AA4
GuiVectorProperty s_stProperty("st", &SetSTProperty);                              // 0x010C2AB0
GuiFloatProperty s_trackingProperty("tracking", &SetTrackingProperty);             // 0x010C2ABC
GuiStringProperty s_stringProperty("string", &SetStringProperty);                  // 0x010C2AC8

struct GuiPropertyTables {
    ApeTSortedMap<const GuiProperty*> common;
    ApeTSortedMap<const GuiProperty*> label;
    ApeTSortedMap<const GuiProperty*> image;

    GuiPropertyTables()
    {
        for (const GuiProperty* p : { (const GuiProperty*)&s_alignProperty, (const GuiProperty*)&s_anchorProperty,
                                      (const GuiProperty*)&s_colorProperty, (const GuiProperty*)&s_colorAlphaProperty,
                                      (const GuiProperty*)&s_expandProperty, (const GuiProperty*)&s_scaleProperty,
                                      (const GuiProperty*)&s_sizeProperty, (const GuiProperty*)&s_positionProperty,
                                      (const GuiProperty*)&s_rotationProperty,
                                      (const GuiProperty*)&s_movePosition2Property,
                                      (const GuiProperty*)&s_visibleProperty })
            common.Add(HashNameNoCase(p->name), p);
        image.Add(HashNameNoCase(s_uvProperty.name), &s_uvProperty);
        image.Add(HashNameNoCase(s_stProperty.name), &s_stProperty);
        label.Add(HashNameNoCase(s_trackingProperty.name), &s_trackingProperty);
        label.Add(HashNameNoCase(s_stringProperty.name), &s_stringProperty);
    }
};

GuiPropertyTables& Tables()
{
    static GuiPropertyTables s_tables;
    return s_tables;
}

} // namespace

ApeTSortedMap<const GuiProperty*>& CommonGuiProperties() { return Tables().common; }
ApeTSortedMap<const GuiProperty*>& LabelGuiProperties() { return Tables().label; }
ApeTSortedMap<const GuiProperty*>& ImageGuiProperties() { return Tables().image; }

const GuiProperty* FindGuiProperty(ApeTSortedMap<const GuiProperty*>* classTable, const char* name)
{
    const uint32_t hash = HashNameNoCase(name);
    if (classTable) {
        if (const GuiProperty* p = *classTable->Find(hash))
            return p;
    }
    return *CommonGuiProperties().Find(hash);
}

const GuiProperty* FindGuiProperty(ApeTSortedMap<const GuiProperty*>* classTable, const char* name,
                                   GuiPropertyType type)
{
    const uint32_t hash = HashNameNoCase(name);
    if (classTable) {
        const GuiProperty* p = *classTable->Find(hash);
        if (p && p->GetType() == type)
            return p;
    }
    const GuiProperty* p = *CommonGuiProperties().Find(hash);
    return (p && p->GetType() == type) ? p : nullptr;
}

// ---------------------------------------------------------------------------
// FindProperty overrides
// ---------------------------------------------------------------------------

// @func 0x004E1368 ApeGuiObject::FindProperty
const GuiProperty* ApeGuiObject::FindProperty(const char* name) const
{
    return FindGuiProperty(nullptr, name);
}

// @func 0x004E13B8 ApeGuiObject::FindProperty(type)
const GuiProperty* ApeGuiObject::FindProperty(const char* name, const void* type) const
{
    return FindGuiProperty(nullptr, name, type);
}

// @func 0x004E2BE4 ApeGuiLabel::FindProperty
const GuiProperty* ApeGuiLabel::FindProperty(const char* name) const
{
    return FindGuiProperty(&LabelGuiProperties(), name);
}

// @func 0x004E2C78 ApeGuiLabel::FindProperty(type)
const GuiProperty* ApeGuiLabel::FindProperty(const char* name, const void* type) const
{
    return FindGuiProperty(&LabelGuiProperties(), name, type);
}

// @func 0x004E6074 ApeGuiImage::FindProperty
const GuiProperty* ApeGuiImage::FindProperty(const char* name) const
{
    return FindGuiProperty(&ImageGuiProperties(), name);
}

// @func 0x004E6108 ApeGuiImage::FindProperty(type)
const GuiProperty* ApeGuiImage::FindProperty(const char* name, const void* type) const
{
    return FindGuiProperty(&ImageGuiProperties(), name, type);
}

} // namespace ape
