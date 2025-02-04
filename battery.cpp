#include "battery.h"

Battery::Battery() {
    batteryVoltage = 0.0;
    batteryCurrent = 0.0;
    batteryPower = 0.0;
    batteryPercentage = 0;
    batteryLastForH = 0.0;
    batteryIsCharging = false;
    batteryTemperature = 0;
}

float Battery::includeCurrent() {
    const float internalResistance = 0.1f; // Example internal resistance in ohms
    return batteryVoltage + (batteryCurrent * internalResistance)/1000;
}

float Battery::includeTemperature(float percentage) {
    if (batteryTemperature < 0)
        return percentage * (0.7f + 0.15f * (batteryTemperature / -30.0f)); // Linear scale from -30°C to 0°C
    else if (batteryTemperature < 10)
        return percentage * (0.85f + 0.05f * (batteryTemperature / 10.0f)); // Linear scale from 0°C to 10°C
    else if (batteryTemperature > 45)
        return percentage * (1.0f - 0.1f * ((batteryTemperature - 45.0f) / 15.0f)); // Scale beyond 45°C
    return percentage;
}

float Battery::estimateBatteryPercentage() {
    float noLoadVoltage = includeCurrent();
    float estPercentage = includeDischargeCurve(noLoadVoltage);
    return includeTemperature(estPercentage);
}

float Battery::includeDischargeCurve(float voltage) {
    const float voltages[] = {4.2f, 4.1f, 4.0f, 3.9f, 3.8f, 3.7f, 3.6f, 3.5f, 3.4f};
    const float percentages[] = {100.0f, 95.0f, 85.0f, 75.0f, 60.0f, 40.0f, 20.0f, 10.0f, 5.0f};
    int size = sizeof(voltages) / sizeof(voltages[0]);
    
    if (voltage >= voltages[0]) return percentages[0];
    if (voltage < voltages[size - 1]) return 0.0f;
    
    for (int i = 0; i < size - 1; i++) {
        if (voltage >= voltages[i + 1]) {
            float v1 = voltages[i];
            float v2 = voltages[i + 1];
            float p1 = percentages[i];
            float p2 = percentages[i + 1];
            return p1 + (p2 - p1) * ((voltage - v1) / (v2 - v1));
        }
    }
    return 0.0f;
}

void Battery::updateBattery(float voltage, float current, float power, float temperature) {
    batteryVoltage = voltage;
    batteryCurrent = current;
    batteryPower = power;
    batteryTemperature = temperature;
    batteryIsCharging = (current < 0);
    
    if (!batteryIsCharging) {
        float batteryCapacityWh = (batteryCapacity * batteryVoltage) / 1000.0f;
        float batteryPowerW = batteryPower / 1000.0f;
        batteryLastForH = (batteryPowerW > 0) ? (batteryCapacityWh / batteryPowerW) : -1;
    } else {
        batteryLastForH = -1;
    }
    batteryPercentage = estimateBatteryPercentage();
}

float Battery::getVoltage() { return batteryVoltage; }
float Battery::getCurrent() { return batteryCurrent; }
float Battery::getPower() { return batteryPower; }
int Battery::getPercentage() { return batteryPercentage; }
float Battery::getEstimatedHours() { return batteryLastForH; }
bool Battery::isCharging() { return batteryIsCharging; }

void Battery::printBatteryStatus() {
    Serial.print("Voltage: "); Serial.print(batteryVoltage); Serial.println("V");
    Serial.print("Current: "); Serial.print(batteryCurrent); Serial.println("mA");
    Serial.print("Power: "); Serial.print(batteryPower); Serial.println("mW");
    Serial.print("Charge: "); Serial.print(batteryPercentage); Serial.println("%");
    Serial.print("Estimated runtime: "); Serial.print(batteryLastForH); Serial.println("h");
    Serial.print("Charging: "); Serial.println(batteryIsCharging ? "Yes" : "No");
}
