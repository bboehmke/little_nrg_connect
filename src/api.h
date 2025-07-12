#pragma once
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

void handleMeasurementsRequest(AsyncWebServerRequest *request);
void handleSettingsRequest(AsyncWebServerRequest *request);
void handleSettingsRequestPut(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
