#ifndef __BLE_TESLA_H__
#define __BLE_TESLA_H__

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>

#include <globals.h>
#include "core/display.h"

struct TeslaInfo {
    String address;
    int rssi;
    unsigned long lastSeen;
    String name;
};

class BLETesla {
public:
    void setup();
    void loop();

private:
    void drawMainBorder();
};

#endif
