#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#define batteryCapacity 800

class Battery {
private:
    float batteryVoltage;     
    float batteryCurrent;      
    float batteryPower;      
    int batteryPercentage;  
    float batteryLastForH;    
    float batteryTemperature;
    bool batteryIsCharging;      

    float estimateBatteryPercentage();
    float includeCurrent();
    float includeTemperature(float percentage);
    float includeDischargeCurve(float voltage);
public:
    Battery(); // Constructor

    void updateBattery(float voltage, float current, float power, float temperature);
    
    // Getters
    float getVoltage();
    float getCurrent();
    float getPower();
    int getPercentage();
    float getEstimatedHours();
    bool isCharging();
    
    void printBatteryStatus(); // Debug function
};

#endif  // BATTERY_H