#include "connecting_bt_state.h"

#include "esp_log.h"

#include "bluetooth/bluetooth.h"

static constexpr auto TAG = "ConnectingBtState";

ConnectingBtState::ConnectingBtState(std::shared_ptr<Bluetooth> bt)
    : m_bt(std::move(bt)) {}

bool ConnectingBtState::enter() {
    ESP_LOGI(TAG, "Enter — bluetooth connecting");
    m_bt->set_connect_callback([this]() { switch_to(StateId::Playing); });
    m_bt->start_connecting();
    return true;
}

void ConnectingBtState::exit() {
    ESP_LOGI(TAG, "Exit");
    m_bt->stop_connecting();
    m_bt->set_connect_callback(nullptr);
}
