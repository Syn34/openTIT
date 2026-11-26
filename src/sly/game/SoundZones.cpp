// openTIT - Sound zones and the zone / mix state registries.
#include "sly/game/SoundZones.h"

#include "ape/core/ApeString.h"
#include "ape/xml/ApeXml.h"

#include <cmath>
#include <map>

namespace sly {
namespace audio {

namespace {
SoundZone* s_activeZones = nullptr;   // 0x00F65B64
bool s_zonesDirty = false;            // 0x00F65B68
std::map<uint32_t, SoundZone*> s_zones;      // 0x010AFEC8
std::map<uint32_t, MixState*> s_mixStates;   // 0x010AFEE0
} // namespace

// @func 0x00441C68 ActivateSoundZone
void ActivateSoundZone(SoundZone* zone)
{
    if (zone->active)
        return;
    if (s_activeZones)
        s_activeZones->newer = zone;
    zone->older = s_activeZones;
    zone->newer = nullptr;
    s_activeZones = zone;
    zone->active = true;
    s_zonesDirty = true;
}

// @func 0x00441CB8 DeactivateSoundZone
void DeactivateSoundZone(SoundZone* zone)
{
    if (!zone->active)
        return;
    if (zone->older)
        zone->older->newer = zone->newer;
    if (zone->newer)
        zone->newer->older = zone->older;
    else
        s_activeZones = zone->older;   // the zone was the head
    zone->active = false;
    s_zonesDirty = true;
}

// @func 0x00441D18 SetSoundZoneWeight
void SetSoundZoneWeight(SoundZone* zone, float weight)
{
    if (std::fabs(weight - zone->weight) < 1e-5f)
        return;
    zone->weight = weight;
    s_zonesDirty = true;
}

SoundZone* ActiveSoundZones()
{
    return s_activeZones;
}

bool SoundZonesDirty()
{
    return s_zonesDirty;
}

void ClearSoundZonesDirty()
{
    s_zonesDirty = false;
}

void RegisterSoundZone(uint32_t nameHash, SoundZone* zone)
{
    s_zones[nameHash] = zone;
}

void RegisterMixState(uint32_t nameHash, MixState* state)
{
    s_mixStates[nameHash] = state;
}

namespace {
constexpr uint32_t kXlsWorkbook = 0x30C0017Au;     // "Workbook"
constexpr uint32_t kXlsWorksheet = 0xE82C68CEu;    // "Worksheet"
constexpr uint32_t kXlsWorksheetNs = 0x058E9C34u;  // "ss:Worksheet"
constexpr uint32_t kXlsTable = 0x04BE43AEu;        // "Table"
constexpr uint32_t kXlsRow = 0x00013DBAu;          // "Row"
constexpr uint32_t kXlsCell = 0x001F8162u;         // "Cell"
constexpr uint32_t kXlsData = 0x001FE7AAu;         // "Data"
constexpr int kZoneColumns = 12;

// Spreadsheet columns of a zone row.
enum ZoneColumn {
    kCol_Name = 0,
    kCol_MixState = 5,
    kCol_Flag3C = 6,
    kCol_Radius = 7,
    kCol_FadeCurve = 8,
    kCol_FadeDistance = 9,
    kCol_Field08 = 10,
    kCol_Field0C = 11,
};

} // namespace

// @func 0x004480D0 LoadSoundZones
// Every worksheet's table holds one zone per row after the title row: the
// zone name, the mix state it applies and its shape numbers. Cells may skip
// columns ("ss:Index", 1-based). Rows whose name starts with "comment" and
// names already loaded are skipped.
void LoadSoundZones(const char* path)
{
    ape::xml::XmlDocument document(path);
    if (!document.LoadFile(0))
        return;
    ape::xml::XmlElement* workbook = document.FirstChildElement(kXlsWorkbook);
    if (!workbook)
        return;
    ape::xml::XmlElement* sheet = workbook->FirstChildElement(kXlsWorksheet);
    if (!sheet)
        sheet = workbook->FirstChildElement(kXlsWorksheetNs);
    // NOTE: like the console, cells a row leaves out keep the previous
    // row's text (only the name column is cleared per row).
    const char* columns[kZoneColumns] = {};
    for (; sheet; ) {
        ape::xml::XmlElement* table = sheet->FirstChildElement(kXlsTable);
        if (!table)
            return;
        ape::xml::XmlElement* row = table->FirstChildElement(kXlsRow);
        while (row) {
            row = row->NextSiblingElement(kXlsRow);   // the first row holds the titles
            if (!row)
                break;
            bool hasData = false;
            columns[kCol_Name] = nullptr;
            ape::xml::XmlElement* cell = row->FirstChildElement(kXlsCell);
            for (int column = 0; column < kZoneColumns; ++column) {
                if (!cell)
                    continue;
                int32_t index = 0;
                if (cell->GetInt("ss:Index", &index, 0) == ape::xml::kQuery_Ok)
                    column = index - 1;
                ape::xml::XmlElement* data = cell->FirstChildElement(kXlsData);
                cell = cell->NextSiblingElement(kXlsCell);
                if (data && column >= 0 && column < kZoneColumns) {
                    hasData = true;
                    columns[column] = data->GetText();
                }
            }
            const char* name = columns[kCol_Name];
            if (!hasData || !name)
                continue;
            const uint32_t nameHash = ape::HashNameNoCase(name);
            if (FindSoundZone(nameHash) || ape::StrNCmp(name, "comment", 7) == 0)
                continue;
            auto* zone = new SoundZone();
            ape::StrNCopy(zone->name, name, sizeof(zone->name));
            zone->name[sizeof(zone->name) - 1] = '\0';
            ape::StrNCopy(zone->mixName, columns[kCol_MixState], sizeof(zone->mixName));
            zone->mixName[sizeof(zone->mixName) - 1] = '\0';
            zone->mixHash = ape::HashNameNoCase(columns[kCol_MixState]);
            if (columns[kCol_Flag3C])
                zone->field_3C = ape::StrToUInt(columns[kCol_Flag3C]) != 0;
            if (columns[kCol_Radius])
                zone->radius = ape::StrToFloat(columns[kCol_Radius]);
            if (columns[kCol_FadeCurve])
                zone->fadeCurve = ape::StrToFloat(columns[kCol_FadeCurve]);
            if (columns[kCol_FadeDistance])
                zone->fadeDistance = ape::StrToFloat(columns[kCol_FadeDistance]);
            if (columns[kCol_Field08])
                zone->field_08 = ape::StrToFloat(columns[kCol_Field08]);
            if (columns[kCol_Field0C])
                zone->field_0C = ape::StrToFloat(columns[kCol_Field0C]);
            RegisterSoundZone(nameHash, zone);
        }
        ape::xml::XmlElement* next = sheet->NextSiblingElement(kXlsWorksheet);
        sheet = next ? next : sheet->NextSiblingElement(kXlsWorksheetNs);
    }
}

// @func 0x0044C404 FindSoundZone
SoundZone* FindSoundZone(uint32_t nameHash)
{
    auto it = s_zones.find(nameHash);
    return it == s_zones.end() ? nullptr : it->second;
}

// @func 0x0044C43C FindMixState
MixState* FindMixState(uint32_t nameHash)
{
    auto it = s_mixStates.find(nameHash);
    return it == s_mixStates.end() ? nullptr : it->second;
}

} // namespace audio
} // namespace sly
