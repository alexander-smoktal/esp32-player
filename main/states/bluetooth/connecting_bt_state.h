#pragma once

#include <memory>

#include "states/state.h"

class Bluetooth;

class ConnectingBtState : public State {
public:
    explicit ConnectingBtState(std::shared_ptr<Bluetooth> bt);

    bool enter() override;
    void exit() override;

private:
    std::shared_ptr<Bluetooth> m_bt;
};
