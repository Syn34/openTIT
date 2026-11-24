// openTIT - Layered state machine base used by gameplay objects.
//
// Stand-in for the engine base class with vtable 0x00B219D0 (members at
// 0x00464AD0-0x00464E9C and 0x007371E8/0x0073726C, outside this module's
// range; nothing here is tagged). Each layer holds one state id (-1 = none)
// plus the time and frame count spent in it. Changing the state of a layer
// sends an exit message for the old state, clears the deeper layers and
// sends an enter message for the new one.
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
    };
    struct Message {
        int32_t type = kMsg_Enter;   // +0x00
        uint32_t layer = 0;          // +0x04
        int32_t state = -1;          // +0x08 state the message is about
        int32_t newState = -1;       // +0x0C requested state
        float dt = 0.0f;             // update messages only
    };

    static constexpr int8_t kNoState = -1;

    explicit StateMachine(uint16_t layerCount = 1)
        : m_states(layerCount, kNoState), m_times(layerCount, 0.0f), m_frames(layerCount, 0)
    {
    }
    virtual ~StateMachine() = default;
    StateMachine(const StateMachine&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;

    // Slot 7.
    virtual bool SetState(uint32_t layer, int state)
    {
        if (layer >= m_states.size())
            return false;
        const int old = m_states[layer];
        if (old != kNoState) {
            Message exit;
            exit.type = kMsg_Exit;
            exit.layer = layer;
            exit.state = old;
            exit.newState = state;
            OnMessage(exit);
        }
        for (size_t i = layer; i < m_states.size(); ++i) {
            m_times[i] = 0.0f;
            m_frames[i] = 0;
            if (i > layer)
                m_states[i] = 0;
        }
        m_states[layer] = static_cast<int8_t>(state);
        if (static_cast<int8_t>(state) != kNoState) {
            Message enter;
            enter.type = kMsg_Enter;
            enter.layer = layer;
            enter.state = m_states[layer];
            enter.newState = state;
            OnMessage(enter);
        }
        return true;
    }
    // Slot 8.
    int GetState(uint32_t layer) const
    {
        return layer < m_states.size() ? m_states[layer] : kNoState;
    }
    float TimeInState(uint32_t layer) const
    {
        return layer < m_times.size() ? m_times[layer] : 0.0f;
    }
    // Slot 9: every state change and update is routed through here.
    virtual bool OnMessage(const Message& message) = 0;

protected:
    std::vector<int8_t> m_states;    // +0x08
    std::vector<float> m_times;      // +0x0C
    std::vector<int32_t> m_frames;   // +0x10
};

} // namespace gameplay
} // namespace sly
