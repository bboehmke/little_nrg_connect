#include "pantabox_api.h"
#include "ble_utils.h"

void handlePantaboxChargerState(AsyncWebServerRequest *request) {
    String mac = request->pathArg(0);
    Serial.print("pantabox state request for ");
    Serial.println(mac);

    BLEDevice device = scanForTargetDevice(mac);
    if (!device) {
        request->send(500, "application/json", "{\"Message\":\"Device not found\"}");
        return;
    }
    if (!connectToDevice(device)) {
        request->send(500, "application/json", "{\"Message\":\"Device connect failed\"}");
        return;
    }
    
    BLECharacteristic powerChar = device.characteristic(POWER_SERVICE);
    if (!(powerChar && powerChar.canRead() && powerChar.read())) {
        request->send(500, "application/json", "{\"Message\":\"Power characteristic not found or not readable\"}");
        return;
    }
    Power power = convertPower(powerChar.value());

    String state;
    switch (power.CPSignal) {
        case 4:
            state = "A";
            break;
        case 3:
            state = "B";
            break;
        case 2:
            state = "C";
            break;
        default:
            state = "A";
            break;
    }
    String json = String("{\"state\": \"") + state + "\"}";
    request->send(200, "application/json", json);
}

void handlePantaboxChargerEnabled(AsyncWebServerRequest *request) {
    String mac = request->pathArg(0);
    Serial.print("pantabox enabled request for ");
    Serial.println(mac);

    BLEDevice device = scanForTargetDevice(mac);
    if (!device) {
        request->send(500, "application/json", "{\"Message\":\"Device not found\"}");
        return;
    }
    if (!connectToDevice(device)) {
        request->send(500, "application/json", "{\"Message\":\"Device connect failed\"}");
        return;
    }
    
    BLECharacteristic infoChar = device.characteristic(INFO_SERVICE);
    if (!(infoChar && infoChar.canRead() && infoChar.read())) {
        request->send(500, "application/json", "{\"Message\":\"Info characteristic not found or not readable\"}");
        return;
    }
    Info info = convertInfo(infoChar.value());

    String json = String("{\"enabled\": \"") + ((info.PauseCharging == 0) ? "1" : "0") + "\"}";
    request->send(200, "application/json", json);
}

void handlePantaboxMeterPower(AsyncWebServerRequest *request) {
    String mac = request->pathArg(0);
    Serial.print("pantabox power request for ");
    Serial.println(mac);

    BLEDevice device = scanForTargetDevice(mac);
    if (!device) {
        request->send(500, "application/json", "{\"Message\":\"Device not found\"}");
        return;
    }
    if (!connectToDevice(device)) {
        request->send(500, "application/json", "{\"Message\":\"Device connect failed\"}");
        return;
    }
    
    BLECharacteristic powerChar = device.characteristic(POWER_SERVICE);
    if (!(powerChar && powerChar.canRead() && powerChar.read())) {
        request->send(500, "application/json", "{\"Message\":\"Power characteristic not found or not readable\"}");
        return;
    }
    Power power = convertPower(powerChar.value());

    String json = String("{\"power\": \"") + String(power.TotalPower) + "\"}";
    request->send(200, "application/json", json);
}

void handlePantaboxChargerMaxCurrent(AsyncWebServerRequest *request) {
    String mac = request->pathArg(0);
    Serial.print("pantabox max current request for ");
    Serial.println(mac);

    BLEDevice device = scanForTargetDevice(mac);
    if (!device) {
        request->send(500, "application/json", "{\"Message\":\"Device not found\"}");
        return;
    }
    if (!connectToDevice(device)) {
        request->send(500, "application/json", "{\"Message\":\"Device connect failed\"}");
        return;
    }
    
    BLECharacteristic infoChar = device.characteristic(INFO_SERVICE);
    if (!(infoChar && infoChar.canRead() && infoChar.read())) {
        request->send(500, "application/json", "{\"Message\":\"Info characteristic not found or not readable\"}");
        return;
    }
    Info info = convertInfo(infoChar.value());

    String json = String("{\"maxCurrent\": \"") + String(info.Current) + "\"}";
    request->send(200, "application/json", json);
}

