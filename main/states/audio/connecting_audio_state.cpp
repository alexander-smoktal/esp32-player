#include "connecting_audio_state.h"

#include "esp_log.h"

#include "audio/wav_player.h"

static constexpr auto TAG = "ConnectingAudioState";

ConnectingAudioState::ConnectingAudioState(std::shared_ptr<WavPlayer> wav_player)
    : m_wav_player(std::move(wav_player)) {}

bool ConnectingAudioState::enter() {
    ESP_LOGI(TAG, "Enter");
    if (xTaskCreatePinnedToCore(play_task, "connecting_play", 3072, this, 4, &m_task, 1) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create play task");
        return false;
    }
    return true;
}

void ConnectingAudioState::exit() {
    ESP_LOGI(TAG, "Exit");
    // Kill the play task first so it stops writing to the buffer
    if (m_task) {
        vTaskDelete(m_task);
        m_task = nullptr;
    }
    // Play the confirmation sound synchronously in the state manager task.
    // This blocks the switcher briefly but ensures the clip is heard before
    // the next state's audio begins.
    m_wav_player->play("connected.wav");
}

void ConnectingAudioState::play_task(void* ctx) {
    auto* self = static_cast<ConnectingAudioState*>(ctx);

    // Play the connecting announcement once, then wait to be deleted by exit().
    self->m_wav_player->play("connecting.wav");
    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
}
