set(SOURCES
    "main.cpp"
    "flash_storage.cpp"
    "audio/audio_buffer.cpp"
    "audio/audio_player.cpp"
    "audio/wav_player.cpp"
    "bluetooth/bluetooth.cpp"
)

include(${CMAKE_CURRENT_LIST_DIR}/states/States.cmake)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "."
                       REQUIRES driver esp_ringbuf fatfs)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_23)

fatfs_create_rawflash_image(sounds "sounds" FLASH_IN_PROJECT)
