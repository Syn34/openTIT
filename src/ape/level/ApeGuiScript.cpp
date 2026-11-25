// openTIT - GUI scripts: loading, players and property animation.
#include "ape/level/ApeGuiScript.h"

#include "ape/core/ApeString.h"
#include "ape/level/ApeGuiTypes.h"
#include "ape/math/ApeMath.h"
#include "ape/sys/ApeGeometry.h"
#include "platform/Platform.h"

#include <cmath>
#include <cstring>

namespace ape {

namespace {

constexpr float kUnset = 3.4028235e38f;   // FLT_MAX: "from" not given, use the current value
constexpr float kDegToRad = 0.017453292f;
constexpr float kRadToDeg = 57.295776f;
constexpr Vec3 kZAxis = { 0.0f, 0.0f, 1.0f, 0.0f };
constexpr xml::Color8 kDefaultColor = { 0x80, 0x80, 0x80, 0x80 };

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

constexpr uint32_t kElemDelay = Hash("delay");
constexpr uint32_t kElemSet = Hash("set");
constexpr uint32_t kElemInterp = Hash("interp");
constexpr uint32_t kElemMessage = 0xA0CED6D9;   // NOTE: element name unknown

// Property names (0x00AEF8CC); index 22 is the object flag 30 attribute.
constexpr uint32_t kPropertyNames[kGuiProp_Count] = {
    0,                 Hash("position"), Hash("position_x"), Hash("position_y"), Hash("position_z"),
    Hash("rotation"),  Hash("color"),    Hash("color_r"),    Hash("color_g"),    Hash("color_b"),
    Hash("color_a"),   Hash("colortl"),  Hash("colortr"),    Hash("colorbl"),    Hash("colorbr"),
    Hash("scale"),     Hash("scale_x"),  Hash("scale_y"),    Hash("size"),       Hash("size_x"),
    Hash("size_y"),    Hash("visible"),  0xFFD1EC91,         Hash("expand"),     Hash("anchor"),
    Hash("align"),
};
static_assert(Hash("position_x") == 0x6F3AEEC2u, "property name hash");

// "ease" (0x00AEF938) and "loop" (0x00AEF948) values.
constexpr uint32_t kEaseNames[4] = { Hash("linear"), Hash("sin"), Hash("log"), Hash("noise") };
constexpr uint32_t kLoopNames[3] = { Hash("none"), Hash("repeat"), Hash("pingpong") };

// Message names of message steps (0x00AEF874): the GUI event types the
// game handles. NOTE: entries 1-4, 7, 8 and 13-16 are only known by hash.
constexpr uint32_t kMessageNames[17] = {
    0,           0x019B17DD,  0x31C7FB76,  0x07397D4A,   0x07367489,  Hash("accept"),
    Hash("cancel"), 0xE052127E, 0x04C4D1BB, Hash("start"), Hash("inc"), Hash("dec"),
    Hash("init"), 0x80FF616F, 0x248DF656,  0x38E9C135,   0x6B7E1EBB,
};

GuiScript* s_scripts = nullptr;          // 0x00F6689C
GuiScriptPlayer* s_players = nullptr;    // 0x00F668A0
uint32_t s_noiseSeed = 0x12345678;       // NOTE: the engine-wide random state 0x01164890 on the console

void Warn(xml::XmlDocument* doc, const char* message)
{
    plat::Log(plat::LogLevel::Warning, "%s: %s\n", doc ? doc->GetDocumentPath() : "", message);
}

// Reads an attribute given as one of the hashed names.
int32_t ReadName(xml::XmlDocument* doc, xml::XmlElement* element, const char* attribute, const uint32_t* names,
                 int32_t count, bool warnMissing)
{
    uint32_t hash = 0;
    if (element->GetHash(attribute, &hash, 0) != 0) {
        if (warnMissing)
            Warn(doc, "missing GUI script attribute");
        return 0;
    }
    for (int32_t i = 0; i < count; ++i) {
        if (names[i] == hash)
            return i;
    }
    Warn(doc, "unknown GUI script value");
    return count;
}

uint32_t Rotl(uint32_t value, int bits)
{
    return (value << bits) | (value >> (32 - bits));
}

// Value kinds of the properties.
enum class ValueKind { None, Float, Vec2, Vec3, Color, Int, Alignment };

ValueKind KindOf(int32_t property)
{
    switch (property) {
    case kGuiProp_Position:
    case kGuiProp_Scale:
        return ValueKind::Vec3;
    case kGuiProp_Size:
        return ValueKind::Vec2;
    case kGuiProp_Color:
    case kGuiProp_ColorTL: case kGuiProp_ColorTR: case kGuiProp_ColorBL: case kGuiProp_ColorBR:
        return ValueKind::Color;
    case kGuiProp_ColorR: case kGuiProp_ColorG: case kGuiProp_ColorB: case kGuiProp_ColorA:
    case kGuiProp_Visible: case kGuiProp_Flag30: case kGuiProp_Expand:
        return ValueKind::Int;
    case kGuiProp_Anchor:
    case kGuiProp_Align:
        return ValueKind::Alignment;
    case kGuiProp_None:
        return ValueKind::None;
    default:
        return ValueKind::Float;
    }
}

// Classes with a size: frames, gradients, tiles and groups.
bool HasSize(const ApeGuiObject* object)
{
    switch (object->GetType()) {
    case kGuiObject_Type0:
    case kGuiObject_Type2:
    case kGuiObject_Type6:
    case kGuiObject_Type7:
        return true;
    default:
        return false;
    }
}

void ParseAlignmentText(const char* text, uint32_t& value)
{
    for (const char* p = text; p && *p; ++p) {
        switch (*p) {
        case 'L': case 'l': value = (value & ~0x03u) | kGuiAlign_Left; break;
        case 'C': case 'c': value = (value & ~0x03u) | kGuiAlign_Center; break;
        case 'R': case 'r': value = (value & ~0x03u) | kGuiAlign_Right; break;
        case 'T': case 't': value = (value & ~0x30u) | kGuiAlign_Top; break;
        case 'M': case 'm': value = (value & ~0x30u) | kGuiAlign_Middle; break;
        case 'B': case 'b': value = (value & ~0x30u) | kGuiAlign_Bottom; break;
        default: break;
        }
    }
}

float Lerp(float from, float to, float t)
{
    return t * (to - from) + from;
}

uint8_t LerpByte(uint8_t from, uint8_t to, int32_t t255)
{
    return (uint8_t)(from + (((int32_t)to - (int32_t)from) * t255 >> 8));
}

xml::Color8 LerpColor(const xml::Color8& from, const xml::Color8& to, int32_t t255)
{
    return { LerpByte(from.r, to.r, t255), LerpByte(from.g, to.g, t255), LerpByte(from.b, to.b, t255),
             LerpByte(from.a, to.a, t255) };
}

uint8_t& Channel(xml::Color8& color, int index)
{
    switch (index) {
    case 0: return color.r;
    case 1: return color.g;
    case 2: return color.b;
    default: return color.a;
    }
}

float& Component(Vec3& v, int index)
{
    return index == 0 ? v.x : (index == 1 ? v.y : v.z);
}

} // namespace

// ---------------------------------------------------------------------------
// Steps
// ---------------------------------------------------------------------------

// @func 0x00795EBC GuiScriptStep::~GuiScriptStep
// @func 0x00795ECC GuiScriptStep::~GuiScriptStep (deleting)
GuiScriptStep::~GuiScriptStep() = default;

// @func 0x00795F64 GuiScriptDelay::~GuiScriptDelay
// @func 0x00795F74 GuiScriptDelay::~GuiScriptDelay (deleting)
GuiScriptDelay::~GuiScriptDelay() = default;

// @func 0x00795FB8 GuiScriptDelay::Start
GuiScriptAction* GuiScriptDelay::Start(GuiScriptPlayer*)
{
    return nullptr;
}

// @func 0x00795FC0 GuiScriptPropertyStep::~GuiScriptPropertyStep
// @func 0x00795FD0 GuiScriptPropertyStep::~GuiScriptPropertyStep (deleting)
GuiScriptPropertyStep::~GuiScriptPropertyStep() = default;

// @func 0x004E3CAC GuiScriptPropertyStep::Start
GuiScriptAction* GuiScriptPropertyStep::Start(GuiScriptPlayer* player)
{
    ApeGuiObject* object = FindGuiObject(target, player->scope);
    if (!object)
        return nullptr;
    return new GuiPropertyAction(this, player, object);
}

// @func 0x00796058 GuiScriptMessageStep::~GuiScriptMessageStep
// @func 0x00796068 GuiScriptMessageStep::~GuiScriptMessageStep (deleting)
GuiScriptMessageStep::~GuiScriptMessageStep() = default;

// @func 0x004E4B40 GuiScriptMessageStep::Start
GuiScriptAction* GuiScriptMessageStep::Start(GuiScriptPlayer* player)
{
    if (message != 0) {
        ApeGuiObject* object = FindGuiObject(target, player->scope);
        SendGuiEvent(message, 0xFFFFFFFFu, object, reinterpret_cast<uintptr_t>(player));
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Actions
// ---------------------------------------------------------------------------

// @func 0x00795F10 GuiScriptAction::~GuiScriptAction
// @func 0x00795F20 GuiScriptAction::~GuiScriptAction (deleting)
GuiScriptAction::~GuiScriptAction() = default;

// @func 0x004E3790 GuiPropertyAction::GuiPropertyAction
// An unset start value is taken from the object.
GuiPropertyAction::GuiPropertyAction(GuiScriptPropertyStep* propertyStep, GuiScriptPlayer* ownerPlayer,
                                     ApeGuiObject* object)
    : GuiScriptAction(propertyStep, ownerPlayer), target(object), from(propertyStep->from)
{
    if (from.f[0] != kUnset || !target)
        return;
    const int32_t property = propertyStep->property;
    switch (property) {
    case kGuiProp_Position: {
        const Vec3& p = target->GetPosition();
        from.f[0] = p.x;
        from.f[1] = p.y;
        from.f[2] = p.z;
        break;
    }
    case kGuiProp_PositionX: case kGuiProp_PositionY: case kGuiProp_PositionZ: {
        Vec3 p = target->GetPosition();
        from.f[0] = Component(p, property - kGuiProp_PositionX);
        break;
    }
    case kGuiProp_Rotation: {
        const Matrix34& m = target->GetTransform();
        from.f[0] = std::atan2(m.xAxis.y, m.xAxis.x) * kRadToDeg;
        break;
    }
    case kGuiProp_Color:
        from.color = target->GetColor();
        break;
    case kGuiProp_ColorR: case kGuiProp_ColorG: case kGuiProp_ColorB: case kGuiProp_ColorA: {
        xml::Color8 color = target->GetColor();
        from.u = Channel(color, property - kGuiProp_ColorR);
        break;
    }
    case kGuiProp_ColorTL: case kGuiProp_ColorTR: case kGuiProp_ColorBL: case kGuiProp_ColorBR:
        from.color = target->GetType() == kGuiObject_Type2
                         ? static_cast<ApeGuiGradient*>(target)->colors[property - kGuiProp_ColorTL]
                         : kDefaultColor;
        break;
    case kGuiProp_Scale: {
        const Vec3& s = target->GetScale();
        from.f[0] = s.x;
        from.f[1] = s.y;
        from.f[2] = s.z;
        break;
    }
    case kGuiProp_ScaleX: case kGuiProp_ScaleY: {
        Vec3 s = target->GetScale();
        from.f[0] = Component(s, property - kGuiProp_ScaleX);
        break;
    }
    case kGuiProp_Size: case kGuiProp_SizeX: case kGuiProp_SizeY: {
        float size[2] = { 0.0f, 0.0f };
        if (HasSize(target))
            target->GetSize(size);
        if (property == kGuiProp_Size) {
            from.f[0] = size[0];
            from.f[1] = size[1];
        } else {
            from.f[0] = size[property - kGuiProp_SizeX];
        }
        break;
    }
    default:
        break;
    }
}

// @func 0x00794A6C GuiPropertyAction::~GuiPropertyAction
// @func 0x00796014 GuiPropertyAction::~GuiPropertyAction (deleting)
GuiPropertyAction::~GuiPropertyAction() = default;

// @func 0x004E3D98 GuiPropertyAction::Update
bool GuiPropertyAction::Update(float dt)
{
    const auto* prop = static_cast<const GuiScriptPropertyStep*>(step);
    time += dt;

    // Progress with the loop mode.
    float t = 1.0f;
    if (prop->duration > 0.0f) {
        t = time / prop->duration;
        const float whole = std::trunc(t);
        switch (prop->loop) {
        case kGuiLoop_None:
            if (1.0f - t < 0.0f)
                t = 1.0f;
            break;
        case kGuiLoop_Repeat:
            t -= whole;
            break;
        case kGuiLoop_PingPong:
            t = ((int32_t)t & 1) ? 1.0f - (t - whole) : t - whole;
            break;
        default:
            break;
        }
    }
    // Easing.
    if (prop->ease == kGuiEase_Sin) {
        t = (FastCos(t * 3.1415927f) - 1.0f) * -0.5f;
    } else if (prop->ease == kGuiEase_Noise) {
        s_noiseSeed = Rotl((Rotl(s_noiseSeed, 4) + Rotl(s_noiseSeed, 16)) ^ 0x87654321u, 16);
        const uint32_t sample = s_noiseSeed & 0xFF;   // bits 16-23 of the unrotated value
        t = sample < (uint32_t)(t * 256.0f) ? 1.0f : 0.0f;
    }

    const int32_t property = prop->property;
    const GuiScriptValue& to = prop->to;
    const int32_t t255 = (int32_t)(t * 255.0f);
    switch (property) {
    case kGuiProp_Position: {
        Vec3 p{};
        p.x = Lerp(from.f[0], to.f[0], t);
        p.y = Lerp(from.f[1], to.f[1], t);
        p.z = Lerp(from.f[2], to.f[2], t);
        target->SetPosition(p);
        break;
    }
    case kGuiProp_PositionX: case kGuiProp_PositionY: case kGuiProp_PositionZ: {
        Vec3 p = target->GetPosition();
        Component(p, property - kGuiProp_PositionX) = Lerp(from.f[0], to.f[0], t);
        target->SetPosition(p);
        break;
    }
    case kGuiProp_Rotation: {
        const float degrees = Lerp(from.f[0], to.f[0], t);
        Matrix34 m{};
        MatrixAxisAngle(degrees * kDegToRad, m, kZAxis);
        m.translation = target->GetPosition();
        target->SetTransform(m);
        break;
    }
    case kGuiProp_Color:
        target->SetColor(LerpColor(from.color, to.color, t255));
        break;
    case kGuiProp_ColorR: case kGuiProp_ColorG: case kGuiProp_ColorB: case kGuiProp_ColorA: {
        // The current colour with one channel animated.
        const xml::Color8 current = target->GetColor();
        xml::Color8 start = current, end = current;
        Channel(start, property - kGuiProp_ColorR) = (uint8_t)from.u;
        Channel(end, property - kGuiProp_ColorR) = (uint8_t)to.u;
        target->SetColor(LerpColor(start, end, t255));
        break;
    }
    case kGuiProp_ColorTL: case kGuiProp_ColorTR: case kGuiProp_ColorBL: case kGuiProp_ColorBR:
        if (target->GetType() == kGuiObject_Type2)
            static_cast<ApeGuiGradient*>(target)->colors[property - kGuiProp_ColorTL] =
                LerpColor(from.color, to.color, t255);
        break;
    case kGuiProp_Scale: {
        Vec3 s{};
        s.x = Lerp(from.f[0], to.f[0], t);
        s.y = Lerp(from.f[1], to.f[1], t);
        s.z = Lerp(from.f[2], to.f[2], t);
        target->SetScale(s);
        break;
    }
    case kGuiProp_ScaleX: case kGuiProp_ScaleY: {
        Vec3 s = target->GetScale();
        Component(s, property - kGuiProp_ScaleX) = Lerp(from.f[0], to.f[0], t);
        target->SetScale(s);
        break;
    }
    case kGuiProp_Size: {
        const float size[2] = { Lerp(from.f[0], to.f[0], t), Lerp(from.f[1], to.f[1], t) };
        if (HasSize(target))
            target->SetSize(size);
        break;
    }
    case kGuiProp_SizeX: case kGuiProp_SizeY:
        if (HasSize(target)) {
            float size[2];
            target->GetSize(size);
            size[property - kGuiProp_SizeX] = Lerp(from.f[0], to.f[0], t);
            target->SetSize(size);
        }
        break;
    // The discrete properties switch to the end value once started.
    case kGuiProp_Visible:
        target->SetVisible((t > 0.0f ? to.u : from.u) != 0);
        break;
    case kGuiProp_Flag30:
        target->SetFlag30((t > 0.0f ? to.u : from.u) != 0);
        break;
    case kGuiProp_Expand:
        target->SetExpand((t > 0.0f ? to.u : from.u) != 0);
        break;
    case kGuiProp_Anchor:
        target->SetAnchor(t > 0.0f ? to.u : from.u);
        break;
    case kGuiProp_Align:
        target->SetAlign(t > 0.0f ? to.u : from.u);
        break;
    default:
        return false;
    }
    target->Invalidate();
    if (prop->loop != kGuiLoop_None)
        return true;
    return time < prop->duration;
}

// ---------------------------------------------------------------------------
// Scripts and players
// ---------------------------------------------------------------------------

GuiScript::GuiScript(const char* scriptName, void* scriptOwner)
    : nameHash(HashNameNoCase(scriptName)), owner(scriptOwner)
{
    StrNCopy(name, scriptName, sizeof(name) - 1);
    GuiScript** link = &s_scripts;
    while (*link)
        link = &(*link)->nextScript;
    *link = this;
}

// @func 0x004E32A4 GuiScript::~GuiScript
// @func 0x004E33D4 GuiScript::~GuiScript (deleting)
// Players of the script are deleted with it.
GuiScript::~GuiScript()
{
    for (GuiScriptPlayer* player = s_players; player;) {
        GuiScriptPlayer* nextPlayer = player->nextPlayer;
        if (player->script == this)
            delete player;
        player = nextPlayer;
    }
    for (GuiScript** link = &s_scripts; *link; link = &(*link)->nextScript) {
        if (*link == this) {
            *link = nextScript;
            break;
        }
    }
    while (firstStep) {
        GuiScriptStep* nextStep = firstStep->next;
        delete firstStep;
        firstStep = nextStep;
    }
}

GuiScript* GuiScript::First()
{
    return s_scripts;
}

GuiScript* GuiScript::FindByHash(uint32_t hash)
{
    for (GuiScript* script = s_scripts; script; script = script->nextScript) {
        if (script->nameHash == hash)
            return script;
    }
    return nullptr;
}

GuiScriptPlayer::GuiScriptPlayer(GuiScript* playScript, ApeGuiObject* playScope, bool deleteWhenDone,
                                 bool deleteScope)
    : script(playScript), scope(playScope), autoDelete(deleteWhenDone), ownsScope(deleteScope)
{
    GuiScriptPlayer** link = &s_players;
    while (*link)
        link = &(*link)->nextPlayer;
    *link = this;
    currentStep = script->firstStep;
    stepDelay = currentStep ? currentStep->delay : 0.0f;
}

// @func 0x004E3528 GuiScriptPlayer::~GuiScriptPlayer
// @func 0x004E3648 GuiScriptPlayer::~GuiScriptPlayer (deleting)
GuiScriptPlayer::~GuiScriptPlayer()
{
    if (ownsScope && scope) {
        delete scope;
        scope = nullptr;
    }
    while (actions) {
        GuiScriptAction* nextAction = actions->next;
        delete actions;
        actions = nextAction;
    }
    currentStep = nullptr;
    stepDelay = 0.0f;
    for (GuiScriptPlayer** link = &s_players; *link; link = &(*link)->nextPlayer) {
        if (*link == this) {
            *link = nextPlayer;
            break;
        }
    }
}

GuiScriptPlayer* GuiScriptPlayer::First()
{
    return s_players;
}

void GuiScriptPlayer::Stop()
{
    while (actions) {
        GuiScriptAction* nextAction = actions->next;
        delete actions;
        actions = nextAction;
    }
    currentStep = nullptr;
    stepDelay = 0.0f;
}

// @func 0x004D3B00 GuiScriptPlayer::Tick
// A looping script restarts at most three times per tick.
bool GuiScriptPlayer::Tick(float dt)
{
    // Update the running actions, dropping the finished ones.
    GuiScriptAction* tail = nullptr;
    for (GuiScriptAction** link = &actions; *link;) {
        GuiScriptAction* action = *link;
        if (action->Update(dt)) {
            tail = action;
            link = &action->next;
        } else {
            *link = action->next;
            delete action;
        }
    }

    uint32_t restarts = 0;
    while (currentStep) {
        if (dt < stepDelay) {
            stepDelay -= dt;
            break;
        }
        if (GuiScriptAction* action = currentStep->Start(this)) {
            if (!action->Update(dt)) {
                delete action;
            } else {
                if (tail)
                    tail->next = action;
                else
                    actions = action;
                tail = action;
            }
        }
        dt -= stepDelay;
        currentStep = currentStep->next;
        if (!currentStep && script->loop && ++restarts < 4) {
            while (actions) {
                GuiScriptAction* nextAction = actions->next;
                delete actions;
                actions = nextAction;
            }
            // NOTE: the console keeps appending after the deleted tail here.
            tail = nullptr;
            currentStep = script->firstStep;
        }
        stepDelay = currentStep ? currentStep->delay : 0.0f;
    }
    return actions != nullptr || currentStep != nullptr;
}

// @func 0x004E2DB0 PlayGuiScript
GuiScriptPlayer* PlayGuiScript(uint32_t nameHash, ApeGuiObject* scope, bool autoDelete, bool ownsScope)
{
    GuiScript* script = GuiScript::FindByHash(nameHash);
    if (!script)
        return nullptr;
    return new GuiScriptPlayer(script, scope, autoDelete, ownsScope);
}

// @func 0x004E2EF0 PlayGuiScript(name)
GuiScriptPlayer* PlayGuiScript(const char* name, ApeGuiObject* scope, bool autoDelete, bool ownsScope)
{
    return PlayGuiScript(HashNameNoCase(name), scope, autoDelete, ownsScope);
}

// @func 0x004E3040 StopGuiScripts
void StopGuiScripts(uint32_t nameHash, ApeGuiObject* scope)
{
    for (GuiScriptPlayer* player = s_players; player;) {
        GuiScriptPlayer* nextPlayer = player->nextPlayer;
        if ((nameHash == 0 || player->script->nameHash == nameHash) && (!scope || scope == player->scope)) {
            player->Stop();
            if (player->autoDelete)
                delete player;
        }
        player = nextPlayer;
    }
}

// @func 0x004E3168 StopGuiScripts(name)
void StopGuiScripts(const char* name, ApeGuiObject* scope)
{
    StopGuiScripts(HashNameNoCase(name), scope);
}

void TickGuiScripts(float dt)
{
    for (GuiScriptPlayer* player = s_players; player;) {
        GuiScriptPlayer* nextPlayer = player->nextPlayer;
        if (!player->Tick(dt) && player->autoDelete)
            delete player;
        player = nextPlayer;
    }
}

void DestroyGuiScripts()
{
    while (s_players)
        delete s_players;
    while (s_scripts)
        delete s_scripts;
}

void DestroyGuiScriptsOf(void* owner)
{
    for (GuiScript* script = s_scripts; script;) {
        GuiScript* nextScript = script->nextScript;
        if (script->owner == owner)
            delete script;
        script = nextScript;
    }
}

// ---------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------

namespace {

// @func 0x004DEF50 CreateGuiScriptMessageStep
GuiScriptStep* CreateGuiScriptMessageStep(xml::XmlDocument* doc, xml::XmlElement* element, float delay)
{
    auto* step = new GuiScriptMessageStep(delay);
    if (const char* target = element->GetString("target"))
        StrNCopy(step->target, target, sizeof(step->target) - 1);
    step->message = (uint32_t)ReadName(doc, element, "msg", kMessageNames, 17, true);
    return step;
}

// @func 0x004DF0D0 CreateGuiScriptPropertyStep
// "set" steps give the end value as "value", "interp" steps as "to".
GuiScriptStep* CreateGuiScriptPropertyStep(xml::XmlDocument* doc, xml::XmlElement* element, float delay)
{
    auto* step = new GuiScriptPropertyStep(delay);
    const uint32_t elementType = element->IsValid() ? element->GetNameHash() : 0;
    element->GetFloat("duration", &step->duration, 0.0f);
    if (const char* target = element->GetString("target"))
        StrNCopy(step->target, target, sizeof(step->target) - 1);
    step->property = ReadName(doc, element, "property", kPropertyNames, kGuiProp_Count, true);
    const char* toName = elementType == kElemSet ? "value" : "to";

    switch (KindOf(step->property)) {
    case ValueKind::Color:
        if (element->GetColor("from", &step->from.color, &kDefaultColor) != 0)
            step->from.f[0] = kUnset;
        element->GetColor(toName, &step->to.color, &kDefaultColor);
        break;
    case ValueKind::Int:
        if (element->GetInt("from", &step->from.i, 0) != 0)
            step->from.f[0] = kUnset;
        element->GetInt(toName, &step->to.i, 0);
        break;
    case ValueKind::Vec3: {
        // NOTE: the console's default "from" vector is not initialised; an
        // unset start value is assumed.
        const float unset[3] = { kUnset, kUnset, kUnset };
        const float zero[3] = { 0.0f, 0.0f, 0.0f };
        element->GetVec3("from", step->from.f, unset);
        element->GetVec3(toName, step->to.f, zero);
        break;
    }
    case ValueKind::Vec2: {
        const float unset[2] = { kUnset, kUnset };
        const float zero[2] = { 0.0f, 0.0f };
        element->GetVec2(Hash("from"), step->from.f, unset);
        element->GetVec2(Hash(toName), step->to.f, zero);
        break;
    }
    case ValueKind::Alignment:
        ParseAlignmentText(element->GetString("from"), step->from.u);
        ParseAlignmentText(element->GetString(toName), step->to.u);
        break;
    case ValueKind::Float:
        element->GetFloat("from", &step->from.f[0], kUnset);
        element->GetFloat(toName, &step->to.f[0], 0.0f);
        break;
    case ValueKind::None:
        break;
    }

    step->ease = ReadName(doc, element, "ease", kEaseNames, 4, false);
    step->loop = ReadName(doc, element, "loop", kLoopNames, 3, false);
    return step;
}

} // namespace

// @func 0x004DF708 CreateGuiScriptStep
GuiScriptStep* CreateGuiScriptStep(xml::XmlDocument* doc, xml::XmlElement* element, GuiScript*)
{
    const uint32_t elementType = element->IsValid() ? element->GetNameHash() : 0;
    float delay = 0.0f;
    element->GetFloat("delay", &delay, 0.0f);
    if (elementType == kElemDelay) {
        float time = delay;
        element->GetFloat("time", &time, delay);
        return new GuiScriptDelay(time);
    }
    if (elementType == kElemSet || elementType == kElemInterp)
        return CreateGuiScriptPropertyStep(doc, element, delay);
    if (elementType == kElemMessage)
        return CreateGuiScriptMessageStep(doc, element, delay);
    Warn(doc, "unknown GUI script step");
    return nullptr;
}

// @func 0x004DF88C CreateGuiScript
GuiScript* CreateGuiScript(xml::XmlDocument* doc, xml::XmlElement* element, void* owner)
{
    const char* name = element->GetString("name");
    if (!name) {
        Warn(doc, "GUI script without a name");
        return nullptr;
    }
    auto* script = new GuiScript(name, owner);
    element->GetBool("loop", &script->loop, false);
    GuiScriptStep* last = script->firstStep;
    for (xml::XmlElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        GuiScriptStep* step = CreateGuiScriptStep(doc, child, script);
        if (!step)
            continue;
        if (last)
            last->next = step;
        else
            script->firstStep = step;
        last = step;
    }
    return script;
}

} // namespace ape
