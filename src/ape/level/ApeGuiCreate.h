// openTIT - Creating GUI objects from GUI file elements.
#pragma once

#include "ape/level/ApeGui.h"

namespace ape {

// Creates one object of a GUI file element. 'base' holds the common
// attributes already read; 'source' is the object a "clone" element copies
// (null otherwise) and provides the defaults of the class attributes.
using GuiObjectFactory = ApeGuiObject* (*)(xml::XmlDocument* doc, xml::XmlElement* element,
                                           const GuiObjectDesc& base, ApeGuiObject* source);

ApeGuiObject* CreateGuiFrame(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                             ApeGuiObject* source);
ApeGuiObject* CreateGuiImage(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                             ApeGuiObject* source);
ApeGuiObject* CreateGuiGradient(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                                ApeGuiObject* source);
ApeGuiObject* CreateGuiLabel(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                             ApeGuiObject* source);
ApeGuiObject* CreateGuiTicker(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                              ApeGuiObject* source);
ApeGuiObject* CreateGuiTile(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                            ApeGuiObject* source);
ApeGuiObject* CreateGuiGroup(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                             ApeGuiObject* source);
ApeGuiObject* CreateGuiType8(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                             ApeGuiObject* source);
ApeGuiObject* CreateGuiClipRegion(xml::XmlDocument* doc, xml::XmlElement* element, const GuiObjectDesc& base,
                                  ApeGuiObject* source);

// Creates the object of 'element' under 'parent' (a root object when null)
// and, recursively, the objects of its child elements.
ApeGuiObject* CreateGuiObject(xml::XmlDocument* doc, xml::XmlElement* element, ApeGuiObject* parent, void* owner);

// Hooks for elements the engine does not know and for clones of objects of
// game classes (0x00F66894 / 0x00F66898).
void SetGuiObjectFactories(GuiObjectFactory elementFactory, GuiObjectFactory cloneFactory);

} // namespace ape
