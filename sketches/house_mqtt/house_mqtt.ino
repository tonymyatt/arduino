#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoHA.h>
#include <Dht11.h>
//#include <OneWire.h>
//#include <DallasTemperature.h>

#define BROKER_ADDR IPAddress(192,168,1,14)

uint8_t mac[] = {0x90, 0xA2, 0xDA, 0x00, 0x51, 0x06};
unsigned long lastSentAt = millis();

// Create a DHT11 sensor object on digital 8
Dht11 Sen1(8);

// Setup a oneWire instance to communicate with any OneWire devices
//OneWire oneWire(9);

// Pass our oneWire reference to Dallas Temperature. 
//DallasTemperature Sen2(&oneWire);

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

  // Start up the DallasTemp lib
  //Sen2.begin();

  mqtt.begin(BROKER_ADDR);
}

void loop()
{ 
  // Read sensor data
  Sen1.read();
  //Sen2.requestTemperatures();

  Ethernet.maintain();
  mqtt.loop();

  // Only send about every 5s
  if ((millis() - lastSentAt) >= 5000) {
      lastSentAt = millis();

      sensor1_temp.setValue(Sen1.getTemperature());
      sensor1_humi.setValue(Sen1.getHumidity());
      sensor1_dewp.setValue(dewPoint(Sen1.getTemperature(), Sen1.getHumidity()));
      //sensor2_temp.setValue(Sen2.getTempCByIndex(0)); // Two infered decimals
      //sensor2_dewp.setValue(dewPoint(Sen2.getTempCByIndex(0), Sen1.getHumidity()));
  }
}

// dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm
//
double dewPoint(double celsius, double humidity)
{
	// (1) Saturation Vapor Pressure = ESGG(T)
	double RATIO = 373.15 / (273.15 + celsius);
	double RHS = -7.90298 * (RATIO - 1);
	RHS += 5.02808 * log10(RATIO);
	RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
	RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
	RHS += log10(1013.246);

        // factor -3 is to adjust units - Vapor Pressure SVP * humidity
	double VP = pow(10, RHS - 3) * humidity;

        // (2) DEWPOINT = F(Vapor Pressure)
	double T = log(VP/0.61078);   // temp var
	return (241.88 * T) / (17.558 - T);
}
