#include "wav_player.h"

#include <cstring>
#include <algorithm>

#include "esp_log.h"

#include "flash_storage.h"
#include "audio_player.h"

static constexpr auto TAG = "WavPlayer";

// Minimal RIFF header
struct RiffHeader {
    char     riff[4];     // "RIFF"
    uint32_t chunk_size;
    char     wave[4];     // "WAVE"
} __attribute__((packed));


static constexpr int EXPECTED_SAMPLE_RATE = 44100;
static constexpr int EXPECTED_BITS        = 16;
static constexpr int EXPECTED_CHANNELS    = 1;
static constexpr size_t CHUNK_FRAMES      = 256;

WavPlayer::WavPlayer(std::shared_ptr<FlashStorage> flash,
                     std::shared_ptr<AudioPlayer> audio)
    : m_flash(std::move(flash))
    , m_audio(std::move(audio)) {}

bool WavPlayer::validate_fmt(const FmtChunk& fmt, const std::string& filename) const {
    if (fmt.audio_format != 1) {
        ESP_LOGE(TAG, "'%s': not PCM (format=%u)", filename.c_str(), fmt.audio_format);
        return false;
    }
    if (fmt.sample_rate != EXPECTED_SAMPLE_RATE) {
        ESP_LOGE(TAG, "'%s': sample rate %u (expected %d)",
                 filename.c_str(), fmt.sample_rate, EXPECTED_SAMPLE_RATE);
        return false;
    }
    if (fmt.bits_per_sample != EXPECTED_BITS) {
        ESP_LOGE(TAG, "'%s': %u-bit (expected %d)",
                 filename.c_str(), fmt.bits_per_sample, EXPECTED_BITS);
        return false;
    }
    if (fmt.num_channels != EXPECTED_CHANNELS) {
        ESP_LOGE(TAG, "'%s': %u channels (expected mono)", filename.c_str(), fmt.num_channels);
        return false;
    }
    return true;
}

uint32_t WavPlayer::play(const std::string& filename) {
    auto file = m_flash->open_file(filename);
    if (!file.is_open()) {
        return 0;
    }

    // --- Read RIFF header ---
    RiffHeader riff{};
    file.read(reinterpret_cast<char*>(&riff), sizeof(riff));
    if (!file) {
        ESP_LOGE(TAG, "'%s': failed to read RIFF header", filename.c_str());
        return 0;
    }

    if (memcmp(riff.riff, "RIFF", 4) != 0 ||
        memcmp(riff.wave, "WAVE", 4) != 0) {
        ESP_LOGE(TAG, "'%s': not a valid WAV file", filename.c_str());
        return 0;
    }

    // --- Find fmt chunk ---
    FmtChunk fmt{};
    bool fmt_found = false;

    while (file.read(reinterpret_cast<char*>(&fmt.id), 4)) {
        file.read(reinterpret_cast<char*>(&fmt.size), 4);

        if (memcmp(fmt.id, "fmt ", 4) == 0) {
            file.read(reinterpret_cast<char*>(&fmt.audio_format),
                      fmt.size);

            fmt_found = true;
            break;
        } else {
            // skip chunk
            file.seekg(fmt.size, std::ios::cur);
        }

        // word alignment
        if (fmt.size % 2 != 0) {
            file.seekg(1, std::ios::cur);
        }
    }

    if (!fmt_found) {
        ESP_LOGE(TAG, "'%s': fmt chunk not found", filename.c_str());
        return 0;
    }

    if (!validate_fmt(fmt, filename)) return 0;

    // --- Find data chunk ---
    char chunk_id[4];
    uint32_t chunk_size = 0;
    bool data_found = false;

    while (file.read(chunk_id, 4)) {
        file.read(reinterpret_cast<char*>(&chunk_size), 4);

        if (memcmp(chunk_id, "data", 4) == 0) {
            data_found = true;
            break;
        }

        // skip chunk
        file.seekg(chunk_size, std::ios::cur);

        // word alignment
        if (chunk_size % 2 != 0) {
            file.seekg(1, std::ios::cur);
        }
    }

    if (!data_found) {
        ESP_LOGE(TAG, "'%s': data chunk not found", filename.c_str());
        return 0;
    }

    uint32_t data_size = chunk_size;
    uint32_t duration_ms =
        (uint32_t)(((uint64_t)data_size * 1000) / fmt.byte_rate);

    ESP_LOGI(TAG,
             "Playing '%s': %u Hz, %u-bit, mono, %u bytes (~%u ms)",
             filename.c_str(),
             fmt.sample_rate,
             fmt.bits_per_sample,
             data_size,
             duration_ms);

    // --- Stream PCM ---
    AudioBuffer& buf = m_audio->buffer();

    int16_t in_buf[CHUNK_FRAMES];
    int32_t out_buf[CHUNK_FRAMES * 2];

    uint32_t frames_remaining = data_size / sizeof(int16_t);

    while (frames_remaining > 0) {
        size_t n = std::min((size_t)frames_remaining, CHUNK_FRAMES);

        file.read(reinterpret_cast<char*>(in_buf), n * sizeof(int16_t));
        size_t got = (size_t)file.gcount() / sizeof(int16_t);
        if (got == 0) break;

        for (size_t i = 0; i < got; i++) {
            int32_t s = (int32_t)in_buf[i] << 16;
            out_buf[2 * i]     = s;
            out_buf[2 * i + 1] = s;
        }

        buf.write_paced(out_buf, got);
        frames_remaining -= (uint32_t)got;
    }

    file.close();
    return duration_ms;
}