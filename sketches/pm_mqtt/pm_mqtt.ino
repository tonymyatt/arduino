#include <Ethernet.h>
#include <ArduinoHA.h>
#include <EmonLib.h>

#define BROKER_ADDR IPAddress(192,168,1,16)

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

HASensor import_amps("import_amps");
HASensor import_watts("import_watts");
HASensor export_amps("export_amps");
HASensor export_watts("export_watts");

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

    // Import Calc Sensors
    import_amps.setUnitOfMeasurement("A");
    import_amps.setDeviceClass("current");
    import_amps.setIcon("mdi:home-import-outline");
    import_amps.setName("Import Current");
    import_watts.setUnitOfMeasurement("W");
    import_watts.setDeviceClass("power");
    import_watts.setIcon("mdi:home-import-outline");
    import_watts.setName("Import Power");

    // Export Calc Sensors
    export_amps.setUnitOfMeasurement("A");
    export_amps.setDeviceClass("current");
    export_amps.setIcon("mdi:home-export-outline");
    export_amps.setName("Export Current");
    export_watts.setUnitOfMeasurement("W");
    export_watts.setDeviceClass("power");
    export_watts.setIcon("mdi:home-export-outline");
    export_watts.setName("Export Power");
    
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
    double solarAmps, solarWatts;
    double houseAmps, houseWatts;
    double importAmps, importWatts;
    double exportAmps, exportWatts;
    
    Ethernet.maintain();
    mqtt.loop();

    // Only send about every 5s
    if ((millis() - lastSentAt) >= 5000) {
        lastSentAt = millis();

        /* CTs have an offset of 0.391 (observed) */
        // For Solar, if under 0.45A, cutoff to zero
        if(Irms1 < 0.391) {
          solarAmps = 0.0;
          solarWatts = 0.0;
        } else {
          solarAmps = Irms1 - 0.391;      // Remove 0.391 offset
          solarWatts = (solarAmps*240);   // Assume power factor of 1.0 and 240VAC
        }
        
        // For house, if under 0.45A, cutoff to zero
        if(Irms2 < 0.45) {
          houseAmps = 0.0;
          houseWatts = 0.0;
        } else {
          houseAmps = Irms2 - 0.391;        // Remove 0.391 offset
          houseWatts = houseAmps*0.9*240;   // Assume power factor of 0.9 and 240VAC
        }

        // Import is house less what solar is generating, dont allow to go negative
        importAmps = houseAmps - solarAmps;
        if(importAmps < 0) importAmps = 0;
        importWatts = houseWatts - solarWatts;
        if(importWatts < 0) importWatts = 0;

        // Export is what solar we are generating less house, dont allow to go negative
        exportAmps = solarAmps - houseAmps;
        if(exportAmps < 0) exportAmps = 0;
        exportWatts = solarWatts - houseWatts;
        if(exportWatts < 0) exportWatts = 0;

        // Update HA Sensors
        solar_amps.setValue(solarAmps);
        solar_watts.setValue(solarWatts);
        house_amps.setValue(houseAmps);
        house_watts.setValue(houseWatts);
        import_amps.setValue(importAmps);
        import_watts.setValue(importWatts);
        export_amps.setValue(exportAmps);
        export_watts.setValue(exportWatts);
    }
}
