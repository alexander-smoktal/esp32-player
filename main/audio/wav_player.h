#pragma once

#include <memory>
#include <string>
#include <cstdint>

class FlashStorage;
class AudioPlayer;

/* Plays WAV files from a FlashStorage partition into AudioPlayer's ring buffer.
   Initialized once; shared across states as shared_ptr<WavPlayer>.

   Expected format: PCM, 44100 Hz, 16-bit, mono.
   Converts: s16 mono → s32 stereo (left-shifts 16 bits, duplicates L=R).
*/
class WavPlayer {
public:
    WavPlayer(std::shared_ptr<FlashStorage> flash, std::shared_ptr<AudioPlayer> audio);

    // Play a WAV file synchronously — blocks until all samples are written to the buffer.
    // Returns the clip duration in milliseconds on success.
    // Returns 0 on error (file not found, wrong format, etc.) — caller should use a fallback delay.
    uint32_t play(const std::string& filename);

private:
    struct FmtChunk {
        char     id[4];
        uint32_t size;
        uint16_t audio_format;
        uint16_t num_channels;
        uint32_t sample_rate;
        uint32_t byte_rate;
        uint16_t block_align;
        uint16_t bits_per_sample;
    } __attribute__((packed));

    bool validate_fmt(const FmtChunk& fmt, const std::string& filename) const;

    std::shared_ptr<FlashStorage> m_flash;
    std::shared_ptr<AudioPlayer> m_audio;
};
