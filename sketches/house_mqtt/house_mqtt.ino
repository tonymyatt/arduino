#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoHA.h>
#include <Dht11.h>
#include <DS18B20.h>

#define BROKER_ADDR IPAddress(192,168,1,16)

uint8_t mac[] = {0x90, 0xA2, 0xDA, 0x00, 0x51, 0x06};
//unsigned long lastSentAt = millis();

// Create a DHT11 sensor object on digital 8
Dht11 Sen1(8);

// DS18B20 temp sensor on digital 9
DS18B20 ds(9);
uint8_t address[] = {40,26,94,130,3,0,0,135};
uint8_t selected;

EthernetClient client;
HADevice device(mac, sizeof(mac));
HAMqtt mqtt(client, device);
HASensor sensor1_temp("s1t");
HASensor sensor1_humi("s1h");
HASensor sensor1_dewp("s1d");
//HASensor sensor2_temp("s2t");
//HASensor sensor2_dewp("s2d");

void setup()
{
  // Avoid pins 4,10,11,12,13 when using ethernet shield
  uint8_t ip[]      = { 192, 168, 1, 78 };
  uint8_t gateway[] = { 192, 168, 1, 6 };
  uint8_t subnet[]  = { 255, 255, 255, 0 };
  Ethernet.begin(mac, ip, gateway, subnet);

  // set device's details (optional)
  device.setName("House_IO");
  device.setSoftwareVersion("1.0");

  // Sensor 1
  sensor1_temp.setUnitOfMeasurement("°C");
  sensor1_temp.setDeviceClass("temperature");
  sensor1_temp.setName("House Temperature");
  sensor1_humi.setUnitOfMeasurement("%");
  sensor1_humi.setDeviceClass("humidity");
  sensor1_humi.setName("House Humidity");
  sensor1_dewp.setUnitOfMeasurement("°C");
  sensor1_dewp.setDeviceClass("temperature");
  sensor1_dewp.setName("House Dew Point");

  // Sensor 2
  //sensor2_temp.setUnitOfMeasurement("°C");
  //sensor2_temp.setDeviceClass("temperature");
  //sensor2_temp.setName("House Temperature (Sensor 2)");
  //sensor2_dewp.setUnitOfMeasurement("°C");
  //sensor2_dewp.setDeviceClass("temperature");
  //sensor2_dewp.setName("House Dew Point (Sensor 2)");

  // Select temp sensor
  selected = ds.select(address);

  mqtt.begin(BROKER_ADDR);
}

void loop()
{ 
  // Read sensor data
  Sen1.read();
  double hum = Sen1.getHumidity();
  double temp = ds.getTempC();

  Ethernet.maintain();
  mqtt.loop();

  // Only send about every 5s
  //if ((millis() - lastSentAt) >= 5000) {
  //    lastSentAt = millis();

      sensor1_temp.setValue(temp);
      sensor1_humi.setValue(hum);
      sensor1_dewp.setValue(dewPoint(temp, hum));
      //sensor2_temp.setValue(Sen2.getTempCByIndex(0)); // Two infered decimals
      //sensor2_dewp.setValue(dewPoint(Sen2.getTempCByIndex(0), Sen1.getHumidity()));
  //}
}

// dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm
//
double dewPoint(double celsius, double humidity)
{
    return (celsius - (14.55 + 0.114 * celsius) * (1 - (0.01 * humidity)) - pow(((2.5 + 0.007 * celsius) * (1 - (0.01 * humidity))),3) - (15.9 + 0.117 * celsius) * pow((1 - (0.01 * humidity)), 14));
}
