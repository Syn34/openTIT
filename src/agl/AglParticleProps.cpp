// openTIT - Agl particle emitter properties and their scene file chunks
// (0x004FFFF0, 0x005284C0-0x00529F98).
#include "agl/AglParticleProps.h"

#include "ape/sys/ApeIff.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace agl {

namespace {

constexpr float kDegToRad = 0.017453292f;
constexpr float kNoFade = 1e8f;

struct LoadCallback {
    void (*callback)(AglEmitProperties* props, void* arg, int phase) = nullptr;
    void* arg = nullptr;
};
LoadCallback g_loadCallback;

void RunLoadCallback(AglEmitProperties* props, int phase)
{
    if (g_loadCallback.callback)
        g_loadCallback.callback(props, g_loadCallback.arg, phase);
}

// Stores a new property object in a slot of the properties: the slot takes
// a reference, the previous object loses its, and the creator's reference
// is dropped.
template <class T>
T* InstallProperty(T*& slot, T* property)
{
    if (slot != property) {
        T* previous = slot;
        slot = property;
        if (property)
            property->AddRef();
        if (previous)
            previous->Release();
    }
    property->DropCreatorRef();
    return property;
}

// Alpha fade rate: the particle life over the fade time (no fade for very
// short times).
float FadeRate(float life, float fadeTime)
{
    return fadeTime > 1e-5f ? life / fadeTime : kNoFade;
}

// Colour keys of a colour chunk: the rest of the chunk is RGBA words; a
// chunk without keys gets two white keys.
void ReadColorKeys(ape::iff::IffReader* reader, AglColorProperty& color)
{
    static const uint8_t kWhite[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
    const uint32_t keyCount = reader->GetChunkRemaining() / 4;
    if (keyCount == 0) {
        color.AddKey(kWhite);
        color.AddKey(kWhite);
        return;
    }
    for (uint32_t i = 0; i < keyCount; ++i) {
        uint8_t rgba[4];
        reader->ReadRaw4(rgba);
        color.AddKey(rgba);
    }
}

} // namespace

void AglColorProperty::AddKey(const uint8_t rgba[4])
{
    // NOTE: the console does not check the capacity.
    if (count >= kMaxKeys)
        return;
    std::memcpy(keys[count], rgba, 4);
    ++count;
    if (count == 1 && count < kMaxKeys)
        std::memcpy(keys[count], rgba, 4);
}

void AglSizeProperty::AddKey(float value)
{
    // NOTE: the console does not check the capacity.
    if (count >= uint32_t(kMaxKeys))
        return;
    keys[count++] = value;
    if (count == 1)
        keys[count] = value;
}

// @func 0x004FFFF0 AglEmitProperties::AglEmitProperties
AglEmitProperties::AglEmitProperties() = default;

AglEmitProperties::~AglEmitProperties()
{
    AglParticleProperty* slots[] = { velocity, color, size, force, rotation, texAnim, collision, path };
    for (AglParticleProperty* property : slots)
        if (property)
            property->Release();
}

// TODO(0x004FF4C0 AglEmitProperties::ApplyRecord) copies and converts the
// emitter record (lifetimes, rates, culling distances, bursts, emit
// meshes); the particle system that reads the result is not ported.
void AglEmitProperties::ApplyRecord(const AglEmitterRecord& record)
{
    ttl = record.ttl;
}

void SetEmitPropertiesLoadCallback(void (*callback)(AglEmitProperties* props, void* arg, int phase), void* arg)
{
    g_loadCallback.callback = callback;
    g_loadCallback.arg = arg;
}

// @func 0x005284C0 LoadEmitPropertiesChunk
// A new properties object filled by the nested chunks.
// NOTE: the console also calls the parent definition's GetName (slot 3) and
// drops the result (stripped debug output).
int LoadEmitPropertiesChunk(ape::iff::IffReader* reader, void* /*arg*/)
{
    auto* props = new AglEmitProperties();
    RunLoadCallback(props, 0);
    reader->ParseChunks(props);
    RunLoadCallback(props, 1);
    return 1;
}

// @func 0x005286BC LoadEmitterRecordChunk
int LoadEmitterRecordChunk(ape::iff::IffReader* reader, void* arg)
{
    AglEmitterRecord record;
    uint8_t version = 0;
    reader->ReadU8(&version);
    reader->ReadFloat(&record.ttl);
    reader->ReadFloat(&record.ttlVariance);
    reader->ReadFloat(&record.emitRate);
    reader->ReadU32(&record.emitObjectHash);
    reader->ReadU8(&record.emitObjectType);
    reader->ReadU8(&record.renderFlags);
    reader->ReadU8(&record.processClass);
    reader->ReadU8(&record.alignment);
    reader->ReadFloat(&record.particleNearZCull);
    reader->ReadFloat(&record.particleFarZCull);
    reader->ReadFloat(&record.emitterFarZCull);
    reader->ReadFloat(&record.emitterFarZFadeDist);
    reader->ReadFloat(&record.emitRateVariance);
    reader->ReadFloat(&record.burstDelay);
    reader->ReadFloat(&record.burstDelayVariance);
    reader->ReadFloat(&record.burstDuration);
    reader->ReadFloat(&record.burstDurationVariance);
    for (uint32_t& hash : record.emitMeshHash)
        reader->ReadU32(&hash);
    static_cast<AglEmitProperties*>(arg)->ApplyRecord(record);
    return 1;
}

// @func 0x00528948 LoadVelocityChunk
// Shape 0 emits from a sphere (radius = size x), shape 1 from a box (half
// the size). The direction cone is given in degrees as offset and spread
// around x and z; it is stored as the start angle and range in radians.
int LoadVelocityChunk(ape::iff::IffReader* reader, void* arg)
{
    uint8_t version = 0, flags = 0, shape = 0;
    float speed = 0.0f, speedVariance = 0.0f;
    float offsetX = 0.0f, spreadX = 0.0f, offsetZ = 0.0f, spreadZ = 0.0f;
    float size[3] = {};
    reader->ReadU8(&version);
    reader->ReadU8(&flags);
    reader->ReadU8(&shape);
    reader->ReadFloat(&speed);
    reader->ReadFloat(&speedVariance);
    reader->ReadFloat(&offsetX);
    reader->ReadFloat(&spreadX);
    reader->ReadFloat(&offsetZ);
    reader->ReadFloat(&spreadZ);
    reader->ReadVec3(size);
    if (shape > AglVelocityProperty::kShape_Box)
        return 0;

    auto* velocity = new AglVelocityProperty();
    velocity->shape = AglVelocityProperty::Shape(shape);
    velocity->fromSurface = (flags & 1) != 0;
    if (shape == AglVelocityProperty::kShape_Sphere) {
        velocity->radius = size[0];
    } else {
        for (int i = 0; i < 3; ++i)
            velocity->halfExtents[i] = size[i] * 0.5f;
    }
    auto* props = static_cast<AglEmitProperties*>(arg);
    InstallProperty(props->velocity, velocity);
    velocity->speed = speed;
    velocity->speedVariance = speedVariance;
    velocity->hasSpread = offsetX != 0.0f || spreadX != 0.0f || offsetZ != 0.0f || spreadZ != 0.0f;
    velocity->minAngleX = (offsetX - spreadX * 0.5f) * kDegToRad;
    velocity->spreadX = spreadX * kDegToRad;
    velocity->minAngleZ = (offsetZ - spreadZ * 0.5f) * kDegToRad;
    velocity->spreadZ = spreadZ * kDegToRad;
    return 1;
}

// @func 0x00528F20 LoadForceChunk
int LoadForceChunk(ape::iff::IffReader* reader, void* arg)
{
    float force[3] = {};
    reader->ReadVec3(force);
    auto* props = static_cast<AglEmitProperties*>(arg);
    AglForceProperty* property = InstallProperty(props->force, new AglForceProperty());
    std::memcpy(property->force, force, sizeof(force));
    property->delay = 0.0f;
    return 1;
}

// @func 0x0052906C LoadDelayedForceChunk
int LoadDelayedForceChunk(ape::iff::IffReader* reader, void* arg)
{
    uint8_t version = 0;
    float force[3] = {};
    float delay = 0.0f;
    reader->ReadU8(&version);
    reader->ReadVec3(force);
    reader->ReadFloat(&delay);
    auto* props = static_cast<AglEmitProperties*>(arg);
    AglForceProperty* property = InstallProperty(props->force, new AglForceProperty());
    std::memcpy(property->force, force, sizeof(force));
    property->delay = delay;
    return 1;
}

// @func 0x005291F8 LoadColorChunk
int LoadColorChunk(ape::iff::IffReader* reader, void* arg)
{
    float fadeIn = 0.0f, fadeOut = 0.0f;
    reader->ReadFloat(&fadeIn);
    reader->ReadFloat(&fadeOut);
    auto* props = static_cast<AglEmitProperties*>(arg);
    AglColorProperty* color = InstallProperty(props->color, new AglColorProperty());
    color->fadeInRate = FadeRate(props->ttl, fadeIn);
    color->fadeOutRate = FadeRate(props->ttl, fadeOut);
    color->shadeVariance = 0;
    ReadColorKeys(reader, *color);
    return 1;
}

// @func 0x005295A0 LoadShadedColorChunk
// The colour chunk with a version byte and a shade variance (stored in 8.8
// fixed point).
int LoadShadedColorChunk(ape::iff::IffReader* reader, void* arg)
{
    uint8_t version = 0;
    float fadeIn = 0.0f, fadeOut = 0.0f, shadeVariance = 0.0f;
    reader->ReadU8(&version);
    reader->ReadFloat(&fadeIn);
    reader->ReadFloat(&fadeOut);
    reader->ReadFloat(&shadeVariance);
    auto* props = static_cast<AglEmitProperties*>(arg);
    AglColorProperty* color = InstallProperty(props->color, new AglColorProperty());
    color->fadeInRate = FadeRate(props->ttl, fadeIn);
    color->fadeOutRate = FadeRate(props->ttl, fadeOut);
    color->shadeVariance = int16_t(int64_t(shadeVariance * 256.0f));
    ReadColorKeys(reader, *color);
    return 1;
}

// @func 0x00529D80 LoadSizeChunk
// A variance (clamped to [0, 1]) followed by the size keys.
int LoadSizeChunk(ape::iff::IffReader* reader, void* arg)
{
    uint8_t version = 0;
    float variance = 0.0f;
    reader->ReadU8(&version);
    reader->ReadFloat(&variance);
    auto* props = static_cast<AglEmitProperties*>(arg);
    AglSizeProperty* size = InstallProperty(props->size, new AglSizeProperty());
    size->variance = std::clamp(variance, 0.0f, 1.0f);
    const uint32_t keyCount = reader->GetChunkRemaining() / 4;
    for (uint32_t i = 0; i < keyCount; ++i) {
        float value = 0.0f;
        reader->ReadFloat(&value);
        size->AddKey(value);
    }
    return 1;
}

// @func 0x00529FE0 LoadRotationChunk
// Header (axis, variance and, from version 1, the initial angle range and
// the random direction switch) followed by rate keys up to the end of the
// chunk.
int LoadRotationChunk(ape::iff::IffReader* reader, void* arg)
{
    constexpr float kDegToRad = 0.017453292f;
    constexpr float kMaxInitialAngle = 359.99999f;
    auto* props = static_cast<AglEmitProperties*>(arg);

    uint8_t version = 1;
    float axis[3] = {};
    float variance = 0.0f, initialAngle = 0.0f;
    uint8_t randomDirection = 0;
    reader->ReadU8(&version);
    reader->ReadVec3(axis);
    reader->ReadFloat(&variance);
    if (version >= 1) {
        reader->ReadFloat(&initialAngle);
        reader->ReadU8(&randomDirection);
    }

    AglRotationProperty* rotation = InstallProperty(props->rotation, new AglRotationProperty());

    const float lengthSq = axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2];
    if (lengthSq == 0.0f) {
        rotation->axis = { 0.0f, 0.0f, 1.0f, 0.0f };   // NOTE: console global 0x01164B70, assumed +Z
    } else {
        const float scale = 1.0f / std::sqrt(lengthSq);
        rotation->axis = { axis[0] * scale, axis[1] * scale, axis[2] * scale, 0.0f };
    }
    // NOTE: compared bitwise against the globals 0x01164B50/60/70, assumed to
    // be the X, Y and Z unit vectors.
    const ape::Vec3& a = rotation->axis;
    if (a.x == 1.0f && a.y == 0.0f && a.z == 0.0f)
        rotation->axisType = AglRotationProperty::kAxis_X;
    else if (a.x == 0.0f && a.y == 1.0f && a.z == 0.0f)
        rotation->axisType = AglRotationProperty::kAxis_Y;
    else if (a.x == 0.0f && a.y == 0.0f && a.z == 1.0f)
        rotation->axisType = AglRotationProperty::kAxis_Z;
    else
        rotation->axisType = AglRotationProperty::kAxis_Any;

