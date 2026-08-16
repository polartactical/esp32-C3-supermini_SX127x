#pragma once

#include <helpers/ESP32Board.h>
#include <Arduino.h>

class SuperMiniC3Board : public ESP32Board {
public:
    void begin() {
        ESP32Board::begin();
    }

    uint16_t getBattMilliVolts() override {
        return 0; // No battery measurement on the supplied wiring.
    }

    const char* getManufacturerName() const override {
        return "ESP32-C3 SuperMini";
    }
};
