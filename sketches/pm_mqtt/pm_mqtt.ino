#include <Ethernet.h>
#include <ArduinoHA.h>
#include <EmonLib.h>

#define BROKER_ADDR IPAddress(192,168,1,14)

uint8_t mac[] = {0x00, 0x10, 0xFA, 0x6E, 0x38, 0x4A};
unsigned long lastSentAt = millis();
double lastValue = 0;

// Energy Monitor for house and solar
EnergyMonitor emon1;
EnergyMonitor emon2;

EthernetClient client;
HADevice device(mac, sizeof(mac));
HAMqtt mqtt(client, device);
HASensor house_current("house_current");
HASensor solar_current("solar_current");

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
    device.setSoftwareVersion("1.0");

    // House Sensor
    house_current.setUnitOfMeasurement("A");
    house_current.setDeviceClass("current");
    house_current.setIcon("mdi:home");
    house_current.setName("House Current");

    // Solar Sensor
    solar_current.setUnitOfMeasurement("A");
    solar_current.setDeviceClass("current");
    solar_current.setIcon("mdi:solar-power");
    solar_current.setName("Solar Current");

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

    if ((millis() - lastSentAt) >= 5000) {
        lastSentAt = millis();
        lastValue = lastValue + 0.5;
        house_current.setValue(Irms1);
        solar_current.setValue(Irms2);
    }
}
