#include <Arduino.h>
#include <ArduinoBLE.h>
#include <ETH.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <map>
#include <ElegantOTA.h>

// UUIDs for characteristic
#define ENERGY_SERVICE "0379e580-ad1b-11e4-8bdd-0002a5d6b15d"
#define POWER_SERVICE "fd005380-b065-11e4-9ce2-0002a5d6b15d"
#define VOLTAGE_CURRENT_SERVICE "171bad00-b066-11e4-aeda-0002a5d6b15d"
#define INFO_SERVICE "8f75bba0-c903-11e4-9fe8-0002a5d6b15d"
#define SETTINGS_SERVICE "14b3afc0-ad1b-11e4-baab-0002a5d6b15d"

// Ethernet server on port 80
AsyncWebServer server(80);

/**
 * @brief Packed struct representing energy-related BLE data.
 */
struct __attribute__((packed)) Energy {
    uint32_t TotalEnergy;
    uint32_t EnergyLastCharge;
    uint32_t Energy2ndLastCharge;
    uint32_t Energy3rdLastCharge;
    uint16_t ChargingEnergyLimit;
    uint8_t Pad;
};

/**
 * @brief Packed struct representing power-related BLE data.
 */
struct __attribute__((packed)) Power {
    uint16_t TotalPower;
    uint16_t L1;
    uint16_t L2;
    uint16_t L3;
    uint16_t PeakPower;
    uint16_t Frequency;
    int16_t Temperature;
    uint16_t RemainingDistance;
    uint16_t Costs;
    int8_t CPSignal;
};

/**
 * @brief Packed struct representing voltage and current BLE data.
 */
struct __attribute__((packed)) VoltageCurrent {
    uint16_t VoltageL1;
    uint16_t VoltageL2;
    uint16_t VoltageL3;
    uint16_t CurrentL1;
    uint16_t CurrentL2;
    uint16_t CurrentL3;
    uint8_t Pad[2];
};

/**
 * @brief Packed struct representing device info BLE data.
 */
struct __attribute__((packed)) Info {
    uint8_t Current; //10
    uint16_t KWhPer100; // 00 AA
    uint8_t AmountPerKWh; // 24
    uint8_t FIEnabled; // 00
    uint8_t ErrorCode; // 00
    uint8_t Efficiency; // 50
    uint8_t ChargingActive; // 00
    uint8_t PauseCharging; // 00
    uint8_t ChargingCurrentMax; // 20
    uint8_t BLETransmissionPower; // 04
    uint8_t Pad[2];
};

/**
 * @brief Packed struct representing settings to be written to BLE device.
 */
struct __attribute__((packed)) Settings {
    uint16_t PIN;                  // H SettingsPIN
    uint8_t Current;               // B SettingsChargingCurrentValue
    uint16_t ChargingEnergyLimit;  // H SettingsChargingEnergyOff
    uint16_t KWhPer100;            // H round(SettingsKWhPer100Value * 10)
    uint8_t AmountPerKWh;          // B round(SettingsAmountPerKWhValue * 100)
    uint8_t Pad[2];                // xx
    uint8_t Efficiency;            // B SettingsEfficacyValue
    uint8_t PauseCharging;         // B 1 if SettingsPauseCharging else 0
    uint8_t BLETransmissionPower;   // b SettingsBLETransmissionPowerValue
    uint8_t PadTail[5];            // xxxxx
};

/**
 * @brief Scan for a BLE device with the given MAC address.
 * @param targetAddress The MAC address to search for.
 * @return BLEDevice object if found, otherwise a default BLEDevice.
 */
BLEDevice scanForTargetDevice(const String& targetAddress) {
    static std::map<String, BLEDevice> deviceCache;
    // Check cache first
    auto it = deviceCache.find(targetAddress);
    if (it != deviceCache.end()) {
        if (it->second) {
            return it->second;
        }
    }
    unsigned long scanStart = millis();
    BLE.scan();
    while (millis() - scanStart < 10000) {
        BLEDevice device = BLE.available();
        if (device) {
            if (device.address() == targetAddress) {
                BLE.stopScan();
                deviceCache[targetAddress] = device;
                return device;
            }
        }
        delay(1); // allow async_tcp and watchdog to run
    }
    BLE.stopScan();
    return BLEDevice(); // return default if not found
}

