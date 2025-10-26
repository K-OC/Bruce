#ifndef __BLE_TESLA_H__
#define __BLE_TESLA_H__

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>

#include <globals.h>
#include "core/display.h"

class BLETesla {
public:
    BLETesla();
    ~BLETesla();

    void setup();
    void loop();

private:
    void drawMainBorder();
};

#endif