    rotation->variance = std::clamp(variance, -1.0f, 1.0f);
    rotation->initialAngleRange = std::clamp(initialAngle, 0.0f, kMaxInitialAngle) / 360.0f;
    rotation->randomDirection = randomDirection != 0;

    const uint32_t keys = reader->GetChunkRemaining() / 4;
    for (uint32_t i = 0; i < keys; ++i) {
        float rate = 0.0f;
        reader->ReadFloat(&rate);
        // NOTE: the console does not bound the key count; extra keys are
        // dropped here.
        const uint32_t index = rotation->keyCount;
        if (index >= AglRotationProperty::kMaxKeys)
            continue;
        rotation->rates[index] = rate;
        rotation->keyCount = index + 1;
        // A single key also covers the segment after it.
        if (rotation->keyCount == 1)
            rotation->rates[1] = rate;
    }

    rotation->lifetime = props->ttl;
    rotation->keyCount = std::min(rotation->keyCount + 1, AglRotationProperty::kMaxKeys);
    float total = 0.0f;
    for (uint32_t i = 0; i < rotation->keyCount; ++i) {
        rotation->angles[i] = total * kDegToRad;
        total += rotation->rates[i];
    }
    return 1;
}

// @func 0x0052A528 LoadTexAnimChunk
// Mode 0 leaves the particles unanimated. A strip of N frames narrows the
// particle's width to one frame, then the aspect is renormalised.
int LoadTexAnimChunk(ape::iff::IffReader* reader, void* arg)
{
    uint8_t version = 0, mode = 0;
    float frames = 0.0f, speed = 0.0f, variance = 0.0f;
    reader->ReadU8(&version);
    reader->ReadU8(&mode);
    reader->ReadFloat(&frames);
    reader->ReadFloat(&speed);
    reader->ReadFloat(&variance);   // NOTE: read but not used
    if (mode == 0)
        return 1;
    auto* props = static_cast<AglEmitProperties*>(arg);
    AglTexAnimProperty* anim = InstallProperty(props->texAnim, new AglTexAnimProperty());
    anim->mode = mode;
    anim->speed = speed;
    anim->frameCount = frames;
    const float width = 1.0f / frames;
    anim->frameWidth = width;
    switch (mode) {
    case 1:
    case 4:
        anim->frameStep = width;
        break;
    case 2:
        anim->frameStep = width;
        anim->field_1C = speed / frames;
        break;
    case 3:
        anim->frameStep = width;
        anim->span = (frames - 1.0f) * 2.0f;
        anim->field_1C = speed / anim->span;
        break;
    default:
        break;
    }
    props->aspect[0] *= width;
    const float largest = std::max(props->aspect[0], props->aspect[1]);
    props->aspect[0] *= 1.0f / largest;
    props->aspect[1] *= 1.0f / largest;
    return 1;
}

