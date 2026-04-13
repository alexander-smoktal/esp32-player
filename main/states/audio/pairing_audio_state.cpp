#include "pairing_audio_state.h"

#include "esp_log.h"

#include "audio/wav_player.h"

static constexpr auto TAG = "PairingAudioState";

PairingAudioState::PairingAudioState(std::shared_ptr<WavPlayer> wav_player)
    : m_wav_player(std::move(wav_player)) {}

bool PairingAudioState::enter() {
    ESP_LOGI(TAG, "Enter");
    if (xTaskCreatePinnedToCore(play_task, "pairing_play", 8136, this, 4, &m_task, 1) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create play task");
        return false;
    }
    return true;
}

void PairingAudioState::exit() {
    ESP_LOGI(TAG, "Exit");
    if (m_task) {
        vTaskDelete(m_task);
        m_task = nullptr;
    }
}

void PairingAudioState::play_task(void* ctx) {
    auto* self = static_cast<PairingAudioState*>(ctx);

    // Play the pairing announcement once, then loop the boop indefinitely.
    // Silence between boops equals the boop clip duration (derived from file metadata).
    self->m_wav_player->play("pairing.wav");

    while (true) {
        uint32_t duration_ms = self->m_wav_player->play("boop.wav");
        vTaskDelay(pdMS_TO_TICKS(duration_ms > 0 ? duration_ms : 500));
    }
}
