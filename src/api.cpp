#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

#include "ble_utils.h"
#include "api.h"

typedef struct {
    char Error[50];
    uint32_t TotalEnergy;
    uint32_t EnergyLastCharge;
    uint16_t ChargingEnergyLimit;
    uint16_t TotalPower;
    uint16_t PowerL1;
    uint16_t PowerL2;
    uint16_t PowerL3;
    uint16_t Frequency;
    int16_t Temperature;
    uint16_t VoltageL1;
    uint16_t VoltageL2;
    uint16_t VoltageL3;
    uint16_t CurrentL1;
    uint16_t CurrentL2;
    uint16_t CurrentL3;
} ApiMeasurements;


ApiMeasurements get_measurements(const String& targetAddress) {
    ApiMeasurements measurements;
    measurements.Error[0] = 0;
    BLEDevice device = scanForTargetDevice(targetAddress);
    if (!device) {
        strcpy(measurements.Error, "not found");
        return measurements;
    }
    if (!connectToDevice(device)) {
        strcpy(measurements.Error, "connect failed");
        return measurements;
    }

    BLECharacteristic energyChar = device.characteristic(ENERGY_SERVICE);
    if (!(energyChar && energyChar.canRead() && energyChar.read())) {
        strcpy(measurements.Error, "energy characteristic read failed");
        return measurements;
    }
    Energy energy = convertEnergy(energyChar.value());
    measurements.TotalEnergy = energy.TotalEnergy;
    measurements.EnergyLastCharge = energy.EnergyLastCharge;
    measurements.ChargingEnergyLimit = energy.ChargingEnergyLimit;

    BLECharacteristic powerChar = device.characteristic(POWER_SERVICE);
    if (!(powerChar && powerChar.canRead() && powerChar.read())) {
        strcpy(measurements.Error, "power characteristic read failed");
        return measurements;
    }
    Power power = convertPower(powerChar.value());
    measurements.TotalPower = power.TotalPower;
    measurements.PowerL1 = power.L1;
    measurements.PowerL2 = power.L2;
    measurements.PowerL3 = power.L3;
    measurements.Frequency = power.Frequency;
    measurements.Temperature = power.Temperature;

    BLECharacteristic vcChar = device.characteristic(VOLTAGE_CURRENT_SERVICE);
    if (!(vcChar && vcChar.canRead() && vcChar.read())) {
        strcpy(measurements.Error, "voltage/current characteristic read failed");
        return measurements;
    }
    VoltageCurrent vc = convertVoltageCurrent(vcChar.value());
    measurements.VoltageL1 = vc.VoltageL1;
    measurements.VoltageL2 = vc.VoltageL2;
    measurements.VoltageL3 = vc.VoltageL3;
    measurements.CurrentL1 = vc.CurrentL1;
    measurements.CurrentL2 = vc.CurrentL2;
    measurements.CurrentL3 = vc.CurrentL3;
    return measurements;
}

void print_measurements(const ApiMeasurements& measurements) {
    if (measurements.Error[0] != 0) {
        Serial.print("Error: ");
        Serial.println(measurements.Error);
    } else {
        Serial.println("Measurements:");
        Serial.print("Total Energy: "); Serial.println(measurements.TotalEnergy / 1000.0);
        Serial.print("Last Charge Energy: "); Serial.println(measurements.EnergyLastCharge / 1000.0);
        Serial.print("Charging Energy Limit: "); Serial.println(measurements.ChargingEnergyLimit / 100.0);
        Serial.print("Total Power: "); Serial.println(measurements.TotalPower / 100.0);
        Serial.print("Power L1: "); Serial.println(measurements.PowerL1 / 100.0);
        Serial.print("Power L2: "); Serial.println(measurements.PowerL2 / 100.0);
        Serial.print("Power L3: "); Serial.println(measurements.PowerL3 / 100.0);
        Serial.print("Frequency: "); Serial.println(measurements.Frequency / 100.0);
        Serial.print("Temperature: "); Serial.println(measurements.Temperature);
        Serial.print("Voltage L1: "); Serial.println(measurements.VoltageL1 / 10.0);
        Serial.print("Voltage L2: "); Serial.println(measurements.VoltageL2 / 10.0);
        Serial.print("Voltage L3: "); Serial.println(measurements.VoltageL3 / 10.0);
        Serial.print("Current L1: "); Serial.println(measurements.CurrentL1 / 100.0);
        Serial.print("Current L2: "); Serial.println(measurements.CurrentL2 / 100.0);
        Serial.print("Current L3: "); Serial.println(measurements.CurrentL3 / 100.0);
    }
}

void measurements2json(const ApiMeasurements& measurements, ArduinoJson::JsonDocument& doc) {
    if (measurements.Error[0] != 0) {
        doc["Message"] = measurements.Error;
        return;
    }
    doc["ChargingCurrentPhase"] = ArduinoJson::JsonArray();
    doc["ChargingCurrentPhase"].add(measurements.CurrentL1 / 100.0);
    doc["ChargingCurrentPhase"].add(measurements.CurrentL2 / 100.0);
    doc["ChargingCurrentPhase"].add(measurements.CurrentL3 / 100.0);
    doc["ChargingEnergy"] = measurements.EnergyLastCharge / 1000.0;
    doc["ChargingEnergyOverAll"] = measurements.TotalEnergy / 1000.0;
    doc["ChargingEnergyPhase"] = ArduinoJson::JsonArray();
    doc["ChargingEnergyPhase"].add(0.0);
    doc["ChargingEnergyPhase"].add(0.0);
    doc["ChargingEnergyPhase"].add(0.0);
    doc["ChargingPower"] = measurements.TotalPower / 100.0;
    doc["ChargingPowerPhase"] = ArduinoJson::JsonArray();
    doc["ChargingPowerPhase"].add(measurements.PowerL1 / 100.0);
    doc["ChargingPowerPhase"].add(measurements.PowerL2 / 100.0);
    doc["ChargingPowerPhase"].add(measurements.PowerL3 / 100.0);
    doc["Frequency"] = measurements.Frequency / 100.0;
    doc["TemperatureMainUnit"] = measurements.Temperature;
    doc["VoltagePhase"] = ArduinoJson::JsonArray();
    doc["VoltagePhase"].add(measurements.VoltageL1 / 10.0);
    doc["VoltagePhase"].add(measurements.VoltageL2 / 10.0);
    doc["VoltagePhase"].add(measurements.VoltageL3 / 10.0);
}