// @func 0x0052996C LoadPathChunk
// The file stores, for each point, its position, the control point that
// leaves it and the control point that arrives at the next one, all in
// absolute coordinates. The spline wants handles relative to their own
// point, so the arriving control is moved to the following point. The first
// point has no incoming handle and the last none going out.
int LoadPathChunk(ape::iff::IffReader* reader, void* arg)
{
    auto* props = static_cast<AglEmitProperties*>(arg);
    uint8_t version = 0;
    uint32_t loop = 0, numPoints = 0;
    reader->ReadU8(&version);
    reader->ReadU32(&loop);
    reader->ReadU32(&numPoints);

    AglPathProperty* path = InstallProperty(props->path, new AglPathProperty());
    ape::ApeSpline& spline = path->spline;
    spline.Init(numPoints, loop == 1, false);
    reader->GetChunkRemaining();   // NOTE: result unused (stripped check)

    for (uint32_t i = 0; i < numPoints; ++i) {
        float position[3], leaving[3], arriving[3];
        reader->ReadVec3(position);
        reader->ReadVec3(leaving);
        reader->ReadVec3(arriving);
        ape::SplinePoint& p = spline.GetPoint(i);
        p.position = { position[0], position[1], position[2], 0.0f };
        p.inTangent = { leaving[0], leaving[1], leaving[2], 0.0f };
        p.outTangent = { arriving[0], arriving[1], arriving[2], 0.0f };
    }

    // A closed path repeats its first point at the end.
    if (spline.IsClosed()) {
        ape::SplinePoint& last = spline.GetPoint(numPoints);
        const ape::SplinePoint& first = spline.GetPoint(0);
        last.position = first.position;
        last.inTangent = first.inTangent;
        last.outTangent = first.outTangent;
    }

    const uint32_t count = spline.GetNumPoints();
    if (count > 1) {
        auto relative = [](const ape::Vec3& to, const ape::Vec3& from) {
            return ape::Vec3{ to.x - from.x, to.y - from.y, to.z - from.z, 0.0f };
        };
        // Walk backwards so each point still sees the raw arriving control
        // of its predecessor.
        for (int i = static_cast<int>(count) - 1; i >= 0; --i) {
            ape::SplinePoint& p = spline.GetPoint(i);
            p.outTangent = relative(p.inTangent, p.position);
            if (i > 0)
                p.inTangent = relative(spline.GetPoint(i - 1).outTangent, p.position);
        }
        const ape::Vec3 zero = { 0.0f, 0.0f, 0.0f, 0.0f };
        spline.GetPoint(0).inTangent = zero;
        spline.GetPoint(count - 1).outTangent = zero;
    }
    spline.ComputeLength();
    return 1;
}

