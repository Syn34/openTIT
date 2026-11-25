// openTIT - GUI files: the resource that loads a GUI XML file.
#include "ape/level/ApeGuiFile.h"

#include "ape/level/ApeGuiCreate.h"
#include "ape/level/ApeGuiScript.h"
#include "ape/world/ApeScene.h"
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

constexpr uint32_t kElemGui = Hash("gui");
constexpr uint32_t kElemAssets = Hash("assets");
constexpr uint32_t kElemTexture = Hash("texture");
// NOTE: element of the bitmap fonts, name unknown.
constexpr uint32_t kElemBitmapFont = 0x447D9F5E;
constexpr uint32_t kElemFontSet = Hash("font");
constexpr uint32_t kElemScripts = Hash("scripts");
constexpr uint32_t kElemScript = Hash("script");
// NOTE: element holding the object tree, name unknown.
constexpr uint32_t kElemObjects = 0xA7D40E5E;
static_assert(kElemAssets == 0x73A36783u, "assets element hash");

} // namespace

// @func 0x004DFA60 ApeGuiFile::Parse
bool ApeGuiFile::Parse(xml::XmlDocument* doc, xml::XmlElement* root)
{
    if (xml::XmlElement* assets = root->FirstChildElement(kElemAssets)) {
        if (const char* sceneName = assets->GetString("name")) {
            scene = world::CreateScene(0);
            if (scene) {
                world::LoadSceneFile(scene, sceneName, false);
                world::FinishSceneLoad(scene);
            }
        }
        for (xml::XmlElement* e = assets->FirstChildElement(kElemTexture); e; e = e->NextSiblingElement(kElemTexture))
            CreateGuiTexture(doc, e, this);
        for (xml::XmlElement* e = assets->FirstChildElement(kElemBitmapFont); e;
             e = e->NextSiblingElement(kElemBitmapFont))
            CreateGuiBitmapFont(doc, e, this);
        for (xml::XmlElement* e = assets->FirstChildElement(kElemFontSet); e; e = e->NextSiblingElement(kElemFontSet))
            CreateGuiFontSet(doc, e, this);
    }
    if (xml::XmlElement* scripts = root->FirstChildElement(kElemScripts)) {
        for (xml::XmlElement* e = scripts->FirstChildElement(kElemScript); e; e = e->NextSiblingElement(kElemScript))
            CreateGuiScript(doc, e, this);
    }
    if (xml::XmlElement* objects = root->FirstChildElement(kElemObjects)) {
        for (xml::XmlElement* e = objects->FirstChildElement(); e; e = e->NextSiblingElement())
            CreateGuiObject(doc, e, nullptr, this);
    }
    return true;
}

// @func 0x004DFD04 ApeGuiFile::Load
uint16_t ApeGuiFile::Load()
{
    uint16_t state = kGuiLoad_Failed;
    xml::XmlDocument doc(m_path);
    if (!doc.LoadFile(0)) {
        plat::Log(plat::LogLevel::Warning, "%s: cannot load GUI file\n", doc.GetDocumentPath());
    } else {
        xml::XmlElement* root = doc.FirstChildElement(kElemGui);
        if (!root || !Parse(&doc, root))
            plat::Log(plat::LogLevel::Warning, "%s: not a GUI file\n", doc.GetDocumentPath());
        else
            state = kGuiLoad_Loaded;
    }
    return state;
}

// @func 0x004DFDDC ApeGuiFile::Unload
// Destroys the scripts, root objects and resources the file created.
void ApeGuiFile::Unload()
{
    DestroyGuiScriptsOf(this);
    for (ApeGuiObject* object = GuiRootObjects(); object;) {
        ApeGuiObject* next = object->nextSibling;
        if (object->owner == this)
            delete object;
        object = next;
    }
    for (ApeGuiResource* resource = ApeGuiResource::First(); resource;) {
        ApeGuiResource* next = resource->GetNext();
        if (resource->GetOwner() == this)
            resource->Release();
        resource = next;
    }
    if (scene) {
        world::DestroyScene(scene);
        scene = nullptr;
    }
}

} // namespace ape
