#include "bluetooth.h"
#include "nv_storage.h"

#include <cassert>
#include <cstring>

#include "esp_log.h"

static constexpr auto DEVICE_NAME = "ESP32 Player";

// File-scope singleton pointer used only by static C callbacks below.
// Not exposed in the header — this is an implementation detail.
static Bluetooth* s_instance = nullptr;

// ---------------------------------------------------------------------------
// Static C callbacks (free functions, registered with ESP-IDF)
// ---------------------------------------------------------------------------

static void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t* param) {
    if (event == ESP_BT_GAP_AUTH_CMPL_EVT) {
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI("Bluetooth", "GAP auth complete: %s", param->auth_cmpl.device_name);
        } else {
            ESP_LOGE("Bluetooth", "GAP auth failed, status: %d", param->auth_cmpl.stat);
        }
    }
}

static void a2d_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t* param) {
    if (!s_instance) return;

    switch (event) {
        case ESP_A2D_CONNECTION_STATE_EVT: {
            auto state = param->conn_stat.state;
            if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
                s_instance->on_connected(param->conn_stat.remote_bda);
            } else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
                s_instance->on_disconnected();
            }
            break;
        }
        case ESP_A2D_AUDIO_STATE_EVT:
            ESP_LOGI("Bluetooth", "A2DP audio state: %d", param->audio_stat.state);
            break;
        case ESP_A2D_AUDIO_CFG_EVT:
            ESP_LOGI("Bluetooth", "A2DP audio config received");
            break;
        default:
            break;
    }
}

static void a2d_data_callback(const uint8_t* data, uint32_t len) {
    if (s_instance) {
        s_instance->on_audio_data(data, len);
    }
}

// ---------------------------------------------------------------------------
// Bluetooth — public
// ---------------------------------------------------------------------------

Bluetooth::Bluetooth(std::shared_ptr<NVStorage> storage)
    : m_storage(std::move(storage))
{
    assert(s_instance == nullptr && "Only one Bluetooth instance is allowed");
    s_instance = this;

    // Release BLE memory — we use classic BT only
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_bt_gap_set_device_name(DEVICE_NAME));

    ESP_ERROR_CHECK(esp_bt_gap_register_callback(gap_callback));
    ESP_ERROR_CHECK(esp_a2d_register_callback(a2d_callback));
    ESP_ERROR_CHECK(esp_a2d_sink_register_data_callback(a2d_data_callback));

    ESP_ERROR_CHECK(esp_a2d_sink_init());

    // Start non-connectable until a state explicitly opens the device
    ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE));

    ESP_LOGI(TAG, "Bluetooth initialized");
}

Bluetooth::~Bluetooth() {
    esp_a2d_sink_deinit();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    s_instance = nullptr;
}

void Bluetooth::start_pairing() {
    ESP_LOGI(TAG, "start_pairing: discoverable + connectable");
    ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE));
}

void Bluetooth::stop_pairing() {
    ESP_LOGI(TAG, "stop_pairing: connectable, non-discoverable");
    ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_NON_DISCOVERABLE));
}

void Bluetooth::start_connecting() {
    ESP_LOGI(TAG, "start_connecting: connectable, non-discoverable");
    ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_NON_DISCOVERABLE));

    if (has_saved_device()) {
        auto bda = m_storage->get_binary(s_bda_key);
        if (bda.size() == 6) {
            ESP_LOGI(TAG, "Connecting to saved device %02x:%02x:%02x:%02x:%02x:%02x",
                     bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
            esp_a2d_sink_connect(bda.data());
        }
    } else {
        ESP_LOGI(TAG, "No saved device — waiting for inbound connection");
    }
}

void Bluetooth::stop_connecting() {
    ESP_LOGI(TAG, "stop_connecting (no-op)");
}

void Bluetooth::set_connect_callback(std::function<void()> cb) {
    m_connect_cb = std::move(cb);
}

void Bluetooth::set_disconnect_callback(std::function<void()> cb) {
    m_disconnect_cb = std::move(cb);
}

void Bluetooth::set_audio_callback(std::function<void(const int16_t*, size_t)> cb) {
    m_audio_cb = std::move(cb);
}

bool Bluetooth::has_saved_device() const {
    return m_storage->has_key(s_bda_key);
}

// ---------------------------------------------------------------------------
// Bluetooth — private (called from static C callbacks)
// ---------------------------------------------------------------------------

void Bluetooth::on_connected(const uint8_t bda[6]) {
    ESP_LOGI(TAG, "A2DP connected — saving device address");
    m_storage->set_binary(s_bda_key, bda, 6);
    if (m_connect_cb) m_connect_cb();
}

void Bluetooth::on_disconnected() {
    ESP_LOGI(TAG, "A2DP disconnected");
    if (m_disconnect_cb) m_disconnect_cb();
}

void Bluetooth::on_audio_data(const uint8_t* data, uint32_t len) {
    // A2DP SBC default: 44100 Hz, 16-bit stereo = 4 bytes per frame
    if (m_audio_cb && len >= 4) {
        m_audio_cb(reinterpret_cast<const int16_t*>(data), len / 4);
    }
}
