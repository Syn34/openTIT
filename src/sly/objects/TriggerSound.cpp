// openTIT - Polled trigger volumes and the ambient sound / sound zone triggers.
#include "sly/objects/TriggerSound.h"

#include "sly/objects/TriggerTuning.h"
#include "sly/objects/TriggerTypes.h"

#include "sly/game/AudioSystem.h"

#include "ape/xml/ApeXml.h"

#include <cmath>

namespace sly {
namespace objects {

namespace {
constexpr float kFadeEpsilon = 1e-5f;
// Collision filter given to polled volumes so that physics ignores them.
constexpr uint32_t kPolledVolumeFilter = 0x1A;
} // namespace

bool IsAudioDisabled()
{
    return false;   // NOTE: console flag 0x010B09E4, set when the audio start-up fails
}

// ===========================================================================
// PolledTrigger
// ===========================================================================

PolledTrigger::PolledTrigger(const TriggerInstanceDesc& desc)
    : TriggerBase(desc, 0x20, true)
{
}

// @func 0x0020FA68 PolledTrigger::RebuildVolume
void PolledTrigger::RebuildVolume()
{
}

// TODO(0x0020FD30 PolledTrigger::IsInside) the box branch is vector code that the
// disassembler could not decode; it is reconstructed here as a local-space test.
bool PolledTrigger::IsInside(const ape::Vec3& point) const
{
    const ape::Vec3& p = Position();
    float dx = point.x - p.x, dy = point.y - p.y, dz = point.z - p.z;
    if (m_shapeKind == kVolumeShape_Sphere)
        return dx * dx + dy * dy + dz * dz < m_radiusSq;
    if (m_shapeKind == kVolumeShape_Box) {
        const ape::Matrix34& m = transform;
        float lx = dx * m.xAxis.x + dy * m.xAxis.y + dz * m.xAxis.z;
        float ly = dx * m.yAxis.x + dy * m.yAxis.y + dz * m.yAxis.z;
        float lz = dx * m.zAxis.x + dy * m.zAxis.y + dz * m.zAxis.z;
        return lx >= m_boxMin.x && lx <= m_boxMax.x && ly >= m_boxMin.y && ly <= m_boxMax.y &&
               lz >= m_boxMin.z && lz <= m_boxMax.z;
    }
    return false;
}

// @func 0x0020FA6C PolledTrigger::SetEnabled
void PolledTrigger::SetEnabled(bool enabled)
{
    // Disabling a polled volume while the listener is inside reports the leave first.
    if (IsPolled() && !enabled && m_inside) {
        m_inside = false;
        OnExit(nullptr);
    }
    ChangeEnabled(enabled);
}

void PolledTrigger::PollListener()
{
    bool inside = false;
    if (m_enabled && IsInside(GetListenerPosition())) {
        inside = true;
        if (!m_inside)
            OnEnter(nullptr);
    } else if (m_inside) {
        OnExit(nullptr);
    }
    m_inside = inside;
}

// @func 0x0020FB9C PolledTrigger::Update
void PolledTrigger::Update(float dt)
{
    if (!IsPolled()) {
        TriggerBase::Update(dt);
        return;
    }
    PollListener();
    ObjEntity::Update(dt);
}

// @func 0x0020FF0C PolledTrigger::OnEnter
void PolledTrigger::OnEnter(ObjEntity* entity)
{
    if (IsPolled())
        return;
    TriggerBase::OnEnter(entity);
}

// @func 0x0020FF60 PolledTrigger::OnExit
void PolledTrigger::OnExit(ObjEntity* entity)
{
    if (IsPolled())
        return;
    TriggerBase::OnExit(entity);
}

// @func 0x0020FFC0 PolledTrigger::SetupShape
void PolledTrigger::SetupShape()
{
    if (!m_volume)
        return;
    m_shapeKind = kVolumeShape_None;
    bool fromShape = !m_aabbVolume;
    VolumeShapeKind kind = fromShape ? VolumeGetShapeKind(m_volume) : kVolumeShape_Box;
    if (fromShape && kind != kVolumeShape_Box) {
        m_shapeKind = kind;
        if (kind == kVolumeShape_Sphere) {
            float r = VolumeGetSphereRadius(m_volume);
            m_radiusSq = r * r;
        } else {
            // Other shapes: a bounding sphere of the local bounds (base behaviour).
            ape::Aabb box = GetBounds(false);
            float hx = box.max.x - (box.min.x + box.max.x) * 0.5f;
            float hy = box.max.y - (box.min.y + box.max.y) * 0.5f;
            float hz = box.max.z - (box.min.z + box.max.z) * 0.5f;
            m_radiusSq = hx * hx + hy * hy + hz * hz;
        }
    } else {
        m_shapeKind = kVolumeShape_Box;
        if (fromShape) {
            ape::Vec3 half = VolumeGetBoxHalfExtents(m_volume);
            m_boxMax = half;
            m_boxMin = { -half.x, -half.y, -half.z, 0.0f };
            m_radiusSq = half.x * half.x + half.y * half.y + half.z * half.z;
        } else {
            ape::Aabb box = GetBounds(false);
            m_boxMin = box.min;
            m_boxMax = box.max;
        }
    }
    if (IsPolled())
        VolumeSetCollisionFilter(m_volume, kPolledVolumeFilter);
}

// ===========================================================================
// Ambient sound
// ===========================================================================

AmbientSoundTriggerDef::AmbientSoundTriggerDef(const TriggerDefDesc& desc, uint32_t bankHash,
                                               uint32_t soundHash)
    : TriggerDef(desc), bank(bankHash), sound(soundHash)
{
}

// @func 0x002101D8 AmbientSoundTriggerDef::~AmbientSoundTriggerDef
// @func 0x00210238 AmbientSoundTriggerDef::~AmbientSoundTriggerDef (deleting)
AmbientSoundTriggerDef::~AmbientSoundTriggerDef()
{
    audio::SoundNameRelease(soundName);
    soundName = nullptr;
}

void AmbientSoundTriggerDef::ResolveSound()
{
    if (audio::SoundNameHasDef(soundName))
        return;
    audio::SoundNameRelease(soundName);
    soundName = FindSoundName(sound, bank);
}

// @func 0x002102BC CreateAmbientSoundTriggerDef
EntityDef* CreateAmbientSoundTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                        const EntityDefDesc& loader, const EntityDef* parent)
{
    TriggerDefDesc desc(kDefClass_Sound | kDefClass_Trigger, loader, "TriggerAmbientSound_");
    desc.Parse(root, elem, loader, parent);
    uint32_t soundHash = 0, bankHash = 0;
    elem->GetHash(kAttr_Sound, &soundHash, 0);
    elem->GetHash(kAttr_Bank, &bankHash, 0);
    return new AmbientSoundTriggerDef(desc, bankHash, soundHash);
}

// @func 0x002105F8 AmbientSoundTriggerDef::Create
ObjEntity* AmbientSoundTriggerDef::Create(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->flags & kPlace_ForceCreate) != 0))
        return nullptr;
    if (IsAudioDisabled())
        return nullptr;
    TriggerInstanceDesc desc(GetInstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    AmbientSoundTrigger* trigger = new AmbientSoundTrigger(desc);
    trigger->OnCreated();
    return trigger;
}

