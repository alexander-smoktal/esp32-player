#include "connecting_bt_state.h"

#include "esp_log.h"

#include "bluetooth/bluetooth.h"

static constexpr auto TAG = "ConnectingBtState";

ConnectingBtState::ConnectingBtState(std::shared_ptr<Bluetooth> bt)
    : m_bt(std::move(bt)) {}

bool ConnectingBtState::enter() {
    ESP_LOGI(TAG, "enter — bluetooth connecting");
    return true;
}

void ConnectingBtState::exit() {
    ESP_LOGI(TAG, "exit");
}
