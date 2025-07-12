#pragma once
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

/**
 * @brief Handles HTTP GET requests for measurements.
 * @param request The web server request pointer.
 */
void handleMeasurementsRequest(AsyncWebServerRequest *request);

/**
 * @brief Handles HTTP GET requests for settings.
 * @param request The web server request pointer.
 */
void handleSettingsRequest(AsyncWebServerRequest *request);

/**
 * @brief Handles HTTP PUT requests to update settings.
 * @param request The web server request pointer.
 * @param data The data buffer received.
 * @param len The length of the data buffer.
 * @param index The index of the current data chunk.
 * @param total The total size of the data to be received.
 */
void handleSettingsRequestPut(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
