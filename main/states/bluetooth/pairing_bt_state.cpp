#include "pairing_bt_state.h"

#include "esp_log.h"

#include "bluetooth/bluetooth.h"

static constexpr auto TAG = "PairingBtState";

PairingBtState::PairingBtState(std::shared_ptr<Bluetooth> bt)
    : m_bt(std::move(bt)) {}

bool PairingBtState::enter() {
    ESP_LOGI(TAG, "enter — bluetooth pairing mode");
    return true;
}

void PairingBtState::exit() {
    ESP_LOGI(TAG, "exit");
}
