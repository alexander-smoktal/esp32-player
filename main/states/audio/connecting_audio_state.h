#pragma once

#include <memory>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "states/state.h"

class WavPlayer;

// Plays connecting.wav on repeat to indicate a connection attempt.
// Silence between repeats equals the clip duration (automatic from file metadata).
class ConnectingAudioState : public State {
public:
    explicit ConnectingAudioState(std::shared_ptr<WavPlayer> wav_player);

    bool enter() override;
    void exit() override;

private:
    static void play_task(void* ctx);

    std::shared_ptr<WavPlayer> m_wav_player;
    TaskHandle_t m_task = nullptr;
};
