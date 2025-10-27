/*
    Tesla BLE Scanner for Bruce
    Based on Esp32vsEvil's TeslaScanner: https://github.com/Esp32vsEvil/TeslaScanner
    Modified by dudgy

    Note: This is experimental software for educational purposes.
    Detection may not work reliably and results are not guaranteed.
*/

#include "ble_tesla.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include <map>
#include <vector>

#define SCAN_INTERVAL 100
#define SCAN_WINDOW 99
#define RSSI_TIMEOUT 5000

static NimBLEScan* pBLEScan;
static bool teslaScanning = false;
static bool scanCancelled = false;

std::map<String, TeslaInfo> activeTeslas;

class TeslaAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        if (scanCancelled) return;

        if (advertisedDevice->haveName()) {
            String deviceName = String(advertisedDevice->getName().c_str());

            /*
            Detect Tesla devices by name pattern assumes:
            - Model S: Names starting with 'S' and 18 characters long ending with 'C'
            - Other Models: Names starting with 'Tesla'
            */
            if ((deviceName.length() >= 18 && deviceName.charAt(0) == 'S' && deviceName.charAt(17) == 'C') ||
                deviceName.startsWith("Tesla")) {

                String address = advertisedDevice->getAddress().toString().c_str();
                int rssi = advertisedDevice->getRSSI();

                // Update or add to active Teslas
                activeTeslas[address] = { address, rssi, millis(), deviceName };
            }
        }
    }
};

void BLETesla::drawMainBorder() {
    ::drawMainBorder(); // Use global function
    tft.drawString("-=Tesla Scanner=-", (tftWidth / 2) - ((16 * 6) / 2), 12);
}

void updateTeslaDisplay() {
    static unsigned long lastUpdate = 0;
    static size_t lastTeslaCount = 0;

    // Only update if enough time has passed OR Tesla count changed
    if (millis() - lastUpdate < 1000 && activeTeslas.size() == lastTeslaCount) {
        return;
    }

    lastUpdate = millis();
    lastTeslaCount = activeTeslas.size();

    // Clear the content area
    tft.fillRect(10, 35, tftWidth - 20, tftHeight - 50, bruceConfig.bgColor);

    // Display active Teslas with signal strength
    int yPos = 35;
    for (const auto& tesla : activeTeslas) {
        String displayText = tesla.second.name + " " + String(tesla.second.rssi) + "dBm";

        // Visual signal strength indicator
        int signalBars = map(constrain(tesla.second.rssi, -100, -50), -100, -50, 1, 5);
        String strength = " [";
        for (int i = 0; i < signalBars; i++) strength += "|";
        for (int i = signalBars; i < 5; i++) strength += " ";
        strength += "]";

        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.drawString(displayText + strength, 10, yPos);
        yPos += 15;

        if (yPos > tftHeight - 20) break;
    }

    if (activeTeslas.empty()) {
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.drawString("No Teslas in range", 10, 35);
    }
}

void BLETesla::setup() {
    tft.setTextSize(1);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new TeslaAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(SCAN_INTERVAL);
    pBLEScan->setWindow(SCAN_WINDOW);
}

void cleanupOldTeslas() {
    unsigned long currentTime = millis();
    for (auto it = activeTeslas.begin(); it != activeTeslas.end(); ) {
        if (currentTime - it->second.lastSeen > RSSI_TIMEOUT) {
            it = activeTeslas.erase(it);  // ← Properly remove old entries
        }
        else {
            ++it;
        }
    }
}

void BLETesla::loop() {
    drawMainBorder();
    displayTextLine("Tesla Radar Starting...");
    delay(1000);

    while (!check(EscPress)) {
        // BLE scan
        pBLEScan->start(1, false);

        // Update display
        updateTeslaDisplay();

        delay(100);
    }

    // Cleanup
    displayTextLine("Exiting...");
    delay(500);

    pBLEScan->stop();
    pBLEScan->clearResults();
    NimBLEDevice::deinit(true);
    activeTeslas.clear();
}
