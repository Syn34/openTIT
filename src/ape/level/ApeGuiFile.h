// openTIT - GUI files: the resource that loads a GUI XML file.
//
// A GUI file (<gui>) has three sections: <assets> (an optional 3D scene
// named by "name", textures, bitmap fonts and font sets), <scripts> and the
// object tree (element hash 0xA7D40E5E). Everything created while loading
// it is owned by the file and destroyed when the file is unloaded.
#pragma once

#include "ape/level/ApeGui.h"

namespace ape {

namespace world {
class ApeScene;
}

// GUI file resource (vtable 0x00B22678; the destructors are the resource
// base ones).
class ApeGuiFile : public ApeGuiResource {
public:
    ApeGuiFile(const char* name, const char* path) : ApeGuiResource(name, path, nullptr) {}
    uint16_t Load() override;
    void Unload() override;

    // Creates the contents of the <gui> element 'root'.
    bool Parse(xml::XmlDocument* doc, xml::XmlElement* root);

    world::ApeScene* scene = nullptr;   // +0x5C the <assets> scene
};

} // namespace ape
