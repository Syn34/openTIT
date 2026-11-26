// openTIT - Game message queue and listener registry.
#include "sly/game/GameMessages.h"

#include "sly/entity/Entity.h"

#include <map>

namespace sly {

namespace {

// Registry node: one listener of one key, linked per key.
struct ListenerNode {
    void* listener = nullptr;     // null = free node
    ListenerNode* next = nullptr;
    bool isEntity = false;
};

struct MessageSystem {
    entity::EntityMessage queue[kMaxQueuedMessages];
    int queued = 0;
    ListenerNode nodes[kMaxListenerNodes];
    int nodeCursor = 0;
    std::map<uint32_t, ListenerNode*> keys;   // key -> first node
};

MessageSystem& System()
{
    static MessageSystem s_system;
    return s_system;
}

void Queue(entity::Entity* sender, void* receiver, uint32_t message, uint32_t argument, bool toEntity)
{
    MessageSystem& sys = System();
    if (sys.queued >= kMaxQueuedMessages)
        return;
    entity::EntityMessage& record = sys.queue[sys.queued++];
    record.sender = sender;
    record.receiver = receiver;
    record.id = message;
    record.argument = argument;
    record.toEntity = toEntity;
}

// Takes the node at the cursor and moves the cursor on to the next free
// node. NOTE: like the console, the pool is assumed never to fill up.
ListenerNode* AllocNode()
{
    MessageSystem& sys = System();
    ListenerNode* node = &sys.nodes[sys.nodeCursor];
    for (int tries = 0; tries < kMaxListenerNodes; ++tries) {
        sys.nodeCursor = (sys.nodeCursor + 1) % kMaxListenerNodes;
        if (!sys.nodes[sys.nodeCursor].listener)
            break;
    }
    return node;
}

void SetRegistered(void* listener, bool isEntity, bool registered)
{
    if (isEntity) {
        auto* ent = static_cast<entity::Entity*>(listener);
        if (registered)
            ent->flags2 |= entity::kEnt2_InListenerGroup;
        else
            ent->flags2 &= static_cast<uint16_t>(~entity::kEnt2_InListenerGroup);
    } else {
        static_cast<GameEventListener*>(listener)->registered = registered;
    }
}

} // namespace

// @func 0x0048F27C GameEventListener::GameEventListener
GameEventListener::GameEventListener() = default;

// @func 0x0048F3F8 GameEventListener::~GameEventListener
// @func 0x0048F6B0 GameEventListener::~GameEventListener (copy 2)
// @func 0x0048F548 GameEventListener::~GameEventListener (deleting)
GameEventListener::~GameEventListener()
{
    if (registered)
        UnregisterListener(this, false);
}

void GameEventListener::OnGameMessage(const entity::EntityMessage&) {}

// @func 0x0048FA54 RegisterListener
void RegisterListener(void* listener, uint32_t key, bool isEntity)
{
    MessageSystem& sys = System();
    ListenerNode*& head = sys.keys[key];
    ListenerNode* node = AllocNode();
    node->listener = listener;
    node->isEntity = isEntity;
    node->next = head;
    head = node;
    SetRegistered(listener, isEntity, true);
}

// @func 0x0048F298 UnregisterListener
void UnregisterListener(void* listener, bool isEntity)
{
    MessageSystem& sys = System();
    for (auto it = sys.keys.begin(); it != sys.keys.end();) {
        ListenerNode*& head = it->second;
        // Unlink the listener's node (each key holds it at most once).
        for (ListenerNode** link = &head; *link; link = &(*link)->next) {
            if ((*link)->listener == listener) {
                ListenerNode* node = *link;
                node->listener = nullptr;
                *link = node->next;
                break;
            }
        }
        if (!head)
            it = sys.keys.erase(it);
        else
            ++it;
    }
    SetRegistered(listener, isEntity, false);
}

// @func 0x0048FC68 HasListeners
bool HasListeners(uint32_t key)
{
    const MessageSystem& sys = System();
    auto it = sys.keys.find(key);
    return it != sys.keys.end() && it->second != nullptr;
}

// @func 0x004705F8 PostGameMessage
void PostGameMessage(entity::Entity* sender, void* receiver, uint32_t message, uint32_t argument,
                     bool toEntity)
{
    if (toEntity && !receiver)
        return;
    Queue(sender, receiver, message, argument, toEntity);
}

// @func 0x00470660 PostToListeners
void PostToListeners(entity::Entity* sender, uint32_t key, uint32_t message, uint32_t argument)
{
    MessageSystem& sys = System();
    auto it = sys.keys.find(key);
    if (it == sys.keys.end())
        return;
    for (ListenerNode* node = it->second; node; node = node->next) {
        if (node->isEntity && !node->listener)
            continue;
        Queue(sender, node->listener, message, argument, node->isEntity);
    }
}

// @func 0x0048F90C PostToListenersInRange
void PostToListenersInRange(float radius, entity::Entity* sender, uint32_t key, uint32_t message,
                            uint32_t argument)
{
    MessageSystem& sys = System();
    auto it = sys.keys.find(key);
    if (it == sys.keys.end())
        return;
    const ape::Vec3& origin = sender->Position();
    for (ListenerNode* node = it->second; node; node = node->next) {
        if (!node->isEntity || !node->listener)
            continue;
        const ape::Vec3& at = static_cast<entity::Entity*>(node->listener)->Position();
        const float dx = at.x - origin.x;
        const float dy = at.y - origin.y;
        const float dz = at.z - origin.z;
        if (dx * dx + dy * dy + dz * dz < radius * radius)
            Queue(sender, node->listener, message, argument, true);
    }
}

// @func 0x0048F800 DispatchGameMessages
void DispatchGameMessages()
{
    MessageSystem& sys = System();
    // Handlers may post more messages; they are delivered in the same pass.
    for (int i = 0; i < sys.queued; ++i) {
        const entity::EntityMessage& msg = sys.queue[i];
        if (!msg.toEntity) {
            static_cast<GameEventListener*>(msg.receiver)->OnGameMessage(msg);
        } else {
            auto* receiver = static_cast<entity::Entity*>(msg.receiver);
            if (receiver->flags & entity::kEnt_Alive)
                receiver->HandleMessage(msg);
        }
    }
    sys.queued = 0;
}

void ResetGameMessages()
{
    MessageSystem& sys = System();
    for (ListenerNode& node : sys.nodes)
        node = ListenerNode();
    sys.nodeCursor = 0;
    sys.queued = 0;
    sys.keys.clear();
}

} // namespace sly
