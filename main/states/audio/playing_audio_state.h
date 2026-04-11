#pragma once

#include <memory>

#include "states/state.h"

class AudioPlayer;

// Passthrough state: silence by default; bluetooth writes audio via AudioPlayer::write_bt_audio().
class PlayingAudioState : public State {
public:
    explicit PlayingAudioState(std::shared_ptr<AudioPlayer> audio);

    bool enter() override;
    void exit() override;

private:
    std::shared_ptr<AudioPlayer> m_audio;
};