// @func 0x0052A7C0 LoadCollisionChunk
// A bounce factor and up to six planes; unused planes have a distance of
// FLT_MAX.
// NOTE: when the properties do not have collisions enabled (flags 0x2A bit
// 7) the console formats a warning whose output was stripped.
int LoadCollisionChunk(ape::iff::IffReader* reader, void* arg)
{
    constexpr float kUnusedPlane = 3.4028235e38f;
    auto* props = static_cast<AglEmitProperties*>(arg);
    AglCollisionProperty* collision = InstallProperty(props->collision, new AglCollisionProperty());
    uint8_t version = 0;
    float bounce = 0.0f;
    float planes[AglCollisionProperty::kMaxPlanes][4] = {};
    reader->ReadU8(&version);
    reader->ReadFloat(&bounce);
    for (auto& plane : planes)
        reader->ReadVec4(plane);
    for (const auto& plane : planes) {
        if (plane[3] == kUnusedPlane)
            continue;
        std::memcpy(collision->planes[collision->count], plane, sizeof(plane));
        ++collision->count;
    }
    collision->bounce = bounce;
    return 1;
}

void RegisterParticleChunkHandlers()
{
    using ape::iff::MakeId;
    using ape::iff::RegisterChunkHandler;
    RegisterChunkHandler(MakeId('E', 'M', 'P', 'C'), LoadEmitPropertiesChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('E', 'M', 'P', 'H'), LoadEmitterRecordChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('E', 'P', 'I', 'B'), LoadVelocityChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('E', 'P', 'M', 'N'), LoadForceChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('E', 'P', 'N', 'M'), LoadDelayedForceChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('E', 'P', 'T', 'C'), LoadColorChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('E', 'P', 'C', 'T'), LoadShadedColorChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('E', 'P', 'T', 'S'), LoadSizeChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('E', 'P', 'U', 'V'), LoadTexAnimChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('E', 'P', 'C', 'P'), LoadCollisionChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('E', 'P', 'S', 'P'), LoadPathChunk, 0, nullptr, 0);
    RegisterChunkHandler(MakeId('E', 'P', 'T', 'R'), LoadRotationChunk, 0, nullptr, 0);
}

} // namespace agl
