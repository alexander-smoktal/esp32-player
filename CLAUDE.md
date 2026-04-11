# ESP32 Player

## Hardware
- **MCU:** ESP32-S3
- **DAC:** PCM5102 (I2S output, no MCLK needed)
- **I2S GPIO:** BCLK=7, WS(LRCK)=5, DOUT=6

## Build
- ESP-IDF project (CMake-based, `idf.py` toolchain)
- Build: `idf.py build`
- Flash: `idf.py flash`
- Monitor: `idf.py monitor`
- Clean: `idf.py fullclean`

## Project structure
- `main/main.cpp` — application entry point (`app_main`)
- `sdkconfig.defaults` — C++ RTTI enabled, 4MB flash
- Minimal build mode (`MINIMAL_BUILD ON`)

## Conventions
- C++ with `extern "C"` linkage for ESP-IDF C headers and `app_main`
- I2S driver: new `driver/i2s_std.h` API (not legacy `driver/i2s.h`)
- Audio: 44100 Hz sample rate, 16-bit stereo, Philips I2S standard
