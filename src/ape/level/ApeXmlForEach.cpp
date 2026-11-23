// openTIT - XML iteration helper shared by the data loaders.
#include "ape/level/ApeXmlForEach.h"

namespace ape {

// @func 0x004F4494 ForEachNamedChild
bool ForEachNamedChild(xml::XmlElement* parent, const char* name, XmlElementCallback callback,
                       void* userData, bool continueOnError)
{
    bool ok = true;
    for (xml::XmlElement* child = parent->FirstChildElement(name); child;
         child = child->NextSiblingElement(name)) {
        if (!ok && !continueOnError)
            return false;
        if (!callback(child, userData))
            ok = false;
    }
    return ok;
}

} // namespace ape