/**
 * @brief Struct holding measurement data for API responses.
 */
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

/**
 * @brief Retrieve measurement data from a BLE device.
 * @param targetAddress The MAC address of the BLE device.
 * @return ApiMeasurements struct with measurement data or error message.
 */
ApiMeasurements get_measurements(const String& targetAddress) {
    ApiMeasurements measurements;
    measurements.Error[0] = 0; // Initialize error message to empty

    BLEDevice device = scanForTargetDevice(targetAddress);
    if (!device) {
        strcpy(measurements.Error, "not found");
        return measurements;
    }
    if (!device.connected()) {
        if (!device.connect()) {
            strcpy(measurements.Error, "connect failed");
            return measurements;
        }
    }
    int retries = 3;
    bool discovered = false;
    for (int i = 0; i < retries; ++i) {
        if (device.discoverAttributes()) {
            discovered = true;
            break;
        } else {
            delay(100); // wait before retrying
        }
    }
    if (!discovered) {
        strcpy(measurements.Error, "attribute discovery failed");
        return measurements;
    }
    BLECharacteristic energyChar = device.characteristic(ENERGY_SERVICE);
    if (!(energyChar && energyChar.canRead() && energyChar.read())) {
        strcpy(measurements.Error, "energy characteristic read failed");
        return measurements;
    }
    Energy* energy = (Energy*)energyChar.value();
    // Invert endianness for Energy fields
    measurements.TotalEnergy = __builtin_bswap32(energy->TotalEnergy);
    measurements.EnergyLastCharge = __builtin_bswap32(energy->EnergyLastCharge);
    measurements.ChargingEnergyLimit = __builtin_bswap16(energy->ChargingEnergyLimit);

    BLECharacteristic powerChar = device.characteristic(POWER_SERVICE);
    if (!(powerChar && powerChar.canRead() && powerChar.read())) {
        strcpy(measurements.Error, "power characteristic read failed");
        return measurements;
    }
    Power* power = (Power*)powerChar.value();
    // Invert endianness for Power fields
    measurements.TotalPower = __builtin_bswap16(power->TotalPower);
    measurements.PowerL1 = __builtin_bswap16(power->L1);
    measurements.PowerL2 = __builtin_bswap16(power->L2);
    measurements.PowerL3 = __builtin_bswap16(power->L3);
    measurements.Frequency = __builtin_bswap16(power->Frequency);
    measurements.Temperature = (int16_t)__builtin_bswap16((uint16_t)power->Temperature);

    BLECharacteristic vcChar = device.characteristic(VOLTAGE_CURRENT_SERVICE);
    if (!(vcChar && vcChar.canRead() && vcChar.read())) {
        strcpy(measurements.Error, "voltage/current characteristic read failed");
        return measurements;
    }
    VoltageCurrent* vc = (VoltageCurrent*)vcChar.value();
    // Invert endianness for VoltageCurrent fields
    measurements.VoltageL1 = __builtin_bswap16(vc->VoltageL1);
    measurements.VoltageL2 = __builtin_bswap16(vc->VoltageL2);
    measurements.VoltageL3 = __builtin_bswap16(vc->VoltageL3);
    measurements.CurrentL1 = __builtin_bswap16(vc->CurrentL1);
    measurements.CurrentL2 = __builtin_bswap16(vc->CurrentL2);
    measurements.CurrentL3 = __builtin_bswap16(vc->CurrentL3);

    return measurements;
}

/**
 * @brief Print measurement data to the serial console.
 * @param measurements The ApiMeasurements struct to print.
 */
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

/**
 * @brief Convert measurement data to a JSON document.
 * @param measurements The ApiMeasurements struct to convert.
 * @param doc The ArduinoJson document to populate.
 */
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
    doc["ChargingEnergyPhase"].add(0.0); // Not available in Measurements
    doc["ChargingEnergyPhase"].add(0.0); // Not available in Measurements
    doc["ChargingEnergyPhase"].add(0.0); // Not available in Measurements
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

