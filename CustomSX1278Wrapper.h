#pragma once

#include "CustomSX1278.h"
#include <helpers/radiolib/RadioLibWrappers.h>

#ifndef USE_SX1278
#define USE_SX1278
#endif

class CustomSX1278Wrapper : public RadioLibWrapper {
public:
    CustomSX1278Wrapper(CustomSX1278& radio, mesh::MainBoard& board)
        : RadioLibWrapper(radio, board) {}

    void setParams(float freq, float bw, uint8_t sf, uint8_t cr) override {
        auto* radio = static_cast<CustomSX1278*>(_radio);

        radio->setFrequency(freq);
        radio->setSpreadingFactor(sf);
        radio->setBandwidth(bw);
        radio->setCodingRate(cr);

        updatePreamble(sf);
    }

    bool isReceivingPacket() override {
        return static_cast<CustomSX1278*>(_radio)->isReceiving();
    }

    float getCurrentRSSI() override {
        return static_cast<CustomSX1278*>(_radio)->getRSSI(false);
    }

    float getLastRSSI() const override {
        return static_cast<CustomSX1278*>(_radio)->getRSSI();
    }

    float getLastSNR() const override {
        return static_cast<CustomSX1278*>(_radio)->getSNR();
    }

    float packetScore(float snr, int packet_len) override {
        auto* radio = static_cast<CustomSX1278*>(_radio);
        return packetScoreInt(snr, radio->spreadingFactor, packet_len);
    }

    uint8_t getSpreadingFactor() const override {
        return static_cast<CustomSX1278*>(_radio)->spreadingFactor;
    }
};
