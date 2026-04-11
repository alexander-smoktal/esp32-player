#include "state.h"

#include "state_manager.h"

const char *state_id_to_string(StateId state_id) {
    switch (state_id) {
        case StateId::Init:       return "Init";
        case StateId::Pairing:    return "Pairing";
        case StateId::Connecting: return "Connecting";
        case StateId::Playing:    return "Playing";
        default:                  return "Unknown";
    }
}

void State::set_manager(std::weak_ptr<StateManager> state_manager) {
    m_state_manager = std::move(state_manager);
}

void State::switch_to(StateId state) {
    if (!m_state_manager.expired()) {
        m_state_manager.lock()->switch_to(state);
    }
}
