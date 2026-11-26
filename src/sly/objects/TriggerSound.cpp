// openTIT - Polled trigger volumes and the ambient sound / sound zone triggers.
#include "sly/objects/TriggerSound.h"

#include "sly/objects/TriggerTuning.h"
#include "sly/objects/TriggerTypes.h"

#include "sly/game/AudioSystem.h"

#include "ape/math/ApeBounds.h"
#include "ape/xml/ApeXml.h"

#include <algorithm>
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

// @func 0x0075A3AC AmbientSoundTrigger::~AmbientSoundTrigger
// @func 0x0075A4FC AmbientSoundTrigger::~AmbientSoundTrigger (deleting)
AmbientSoundTrigger::~AmbientSoundTrigger()
{
    RebuildVolume();
}

// TODO(0x002108A8 AmbientSoundTrigger::RebuildVolume): the phantom shape of
// the sound volume (vector code) is not ported.
void AmbientSoundTrigger::RebuildVolume()
{
    PolledTrigger::RebuildVolume();
}

// @func 0x0075AF24 SoundZoneTrigger::~SoundZoneTrigger
// @func 0x0075B074 SoundZoneTrigger::~SoundZoneTrigger (deleting)
SoundZoneTrigger::~SoundZoneTrigger()
{
    RebuildVolume();
}

// TODO(0x00213398 SoundZoneTrigger::RebuildVolume): not ported.
void SoundZoneTrigger::RebuildVolume()
{
    PolledTrigger::RebuildVolume();
}

// @func 0x0020FD30 PolledTrigger::IsInside
// Sphere volumes compare the squared distance with the squared radius (strict);
// box volumes bring the point into the trigger's frame (transposed rotation of
// the world transform) and test it against the local box (inclusive).
bool PolledTrigger::IsInside(const ape::Vec3& point) const
{
    const ape::Vec3& p = Position();
    const float dx = point.x - p.x, dy = point.y - p.y, dz = point.z - p.z;
    if (m_shapeKind == kVolumeShape_Sphere)
        return dx * dx + dy * dy + dz * dz < m_radiusSq;
    if (m_shapeKind == kVolumeShape_Box) {
        const ape::Matrix34& m = transform;
        const ape::Vec3 local = { dx * m.xAxis.x + dy * m.xAxis.y + dz * m.xAxis.z,
                                  dx * m.yAxis.x + dy * m.yAxis.y + dz * m.yAxis.z,
                                  dx * m.zAxis.x + dy * m.zAxis.y + dz * m.zAxis.z, 0.0f };
        const ape::Aabb box = { m_boxMin, m_boxMax };
        return ape::AabbContainsPoint(box, local);
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
bool PolledTrigger::Update(float dt)
{
    if (!IsPolled()) {
        return TriggerBase::Update(dt);
    }
    PollListener();
    return Entity::Update(dt);
}

// @func 0x0020FF0C PolledTrigger::OnEnter
void PolledTrigger::OnEnter(Entity* entity)
{
    if (IsPolled())
        return;
    TriggerBase::OnEnter(entity);
}

// @func 0x0020FF60 PolledTrigger::OnExit
void PolledTrigger::OnExit(Entity* entity)
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

// @func 0x002105F8 AmbientSoundTriggerDef::Spawn
Entity* AmbientSoundTriggerDef::Spawn(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->flags & kPlace_ForceCreate) != 0))
        return nullptr;
    if (IsAudioDisabled())
        return nullptr;
    TriggerInstanceDesc desc(InstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    AmbientSoundTrigger* trigger = new AmbientSoundTrigger(desc);
    trigger->OnSpawn();
    return trigger;
}

// @func 0x00210478 AmbientSoundTrigger::AmbientSoundTrigger
AmbientSoundTrigger::AmbientSoundTrigger(const TriggerInstanceDesc& desc)
    : PolledTrigger(desc)
{
    // A missing sound is reported when script warnings are on (stripped).
    SoundDef()->ResolveSound();
}

