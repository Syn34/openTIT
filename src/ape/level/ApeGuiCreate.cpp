// openTIT - Creating GUI objects from GUI file elements.
//
// Every object element carries the common attributes (name, position,
// rotation, scale, color, blend, anchor, align and the flags) and the
// element's own ones. A "clone" element copies an existing object ("from",
// resolved like FindGuiObject relative to the parent): the copied object's
// values become the defaults of the common attributes and of the class
// attributes, and its children are copied too unless the element says
// otherwise. Unknown elements (and clones of unknown classes) go to hooks
// the game can install.
#include "ape/level/ApeGuiCreate.h"

#include "ape/core/ApeString.h"
#include "ape/level/ApeGuiTypes.h"
#include "ape/math/ApeMatrixOps.h"
#include "ape/sys/ApeGeometry.h"
#include "platform/Platform.h"

namespace ape {

namespace {

constexpr uint32_t Hash(const char* name)
{
    uint32_t hash = 0;
    for (; *name; ++name) {
        int c = (signed char)*name;
        if (c >= 'a' && c <= 'z')
            c -= 'a' - 'A';
        hash = hash * 31u + (uint32_t)c;
    }
    return hash;
}

// Element names.
constexpr uint32_t kElemFrame = Hash("frame");
constexpr uint32_t kElemImage = Hash("image");
constexpr uint32_t kElemGradient = Hash("gradient");
constexpr uint32_t kElemLabel = Hash("label");
constexpr uint32_t kElemTicker = Hash("ticker");
constexpr uint32_t kElemTile = Hash("tile");
constexpr uint32_t kElemGroup = Hash("group");
constexpr uint32_t kElemClipRegion = Hash("clipregion");
constexpr uint32_t kElemClone = Hash("clone");
// NOTE: element of GUI type 8, name unknown.
constexpr uint32_t kElemType8 = 0x30CF0025;
static_assert(kElemClipRegion == 0xAA8CFA44u, "clip region element hash");

// Common attributes.
constexpr uint32_t kAttrPosition = Hash("position");
constexpr uint32_t kAttrRotation = Hash("rotation");
constexpr uint32_t kAttrScale = Hash("scale");
constexpr uint32_t kAttrColor = Hash("color");
constexpr uint32_t kAttrVisible = Hash("visible");
constexpr uint32_t kAttrFlag30 = 0xFFD1EC91;       // NOTE: name unknown (ApeGuiObject::kFlag_30)
constexpr uint32_t kAttrExpand = Hash("expand");
constexpr uint32_t kAttrInheritScale = Hash("inheritscale");
constexpr uint32_t kAttrInheritRgb = Hash("inheritrgb");
constexpr uint32_t kAttrInheritAlpha = Hash("inheritalpha");
constexpr uint32_t kAttrRender3d = Hash("render3d");
// NOTE: boolean of "clone" elements (default true) deciding whether the
// copied object's children are copied as well; name unknown.
constexpr uint32_t kAttrCloneChildren = 0x242AA215;
static_assert(kAttrExpand == 0x7ABBD29Au, "expand attribute hash");

// Class attributes.
constexpr uint32_t kAttrSize = Hash("size");
constexpr uint32_t kAttrNoClone = Hash("noclone");
constexpr uint32_t kAttrUv = Hash("uv");
constexpr uint32_t kAttrSt = Hash("st");
constexpr uint32_t kAttrColorTL = Hash("colortl");
constexpr uint32_t kAttrColorTR = Hash("colortr");
constexpr uint32_t kAttrColorBL = Hash("colorbl");
constexpr uint32_t kAttrColorBR = Hash("colorbr");
constexpr uint32_t kAttrOffset = Hash("offset");
constexpr uint32_t kAttrArrange = Hash("arrange");
constexpr uint32_t kAttrSpacing = Hash("spacing");
// NOTE: group attribute selecting "column" or "row"; name unknown.
constexpr uint32_t kAttrGroupFlow = 0xFBA47770;
// NOTE: clip region attribute (see GuiClipRegionDesc::projectedSize).
constexpr uint32_t kAttrProjectedSize = 0x76A60DC1;

// Value tables of the group enums (0x00AEF590, 0x00AEF5A0).
constexpr uint32_t kGroupFlowNames[2] = { Hash("column"), Hash("row") };
constexpr uint32_t kGroupArrangeNames[2] = { Hash("center"), Hash("padding") };

const char* const kBlendNames[3] = { "av", "ad", "su" };

constexpr float kDegToRad = 0.017453292f;
constexpr Vec3 kZAxis = { 0.0f, 0.0f, 1.0f, 0.0f };   // 0x01164B70

GuiObjectFactory s_elementFactory = nullptr;   // 0x00F66894
GuiObjectFactory s_cloneFactory = nullptr;     // 0x00F66898

const char* DocPath(xml::XmlDocument* doc)
{
    return doc ? doc->GetDocumentPath() : "";
}

void Warn(xml::XmlDocument* doc, const char* message, const char* detail = "")
{
    plat::Log(plat::LogLevel::Warning, "%s: %s%s\n", DocPath(doc), message, detail);
}

// Warns when 'source' (the object a clone copies) is not of 'type'.
bool CheckSourceType(xml::XmlDocument* doc, ApeGuiObject* source, int32_t type)
{
    if (!source)
        return false;
    if (source->GetType() == type)
        return true;
    Warn(doc, "GUI clone source of a different class");
    return false;
}

// Reads an enum attribute given as one of the hashed names in 'names'.
void ReadEnum(xml::XmlDocument* doc, xml::XmlElement* element, uint32_t attribute, const uint32_t* names,
              uint32_t count, uint32_t& value)
{
    uint32_t hash = 0;
    if (element->GetHash(attribute, &hash, 0) != 0)
        return;
    for (uint32_t i = 0; i < count; ++i) {
        if (names[i] == hash) {
            value = i;
            return;
        }
    }
    Warn(doc, "unknown GUI enum value");
}

// "anchor" / "align": letters L C R (horizontal) and T M B (vertical).
void ParseAlignment(const char* text, uint32_t& value)
{
    for (const char* p = text; p && *p; ++p) {
        switch (*p) {
        case 'L': case 'l': value = (value & ~0x03u) | kGuiAlign_Left; break;
        case 'C': case 'c': value = (value & ~0x03u) | kGuiAlign_Center; break;
        case 'R': case 'r': value = (value & ~0x03u) | kGuiAlign_Right; break;
        case 'T': case 't': value = (value & ~0x30u) | kGuiAlign_Top; break;
        case 'M': case 'm': value = (value & ~0x30u) | kGuiAlign_Middle; break;
        case 'B': case 'b': value = (value & ~0x30u) | kGuiAlign_Bottom; break;
        default: break;
        }
    }
}

// The common attribute values of an existing object.
void CopyCommonValues(GuiObjectDesc& desc, ApeGuiObject* source)
{
    desc.transform = source->GetTransform();
    desc.scale = source->GetScale();
    desc.color = source->GetColor();
    desc.blendMode = source->GetBlendMode();
    desc.align = source->GetAlign();
    desc.anchor = source->GetAnchor();
    desc.visible = source->IsVisible();
    desc.flag30 = source->GetFlag30();
    desc.expand = source->GetExpand();
    desc.inheritScale = source->GetInheritScale();
    desc.inheritRgb = source->GetInheritRgb();
    desc.inheritAlpha = source->GetInheritAlpha();
    desc.render3d = source->GetRender3d();
}

Matrix34 IdentityTransform()
{
    Matrix34 m{};
    m.xAxis.x = 1.0f;
    m.yAxis.y = 1.0f;
    m.zAxis.z = 1.0f;
    return m;
}

// Copies the common part of a description into a class description.
template <typename Desc>
Desc MakeDesc(const GuiObjectDesc& base)
{
    Desc desc;
    static_cast<GuiObjectDesc&>(desc) = base;
    return desc;
}

} // namespace

void SetGuiObjectFactories(GuiObjectFactory elementFactory, GuiObjectFactory cloneFactory)
{
    s_elementFactory = elementFactory;
    s_cloneFactory = cloneFactory;
}

// @func 0x004DDD2C CreateGuiFrame
ApeGuiObject* CreateGuiFrame(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                             ApeGuiObject* source)
{
    GuiFrameDesc desc = MakeDesc<GuiFrameDesc>(base);
    float defaultSize[2] = { 0.0f, 0.0f };
    if (CheckSourceType(doc, source, kGuiObject_Type0))
        source->GetSize(defaultSize);
    element->GetVec2(kAttrSize, desc.size, defaultSize);
    return new ApeGuiFrame(desc);
}

// @func 0x004DD800 CreateGuiImage
ApeGuiObject* CreateGuiImage(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                             ApeGuiObject* source)
{
    GuiImageDesc desc = MakeDesc<GuiImageDesc>(base);
    float defaultUv[2] = { 0.0f, 0.0f };
    float defaultSt[2] = { 1.0f, 1.0f };
    const float defaultSize[2] = { -1.0f, -1.0f };
    ApeGuiTexture* texture = nullptr;
    if (CheckSourceType(doc, source, kGuiObject_Type1)) {
        const auto* image = static_cast<const ApeGuiImage*>(source);
        defaultUv[0] = image->uv[0];
        defaultUv[1] = image->uv[1];
        defaultSt[0] = image->st[0];
        defaultSt[1] = image->st[1];
        texture = image->texture;
    }
    if (const char* textureName = element->GetString("texture")) {
        texture = static_cast<ApeGuiTexture*>(ApeGuiResource::FindByName(HashNameNoCase(textureName)));
        if (!texture)
            Warn(doc, "unknown GUI texture ", textureName);
    }
    desc.texture = texture;
    element->GetVec2(kAttrUv, desc.uv, defaultUv);
    element->GetVec2(kAttrSt, desc.st, defaultSt);
    element->GetVec2(kAttrSize, desc.size, defaultSize);
    return new ApeGuiImage(desc);
}

// @func 0x004DD1F0 CreateGuiGradient
// Every corner defaults to the object's colour.
ApeGuiObject* CreateGuiGradient(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                                ApeGuiObject* source)
{
    GuiGradientDesc desc = MakeDesc<GuiGradientDesc>(base);
    float defaultSize[2] = { 0.0f, 0.0f };
    if (CheckSourceType(doc, source, kGuiObject_Type2))
        source->GetSize(defaultSize);
    element->GetVec2(kAttrSize, desc.size, defaultSize);
    const uint32_t corners[4] = { kAttrColorTL, kAttrColorTR, kAttrColorBL, kAttrColorBR };
    for (int i = 0; i < 4; ++i)
        element->GetColor(corners[i], &desc.colors[i], &base.color);
    return new ApeGuiGradient(desc);
}

// @func 0x004D8460 CreateGuiGroup
ApeGuiObject* CreateGuiGroup(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                             ApeGuiObject* source)
{
    GuiGroupDesc desc = MakeDesc<GuiGroupDesc>(base);
    float defaultSize[2] = { 0.0f, 0.0f };
    const float defaultOffset[2] = { 0.0f, 0.0f };
    float defaultSpacing = 3.4028235e38f;
    if (CheckSourceType(doc, source, kGuiObject_Type7)) {
        const auto* group = static_cast<const ApeGuiGroup*>(source);
        source->GetSize(defaultSize);
        desc.flow = group->flow;
        desc.arrange = group->arrange;
        defaultSpacing = group->spacing;
    }
    element->GetVec2(kAttrSize, desc.size, defaultSize);
    element->GetVec2(kAttrOffset, desc.offset, defaultOffset);
    ReadEnum(doc, element, kAttrGroupFlow, kGroupFlowNames, 2, desc.flow);
    ReadEnum(doc, element, kAttrArrange, kGroupArrangeNames, 2, desc.arrange);
    element->GetFloat(kAttrSpacing, &desc.spacing, defaultSpacing);
    return new ApeGuiGroup(desc);
}

// @func 0x004D7910 CreateGuiClipRegion
ApeGuiObject* CreateGuiClipRegion(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                                  ApeGuiObject* source)
{
    GuiClipRegionDesc desc = MakeDesc<GuiClipRegionDesc>(base);
    float defaultSize[2] = { 0.0f, 0.0f };
    const float defaultProjected[2] = { 0.0f, 0.0f };
    if (CheckSourceType(doc, source, kGuiObject_ClipRegion))
        source->GetSize(defaultSize);
    element->GetVec2(kAttrSize, desc.size, defaultSize);
    element->GetVec2(kAttrProjectedSize, desc.projectedSize, defaultProjected);
    return new ApeGuiClipRegion(desc);
}

// @func 0x004DE0F8 CreateGuiObject
ApeGuiObject* CreateGuiObject(xml::XmlDocument* doc, xml::XmlElement* element, ApeGuiObject* parent, void* owner)
{
    const uint32_t elementType = element->IsValid() ? element->GetNameHash() : 0;
    const char* name = element->GetString("name");
    if (!name) {
        Warn(doc, "GUI object without a name");
        return nullptr;
    }

    GuiObjectDesc desc;
    desc.name = name;
    desc.transform = IdentityTransform();
    desc.scale = { 1.0f, 1.0f, 1.0f, 0.0f };
    desc.color = { 0x80, 0x80, 0x80, 0x80 };
    GuiObjectDesc defaults = desc;   // defaults of the common attributes
    bool cloneChildren = true;

    ApeGuiObject* source = nullptr;
    if (elementType == kElemClone) {
        const char* from = element->GetString("from");
        if (!from) {
            Warn(doc, "GUI clone without 'from'");
            return nullptr;
        }
        source = FindGuiObject(from, parent);
        if (!source) {
            Warn(doc, "GUI clone source not found: ", from);
            return nullptr;
        }
        CopyCommonValues(defaults, source);
        element->GetBool(kAttrCloneChildren, &cloneChildren, true);
    }

    desc.parent = parent;
    desc.owner = owner;
    if (parent)
        defaults.render3d = parent->HasFlag(ApeGuiObject::kFlag_Render3d);

    float scale[3];
    const float defaultScale[3] = { defaults.scale.x, defaults.scale.y, defaults.scale.z };
    element->GetVec3(kAttrScale, scale, defaultScale);
    desc.scale = { scale[0], scale[1], scale[2], 0.0f };
    element->GetColor(kAttrColor, &desc.color, &defaults.color);
    element->GetBool(kAttrVisible, &desc.visible, defaults.visible);
    element->GetBool(kAttrFlag30, &desc.flag30, defaults.flag30);
    element->GetBool(kAttrExpand, &desc.expand, defaults.expand);
    element->GetBool(kAttrInheritScale, &desc.inheritScale, defaults.inheritScale);
    element->GetBool(kAttrInheritRgb, &desc.inheritRgb, defaults.inheritRgb);
    element->GetBool(kAttrInheritAlpha, &desc.inheritAlpha, defaults.inheritAlpha);
    element->GetBool(kAttrRender3d, &desc.render3d, defaults.render3d);

    desc.transform = defaults.transform;
    float position[3] = { desc.transform.translation.x, desc.transform.translation.y,
                          desc.transform.translation.z };
    element->GetVec3(kAttrPosition, position, position);
    desc.transform.translation = { position[0], position[1], position[2], 0.0f };
    float degrees = 0.0f;
    if (element->GetFloat(kAttrRotation, &degrees, 0.0f) == 0) {
        // A rotation about Z replaces the axes; the position stays.
        Matrix34 rotation{};
        MatrixAxisAngle(degrees * kDegToRad, rotation, kZAxis);
        rotation.translation = desc.transform.translation;
        desc.transform = rotation;
    }

    desc.blendMode = defaults.blendMode;
    if (const char* blend = element->GetString("blend")) {
        uint32_t mode = 0;
        while (mode < 3 && StrNCaseCompare(blend, kBlendNames[mode], 2) != 0)
            ++mode;
        if (mode < 3)
            desc.blendMode = mode;
        else
            Warn(doc, "unknown GUI blend mode ", blend);
    }

    desc.anchor = defaults.anchor;
    desc.align = defaults.align;
    ParseAlignment(element->GetString("anchor"), desc.anchor);
    ParseAlignment(element->GetString("align"), desc.align);

    ApeGuiObject* object = nullptr;
    bool known = true;
    switch (elementType) {
    case kElemFrame: object = CreateGuiFrame(doc, element, desc, source); break;
    case kElemImage: object = CreateGuiImage(doc, element, desc, source); break;
    case kElemGradient: object = CreateGuiGradient(doc, element, desc, source); break;
    case kElemLabel: object = CreateGuiLabel(doc, element, desc, source); break;
    case kElemTicker: object = CreateGuiTicker(doc, element, desc, source); break;
    case kElemTile: object = CreateGuiTile(doc, element, desc, source); break;
    case kElemGroup: object = CreateGuiGroup(doc, element, desc, source); break;
    case kElemType8: object = CreateGuiType8(doc, element, desc, source); break;
    case kElemClipRegion: object = CreateGuiClipRegion(doc, element, desc, source); break;
    case kElemClone:
        switch (source->GetType()) {
        case kGuiObject_Type0: object = CreateGuiFrame(doc, element, desc, source); break;
        case kGuiObject_Type1: object = CreateGuiImage(doc, element, desc, source); break;
        case kGuiObject_Type2: object = CreateGuiGradient(doc, element, desc, source); break;
        case kGuiObject_Text: object = CreateGuiLabel(doc, element, desc, source); break;
        case kGuiObject_Type5: object = CreateGuiTicker(doc, element, desc, source); break;
        case kGuiObject_Type6: object = CreateGuiTile(doc, element, desc, source); break;
        case kGuiObject_Type7: object = CreateGuiGroup(doc, element, desc, source); break;
        case kGuiObject_Type8: object = CreateGuiType8(doc, element, desc, source); break;
        case kGuiObject_ClipRegion: object = CreateGuiClipRegion(doc, element, desc, source); break;
        default:
            if (s_cloneFactory)
                object = s_cloneFactory(doc, element, desc, source);
            known = object != nullptr;
            break;
        }
        break;
    default:
        if (s_elementFactory)
            object = s_elementFactory(doc, element, desc, source);
        known = object != nullptr;
        break;
    }
    if (!known)
        Warn(doc, "unknown GUI element ", name);
    if (!object)
        return nullptr;

    if (source && cloneChildren)
        source->CloneChildren(object, owner);
    for (xml::XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement())
        CreateGuiObject(doc, child, object, owner);
    return object;
}

} // namespace ape

