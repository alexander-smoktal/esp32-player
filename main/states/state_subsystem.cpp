#include "state_subsystem.h"

#include "esp_log.h"

static constexpr auto TAG = "StateSubsystem";

void StateSubsystem::add_state(StateId state, State::Pointer state_instance) {
    ESP_LOGI(TAG, "Adding state: '%s' to the '%s' subsystem", state_id_to_string(state), m_name.c_str());

    assert(m_states.count(state) == 0 && "State already exists");

    m_states.emplace(state, state_instance);
    state_instance->set_manager(m_manager);
}

bool StateSubsystem::switch_to(StateId state) {
    ESP_LOGI(TAG, "Switching to state: '%s' of the '%s' subsystem", state_id_to_string(state), m_name.c_str());

    auto new_state = m_states.find(state);
    if (new_state == m_states.end()) {
        ESP_LOGW(TAG, "State '%s' does not exist in the '%s' subsystem", state_id_to_string(state), m_name.c_str());
        return true;
    }

    // First exit all current states
    if (!m_current_states.empty()) {
        for (const auto& current_state : m_current_states) {
            current_state->exit();
        }
        m_current_states.clear();
    }

    new_state->second->enter();
    m_current_states.push_back(new_state->second);
    ESP_LOGI(TAG, "Switched to state: '%s' of the '%s' subsystem", state_id_to_string(state), m_name.c_str());

    return true;
}
