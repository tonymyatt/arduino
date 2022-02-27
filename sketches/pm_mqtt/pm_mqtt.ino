#include <Ethernet.h>
#include <ArduinoHA.h>
#include <EmonLib.h>

#define BROKER_ADDR IPAddress(192,168,1,14)

uint8_t mac[] = {0x00, 0x10, 0xFA, 0x6E, 0x38, 0x4A};
unsigned long lastSentAt = millis();

// Energy Monitor for house and solar
EnergyMonitor emon1;
EnergyMonitor emon2;

EthernetClient client;
HADevice device(mac, sizeof(mac));
HAMqtt mqtt(client, device);
HASensor solar_amps("solar_amps");
HASensor solar_watts("solar_watts");
HASensor house_amps("house_amps");
HASensor house_watts("house_watts");

void onBeforeSwitchStateChanged(bool state, HASwitch* s)
{
    // this callback will be called before publishing new state to HA
    // in some cases there may be delay before onStateChanged is called due to network latency
}

void setup() {
    Serial.begin(9600);
    // you don't need to verify return status
    uint8_t ip[]      = { 192, 168, 1, 79 };
    uint8_t gateway[] = { 192, 168, 1, 6 };
    uint8_t subnet[]  = { 255, 255, 255, 0 };
    Ethernet.begin(mac, ip, gateway, subnet);

    // set device's details (optional)
    device.setName("Power_Monitor");
    device.setSoftwareVersion("1.1");

    // Solar Sensors
    solar_amps.setUnitOfMeasurement("A");
    solar_amps.setDeviceClass("current");
    solar_amps.setIcon("mdi:solar-power");
    solar_amps.setName("Solar Current");
    solar_watts.setUnitOfMeasurement("W");
    solar_watts.setDeviceClass("power");
    solar_watts.setIcon("mdi:solar-power");
    solar_watts.setName("Solar Power");

    // House Sensors
    house_amps.setUnitOfMeasurement("A");
    house_amps.setDeviceClass("current");
    house_amps.setIcon("mdi:home");
    house_amps.setName("House Current");
    house_watts.setUnitOfMeasurement("W");
    house_watts.setDeviceClass("power");
    house_watts.setIcon("mdi:home");
    house_watts.setName("House Power");
    
    mqtt.begin(BROKER_ADDR);

    // Current: input pin & calibration
    emon1.current(0, 29.0);
    emon2.current(1, 29.0);
  
    // First values are rubbish, don't use
    emon1.calcIrms(5000);
    emon2.calcIrms(5000);
}

void loop() {
    double Irms1 = emon1.calcIrms(2000);  // Calculate Irms only
    double Irms2 = emon2.calcIrms(2000);  // Calculate Irms only
    
    Ethernet.maintain();
    mqtt.loop();

    // Only send about every 5s
    if ((millis() - lastSentAt) >= 5000) {
        lastSentAt = millis();

        /* CTs have an offset of 0.391 (observed) */
        // For Solar, if under 0.45A, cutoff to zero
        if(Irms1 < 0.45) {
          solar_amps.setValue(0);
          solar_watts.setValue(0);
        } else {
          solar_amps.setValue(Irms1 - 0.391);           // Remove 0.391 offset
          solar_watts.setValue((Irms1 - 0.391)*240);   // Assume power factor of 1.0 and 240VAC
        }
        // For Solar, if under 0.45A, cutoff to zero
        if(Irms2 < 0.45) {
          house_amps.setValue(0);
          house_watts.setValue(0);
        } else {
          house_amps.setValue(Irms2 - 0.391);           // Remove 0.391 offset
          house_watts.setValue((Irms2 - 0.391)*0.9*240);   // Assume power factor of 0.9 and 240VAC
        }
    }
}
