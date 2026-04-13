#include "playing_bt_state.h"

#include "esp_log.h"

#include "bluetooth/bluetooth.h"
#include "audio/audio_player.h"

static constexpr auto TAG = "PlayingBtState";

PlayingBtState::PlayingBtState(std::shared_ptr<Bluetooth> bt, std::shared_ptr<AudioPlayer> audio)
    : m_bt(std::move(bt))
    , m_audio(std::move(audio)) {}

bool PlayingBtState::enter() {
    ESP_LOGI(TAG, "Enter — bluetooth playing");
    m_bt->set_audio_callback([this](const int16_t* data, size_t frames) {
        m_audio->write_bt_audio(data, frames);
    });
    m_bt->set_disconnect_callback([this]() {
        switch_to(StateId::Connecting);
    });
    return true;
}

void PlayingBtState::exit() {
    ESP_LOGI(TAG, "Exit");
    m_bt->set_audio_callback(nullptr);
    m_bt->set_disconnect_callback(nullptr);
}
