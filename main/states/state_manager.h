#pragma once

#include <vector>
#include <memory>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "state_subsystem.h"

class AudioPlayer;
class Bluetooth;
class WavPlayer;

/* Coordinates multiple subsystems, each managing their own state.
   Switching is asynchronous — requests go through a FreeRTOS task notification
   to allow states to call switch_to() without deadlocking.
*/
class StateManager : public std::enable_shared_from_this<StateManager> {
public:
    StateManager();

    void registerStates(std::shared_ptr<AudioPlayer> audio,
                        std::shared_ptr<Bluetooth> bt,
                        std::shared_ptr<WavPlayer> wav_player);

    std::shared_ptr<StateSubsystem> add_subsystem(int priority, const std::string &&name);

    // Enqueues an async state switch request (safe to call from within a state)
    void switch_to(StateId state);

private:
    void perform_switch(StateId new_state);
    static void switch_loop(void* context);

    StateId m_current_state = StateId::Init;
    std::vector<std::shared_ptr<StateSubsystem>> m_subsystems = {};
    TaskHandle_t m_task_handle = nullptr;
};
