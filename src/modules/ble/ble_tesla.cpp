#include "ble_tesla.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include <functional>
#include <vector>

#define SCAN_TIME 5
#define SCAN_INTERVAL 100
#define SCAN_WINDOW 99

static NimBLEScan* pBLEScan;
static bool teslaScanning = false;
static bool scanCancelled = false;

void displayTeslaInfo(String name, String address, int rssi) {
    ::drawMainBorder();
    tft.setTextColor(bruceConfig.priColor);
    tft.drawCentreString("-=Tesla Found=-", tftWidth / 2, 28, SMOOTH_FONT);
    tft.drawString("Name: " + name, 10, 48);
    tft.drawString("Address: " + address, 10, 66);
    tft.drawString("Signal: " + String(rssi) + " dBm", 10, 84);
    tft.drawCentreString("Press " + String(BTN_ALIAS) + " to continue", tftWidth / 2, tftHeight - 20, 1);

    delay(300);
    while (!check(SelPress)) { delay(50); }
}

class TeslaAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        if (scanCancelled) return;

        if (advertisedDevice->haveName()) {
            String deviceName = String(advertisedDevice->getName().c_str());

            // Tesla detection logic
            if ((deviceName.length() >= 18 && deviceName.charAt(0) == 'S' && deviceName.charAt(17) == 'C') ||
                deviceName.startsWith("Tesla")) {

                String address = advertisedDevice->getAddress().toString().c_str();
                int rssi = advertisedDevice->getRSSI();

                // Use the static method
                options.emplace_back(deviceName.c_str(), [=]() {
                    displayTeslaInfo(deviceName, address, rssi);
                    });
            }
        }
    }
};
BLETesla::BLETesla() { setup(); }

BLETesla::~BLETesla() {
    if (teslaScanning) {
        scanCancelled = true;
        pBLEScan->stop();
        teslaScanning = false;
    }
    pBLEScan->clearResults();
}

void BLETesla::setup() {
    tft.setTextSize(1);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(SCAN_INTERVAL);
    pBLEScan->setWindow(SCAN_WINDOW);

    delay(100);
    loop();
}

void BLETesla::drawMainBorder() {
    ::drawMainBorder(); // Use global function
    tft.drawString("-=Tesla Scanner=-", (tftWidth / 2) - ((16 * 6) / 2), 12);
}

void BLETesla::loop() {
    while (!check(EscPress)) {
        scanCancelled = false;
        options.clear();

        drawMainBorder();
        displayTextLine("Scanning for Teslas...");
        displayTextLine("Press ESC to cancel", 2);

        // Setup callback for Tesla detection
        pBLEScan->setAdvertisedDeviceCallbacks(new TeslaAdvertisedDeviceCallbacks(), true);

        // Start scan and check for ESC periodically
        teslaScanning = true;
        pBLEScan->start(0, true); // Start scanning indefinitely (we'll manage the timing)

        unsigned long startTime = millis();
        while ((millis() - startTime) < (SCAN_TIME * 1000) && !scanCancelled) {
            if (check(EscPress)) {
                scanCancelled = true;
                break;
            }
            delay(100); // Check for ESC every 100ms
        }

        // Stop scanning
        pBLEScan->stop();
        teslaScanning = false;

        if (scanCancelled) {
            displayTextLine("Scan cancelled");
            delay(1000);
            return;
        }

        if (options.empty()) {
            displayTextLine("No Teslas found. Retry...");
            delay(2000);
            pBLEScan->clearResults();
            continue;
        }

        // Add navigation options
        options.push_back({ "Scan Again", [&]() {} });
        options.push_back({ "Main Menu", [&]() { returnToMenu = true; } });

        bool returnToMenu = false;
        loopOptions(options);

        if (returnToMenu) return;

        pBLEScan->clearResults();
    }
}
