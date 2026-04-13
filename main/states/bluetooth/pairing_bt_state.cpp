#include "pairing_bt_state.h"

#include "esp_log.h"

#include "bluetooth/bluetooth.h"

static constexpr auto TAG = "PairingBtState";

PairingBtState::PairingBtState(std::shared_ptr<Bluetooth> bt)
    : m_bt(std::move(bt)) {}

bool PairingBtState::enter() {
    ESP_LOGI(TAG, "Enter — bluetooth pairing mode");
    m_bt->set_connect_callback([this]() { switch_to(StateId::Playing); });
    m_bt->start_pairing();
    return true;
}

void PairingBtState::exit() {
    ESP_LOGI(TAG, "Exit");
    m_bt->stop_pairing();
    m_bt->set_connect_callback(nullptr);
}