// @func 0x00210478 AmbientSoundTrigger::AmbientSoundTrigger
AmbientSoundTrigger::AmbientSoundTrigger(const TriggerInstanceDesc& desc)
    : PolledTrigger(desc)
{
    // A missing sound is reported when script warnings are on (stripped).
    SoundDef()->ResolveSound();
}

// @func 0x00210CA4 AmbientSoundTrigger::OnCreated
void AmbientSoundTrigger::OnCreated()
{
    m_voice = 0;
    SoundDef()->ResolveSound();
    PolledTrigger::OnCreated();
}

// TODO(0x00210D80 AmbientSoundTrigger::ComputeSoundParams) vector code the
// disassembler could not decode; the emitter is placed at the trigger.
void AmbientSoundTrigger::ComputeSoundParams(ape::Vec3& position, float& volume) const
{
    position = Position();
    volume = 1.0f;
}

// @func 0x002112C8 AmbientSoundTrigger::Update
void AmbientSoundTrigger::Update(float dt)
{
    if (!m_enabled)
        return;
    if (audio::SoundNameHasDef(SoundDef()->soundName) && audio::IsVoiceValid(m_voice)) {
        ape::Vec3 position;
        float volume;
        ComputeSoundParams(position, volume);
        SetVoice3D(m_voice, position, volume);
    }
    if (!IsPolled()) {
        TriggerBase::Update(dt);
        return;
    }
    PollListener();
    ObjEntity::Update(dt);
}