namespace ape {

// @func 0x004D9DA0 CreateGuiTile
ApeGuiObject* CreateGuiTile(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                            ApeGuiObject* source)
{
    // Attribute names of the nine textures (0x00F668A8).
    static const char* const kTextureAttributes[9] = {
        "textureTL", "textureTC", "textureTR", "textureML", "textureMC",
        "textureMR", "textureBL", "textureBC", "textureBR",
    };
    GuiTileDesc desc = MakeDesc<GuiTileDesc>(base);
    float defaultSize[2] = { 0.0f, 0.0f };
    ApeGuiTexture* defaultTextures[9] = {};
    if (CheckSourceType(doc, source, kGuiObject_Type6)) {
        const auto* tile = static_cast<const ApeGuiTile*>(source);
        for (int i = 0; i < 9; ++i)
            defaultTextures[i] = tile->textures[i];
        source->GetSize(defaultSize);
    }
    for (int i = 0; i < 9; ++i) {
        desc.textures[i] = defaultTextures[i];
        if (const char* textureName = element->GetString(kTextureAttributes[i])) {
            desc.textures[i] =
                static_cast<ApeGuiTexture*>(ApeGuiResource::FindByName(HashNameNoCase(textureName)));
            if (!desc.textures[i])
                Warn(doc, "unknown GUI texture ", textureName);
        }
    }
    element->GetVec2(kAttrSize, desc.size, defaultSize);
    return new ApeGuiTile(desc);
}

// @func 0x004D8200 CreateGuiType8
ApeGuiObject* CreateGuiType8(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                             ApeGuiObject* source)
{
    GuiType8Desc desc = MakeDesc<GuiType8Desc>(base);
    // NOTE: a clone source is only checked; its instance is not used.
    CheckSourceType(doc, source, kGuiObject_Type8);
    if (const char* objName = element->GetString("objName"))
        desc.objName = objName;
    element->GetBool(kAttrNoClone, &desc.noClone, false);
    return new ApeGuiType8(desc);
}

} // namespace ape