typedef struct {
    char Error[50];
    bool Charging;
    uint8_t Current;
    uint16_t ChargingEnergyLimit;
    uint16_t KWhPer100;
    uint8_t AmountPerKWh;
    uint8_t Efficiency;
    uint8_t PauseCharging;
    uint8_t BLETransmissionPower;
} ApiSettings;

ApiSettings get_settings(const String& targetAddress) {
    ApiSettings settings;
    settings.Error[0] = 0;
    BLEDevice device = scanForTargetDevice(targetAddress);
    if (!device) {
        strcpy(settings.Error, "not found");
        return settings;
    }
    if (!connectToDevice(device)) {
        strcpy(settings.Error, "connect failed");
        return settings;
    }

    BLECharacteristic infoChar = device.characteristic(INFO_SERVICE);
    if (!(infoChar && infoChar.canRead() && infoChar.read())) {
        strcpy(settings.Error, "info characteristic read failed");
        return settings;
    }
    Info info = convertInfo(infoChar.value());
    settings.Charging = info.ChargingActive == 1 ? true : false;
    settings.Current = info.Current;
    settings.KWhPer100 = info.KWhPer100;
    settings.AmountPerKWh = info.AmountPerKWh;
    settings.Efficiency = info.Efficiency;
    settings.PauseCharging = info.PauseCharging == 1 ? true : false;
    settings.BLETransmissionPower = info.BLETransmissionPower;

    BLECharacteristic energyChar = device.characteristic(ENERGY_SERVICE);
    if (!(energyChar && energyChar.canRead() && energyChar.read())) {
        strcpy(settings.Error, "energy characteristic read failed");
        return settings;
    }
    Energy energy = convertEnergy(energyChar.value());
    settings.ChargingEnergyLimit = energy.ChargingEnergyLimit;
    return settings;
}

void settings2json(const ApiSettings& settings, ArduinoJson::JsonDocument& doc) {
    if (settings.Error[0] != 0) {
        doc["Error"] = settings.Error;
        return;
    }
    ArduinoJson::JsonObject values = doc["Values"].to<JsonObject>();
    ArduinoJson::JsonObject chargingStatus = values["ChargingStatus"].to<JsonObject>();
    chargingStatus["Charging"] = settings.Charging;
    ArduinoJson::JsonObject chargingCurrent = values["ChargingCurrent"].to<JsonObject>();
    chargingCurrent["Value"] = settings.Current;
}



void handleMeasurementsRequest(AsyncWebServerRequest *request) {
    String mac = request->pathArg(0);
    Serial.print("measurements request for ");
    Serial.println(mac);
    ApiMeasurements measurements = get_measurements(mac);
    ArduinoJson::JsonDocument doc;
    measurements2json(measurements, doc);
    if (measurements.Error[0] != 0) {
        Serial.print("> Error: ");
        Serial.println(measurements.Error);
    }
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void handleSettingsRequest(AsyncWebServerRequest *request) {
    String mac = request->pathArg(0);
    Serial.print("settings request for ");
    Serial.println(mac);
    ApiSettings settings = get_settings(mac);
    ArduinoJson::JsonDocument doc;
    settings2json(settings, doc);
    if (settings.Error[0] != 0) {
        Serial.print("> Error: ");
        Serial.println(settings.Error);
    }
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void handleSettingsRequestPut(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String body;
    if (index == 0) body = "";
    body += String((const char*)data, len);
    if (index + len != total) return;

    String mac = request->pathArg(0);
    Serial.print("settings PUT request for ");
    Serial.println(mac);
    Serial.print("Received body: ");
    Serial.println(body);

    ArduinoJson::JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
        request->send(400, "application/json", "{\"Message\":\"Invalid JSON\"}");
        return;
    }

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
        request->send(500, "application/json", "{\"Message\":\"Info characteristic read failed\"}");
        return;
    }
    Info info = convertInfo(infoChar.value());

    uint8_t pin = 0;
    if (doc["Values"]["DeviceMetadata"]["Password"].is<const char*>()) {
        pin = doc["Values"]["DeviceMetadata"]["Password"].as<int>();
    }

    Settings setSettings = convertToSettings(info, pin);
    if (doc["Values"]["ChargingStatus"]["Charging"].is<bool>()) {
        setSettings.PauseCharging = doc["Values"]["ChargingStatus"]["Charging"].as<bool>() ? 0 : 1;
    }
    if (doc["Values"]["ChargingCurrent"]["Value"].is<float>() || 
        doc["Values"]["ChargingCurrent"]["Value"].is<int>()) {
        setSettings.Current = doc["Values"]["ChargingCurrent"]["Value"].as<int>();
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
    request->send(200, "application/json", "{}");
}
