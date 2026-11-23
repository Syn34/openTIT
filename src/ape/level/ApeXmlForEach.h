// openTIT - XML iteration helper shared by the data loaders.
#pragma once

#include "ape/xml/ApeXml.h"

namespace ape {

using XmlElementCallback = bool (*)(xml::XmlElement* element, void* userData);

// Calls 'callback' for every child of 'parent' named 'name'. Returns false if
// any call failed; unless 'continueOnError' is set the walk stops at the
// first failure.
bool ForEachNamedChild(xml::XmlElement* parent, const char* name, XmlElementCallback callback,
                       void* userData, bool continueOnError);

} // namespace ape
