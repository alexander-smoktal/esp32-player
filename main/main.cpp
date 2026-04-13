extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

#include <memory>
#include <esp_log.h>

#include "audio/audio_player.h"
#include "audio/wav_player.h"
#include "bluetooth/bluetooth.h"
#include "flash_storage.h"
#include "nv_storage.h"
#include "states/state_manager.h"

static constexpr auto TAG = "Main";

extern "C" void app_main() {
    auto storage = std::make_shared<NVStorage>();   // must be first — calls nvs_flash_init
    auto audio   = std::make_shared<AudioPlayer>();
    auto bt      = std::make_shared<Bluetooth>(storage);
    auto sounds  = std::make_shared<FlashStorage>("/sounds", "sounds");

    auto wav_player = std::make_shared<WavPlayer>(sounds, audio);

    auto state_manager = std::make_shared<StateManager>();
    state_manager->registerStates(audio, bt, wav_player);

    // Add some before playing anything to let user lock in
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    state_manager->switch_to(StateId::Pairing);

    ESP_LOGI(TAG, "Device has started");
    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
