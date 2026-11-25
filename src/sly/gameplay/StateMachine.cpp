// openTIT - Layered state machine base (vtable 0x00B219D0).
#include "sly/gameplay/StateMachine.h"

#include <algorithm>
#include <cstring>

namespace sly {
namespace gameplay {

// @func 0x0045CAA0 StateMachine::StateMachine
// @func 0x0045CBE8 StateMachine::StateMachine (thunk)
StateMachine::StateMachine(uint16_t layerCount, uint32_t dataSize, uint8_t initialState)
    : m_layerCount(layerCount), m_dataSize(static_cast<uint16_t>((dataSize + 15) & ~15u))
{
    if (layerCount == 0)
        return;
    m_states.assign(layerCount, 0);
    m_times.assign(layerCount, 0.0f);
    m_frames.assign(layerCount, 0);
    if (m_dataSize)
        m_data.assign(size_t(layerCount) * m_dataSize, 0);
    m_states[0] = initialState;
}

// @func 0x00464A24 StateMachine::~StateMachine
// @func 0x00464AD0 StateMachine::~StateMachine (copy 2)
// @func 0x00490A88 StateMachine::~StateMachine (copy 3)
// @func 0x00490B34 StateMachine::~StateMachine (copy 4)
// @func 0x00464A6C StateMachine::~StateMachine (deleting)
// @func 0x00490AD0 StateMachine::~StateMachine (deleting, copy 2)
StateMachine::~StateMachine() = default;

// @func 0x00464B18 StateMachine::Begin
void StateMachine::Begin()
{
    if (m_states.empty() || m_states[0] == kNoState)
        return;
    Message enter;
    enter.type = kMsg_Enter;
    enter.layer = 0;
    enter.state = kNoState;
    enter.newState = m_states[0];
    OnMessage(enter);
}

// @func 0x00464B90 StateMachine::Stop
void StateMachine::Stop()
{
    SetState(0, kNoState);
}

// @func 0x00464BD4 StateMachine::Update
bool StateMachine::Update(float dt)
{
    Message update;
    update.type = kMsg_Update;
    update.dt = dt;
    update.abort = false;
    do {
        if (update.abort)
            return false;
    } while (!OnMessage(update));
    if (update.abort)
        return false;
    for (uint32_t i = 0; i < m_layerCount; ++i) {
        m_times[i] += dt;
        ++m_frames[i];
    }
    return true;
}

// @func 0x00464CBC StateMachine::LateUpdate
bool StateMachine::LateUpdate(float dt)
{
    Message message;
    message.type = kMsg_LateUpdate;
    message.dt = dt;
    message.abort = false;
    OnMessage(message);
    return true;
}

// @func 0x007371E8 StateMachine::AdvanceState
void StateMachine::AdvanceState(uint32_t layer)
{
    SetState(layer, GetState(layer) + 1);
}

// @func 0x0073726C StateMachine::GetState
uint8_t StateMachine::GetState(uint32_t layer) const
{
    return m_states[layer];
}

// @func 0x00464D1C StateMachine::SetState
bool StateMachine::SetState(uint32_t layer, int state)
{
    const uint8_t newState = static_cast<uint8_t>(state);
    const uint8_t old = m_states[layer];
    if (old != kNoState) {
        Message exit;
        exit.type = kMsg_Exit;
        exit.layer = layer;
        exit.state = old;
        exit.newState = newState;
        OnMessage(exit);
    }
    for (uint32_t i = layer; i < m_layerCount; ++i) {
        m_times[i] = 0.0f;
        m_frames[i] = 0;
        if (m_dataSize)
            std::memset(&m_data[size_t(i) * m_dataSize], 0, m_dataSize);
        if (i > layer)
            m_states[i] = 0;
    }
    m_states[layer] = newState;
    if (newState != kNoState) {
        Message enter;
        enter.type = kMsg_Enter;
        enter.layer = layer;
        enter.state = m_states[layer];
        enter.newState = newState;
        OnMessage(enter);
    }
    return true;
}

} // namespace gameplay
} // namespace sly
