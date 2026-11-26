// openTIT - The <Animation> block of the entity files: anim trees, blend
// maps, bone weight lists ("ChannelMap") and anim maps (0x00288728,
// 0x00347F20-0x003489F4).
#include "sly/anim/AnimController.h"
#include "sly/anim/AnimLayer.h"

#include "ape/xml/ApeXml.h"
#include "sly/entity/EntityHooks.h"

namespace sly {
namespace anim {

using ape::xml::XmlElement;

namespace {

constexpr uint32_t kAttr_Name = 0x0024728Bu;          // "name"
constexpr uint32_t kAttr_Alpha = 0x03B7B93Eu;         // "Alpha"
constexpr uint32_t kAttr_ChannelMap = 0xA9D45179u;    // "ChannelMap"
constexpr uint32_t kAttr_HoldLastFrame = 0xEDD6EBDFu; // NOTE: name unknown
constexpr uint32_t kAttr_From = 0x00210FAAu;          // "from"
constexpr uint32_t kAttr_To = 0x00000A7Bu;            // "to"
constexpr uint32_t kAttr_Blend = 0x03C5A831u;         // "blend"
constexpr uint32_t kAttr_BlendType = 0xDB8BAEEBu;     // "BlendType"
constexpr uint32_t kAttr_Bone = 0x001F32C4u;          // "bone"
constexpr uint32_t kAttr_RotAlpha = 0x2C3B75E7u;      // "RotAlpha"
constexpr uint32_t kAttr_TransAlpha = 0xB5B113B6u;    // "TransAlpha"
constexpr uint32_t kAttr_Children = 0xF5897C08u;      // NOTE: name unknown

constexpr uint32_t kElem_Tree = 0x00276B9Eu;          // "tree"
constexpr uint32_t kElem_Map = 0x0001293Cu;           // "map"
constexpr uint32_t kElem_BlendMap = 0xF690C76Bu;      // NOTE: name unknown
constexpr uint32_t kElem_ChannelMap = 0xA9D45179u;    // "ChannelMap"
constexpr uint32_t kElem_Transition = 0xB6255955u;    // "transition"
constexpr uint32_t kElem_Channel = 0x56D708E3u;       // "channel"

// Blend curve names, in AnimBlendTransition::Curve order (0x00ADBF44).
constexpr uint32_t kCurveNames[3] = {0x920469AEu /* "Smooth" */, 0x85D98325u /* "Linear" */,
                                     0x0026E201u /* "Slow" */};

uint32_t CountChildren(XmlElement* element)
{
    uint32_t count = 0;
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement())
        ++count;
    return count;
}

// The element name hash a compiled element carries (0 for others).
uint32_t ElementKind(XmlElement* element)
{
    return element->GetNameHash();
}

} // namespace

// @func 0x00347F20 ParseAnimTree
AnimTree* ParseAnimTree(XmlElement* root, XmlElement* element)
{
    (void)root;   // only named in compiled-out warnings
    if (!element->GetString("name"))
        return nullptr;
    uint32_t name = 0;
    element->GetHash(kAttr_Name, &name, 0);
    const uint32_t count = CountChildren(element);
    if (count == 0)
        return nullptr;
    auto* tree = new AnimTree;
    tree->count = count;
    tree->layers = new AnimTreeLayer[count];
    AnimTrees().Add(name, tree);
    AnimTreeLayer* layer = tree->layers;
    for (XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement(), ++layer) {
        AnimTreeLayer entry;
        entry.kind = ElementKind(child);
        child->GetHash(kAttr_Name, &entry.anim, 0);
        child->GetFloat(kAttr_Alpha, &entry.alpha, 0.0f);
        if (entry.kind == AnimTreeLayer::kKind_Blend) {
            child->GetHash(kAttr_ChannelMap, &entry.weightList, 0);
            child->GetBool(kAttr_HoldLastFrame, &entry.holdLastFrame, false);
        }
        *layer = entry;
    }
    return tree;
}

// @func 0x003482F8 ParseAnimBlendMap
AnimBlendMap* ParseAnimBlendMap(XmlElement* root, XmlElement* element)
{
    (void)root;
    if (!element->GetString("name"))
        return nullptr;
    uint32_t name = 0;
    element->GetHash(kAttr_Name, &name, 0);
    const uint32_t count = CountChildren(element);
    if (count == 0)
        return nullptr;
    auto* map = new AnimBlendMap;
    map->count = count;
    map->transitions = new AnimBlendTransition[count];
    AnimBlendMaps().Add(name, map);
    AnimBlendTransition* entry = map->transitions;
    for (XmlElement* child = element->FirstChildElement(kElem_Transition); child;
         child = child->NextSiblingElement(kElem_Transition), ++entry) {
        AnimBlendTransition transition;
        child->GetHash(kAttr_From, &transition.from, 0);
        child->GetHash(kAttr_To, &transition.to, 0);
        child->GetFloat(kAttr_Blend, &transition.blend, 0.0f);
        uint32_t curveName = 0;
        child->GetHash(kAttr_BlendType, &curveName, 0);
        for (uint32_t i = 0; i < 3; ++i) {
            if (kCurveNames[i] == curveName) {
                transition.curve = i;
                break;
            }
        }
        *entry = transition;
    }
    return map;
}

// @func 0x00348698 ParseBoneWeightList
BoneWeightList* ParseBoneWeightList(XmlElement* root, XmlElement* element)
{
    (void)root;
    if (!element->GetString("name"))
        return nullptr;
    uint32_t name = 0;
    element->GetHash(kAttr_Name, &name, 0);
    const uint32_t count = CountChildren(element);
    if (count == 0)
        return nullptr;
    auto* list = new BoneWeightList;
    list->count = count;
    list->entries = new BoneWeight[count];
    BoneWeightLists().Add(name, list);
    BoneWeight* entry = list->entries;
    for (XmlElement* child = element->FirstChildElement(kElem_Channel); child;
         child = child->NextSiblingElement(kElem_Channel), ++entry) {
        BoneWeight weight;
        child->GetHash(kAttr_Bone, &weight.bone, 0);
        child->GetFloat(kAttr_RotAlpha, &weight.weight, 0.0f);
        child->GetFloat(kAttr_TransAlpha, &weight.minWeight, 0.0f);
        child->GetBool(kAttr_Children, &weight.children, false);
        *entry = weight;
    }
    return list;
}

// TODO(0x00287FAC ParseAnimMap): the anim map (registry 0x0105AE30) of
// "set" groups with their slots and clip names.
static void ParseAnimMap(XmlElement* root, XmlElement* element, uint8_t scope)
{
    (void)root; (void)element; (void)scope;
}

} // namespace anim

namespace entity {

// @func 0x00288728 LoadAnimationDefs
void LoadAnimationDefs(ape::xml::XmlElement* root, ape::xml::XmlElement* elem, uint8_t scope)
{
    using namespace anim;
    for (XmlElement* e = elem->FirstChildElement(kElem_Tree); e; e = e->NextSiblingElement(kElem_Tree))
        ParseAnimTree(root, e);
    for (XmlElement* e = elem->FirstChildElement(kElem_Map); e; e = e->NextSiblingElement(kElem_Map))
        ParseAnimMap(root, e, scope);
    for (XmlElement* e = elem->FirstChildElement(kElem_BlendMap); e; e = e->NextSiblingElement(kElem_BlendMap))
        ParseAnimBlendMap(root, e);
    for (XmlElement* e = elem->FirstChildElement(kElem_ChannelMap); e; e = e->NextSiblingElement(kElem_ChannelMap))
        ParseBoneWeightList(root, e);
}

} // namespace entity
} // namespace sly
