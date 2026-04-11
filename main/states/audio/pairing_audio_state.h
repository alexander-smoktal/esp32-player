#pragma once

#include <memory>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "states/state.h"

class WavPlayer;

// Plays pairing.wav on repeat to indicate pairing mode.
// Silence between repeats equals the clip duration (automatic from file metadata).
class PairingAudioState : public State {
public:
    explicit PairingAudioState(std::shared_ptr<WavPlayer> wav_player);

    bool enter() override;
    void exit() override;

private:
    static void play_task(void* ctx);

    std::shared_ptr<WavPlayer> m_wav_player;
    TaskHandle_t m_task = nullptr;
};