// @func 0x002114FC AmbientSoundTrigger::OnEnter
void AmbientSoundTrigger::OnEnter(ObjEntity* entity)
{
    if (!IsPolled() && entity != GetPlayerEntity())
        return;
    audio::StopVoice(m_voice, true);
    if (audio::SoundNameHasDef(SoundDef()->soundName)) {
        ape::Vec3 position;
        float volume;
        ComputeSoundParams(position, volume);
        if (!g_audioOptimizationDisableVolumetricSounds.Get())
            m_voice = PlaySound3D(SoundDef()->soundName, position, volume, m_bus);
    }
    if (!IsPolled())
        TriggerBase::OnEnter(entity);
}

// @func 0x0021167C AmbientSoundTrigger::OnExit
void AmbientSoundTrigger::OnExit(ObjEntity* entity)
{
    audio::StopVoice(m_voice, true);
    if (!IsPolled())
        TriggerBase::OnExit(entity);
}

// @func 0x002116F4 AmbientSoundTrigger::IsDebugDisplayed
bool AmbientSoundTrigger::IsDebugDisplayed() const
{
    return TriggerDebugSwitch(g_debugDisplayTriggerAmbient.Get());
}

// ===========================================================================
// Sound zone
// ===========================================================================

SoundZoneTriggerDef::SoundZoneTriggerDef(const TriggerDefDesc& desc, uint32_t zoneHash,
                                         uint32_t mixHash, float fade)
    : TriggerDef(desc), zoneName(zoneHash), mixStateName(mixHash), mixFade(fade)
{
}

// @func 0x00212DC8 CreateSoundZoneTriggerDef
EntityDef* CreateSoundZoneTriggerDef(ape::xml::XmlElement* root, ape::xml::XmlElement* elem,
                                     const EntityDefDesc& loader, const EntityDef* parent)
{
    TriggerDefDesc desc(0x04000000u | kDefClass_Trigger, loader, "TriggerSoundZone_");
    desc.field_20 = 1;
    desc.Parse(root, elem, loader, parent);
    uint32_t zoneHash = 0, mixHash = 0;
    float fade = 0.0f;
    elem->GetHash(kAttr_SoundZone, &zoneHash, 0);
    elem->GetHash(kAttr_MixState, &mixHash, 0);
    elem->GetFloat(kAttr_MixFade, &fade, 0.0f);
    return new SoundZoneTriggerDef(desc, zoneHash, mixHash, fade);
}

// @func 0x002130D0 SoundZoneTriggerDef::Create
ObjEntity* SoundZoneTriggerDef::Create(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->flags & 0x04000000u) != 0))
        return nullptr;
    if (IsAudioDisabled())
        return nullptr;
    TriggerInstanceDesc desc(GetInstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    SoundZoneTrigger* trigger = new SoundZoneTrigger(desc);
    trigger->OnCreated();
    return trigger;
}

// @func 0x00212FC8 SoundZoneTrigger::SoundZoneTrigger
SoundZoneTrigger::SoundZoneTrigger(const TriggerInstanceDesc& desc)
    : PolledTrigger(desc)
{
}

// @func 0x00214038 SoundZoneTrigger::OnCreated
void SoundZoneTrigger::OnCreated()
{
    if (SoundZoneTriggerDef* zoneDef = ZoneDef()) {
        zoneDef->zone = FindSoundZone(zoneDef->zoneName);
        zoneDef->mixState = FindMixState(zoneDef->mixStateName);
    }
    m_next = nullptr;
    m_prev = nullptr;
    PolledTrigger::OnCreated();
}

