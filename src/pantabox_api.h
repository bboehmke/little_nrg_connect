#pragma once
#include <ESPAsyncWebServer.h>
void handlePantaboxChargerState(AsyncWebServerRequest *request);
void handlePantaboxChargerEnabled(AsyncWebServerRequest *request);
void handlePantaboxMeterPower(AsyncWebServerRequest *request);
void handlePantaboxChargerMaxCurrent(AsyncWebServerRequest *request);
// Set charger enable/disable (POST /pantabox/[MAC]/api/charger/enable)
void handlePantaboxChargerEnableSet(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
// Set charger max current (POST /pantabox/[MAC]/api/charger/current)
void handlePantaboxChargerCurrentSet(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
