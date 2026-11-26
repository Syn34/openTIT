// openTIT - Game messages: the queue of messages from one entity (or the
// game) to an entity or a listener object, and the listener registry that
// lets objects receive the messages posted under a key.
//
// Console layout: queue of 128 records (count 0x010BA134, records at
// 0x010BA138), 256 registry nodes (0x010BAB38, allocation cursor 0x010BA130)
// and the key map 0x010BB738 (key -> first node). Messages are delivered in
// one batch per frame by DispatchGameMessages.
#pragma once

#include <cstdint>

namespace sly {

namespace entity {
class Entity;
struct EntityMessage;
}

// Base of the objects (goals, minigames, component helpers) that receive
// game messages without being entities (vtable 0x00B21A30).
class GameEventListener {
public:
    GameEventListener();
    virtual ~GameEventListener();
    // Slot 2: a message addressed to this listener.
    virtual void OnGameMessage(const entity::EntityMessage& msg);

    GameEventListener(const GameEventListener&) = delete;
    GameEventListener& operator=(const GameEventListener&) = delete;

    bool registered = false;   // +0x04 listening to at least one key
};

constexpr int kMaxQueuedMessages = 128;
constexpr int kMaxListenerNodes = 256;

// Adds a listener (an entity when 'isEntity') to the receivers of 'key'.
void RegisterListener(void* listener, uint32_t key, bool isEntity);
// Removes a listener from every key it listens to.
void UnregisterListener(void* listener, bool isEntity);
// True when somebody listens to 'key'.
bool HasListeners(uint32_t key);

// Queues a message for one receiver. A message meant for an entity is
// dropped when there is no receiver; nothing is queued past 128 messages.
void PostGameMessage(entity::Entity* sender, void* receiver, uint32_t message, uint32_t argument,
                     bool toEntity);
// Queues a message for every listener of 'key'.
void PostToListeners(entity::Entity* sender, uint32_t key, uint32_t message, uint32_t argument);
// Queues a message for the entity listeners of 'key' closer than 'radius'
// to the sender.
void PostToListenersInRange(float radius, entity::Entity* sender, uint32_t key, uint32_t message,
                            uint32_t argument);
// Delivers the queued messages (listeners: slot 2; entities: HandleMessage,
// only while they are alive) and empties the queue.
void DispatchGameMessages();
// Forgets every queued message and listener (level reset; inline in the
// game world's reset on the console).
void ResetGameMessages();

} // namespace sly
