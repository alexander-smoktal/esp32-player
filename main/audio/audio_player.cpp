#include "audio_player.h"

#include <cstring>

#include "esp_log.h"
#include "esp_heap_caps.h"

static constexpr auto TAG = "AudioPlayer";

static constexpr int SAMPLE_RATE  = 44100;
static constexpr int FEED_FRAMES  = 512;  // I2S DMA chunk size

// ================= I2S INIT =================

static i2s_chan_handle_t i2s_init() {
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);

    i2s_chan_handle_t chan = nullptr;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &chan, nullptr));

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
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
                .ws_inv   = 0,
            },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(chan));

    ESP_LOGI(TAG, "I2S initialized: %d Hz, 32-bit stereo", SAMPLE_RATE);
    return chan;
}

// ================= AUDIO PLAYER =================

AudioPlayer::AudioPlayer() : m_buffer(2048) {
    m_i2s_chan = i2s_init();

    if (xTaskCreatePinnedToCore(feed_task, "audio_feed", 4096, this, 5, &m_feed_task, 1) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create feed task");
    }
}

AudioPlayer::~AudioPlayer() {
    if (m_feed_task) {
        vTaskDelete(m_feed_task);
        m_feed_task = nullptr;
    }
    if (m_i2s_chan) {
        i2s_channel_disable(m_i2s_chan);
        i2s_del_channel(m_i2s_chan);
    }
}

void AudioPlayer::write_bt_audio(const int16_t* samples, size_t frame_count) {
    // Upscale s16 stereo → s32 stereo and write to ring buffer
    // PCM5102 expects 32-bit; shift left by 16 to use full dynamic range
    constexpr size_t CHUNK = 128;
    int32_t buf[CHUNK * 2];

    while (frame_count > 0) {
        size_t n = (frame_count > CHUNK) ? CHUNK : frame_count;
        for (size_t i = 0; i < n; i++) {
            buf[2 * i]     = (int32_t)samples[2 * i]     << 16;
            buf[2 * i + 1] = (int32_t)samples[2 * i + 1] << 16;
        }
        m_buffer.write(buf, n);
        samples     += n * 2;
        frame_count -= n;
    }
}

// ================= FEED TASK =================

void AudioPlayer::feed_task(void* ctx) {
    auto* self = static_cast<AudioPlayer*>(ctx);

    // DMA-safe buffer allocated on heap
    int32_t* chunk = static_cast<int32_t*>(
        heap_caps_malloc(FEED_FRAMES * 2 * sizeof(int32_t), MALLOC_CAP_DMA | MALLOC_CAP_8BIT));

    if (!chunk) {
        ESP_LOGE(TAG, "Failed to allocate feed buffer");
        vTaskDelete(nullptr);
        return;
    }

    while (true) {
        size_t frames = self->m_buffer.read(chunk, FEED_FRAMES, pdMS_TO_TICKS(10));

        if (frames < FEED_FRAMES) {
            // Fill the remainder with silence so I2S always gets a full chunk
            memset(chunk + frames * 2, 0, (FEED_FRAMES - frames) * 2 * sizeof(int32_t));
        }

        size_t bytes_written = 0;
        i2s_channel_write(self->m_i2s_chan,
                          chunk,
                          FEED_FRAMES * 2 * sizeof(int32_t),
                          &bytes_written,
                          portMAX_DELAY);
    }
}
