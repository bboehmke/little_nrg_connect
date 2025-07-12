#pragma once
#include <ESPAsyncWebServer.h>

/**
 * @brief Handles Pantabox charger state requests.
 * @param request The web server request pointer.
 */
void handlePantaboxChargerState(AsyncWebServerRequest *request);

/**
 * @brief Handles Pantabox charger enabled requests.
 * @param request The web server request pointer.
 */
void handlePantaboxChargerEnabled(AsyncWebServerRequest *request);

/**
 * @brief Handles Pantabox meter power requests.
 * @param request The web server request pointer.
 */
void handlePantaboxMeterPower(AsyncWebServerRequest *request);

/**
 * @brief Handles Pantabox charger max current requests.
 * @param request The web server request pointer.
 */
void handlePantaboxChargerMaxCurrent(AsyncWebServerRequest *request);

/**
 * @brief Handles Pantabox charger enable set requests (POST).
 * @param request The web server request pointer.
 * @param data The data buffer received.
 * @param len The length of the data buffer.
 * @param index The index of the current data chunk.
 * @param total The total size of the data to be received.
 */
void handlePantaboxChargerEnableSet(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);

/**
 * @brief Handles Pantabox charger current set requests (POST).
 * @param request The web server request pointer.
 * @param data The data buffer received.
 * @param len The length of the data buffer.
 * @param index The index of the current data chunk.
 * @param total The total size of the data to be received.
 */
void handlePantaboxChargerCurrentSet(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
