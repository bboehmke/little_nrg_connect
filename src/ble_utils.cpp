#include "ble_utils.h"

BLEDevice scanForTargetDevice(const String& targetAddress) {
    static std::map<String, BLEDevice> deviceCache;
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
        delay(1);
    }
    BLE.stopScan();
    return BLEDevice();
}

bool connectToDevice(BLEDevice& device) {
    if (!device.connected()) {
        if (!device.connect()) {
            return false;
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
        return false;
    }
    return true;
}

Energy convertEnergy(const uint8_t* data) {
    Energy* energy = (Energy*)data;
    energy->TotalEnergy = __builtin_bswap32(energy->TotalEnergy);
    energy->EnergyLastCharge = __builtin_bswap32(energy->EnergyLastCharge);
    energy->Energy2ndLastCharge = __builtin_bswap32(energy->Energy2ndLastCharge);
    energy->Energy3rdLastCharge = __builtin_bswap32(energy->Energy3rdLastCharge);
    energy->ChargingEnergyLimit = __builtin_bswap16(energy->ChargingEnergyLimit);
    return *energy;
}
Power convertPower(const uint8_t* data) {
    Power* power = (Power*)data;
    power->TotalPower = __builtin_bswap16(power->TotalPower);
    power->L1 = __builtin_bswap16(power->L1);
    power->L2 = __builtin_bswap16(power->L2);
    power->L3 = __builtin_bswap16(power->L3);
    power->Frequency = __builtin_bswap16(power->Frequency);
    power->Temperature = (int16_t)__builtin_bswap16((uint16_t)power->Temperature);
    return *power;
}
VoltageCurrent convertVoltageCurrent(const uint8_t* data) {
    VoltageCurrent* vc = (VoltageCurrent*)data;
    vc->VoltageL1 = __builtin_bswap16(vc->VoltageL1);
    vc->VoltageL2 = __builtin_bswap16(vc->VoltageL2);
    vc->VoltageL3 = __builtin_bswap16(vc->VoltageL3);
    vc->CurrentL1 = __builtin_bswap16(vc->CurrentL1);
    vc->CurrentL2 = __builtin_bswap16(vc->CurrentL2);
    vc->CurrentL3 = __builtin_bswap16(vc->CurrentL3);
    return *vc;
}
Info convertInfo(const uint8_t* data) {
    Info* info = (Info*)data;
    info->KWhPer100 = __builtin_bswap16(info->KWhPer100);
    return *info;
}

Settings convertToSettings(Info& info, uint16_t pin) {
    Settings setSettings;
    setSettings.PIN = __builtin_bswap16(pin);
    setSettings.Current = info.Current;
    setSettings.ChargingEnergyLimit = __builtin_bswap16(19997); //  magic const for "disable"
    setSettings.KWhPer100 = __builtin_bswap16(info.KWhPer100);
    setSettings.AmountPerKWh = info.AmountPerKWh;
    setSettings.Efficiency = info.Efficiency;
    setSettings.PauseCharging = info.PauseCharging == 1 ? 0 : 1;
    setSettings.BLETransmissionPower = info.BLETransmissionPower;
    return setSettings;
}