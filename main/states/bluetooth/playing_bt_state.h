#pragma once

#include <memory>

#include "states/state.h"

class Bluetooth;
class AudioPlayer;

// Pre-wired to AudioPlayer so that write_bt_audio() can be called
// when Bluetooth A2DP data callbacks are implemented.
class PlayingBtState : public State {
public:
    PlayingBtState(std::shared_ptr<Bluetooth> bt, std::shared_ptr<AudioPlayer> audio);

    bool enter() override;
    void exit() override;

private:
    std::shared_ptr<Bluetooth> m_bt;
    std::shared_ptr<AudioPlayer> m_audio;
};
