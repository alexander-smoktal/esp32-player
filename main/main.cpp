extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2s_std.h"
#include "esp_log.h"
#include "esp_random.h"
}

#include <cmath>
#include <cstdlib>
#include <cstring>

static const char *TAG = "GUITAR_TONE";

static i2s_chan_handle_t tx_chan;

// Audio parameters
constexpr int SAMPLE_RATE = 44100;
constexpr int32_t AMPLITUDE = 600000000;

// Classic guitar range: E2 (82.41 Hz) to E6 (1318.5 Hz)
// Using MIDI note numbers: E2=40, E6=88
constexpr int MIDI_MIN = 40;
constexpr int MIDI_MAX = 88;

// DMA-safe buffer: heap-allocated, size in frames (stereo pairs)
constexpr int BUF_FRAMES = 512;
static int32_t *audio_buf = nullptr;

// ================= I2S INIT =================

static void i2s_init()
{
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_32BIT,
            I2S_SLOT_MODE_STEREO
        ),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = GPIO_NUM_7,
            .ws   = GPIO_NUM_5,
            .dout = GPIO_NUM_6,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = 0,
                .bclk_inv = 0,
                .ws_inv = 0
            }
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));

    ESP_LOGI(TAG, "I2S initialized: %d Hz, 32-bit stereo", SAMPLE_RATE);
}

// ================= TONE GENERATOR =================

static float midi_to_freq(int midi_note)
{
    return 440.0f * powf(2.0f, (midi_note - 69) / 12.0f);
}

static void audio_task(void *)
{
    float phase = 0.0f;
    float current_freq = midi_to_freq(MIDI_MIN + (MIDI_MAX - MIDI_MIN) / 2);
    float phase_inc = (2.0f * (float)M_PI * current_freq) / SAMPLE_RATE;

    // How long the current note plays (in samples)
    int note_samples_total = SAMPLE_RATE / 2;  // 0.5 second per note
    int note_samples_left = note_samples_total;

    // Simple fade-in/fade-out envelope to avoid clicks
    constexpr int FADE_SAMPLES = 800; // ~18 ms fade

    while (true) {
        for (int i = 0; i < BUF_FRAMES; i++) {
            // Envelope: fade in at note start, fade out at note end
            float envelope = 1.0f;
            int pos = note_samples_total - note_samples_left;
            if (pos < FADE_SAMPLES) {
                envelope = (float)pos / FADE_SAMPLES;
            } else if (note_samples_left < FADE_SAMPLES) {
                envelope = (float)note_samples_left / FADE_SAMPLES;
            }

            float sample_f = AMPLITUDE * envelope * sinf(phase);
            int32_t sample = (int32_t)sample_f;

            // Stereo: L and R identical
            audio_buf[2 * i]     = sample;
            audio_buf[2 * i + 1] = sample;

            phase += phase_inc;
            if (phase >= 2.0f * (float)M_PI) {
                phase -= 2.0f * (float)M_PI;
            }

            note_samples_left--;
            if (note_samples_left <= 0) {
                // Pick a new random note in the guitar range
                int midi = MIDI_MIN + (esp_random() % (MIDI_MAX - MIDI_MIN + 1));
                current_freq = midi_to_freq(midi);
                phase_inc = (2.0f * (float)M_PI * current_freq) / SAMPLE_RATE;

                // Randomize note duration: 0.3 to 1.0 seconds
                note_samples_total = SAMPLE_RATE * 3 / 10
                    + (esp_random() % (SAMPLE_RATE * 7 / 10));
                note_samples_left = note_samples_total;

                ESP_LOGI(TAG, "Note: MIDI %d (%.1f Hz), dur %.2fs",
                         midi, current_freq,
                         (float)note_samples_total / SAMPLE_RATE);
            }
        }

        size_t bytes_written = 0;
        i2s_channel_write(tx_chan, audio_buf,
                          BUF_FRAMES * 2 * sizeof(int32_t),
                          &bytes_written, portMAX_DELAY);
    }
}

// ================= MAIN =================

extern "C" void app_main()
{
    // Heap-allocate audio buffer (DMA-safe, not on stack)
    audio_buf = (int32_t *)heap_caps_calloc(
        BUF_FRAMES * 2, sizeof(int32_t), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!audio_buf) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer");
        return;
    }

    i2s_init();

    // Run audio generation in a dedicated task with sufficient stack
    xTaskCreatePinnedToCore(audio_task, "audio", 4096, NULL, 5, NULL, 1);
}
