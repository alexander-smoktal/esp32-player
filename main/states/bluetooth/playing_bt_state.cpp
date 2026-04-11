#include "playing_bt_state.h"

#include "esp_log.h"

#include "bluetooth/bluetooth.h"
#include "audio/audio_player.h"

static constexpr auto TAG = "PlayingBtState";

PlayingBtState::PlayingBtState(std::shared_ptr<Bluetooth> bt, std::shared_ptr<AudioPlayer> audio)
    : m_bt(std::move(bt))
    , m_audio(std::move(audio)) {}

bool PlayingBtState::enter() {
    ESP_LOGI(TAG, "enter — bluetooth playing");
    // TODO: register A2DP data callback → call m_audio->write_bt_audio()
    return true;
}

void PlayingBtState::exit() {
    ESP_LOGI(TAG, "exit");
    // TODO: unregister A2DP data callback
}
