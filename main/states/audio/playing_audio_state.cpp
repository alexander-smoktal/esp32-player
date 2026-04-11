#include "playing_audio_state.h"

#include "esp_log.h"

#include "audio/audio_player.h"

static constexpr auto TAG = "PlayingAudioState";

PlayingAudioState::PlayingAudioState(std::shared_ptr<AudioPlayer> audio)
    : m_audio(std::move(audio)) {}

bool PlayingAudioState::enter() {
    ESP_LOGI(TAG, "enter — bluetooth audio passthrough active");
    return true;
}

void PlayingAudioState::exit() {
    ESP_LOGI(TAG, "exit");
}
