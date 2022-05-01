#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoHA.h>
#include <Dht11.h>
#include <DS18B20.h>

#define BROKER_ADDR IPAddress(192,168,1,16)

uint8_t mac[] = {0x90, 0xA2, 0xDA, 0x00, 0x51, 0x06};
unsigned long lastSentAt = millis();

// Create a DHT11 sensor object on digital 8
Dht11 Sen1(8);

// DS18B20 temp sensor on digital 9
DS18B20 Sen2(9);
uint8_t address[] = {40,26,94,130,3,0,0,135};
uint8_t selected;

EthernetClient client;
HADevice device(mac, sizeof(mac));
HAMqtt mqtt(client, device);
HASensor sensor1_temp("s1t");
HASensor sensor1_humi("s1h");
HASensor sensor2_temp("s2t");

void setup()
{
  // Avoid pins 4,10,11,12,13 when using ethernet shield
  uint8_t ip[]      = { 192, 168, 1, 78 };
  uint8_t gateway[] = { 192, 168, 1, 6 };
  uint8_t subnet[]  = { 255, 255, 255, 0 };
  Ethernet.begin(mac, ip, gateway, subnet);

  // set device's details (optional)
  device.setName("House_IO");
  device.setSoftwareVersion("1.1");

  // Sensor 1
  sensor1_temp.setUnitOfMeasurement("°C");
  sensor1_temp.setDeviceClass("temperature");
  sensor1_temp.setName("House Temperature");
  
  sensor1_humi.setUnitOfMeasurement("%");
  sensor1_humi.setDeviceClass("humidity");
  sensor1_humi.setName("House Humidity");

  // Sensor 2
  sensor2_temp.setUnitOfMeasurement("°C");
  sensor2_temp.setDeviceClass("temperature");
  sensor2_temp.setName("House Temperature");

  // Select temp sensor
  selected = Sen2.select(address);

  mqtt.begin(BROKER_ADDR);
}

void loop()
{ 
  Ethernet.maintain();
  mqtt.loop();

  // Only send about every 5s
  if ((millis() - lastSentAt) >= 5000) {
    lastSentAt = millis();

    // Read sensor data, sensor 2 read not required
    Sen1.read();

    sensor1_temp.setValue(Sen1.getTemperature());
    sensor1_humi.setValue(Sen1.getHumidity());
    sensor2_temp.setValue(Sen2.getTempC());
  }
}
