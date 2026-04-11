#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include "state.h"

class StateManager;

/* State subsystem that has a bunch of states.
   When requested, tries to change the state. If a given subsystem
   doesn't have a state for the StateId, remains on the current state.
*/
class StateSubsystem {
public:
    StateSubsystem(std::shared_ptr<StateManager> manager, int priority, const std::string &&name)
        : m_priority(priority)
        , m_name(std::move(name))
        , m_manager(manager) {}

    void add_state(StateId state, State::Pointer state_instance);
    bool switch_to(StateId state);

    int priority() const { return m_priority; }
    const std::string& name() const { return m_name; }

private:
    int m_priority;
    const std::string m_name;
    std::vector<State::Pointer> m_current_states = {};
    std::unordered_map<StateId, State::Pointer> m_states = {};
    std::weak_ptr<StateManager> m_manager;
};