/**
 * @brief HTTP handler for /api/measurements/* GET requests.
 * @param request The web server request object.
 */
void handleMeasurementsRequest(AsyncWebServerRequest *request) {
    String uri = request->url();
    String prefix = "/api/measurements/";
    String mac = uri.substring(prefix.length());

    Serial.print("measurements request for ");
    Serial.println(uri);

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


/**
 * @brief Struct holding settings data for API responses.
 */
typedef struct {
    char Error[50];

    bool Charging;

    uint8_t Current;
    uint16_t ChargingEnergyLimit; // energy service
    uint16_t KWhPer100;
    uint8_t AmountPerKWh;
    uint8_t Efficiency;
    uint8_t PauseCharging;
    uint8_t BLETransmissionPower;
} ApiSettings;


/**
 * @brief Retrieve settings data from a BLE device.
 * @param targetAddress The MAC address of the BLE device.
 * @return ApiSettings struct with settings data or error message.
 */
ApiSettings get_settings(const String& targetAddress) {
    ApiSettings settings;
    settings.Error[0] = 0; // Initialize error message to empty

    BLEDevice device = scanForTargetDevice(targetAddress);
    if (!device) {
        strcpy(settings.Error, "not found");
        return settings;
    }
    if (!device.connected()) {
        if (!device.connect()) {
            strcpy(settings.Error, "connect failed");
            return settings;
        }
    }
    int retries = 3;
    bool discovered = false;
    for (int i = 0; i < retries; ++i) {
        if (device.discoverAttributes()) {
            discovered = true;
            break;
        } else {
            delay(100); // wait before retrying
        }
    }
    if (!discovered) {
        strcpy(settings.Error, "attribute discovery failed");
        return settings;
    }
    BLECharacteristic infoChar = device.characteristic(INFO_SERVICE);
    if (!(infoChar && infoChar.canRead() && infoChar.read())) {
        strcpy(settings.Error, "info characteristic read failed");
        return settings;
    }
    Info* info = (Info*)infoChar.value();

    settings.Charging = info->ChargingActive == 1 ? true : false;

    settings.Current = info->Current;
    settings.KWhPer100 = __builtin_bswap16(info->KWhPer100);
    settings.AmountPerKWh = info->AmountPerKWh;
    settings.Efficiency = info->Efficiency;
    settings.PauseCharging = info->PauseCharging == 1 ? true : false;
    settings.BLETransmissionPower = info->BLETransmissionPower;

    BLECharacteristic energyChar = device.characteristic(ENERGY_SERVICE);
    if (!(energyChar && energyChar.canRead() && energyChar.read())) {
        strcpy(settings.Error, "energy characteristic read failed");
        return settings;
    }
    Energy* energy = (Energy*)energyChar.value();
    settings.ChargingEnergyLimit = __builtin_bswap16(energy->ChargingEnergyLimit);

    return settings;
}


/**
 * @brief Convert settings data to a JSON document.
 * @param settings The ApiSettings struct to convert.
 * @param doc The ArduinoJson document to populate.
 */
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

/**
 * @brief HTTP handler for /api/settings/* GET requests.
 * @param request The web server request object.
 */
void handleSettingsRequest(AsyncWebServerRequest *request) {
    String uri = request->url();
    String prefix = "/api/settings/";
    String mac = uri.substring(prefix.length());

    Serial.print("settings request for ");
    Serial.println(uri);

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

/**
 * @brief HTTP handler for /api/settings/* PUT requests.
 * @param request The web server request object.
 * @param data The received data buffer.
 * @param len Length of the received data.
 * @param index Index of the current data chunk.
 * @param total Total length of the data.
 */
void handleSettingsRequestPut(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    static String body;
    if (index == 0) body = "";
    body += String((const char*)data, len);
    if (index + len != total) return;

    String uri = request->url();
    String prefix = "/api/settings/";
    String mac = uri.substring(prefix.length());

    Serial.print("settings PUT request for ");
    Serial.println(uri);
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

    ApiSettings settings = get_settings(mac);
    if (settings.Error[0] != 0) {
        Serial.print("Error getting settings: ");
        Serial.println(settings.Error);
        request->send(500, "application/json", "{\"Message\":\"Failed to get current settings\"}");
        return;
    }

    Settings setSettings;
    setSettings.PIN = 0; // must be set by caller
    setSettings.Current = settings.Current;
    setSettings.ChargingEnergyLimit = __builtin_bswap16(settings.ChargingEnergyLimit);
    setSettings.KWhPer100 = __builtin_bswap16(settings.KWhPer100);
    setSettings.AmountPerKWh = settings.AmountPerKWh;
    setSettings.Efficiency = settings.Efficiency;
    setSettings.PauseCharging = settings.PauseCharging ? 1 : 0;
    setSettings.BLETransmissionPower = settings.BLETransmissionPower;

    if (doc["Values"]["ChargingStatus"]["Charging"].is<bool>()) {
        setSettings.PauseCharging = doc["Values"]["ChargingStatus"]["Charging"].as<bool>() ? 0 : 1;
    }
    if (doc["Values"]["ChargingCurrent"]["Value"].is<float>() || 
        doc["Values"]["ChargingCurrent"]["Value"].is<int>()) {
        setSettings.Current = doc["Values"]["ChargingCurrent"]["Value"].as<int>();
    }
    if (doc["Values"]["DeviceMetadata"]["Password"].is<const char*>()) {
        setSettings.PIN = __builtin_bswap16(doc["Values"]["DeviceMetadata"]["Password"].as<int>());
    }

    // Print setSettings values
    /*Serial.println("setSettings:");
    Serial.print("PIN: "); Serial.println(__builtin_bswap16(setSettings.PIN));
    Serial.print("Current: "); Serial.println(setSettings.Current);
    Serial.print("ChargingEnergyLimit: "); Serial.println(__builtin_bswap16(setSettings.ChargingEnergyLimit));
    Serial.print("KWhPer100: "); Serial.println(__builtin_bswap16(setSettings.KWhPer100));
    Serial.print("AmountPerKWh: "); Serial.println(setSettings.AmountPerKWh);
    Serial.print("Efficiency: "); Serial.println(setSettings.Efficiency);
    Serial.print("PauseCharging: "); Serial.println(setSettings.PauseCharging);
    Serial.print("BLETransmissionPower: "); Serial.println(setSettings.BLETransmissionPower);*/

    // Write setSettings struct to SETTINGS_SERVICE characteristic
    BLEDevice device = scanForTargetDevice(mac);
    if (!device) {
        Serial.println("Device not found for writing settings");
        request->send(500, "application/json", "{\"Message\":\"Device not found\"}");
        return;
    }
    if (!device.connected()) {
        if (!device.connect()) {
            Serial.println("Device connect failed for writing settings");
            request->send(500, "application/json", "{\"Message\":\"Device connect failed\"}");
            return;
        }
    }
    int retries = 3;
    bool discovered = false;
    for (int i = 0; i < retries; ++i) {
        if (device.discoverAttributes()) {
            discovered = true;
            break;
        } else {
            delay(100);
        }
    }
    if (!discovered) {
        Serial.println("Attribute discovery failed for writing settings");
        request->send(500, "application/json", "{\"Message\":\"Attribute discovery failed\"}");
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

    request->send(200, "application/json", "{}");
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
    server.on("/api/measurements/*", HTTP_GET, handleMeasurementsRequest);
    server.on("/api/settings/*", HTTP_GET, handleSettingsRequest);
    server.on("/api/settings/*", HTTP_PUT, [](AsyncWebServerRequest *request){}, NULL, handleSettingsRequestPut);
    ElegantOTA.begin(&server);
    server.begin();

    esp_task_wdt_init(30, true); // Set watchdog timeout to 30 seconds
}

/**
 * @brief Arduino loop function. Handles OTA updates.
 */
void loop() {
    ElegantOTA.loop();
}
