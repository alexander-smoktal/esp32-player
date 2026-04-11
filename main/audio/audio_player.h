#pragma once

#include <memory>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"

#include "audio_buffer.h"

/* Owns the I2S output channel, the audio ring buffer, and the feed task.
   The feed task continuously drains the buffer into I2S, writing silence when empty.

   Audio states write generated sound via buffer().
   Bluetooth writes decoded PCM via write_bt_audio() — AudioPlayer handles s16→s32 conversion.
*/
class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    // For audio states: direct access to the 32-bit stereo ring buffer
    AudioBuffer& buffer() { return m_buffer; }

    // For bluetooth: write raw A2DP PCM (16-bit stereo, 44100 Hz)
    // Converts s16 → s32 internally before writing to the ring buffer
    void write_bt_audio(const int16_t* samples, size_t frame_count);

private:
    static void feed_task(void* ctx);

    i2s_chan_handle_t m_i2s_chan = nullptr;
    AudioBuffer m_buffer;
    TaskHandle_t m_feed_task = nullptr;
};