// @func 0x00210CA4 AmbientSoundTrigger::OnSpawn
void AmbientSoundTrigger::OnSpawn()
{
    m_voice = 0;
    SoundDef()->ResolveSound();
    PolledTrigger::OnSpawn();
}

// @func 0x00210D80 AmbientSoundTrigger::ComputeSoundParams
// Sphere: the sound comes from the centre; outside the inner radius its spread
// narrows to the angle the inner sphere covers. Box: inside the inner box the
// sound is at the listener; outside it comes from the nearest point of the
// inner box and its spread shrinks with the cube of the remaining falloff.
void AmbientSoundTrigger::ComputeSoundParams(ape::Vec3& position, float& volume, float& spread)
{
    volume = 1.0f;
    spread = 360.0f;

    // Listener in the trigger's frame.
    const ape::Matrix34& m = transform;
    const ape::Vec3& origin = Position();
    const float dx = position.x - origin.x, dy = position.y - origin.y, dz = position.z - origin.z;
    float local[3] = { dx * m.xAxis.x + dy * m.xAxis.y + dz * m.xAxis.z,
                       dx * m.yAxis.x + dy * m.yAxis.y + dz * m.yAxis.z,
                       dx * m.zAxis.x + dy * m.zAxis.y + dz * m.zAxis.z };

    if (m_shapeKind == kVolumeShape_Box) {
        const float innerMin[3] = { m_innerMin.x, m_innerMin.y, m_innerMin.z };
        const float innerMax[3] = { m_innerMax.x, m_innerMax.y, m_innerMax.z };
        const ape::Aabb inner = { m_innerMin, m_innerMax };
        const ape::Vec3 localPoint = { local[0], local[1], local[2], 0.0f };
        if (ape::AabbContainsPoint(inner, localPoint)) {
            // Inside: the sound surrounds the listener (position unchanged).
            m_falloff = 0.0f;
        } else {
            const float falloffMax[3] = { m_falloffMax.x, m_falloffMax.y, m_falloffMax.z };
            const float falloffMin[3] = { m_falloffMin.x, m_falloffMin.y, m_falloffMin.z };
            float falloff = 0.0f;
            for (int i = 0; i < 3; ++i) {
                float t = 0.0f;
                if (falloffMax[i] > 0.0f && local[i] > innerMax[i])
                    t = (local[i] - innerMax[i]) / falloffMax[i];
                else if (falloffMin[i] < 0.0f && local[i] < innerMin[i])
                    t = (local[i] - innerMin[i]) / falloffMin[i];
                if (t > falloff)
                    falloff = t;
            }
            for (int i = 0; i < 3; ++i)
                local[i] = std::min(std::max(local[i], innerMin[i]), innerMax[i]);
            position = { origin.x + local[0] * m.xAxis.x + local[1] * m.yAxis.x + local[2] * m.zAxis.x,
                         origin.y + local[0] * m.xAxis.y + local[1] * m.yAxis.y + local[2] * m.zAxis.y,
                         origin.z + local[0] * m.xAxis.z + local[1] * m.yAxis.z + local[2] * m.zAxis.z, 0.0f };
            const float remaining = 1.0f - falloff;
            spread = remaining * remaining * remaining * 360.0f;
            m_falloff = falloff;
        }
    } else if (m_shapeKind == kVolumeShape_Sphere) {
        const float distance = std::sqrt(local[0] * local[0] + local[1] * local[1] + local[2] * local[2]);
        float falloff = 0.0f;
        if (distance > m_innerRadius) {
            spread = std::atan(m_innerRadius / distance) * 4.0f * 57.2957764f;
            falloff = (distance - m_innerRadius) / (m_outerRadius - m_innerRadius);
        }
        position = origin;
        m_falloff = falloff;
    }
    volume = std::min(std::max(volume, 0.0f), 1.0f);
}

