// openTIT - Entity files: the "Entities" XML documents a level loads its
// definitions, placements, component templates and game data tables from.
#include "sly/entity/Entity.h"
#include "sly/entity/EntityHooks.h"

#include "ape/core/ApeString.h"
#include "ape/file/ApeFile.h"
#include "ape/xml/ApeXml.h"
#include "sly/components/ComponentDescs2.h"
#include "sly/components/IKSolverDefs.h"
#include "sly/components/InventoryDescs.h"
#include "sly/objects/EntityPlacements.h"

namespace sly {
namespace entity {

namespace {

// Child elements of the "Entities" root (HashNameNoCase).
constexpr uint32_t kElem_Entities   = 0xB15A37C1u;   // "Entities"
constexpr uint32_t kElem_Hud        = 0x000118D7u;   // "hud"
constexpr uint32_t kElem_IK         = 0x00000922u;   // "IK"
constexpr uint32_t kElem_Animation  = 0xE8C32B64u;   // "Animation"
constexpr uint32_t kElem_E452FC60   = 0xE452FC60u;
constexpr uint32_t kElem_Instances  = 0xA7D40E5Eu;   // "Instances": placements
constexpr uint32_t kElem_Include    = 0x9E94E088u;   // "Include": another entity file
constexpr uint32_t kElem_Combat     = 0x76D1F2F4u;   // "Combat"
constexpr uint32_t kElem_Inventory  = 0x2DA8293Cu;   // NOTE: inventory groups (from the loader)
constexpr uint32_t kElem_1F7B9E20   = 0x1F7B9E20u;
constexpr uint32_t kElem_Shooting   = 0x1A119B63u;   // "Shooting"
constexpr uint32_t kElem_Spawn      = 0x04B6FB5Bu;   // "Spawn"
constexpr uint32_t kElem_Components = 0x023E6F16u;   // NOTE: component templates (from the loader)
constexpr uint32_t kElem_Defs       = 0x001FF50Eu;   // NOTE: entity definitions (from the loader)

// Load scopes handed out to files loaded with kLoadScopeNew (0x00F64730;
// reset to 1 by the entity module's level start 0x000CE0A8).
uint8_t g_nextLoadScope = 1;

} // namespace

void ResetEntityLoadScopes() { g_nextLoadScope = 1; }

// @func 0x000D5F58 LoadEntityDocument
uint8_t LoadEntityDocument(ape::xml::XmlElement* root, const char* path, uint8_t scope)
{
    ape::xml::XmlElement* entities = root->FirstChildElement(kElem_Entities);
    if (!entities)
        return scope;
    if (scope == kLoadScopeNew)
        scope = g_nextLoadScope++;

    for (ape::xml::XmlElement* elem = entities->FirstChildElement(); elem; elem = elem->NextSiblingElement()) {
        switch (elem->GetNameHash()) {
        case kElem_Hud:
            LoadHudDefs(root, elem);
            break;
        case kElem_IK:
            components::ParseIKSolverDefs(root, elem, scope);
            break;
        case kElem_Animation:
            LoadAnimationDefs(root, elem, scope);
            break;
        case kElem_E452FC60:
            LoadDefsE452FC60(root, elem, scope);
            break;
        case kElem_Instances:
            for (ape::xml::XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
                objects::LoadEntityPlacement(root, child);
            break;
        case kElem_Include: {
            // The file name is relative to the folder of this file.
            const char* fileName = elem->GetString("fileName");
            if (!fileName)
                break;
            char fullPath[0x80];
            char* tail = fullPath;
            const int length = ape::StrLen(path);
            const char* slash = path + length - 1;
            while (slash > path && *slash != '\\' && *slash != '/')
                --slash;
            if (slash >= path) {
                const size_t folder = size_t(slash + 1 - path);
                ape::StrNCopy(fullPath, path, folder);
                tail = fullPath + folder;
            }
            ape::StrCopy(tail, fileName);
            LoadEntityFile(fullPath, scope);
            break;
        }
        case kElem_Combat:
            LoadCombatDefs(root, elem, scope);
            break;
        case kElem_Inventory:
            components::ParseInventoryGroupFile(root, elem, scope != 0);
            break;
        case kElem_1F7B9E20:
            LoadDefs1F7B9E20(root, elem, scope);
            break;
        case kElem_Shooting:
            LoadShootingDefs(root, elem, scope);
            break;
        case kElem_Spawn:
            LoadSpawnDefs(root, elem);
            break;
        case kElem_Components:
            for (ape::xml::XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
                components::ParseComponentDesc(root, child, scope);
            break;
        case kElem_Defs:
            for (ape::xml::XmlElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
                LoadEntityDef(root, child, scope);
            break;
        default:
            break;
        }
    }
    return scope;
}

// @func 0x000D6398 LoadEntityFile
uint8_t LoadEntityFile(const char* path, uint8_t scope)
{
    ape::xml::XmlDocument document(path);
    if (!document.LoadFile(0))
        return scope;
    return LoadEntityDocument(&document, path, scope);
}

// @func 0x000D643C LoadEntityFileFromStream
uint8_t LoadEntityFileFromStream(ape::file::File& file, uint8_t scope)
{
    ape::xml::XmlDocument document;
    if (!document.LoadFromStream(file, nullptr))
        return scope;
    return LoadEntityDocument(&document, file.GetName(), scope);
}

} // namespace entity
} // namespace sly
