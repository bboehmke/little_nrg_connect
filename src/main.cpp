#include <Arduino.h>
#include <ArduinoBLE.h>
#include <ETH.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <map>
#include <ElegantOTA.h>

#include "ble_utils.h"
#include "api.h"

// Ethernet server on port 80
AsyncWebServer server(80);

void handleNotFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found " + request->url());
}

/**
 * @brief Arduino setup function. Initializes serial, Ethernet, BLE, and web server.
 */
void setup() {
    Serial.begin(9600);
    while (!Serial);
    ETH.begin();
    if (!BLE.begin()) {
        Serial.println("starting BLE failed!");
        while (1);
    }
    Serial.println("BLE Central - Starting");
    Serial.print("Waiting for Ethernet connection");
    while (!ETH.linkUp()) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected! IP address: ");
    Serial.println(ETH.localIP());
    server.on("^\\/api\\/measurements\\/(.+)$", HTTP_GET, handleMeasurementsRequest);
    server.on("^\\/api\\/settings\\/(.+)$", HTTP_GET, handleSettingsRequest);
    server.on("^\\/api\\/settings\\/(.+)$", HTTP_PUT, [](AsyncWebServerRequest *request){}, NULL, handleSettingsRequestPut);
    server.onNotFound(handleNotFound);
    ElegantOTA.begin(&server);
    server.begin();
    esp_task_wdt_init(30, true);
}

/**
 * @brief Arduino loop function. Handles OTA updates.
 */
void loop() {
    ElegantOTA.loop();
}
