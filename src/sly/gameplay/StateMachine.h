// openTIT - Layered state machine base used by gameplay objects.
//
// Engine base class with vtable 0x00B219D0. Each layer holds one state id
// (kNoState = none) plus the time and frame count spent in it, and
// optionally a block of per-layer state data that is cleared on every state
// change. Changing the state of a layer sends an exit message for the old
// state, resets that layer and the deeper ones and sends an enter message
// for the new state. Every message goes through OnMessage().
#pragma once

#include <cstdint>
#include <vector>

namespace sly {
namespace gameplay {

class StateMachine {
public:
    enum MessageType : int32_t {
        kMsg_Enter = 0,
        kMsg_Exit = 1,
        kMsg_Update = 2,
        // NOTE: sent by LateUpdate(); its purpose is a guess.
        kMsg_LateUpdate = 3,
    };
    // Enter/exit messages use layer, state and newState; update messages
    // use dt and abort (the original overlays them).
    struct Message {
        int32_t type = kMsg_Enter;   // +0x00
        uint32_t layer = 0;          // +0x04
        int32_t state = 0xFF;        // +0x08 exit: the old state; enter: the new state
                                     //       (kNoState when sent by Begin)
        int32_t newState = 0xFF;     // +0x0C the state being entered
        float dt = 0.0f;             // +0x04 update messages
        // Update messages: a handler sets this to stop the update (the
        // timers are then not advanced).
        mutable bool abort = false;  // +0x08 update messages
    };

    static constexpr uint8_t kNoState = 0xFF;

    // 'dataSize' bytes of state data per layer (rounded up to 16);
    // 'initialState' is the state of layer 0 (no enter message is sent),
    // deeper layers start in state 0.
    explicit StateMachine(uint16_t layerCount = 1, uint32_t dataSize = 0, uint8_t initialState = 0);
    virtual ~StateMachine();
    StateMachine(const StateMachine&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;

    // Slot 2: sends the enter message for the current state of layer 0.
    virtual void Begin();
    // Slot 3: leaves the state of layer 0.
    virtual void Stop();
    // Slot 4: sends an update message (repeated while the handler returns
    // false, so a handler can change state and run the new one in the same
    // frame), then advances the timers. False when a handler aborted.
    virtual bool Update(float dt);
    // Slot 5.
    virtual bool LateUpdate(float dt);
    // Slot 6: moves 'layer' to the next state id.
    virtual void AdvanceState(uint32_t layer);
    // Slot 7.
    virtual bool SetState(uint32_t layer, int state);
    // Slot 8.
    virtual uint8_t GetState(uint32_t layer) const;
    // Slot 9: every state change and update is routed through here.
    virtual bool OnMessage(const Message& message) = 0;

    float TimeInState(uint32_t layer) const { return layer < m_layerCount ? m_times[layer] : 0.0f; }
    int32_t FramesInState(uint32_t layer) const { return layer < m_layerCount ? m_frames[layer] : 0; }
    // Per-layer state data (null without state data).
    uint8_t* StateData(uint32_t layer) { return m_dataSize ? &m_data[layer * m_dataSize] : nullptr; }

protected:
    uint16_t m_layerCount = 0;       // +0x04
    uint16_t m_dataSize = 0;         // +0x06 bytes of state data per layer
    std::vector<uint8_t> m_states;   // +0x08 (one allocation holds all arrays)
    std::vector<float> m_times;      // +0x0C
    std::vector<int32_t> m_frames;   // +0x10
    std::vector<uint8_t> m_data;     // +0x14
};

} // namespace gameplay
} // namespace sly
