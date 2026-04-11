set(STATES_SOURCES
    "states/state.cpp"
    "states/state_manager.cpp"
    "states/state_subsystem.cpp"
    "states/audio/pairing_audio_state.cpp"
    "states/audio/connecting_audio_state.cpp"
    "states/audio/playing_audio_state.cpp"
    "states/bluetooth/pairing_bt_state.cpp"
    "states/bluetooth/connecting_bt_state.cpp"
    "states/bluetooth/playing_bt_state.cpp"
)

list(APPEND SOURCES ${STATES_SOURCES})
