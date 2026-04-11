#include "state_manager.h"

#include <algorithm>

#include "esp_log.h"

#include "audio/wav_player.h"
#include "audio/pairing_audio_state.h"
#include "audio/connecting_audio_state.h"
#include "audio/playing_audio_state.h"
#include "bluetooth/pairing_bt_state.h"
#include "bluetooth/connecting_bt_state.h"
#include "bluetooth/playing_bt_state.h"

static constexpr auto TAG = "StateManager";

StateManager::StateManager() {
    // State switches are handled asynchronously so that states can call
    // switch_to() without blocking the state machine on themselves.
    if (xTaskCreate(&StateManager::switch_loop, "State Switcher", 8192, this, 10, &m_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create state switcher task");
        return;
    }
    ESP_LOGI(TAG, "StateManager initialized");
}

void StateManager::registerStates(std::shared_ptr<AudioPlayer> audio,
                                   std::shared_ptr<Bluetooth> bt,
                                   std::shared_ptr<WavPlayer> wav_player) {
    auto audio_subsystem = add_subsystem(1, "Audio");
    audio_subsystem->add_state(StateId::Pairing,    std::make_shared<PairingAudioState>(wav_player));
    audio_subsystem->add_state(StateId::Connecting, std::make_shared<ConnectingAudioState>(wav_player));
    audio_subsystem->add_state(StateId::Playing,    std::make_shared<PlayingAudioState>(audio));

    auto bt_subsystem = add_subsystem(2, "Bluetooth");
    bt_subsystem->add_state(StateId::Pairing,    std::make_shared<PairingBtState>(bt));
    bt_subsystem->add_state(StateId::Connecting, std::make_shared<ConnectingBtState>(bt));
    bt_subsystem->add_state(StateId::Playing,    std::make_shared<PlayingBtState>(bt, audio));
}

std::shared_ptr<StateSubsystem> StateManager::add_subsystem(int priority, const std::string &&name) {
    ESP_LOGI(TAG, "Adding subsystem: '%s' with priority: %d", name.c_str(), priority);

    auto subsystem = std::make_shared<StateSubsystem>(shared_from_this(), priority, std::move(name));
    m_subsystems.push_back(subsystem);

    std::sort(m_subsystems.begin(), m_subsystems.end(), [](const auto& a, const auto& b) {
        return a->priority() > b->priority();
    });

    return subsystem;
}

void StateManager::switch_to(StateId state) {
    // Note: uses task notification so it is safe to call from within a running state
    xTaskNotify(m_task_handle, static_cast<uint32_t>(state), eSetValueWithOverwrite);
    ESP_LOGI(TAG, "State switch request: '%s'", state_id_to_string(state));
}

void StateManager::perform_switch(StateId new_state) {
    ESP_LOGI(TAG, "Switching to state: '%s'", state_id_to_string(new_state));

    std::vector<std::shared_ptr<StateSubsystem>> migrated;
    for (const auto& subsystem : m_subsystems) {
        if (subsystem->switch_to(new_state)) {
            migrated.push_back(subsystem);
        } else {
            ESP_LOGW(TAG, "Subsystem '%s' failed to switch to '%s'",
                     subsystem->name().c_str(), state_id_to_string(new_state));

            if (m_current_state == StateId::Init) {
                ESP_LOGE(TAG, "Cannot rollback from initial state");
                return;
            }

            for (const auto& ms : migrated) {
                ms->switch_to(m_current_state);
                ESP_LOGI(TAG, "Rolled back subsystem '%s' to '%s'",
                         ms->name().c_str(), state_id_to_string(m_current_state));
            }
            return;
        }
    }

    m_current_state = new_state;
    ESP_LOGI(TAG, "Switched to state: '%s' successfully", state_id_to_string(new_state));
}

void StateManager::switch_loop(void* context) {
    auto* self = static_cast<StateManager*>(context);
    ESP_LOGI(TAG, "StateManager loop started");

    uint32_t notification = 0;
    while (true) {
        if (xTaskNotifyWait(ULONG_MAX, 0, &notification, portMAX_DELAY) == pdTRUE) {
            self->perform_switch(static_cast<StateId>(notification));
        }
    }
}
