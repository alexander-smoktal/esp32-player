#pragma once

#include <functional>
#include <memory>
#include <cstdint>

extern "C" {
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
}

class NVStorage;

/* Bluetooth A2DP sink subsystem.
   Wraps the Bluedroid classic BT stack. Only one instance may exist at a time
   (the C callback dispatch uses a file-scope static in bluetooth.cpp).

   Lifecycle:
     Construction       — initialises Bluedroid stack; registers all callbacks.
     start_pairing()    — GAP: connectable + discoverable
     stop_pairing()     — GAP: connectable + non-discoverable
     start_connecting() — GAP: connectable + non-discoverable; auto-connects saved BDA
     stop_connecting()  — no-op (connection attempt may still complete)
     Destruction        — deinitialises Bluedroid stack
*/
class Bluetooth {
public:
    explicit Bluetooth(std::shared_ptr<NVStorage> storage);
    ~Bluetooth();

    // GAP scan-mode control — called by Bluetooth states
    void start_pairing();
    void stop_pairing();

    void start_connecting();
    void stop_connecting();

    // Callback registration — pass nullptr to unregister
    void set_connect_callback(std::function<void()> cb);
    void set_disconnect_callback(std::function<void()> cb);
    void set_audio_callback(std::function<void(const int16_t*, size_t frames)> cb);

    bool has_saved_device() const;

    // Called from file-scope static C callbacks in bluetooth.cpp — must be public
    void on_connected(const uint8_t bda[6]);
    void on_disconnected();
    void on_audio_data(const uint8_t* data, uint32_t len);

private:

    static constexpr auto TAG = "Bluetooth";
    static constexpr auto s_bda_key = "bt_addr";

    std::shared_ptr<NVStorage> m_storage;
    std::function<void()> m_connect_cb;
    std::function<void()> m_disconnect_cb;
    std::function<void(const int16_t*, size_t)> m_audio_cb;
};
