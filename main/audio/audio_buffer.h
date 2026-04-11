#pragma once

#include <cstddef>

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"

/* Thread-safe ring buffer for stereo 32-bit PCM audio frames.
   One frame = left sample (int32_t) + right sample (int32_t).
   Uses ESP-IDF byte ring buffer internally.

   Tries to allocate from PSRAM first (for a large 2-second buffer);
   falls back to a small SRAM buffer if PSRAM is unavailable.
*/
class AudioBuffer {
public:
    // Requests frame_capacity frames. If PSRAM allocation fails the actual
    // capacity falls back to SRAM_FALLBACK_FRAMES.
    explicit AudioBuffer(size_t frame_capacity = 44100 * 2);  // request 2 seconds
    ~AudioBuffer();

    // Write stereo frames; non-blocking, returns frames actually written (0 if full)
    size_t write(const int32_t* frames, size_t frame_count);

    // Write stereo frames with occupancy-based pacing.
    // Blocks (10 ms polls) until used_frames() drops below watermark_frames(),
    // then writes the chunk. Use this for streaming playback to avoid overflow.
    void write_paced(const int32_t* frames, size_t frame_count);

    // Read up to frame_count frames into out; returns frames actually read
    size_t read(int32_t* out, size_t frame_count, TickType_t timeout = pdMS_TO_TICKS(10));

    // Frames currently queued (produced but not yet consumed)
    size_t used_frames() const;

    // Total buffer capacity in frames (may be less than requested if PSRAM unavailable)
    size_t capacity_frames() const { return m_capacity_frames; }

    // High-watermark for write_paced(): 1 second if PSRAM buffer, else 3/4 of capacity
    size_t watermark_frames() const;

private:
    static constexpr size_t SRAM_FALLBACK_FRAMES = 8192;  // ~185 ms

    RingbufHandle_t m_ringbuf        = nullptr;
    bool            m_uses_caps      = false;   // true → must use vRingbufferDeleteWithCaps
    size_t          m_capacity_frames = 0;
    size_t          m_capacity_bytes  = 0;      // actual usable bytes, queried after creation
};