// @func 0x002112C8 AmbientSoundTrigger::Update
bool AmbientSoundTrigger::Update(float dt)
{
    if (!m_enabled)
        return true;
    if (audio::SoundNameHasDef(SoundDef()->soundName) && audio::IsVoiceValid(m_voice)) {
        ape::Vec3 position = GetListenerPosition();
        float volume, spread;
        // NOTE: the spread angle is not passed on by the voice hooks yet.
        ComputeSoundParams(position, volume, spread);
        SetVoice3D(m_voice, position, volume);
    }
    if (!IsPolled()) {
        return TriggerBase::Update(dt);
    }
    PollListener();
    return Entity::Update(dt);
}

// @func 0x002114FC AmbientSoundTrigger::OnEnter
void AmbientSoundTrigger::OnEnter(Entity* entity)
{
    if (!IsPolled() && entity != GetPlayerEntity())
        return;
    audio::StopVoice(m_voice, true);
    if (audio::SoundNameHasDef(SoundDef()->soundName)) {
        ape::Vec3 position = GetListenerPosition();
        float volume, spread;
        ComputeSoundParams(position, volume, spread);
        if (!g_audioOptimizationDisableVolumetricSounds.Get())
            m_voice = PlaySound3D(SoundDef()->soundName, position, volume, m_bus);
    }
    if (!IsPolled())
        TriggerBase::OnEnter(entity);
}

// @func 0x0021167C AmbientSoundTrigger::OnExit
void AmbientSoundTrigger::OnExit(Entity* entity)
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

// @func 0x002130D0 SoundZoneTriggerDef::Spawn
Entity* SoundZoneTriggerDef::Spawn(EntityPlacement* placement)
{
    if (TriggerPlacementSkips(placement, (placement->flags & 0x04000000u) != 0))
        return nullptr;
    if (IsAudioDisabled())
        return nullptr;
    TriggerInstanceDesc desc(InstanceName(placement), this);
    placement->FillInstanceDesc(desc);
    SoundZoneTrigger* trigger = new SoundZoneTrigger(desc);
    trigger->OnSpawn();
    return trigger;
}

// @func 0x00212FC8 SoundZoneTrigger::SoundZoneTrigger
SoundZoneTrigger::SoundZoneTrigger(const TriggerInstanceDesc& desc)
    : PolledTrigger(desc)
{
}

