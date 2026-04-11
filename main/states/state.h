#pragma once

#include <memory>

class StateManager;

enum class StateId {
    Init,
    Pairing,
    Connecting,
    Playing,
};

const char *state_id_to_string(StateId state_id);

class State {
public:
    using Pointer = std::shared_ptr<State>;

    State() = default;
    virtual ~State() = default;

    virtual bool enter() = 0;
    virtual void exit() = 0;

    void set_manager(std::weak_ptr<StateManager> state_manager);

    // Switches to a new state using the state manager.
    // Can be used inside the active state safely.
    void switch_to(StateId state);

private:
    std::weak_ptr<StateManager> m_state_manager = {};
};
