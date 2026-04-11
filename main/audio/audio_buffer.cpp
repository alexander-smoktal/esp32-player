#include "audio_buffer.h"

#include <cstring>
#include <algorithm>

#include "freertos/task.h"

#include "esp_log.h"
#include "esp_heap_caps.h"

static constexpr auto TAG = "AudioBuffer";

AudioBuffer::AudioBuffer(size_t frame_capacity) {
    // Try to allocate from PSRAM first — allows a large look-ahead buffer
    size_t bytes = frame_capacity * 2 * sizeof(int32_t);

    m_ringbuf = xRingbufferCreateWithCaps(bytes, RINGBUF_TYPE_BYTEBUF,
                                           MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (m_ringbuf) {
        m_uses_caps      = true;
        m_capacity_frames = frame_capacity;
        ESP_LOGI(TAG, "Ring buffer: %u frames (%u KB, PSRAM)",
                 (unsigned)frame_capacity, (unsigned)(bytes / 1024));
    } else {
        // Fall back to SRAM with a smaller buffer
        ESP_LOGW(TAG, "PSRAM unavailable, falling back to %u-frame SRAM buffer",
                 (unsigned)SRAM_FALLBACK_FRAMES);
        bytes     = SRAM_FALLBACK_FRAMES * 2 * sizeof(int32_t);
        m_ringbuf = xRingbufferCreate(bytes, RINGBUF_TYPE_BYTEBUF);
        m_capacity_frames = SRAM_FALLBACK_FRAMES;
        if (m_ringbuf) {
            ESP_LOGI(TAG, "Ring buffer: %u frames (%u KB, SRAM)",
                     (unsigned)SRAM_FALLBACK_FRAMES, (unsigned)(bytes / 1024));
        } else {
            ESP_LOGE(TAG, "Ring buffer allocation failed entirely");
            return;
        }
    }

    // Query the actual usable capacity (may differ slightly from requested due to alignment)
    m_capacity_bytes = xRingbufferGetCurFreeSize(m_ringbuf);
}

AudioBuffer::~AudioBuffer() {
    if (!m_ringbuf) return;
    if (m_uses_caps) {
        vRingbufferDeleteWithCaps(m_ringbuf);
    } else {
        vRingbufferDelete(m_ringbuf);
    }
}

size_t AudioBuffer::write(const int32_t* frames, size_t frame_count) {
    if (!m_ringbuf || frame_count == 0) return 0;

    size_t bytes = frame_count * 2 * sizeof(int32_t);
    if (xRingbufferSend(m_ringbuf, frames, bytes, 0) == pdTRUE) {
        return frame_count;
    }
    return 0;
}

size_t AudioBuffer::read(int32_t* out, size_t frame_count, TickType_t timeout) {
    if (!m_ringbuf || frame_count == 0) return 0;

    size_t max_bytes = frame_count * 2 * sizeof(int32_t);
    size_t received_bytes = 0;

    void* item = xRingbufferReceiveUpTo(m_ringbuf, &received_bytes, timeout, max_bytes);
    if (!item) return 0;

    size_t frames = received_bytes / (2 * sizeof(int32_t));
    memcpy(out, item, frames * 2 * sizeof(int32_t));
    vRingbufferReturnItem(m_ringbuf, item);

    return frames;
}

size_t AudioBuffer::watermark_frames() const {
    constexpr size_t ONE_SECOND = 44100;
    return std::min(ONE_SECOND, m_capacity_frames * 3 / 4);
}

void AudioBuffer::write_paced(const int32_t* frames, size_t frame_count) {
    if (!m_ringbuf || frame_count == 0) return;

    const size_t watermark = watermark_frames();

    while (frame_count > 0) {
        while (used_frames() >= watermark) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        size_t n = std::min(frame_count, watermark - used_frames());
        if (n == 0) n = 1;  // guard against edge case

        if (write(frames, n) > 0) {
            frames      += n * 2;
            frame_count -= n;
        }
    }
}

size_t AudioBuffer::used_frames() const {
    if (!m_ringbuf || m_capacity_bytes == 0) return 0;

    size_t free_bytes = xRingbufferGetCurFreeSize(m_ringbuf);
    if (free_bytes >= m_capacity_bytes) return 0;

    return (m_capacity_bytes - free_bytes) / (2 * sizeof(int32_t));
}
