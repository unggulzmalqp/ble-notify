# Send and Recieve data using BLE on ESP32
This project intents retrieve from sensors and send it via BLE.
## Todo
- [x] Send Data
- [x] Receive Data
- [ ] Timestamp data going to be sent

## How to build PlatformIO based project
1. [Install PlatformIO Core](http://docs.platformio.org/page/core.html)
2. Download [development platform with examples](https://github.com/platformio/platform-espressif32/archive/develop.zip)
3. Extract ZIP archive
4. Run these commands:
   * Change directory
   > cd ble-notify
   * Build project
   > platformio run
   * Upload firmware
   > platformio run --target upload
   * Build specific environment
   > platformio run -e esp32dev
   * Upload firmware for the specific environment
   > platformio run -e esp32dev --target upload
   * Clean build files
   > platformio run --target clean