void SoundZoneTrigger::LinkActive()
{
    SoundZone* zone = ZoneDef()->zone;
    SoundZoneTrigger* head = static_cast<SoundZoneTrigger*>(SoundZoneActiveTrigger(zone));
    if (!head)
        SoundZoneActivate(zone);
    else
        head->m_prev = this;
    m_prev = nullptr;
    m_next = head;
    SetSoundZoneActiveTrigger(zone, this);
    m_linked = true;
}

void SoundZoneTrigger::UnlinkActive()
{
    SoundZone* zone = ZoneDef()->zone;
    if (!m_prev)
        SetSoundZoneActiveTrigger(zone, m_next);
    else
        m_prev->m_next = m_next;
    if (m_next)
        m_next->m_prev = m_prev;
    if (!SoundZoneActiveTrigger(zone))
        SoundZoneDeactivate(zone);
    m_linked = false;
}

// TODO(0x00213870 SoundZoneTrigger::ComputeFadeWeight) not reimplemented.
float SoundZoneTrigger::ComputeFadeWeight() const
{
    return 1.0f;
}

namespace {
// Cross-fade time of a zone: its own value when set (>= 0), otherwise
// "Audio.ZoneXFadeTime"; no zone means no fade.
float ZoneFadeTime(const SoundZone* zone)
{
    if (!zone)
        return 0.0f;
    float own = SoundZoneFadeDistance(zone);
    return own >= 0.0f ? own : g_audioZoneXFadeTime.Get();
}
} // namespace

// @func 0x002140C0 SoundZoneTrigger::OnEnter
void SoundZoneTrigger::OnEnter(ObjEntity* entity)
{
    if (!IsPolled() && entity != GetPlayerEntity())
        return;
    SoundZoneTriggerDef* zoneDef = ZoneDef();
    if (!m_fades) {
        float fade = ZoneFadeTime(zoneDef ? zoneDef->zone : nullptr);
        m_weight = std::fabs(fade) < kFadeEpsilon ? 1.0f : 0.0f;
    } else {
        m_weight = ComputeFadeWeight();
    }
    if (zoneDef && zoneDef->zone && !m_linked)
        LinkActive();
    if (zoneDef && zoneDef->zone && SoundZoneActiveTrigger(zoneDef->zone) == this)
        SoundZoneSetWeight(zoneDef->zone, m_weight);
    if (zoneDef && zoneDef->mixState)
        MixStatePush(zoneDef->mixState, zoneDef->mixFade);
    if (!IsPolled())
        TriggerBase::OnEnter(entity);
}

// @func 0x002142E0 SoundZoneTrigger::OnExit
void SoundZoneTrigger::OnExit(ObjEntity* entity)
{
    SoundZoneTriggerDef* zoneDef = ZoneDef();
    // Without a fade the zone is dropped at once; fading zones are faded out by Update.
    bool dropNow = m_fades || std::fabs(ZoneFadeTime(zoneDef ? zoneDef->zone : nullptr)) < kFadeEpsilon;
    if (dropNow) {
        m_weight = 0.0f;
        if (zoneDef && zoneDef->zone && SoundZoneActiveTrigger(zoneDef->zone) == this)
            SoundZoneSetWeight(zoneDef->zone, 0.0f);
        if (zoneDef && zoneDef->zone && m_linked)
            UnlinkActive();
    }
    if (zoneDef && zoneDef->mixState)
        MixStatePop(zoneDef->mixState, zoneDef->mixFade);
    if (!IsPolled())
        TriggerBase::OnExit(entity);
}

// @func 0x0021452C SoundZoneTrigger::IsDebugDisplayed
bool SoundZoneTrigger::IsDebugDisplayed() const
{
    return TriggerDebugSwitch(g_debugDisplayTriggerSoundZone.Get());
}

// @func 0x00214570 SoundZoneTrigger::IsInside
bool SoundZoneTrigger::IsInside(const ape::Vec3& point) const
{
    if (IsPolled())
        return PolledTrigger::IsInside(point);
    ape::Aabb box = GetBounds(true);
    return ape::AabbContainsPoint(box, point);
}

} // namespace objects
} // namespace sly
