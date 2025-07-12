// ble_utils.h
#pragma once
#include <ArduinoBLE.h>
#include <map>
#include <Arduino.h>


// UUIDs for characteristic
#define ENERGY_SERVICE "0379e580-ad1b-11e4-8bdd-0002a5d6b15d"
#define POWER_SERVICE "fd005380-b065-11e4-9ce2-0002a5d6b15d"
#define VOLTAGE_CURRENT_SERVICE "171bad00-b066-11e4-aeda-0002a5d6b15d"
#define INFO_SERVICE "8f75bba0-c903-11e4-9fe8-0002a5d6b15d"
#define SETTINGS_SERVICE "14b3afc0-ad1b-11e4-baab-0002a5d6b15d"

struct __attribute__((packed)) Energy {
    uint32_t TotalEnergy;
    uint32_t EnergyLastCharge;
    uint32_t Energy2ndLastCharge;
    uint32_t Energy3rdLastCharge;
    uint16_t ChargingEnergyLimit;
    uint8_t Pad;
};

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

struct __attribute__((packed)) VoltageCurrent {
    uint16_t VoltageL1;
    uint16_t VoltageL2;
    uint16_t VoltageL3;
    uint16_t CurrentL1;
    uint16_t CurrentL2;
    uint16_t CurrentL3;
    uint8_t Pad[2];
};

struct __attribute__((packed)) Info {
    uint8_t Current;
    uint16_t KWhPer100;
    uint8_t AmountPerKWh;
    uint8_t FIEnabled;
    uint8_t ErrorCode;
    uint8_t Efficiency;
    uint8_t ChargingActive;
    uint8_t PauseCharging;
    uint8_t ChargingCurrentMax;
    uint8_t BLETransmissionPower;
    uint8_t Pad[2];
};

struct __attribute__((packed)) Settings {
    uint16_t PIN;
    uint8_t Current;
    uint16_t ChargingEnergyLimit;
    uint16_t KWhPer100;
    uint8_t AmountPerKWh;
    uint8_t Pad[2];
    uint8_t Efficiency;
    uint8_t PauseCharging;
    uint8_t BLETransmissionPower;
    uint8_t PadTail[5];
};

BLEDevice scanForTargetDevice(const String& targetAddress);
bool connectToDevice(BLEDevice& device);

Energy convertEnergy(const uint8_t* data);
Power convertPower(const uint8_t* data);
VoltageCurrent convertVoltageCurrent(const uint8_t* data);
Info convertInfo(const uint8_t* data);

Settings convertToSettings(Info& info, uint16_t pin);