void handlePantaboxChargerEnableSet(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String mac = request->pathArg(0);
    String pin = request->pathArg(1);

    String body;
    if (index == 0) body = "";
    body += String((const char*)data, len);
    if (index + len != total) return;

    Serial.print("pantabox (POST) set enable request for ");
    Serial.println(mac);

    BLEDevice device = scanForTargetDevice(mac);
    if (!device) {
        Serial.println("Device not found");
        request->send(500, "application/json", "{\"Message\":\"Device not found\"}");
        return;
    }
    if (!connectToDevice(device)) {
        Serial.println("Device connect failed");
        request->send(500, "application/json", "{\"Message\":\"Device connect failed\"}");
        return;
    }
    BLECharacteristic infoChar = device.characteristic(INFO_SERVICE);
    if (!(infoChar && infoChar.canRead() && infoChar.read())) {
        Serial.println("Info characteristic not found or not readable");
        request->send(500, "application/json", "{\"Message\":\"Info characteristic not writable\"}");
        return;
    }
    // Prepare Info struct and set PauseCharging
    Info info = convertInfo(infoChar.value());

    Settings setSettings = convertToSettings(info, pin.toInt());
    setSettings.PauseCharging = (body == "true") ? 0 : 1;

    BLECharacteristic settingsChar = device.characteristic(SETTINGS_SERVICE);
    if (!(settingsChar && settingsChar.canWrite())) {
        Serial.println("Settings characteristic not found or not writable");
        request->send(500, "application/json", "{\"Message\":\"Settings characteristic not found or not writable\"}");
        return;
    }
    if (!settingsChar.writeValue((uint8_t*)&setSettings, sizeof(setSettings))) {
        Serial.println("Failed to write settings");
        request->send(500, "application/json", "{\"Message\":\"Failed to write settings\"}");
        return;
    }
    request->send(200, "application/json", "{\"success\":true}");
}

void handlePantaboxChargerCurrentSet(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String mac = request->pathArg(0);
    String pin = request->pathArg(1);

    String body;
    if (index == 0) body = "";
    body += String((const char*)data, len);
    if (index + len != total) return;

    Serial.print("pantabox (POST) set current request for ");
    Serial.println(mac);

    BLEDevice device = scanForTargetDevice(mac);
    if (!device) {
        Serial.println("Device not found");
        request->send(500, "application/json", "{\"Message\":\"Device not found\"}");
        return;
    }
    if (!connectToDevice(device)) {
        Serial.println("Device connect failed");
        request->send(500, "application/json", "{\"Message\":\"Device connect failed\"}");
        return;
    }
    BLECharacteristic infoChar = device.characteristic(INFO_SERVICE);
    if (!(infoChar && infoChar.canRead() && infoChar.read())) {
        Serial.println("Info characteristic not found or not readable");
        request->send(500, "application/json", "{\"Message\":\"Info characteristic not writable\"}");
        return;
    }
    // Prepare Info struct and set PauseCharging
    Info info = convertInfo(infoChar.value());

    Settings setSettings = convertToSettings(info, pin.toInt());
    setSettings.Current = body.toInt();

    if (setSettings.Current < 6 || setSettings.Current > 32) {
        request->send(400, "application/json", "{\"Message\":\"Invalid current value\"}");
        return;
    }

    BLECharacteristic settingsChar = device.characteristic(SETTINGS_SERVICE);
    if (!(settingsChar && settingsChar.canWrite())) {
        Serial.println("Settings characteristic not found or not writable");
        request->send(500, "application/json", "{\"Message\":\"Settings characteristic not found or not writable\"}");
        return;
    }
    if (!settingsChar.writeValue((uint8_t*)&setSettings, sizeof(setSettings))) {
        Serial.println("Failed to write settings");
        request->send(500, "application/json", "{\"Message\":\"Failed to write settings\"}");
        return;
    }
    request->send(200, "application/json", "{\"success\":true}");
}