// @func 0x00214038 SoundZoneTrigger::OnSpawn
void SoundZoneTrigger::OnSpawn()
{
    if (SoundZoneTriggerDef* zoneDef = ZoneDef()) {
        zoneDef->zone = FindSoundZone(zoneDef->zoneName);
        zoneDef->mixState = FindMixState(zoneDef->mixStateName);
    }
    m_next = nullptr;
    m_prev = nullptr;
    PolledTrigger::OnSpawn();
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

// @func 0x00213870 SoundZoneTrigger::ComputeFadeWeight
// 1 inside the inner volume, falling to 0 at the outer edge as
// 1 - f^curve, f being how far into the fade region the listener is (the
// largest axis fraction for boxes).
// NOTE: the console evaluates the power with the bit-pattern approximation
// (exponent arithmetic on the double's bits); std::pow is used here.
float SoundZoneTrigger::ComputeFadeWeight(const ape::Vec3& listener) const
{
    const SoundZoneTriggerDef* zoneDef = ZoneDef();
    const float curve = zoneDef && zoneDef->zone ? SoundZoneFadeCurve(zoneDef->zone) : 1.0f;

    const ape::Matrix34& m = transform;
    const ape::Vec3& origin = Position();
    const float dx = listener.x - origin.x, dy = listener.y - origin.y, dz = listener.z - origin.z;
    const float local[3] = { dx * m.xAxis.x + dy * m.xAxis.y + dz * m.xAxis.z,
                             dx * m.yAxis.x + dy * m.yAxis.y + dz * m.yAxis.z,
                             dx * m.zAxis.x + dy * m.zAxis.y + dz * m.zAxis.z };

    float fraction = 0.0f;
    if (m_shapeKind == kVolumeShape_Box) {
        const ape::Aabb inner = { m_innerMin, m_innerMax };
        if (ape::AabbContainsPoint(inner, { local[0], local[1], local[2], 0.0f }))
            return 1.0f;
        const float innerMin[3] = { m_innerMin.x, m_innerMin.y, m_innerMin.z };
        const float innerMax[3] = { m_innerMax.x, m_innerMax.y, m_innerMax.z };
        const float falloffMax[3] = { m_falloffMax.x, m_falloffMax.y, m_falloffMax.z };
        const float falloffMin[3] = { m_falloffMin.x, m_falloffMin.y, m_falloffMin.z };
        for (int i = 0; i < 3; ++i) {
            float t = 0.0f;
            if (falloffMax[i] > 0.0f && local[i] > innerMax[i])
                t = (local[i] - innerMax[i]) / falloffMax[i];
            else if (falloffMin[i] < 0.0f && local[i] < innerMin[i])
                t = (local[i] - innerMin[i]) / falloffMin[i];
            fraction = std::max(fraction, t);
        }
    } else if (m_shapeKind == kVolumeShape_Sphere) {
        const float distance = std::sqrt(local[0] * local[0] + local[1] * local[1] + local[2] * local[2]);
        if (distance <= m_innerRadius)
            return 1.0f;
        fraction = (distance - m_innerRadius) / (m_outerRadius - m_innerRadius);
    } else {
        return 0.0f;
    }

    if (fraction >= 1.0f)
        return 0.0f;
    float scaled;
    if (std::fabs(fraction - 1.0f) < 1e-05f)
        scaled = fraction;
    else if (curve == 0.0f)
        scaled = 1.0f;
    else if (fraction == 0.0f)
        scaled = 0.0f;
    else
        scaled = std::pow(fraction, curve);
    return 1.0f - scaled;
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

// @func 0x00213C78 SoundZoneTrigger::Update
// Fading volumes follow the listener's position; the others ramp their weight
// up while occupied and down (unlinking at zero) when empty, over the zone's
// cross-fade time. The active trigger of the zone hands the weight on.
bool SoundZoneTrigger::Update(float dt)
{
    if (!m_enabled)
        return true;
    SoundZoneTriggerDef* zoneDef = ZoneDef();
    SoundZone* zone = zoneDef ? zoneDef->zone : nullptr;
    if (m_fades) {
        m_weight = ComputeFadeWeight(GetListenerPosition());
        if (zone && SoundZoneActiveTrigger(zone) == this)
            SoundZoneSetWeight(zone, m_weight);
    } else {
        const float fadeTime = ZoneFadeTime(zone);
        const bool occupied = IsPolled() ? m_inside : OccupantCount() != 0;
        if (occupied) {
            if (m_weight < 1.0f) {
                m_weight = std::min(m_weight + dt / fadeTime, 1.0f);
                if (zone && SoundZoneActiveTrigger(zone) == this)
                    SoundZoneSetWeight(zone, m_weight);
            }
        } else if (m_weight > 0.0f) {
            m_weight -= dt / fadeTime;
            if (m_weight <= 0.0f) {
                if (zone && m_linked)
                    UnlinkActive();
                m_weight = 0.0f;
            }
            if (zone && SoundZoneActiveTrigger(zone) == this)
                SoundZoneSetWeight(zone, m_weight);
        }
    }
    return PolledTrigger::Update(dt);
}

// @func 0x002140C0 SoundZoneTrigger::OnEnter
void SoundZoneTrigger::OnEnter(Entity* entity)
{
    if (!IsPolled() && entity != GetPlayerEntity())
        return;
    SoundZoneTriggerDef* zoneDef = ZoneDef();
    if (!m_fades) {
        float fade = ZoneFadeTime(zoneDef ? zoneDef->zone : nullptr);
        m_weight = std::fabs(fade) < kFadeEpsilon ? 1.0f : 0.0f;
    } else {
        m_weight = ComputeFadeWeight(GetListenerPosition());
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
void SoundZoneTrigger::OnExit(Entity* entity)
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
    // Entity::GetBounds is not const in the port.
    ape::Aabb box = const_cast<SoundZoneTrigger*>(this)->GetBounds(true);
    return ape::AabbContainsPoint(box, point);
}

} // namespace objects
} // namespace sly
