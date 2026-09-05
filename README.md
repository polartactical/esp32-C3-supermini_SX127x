# esp32-C3-supermini_SX127x
Meschore configuration for ESP32 Supermini C3 with SX127x LoRa

<h2>Work in progress! use at own risk</h2>

Wiring

SCK   -> 4,\
MISO  -> 5,\
MOSI  -> 6,\
SS    -> 7,\
RST   -> 3,\
DIO0  -> 1,\
VCC   -> 3V3,\
GND   -> GND,\
USER_BTN  -> 10,

Note special wiring needed.

Create a new folder inside ./meshcore-main/variants/ called "esp32-C3-supermini_SX1278
Choose the correct env inside PlatformIO and compile.

TODO: Finish adding a advert button and test functions.
