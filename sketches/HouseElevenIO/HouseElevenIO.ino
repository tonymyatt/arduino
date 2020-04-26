#include <avr/wdt.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Mudbus.h>
#include <Dht11.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//Function codes 1(read coils), 3(read registers), 5(write coil), 6(write register)
//signed int Mb.R[0 to 125] and bool Mb.C[0 to 128] MB_N_R MB_N_C
//Port 502 (defined in Mudbus.h) MB_PORT
Mudbus Mb;

// Create a DHT11 sensor object on digital 8
Dht11 Sen1(8);

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(9);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature Sen2(&oneWire);

// Number of loops not modbus read has not been requested
int noReadCount;

void setup()
{
  uint8_t mac[]     = { 0x90, 0xA2, 0xDA, 0x00, 0x51, 0x06 };
  uint8_t ip[]      = { 192, 168, 1, 70 };
  uint8_t gateway[] = { 192, 168, 1, 1 };
  uint8_t subnet[]  = { 255, 255, 255, 0 };
  
  // Avoid pins 4,10,11,12,13 when using ethernet shield
  Ethernet.begin(mac, ip, gateway, subnet);

  // Start up the DallasTemp lib
  Sen2.begin();
  
  // Start with count at zero
  noReadCount = 0;

  // Enable watchdog with 8s timeout
  wdt_enable(WDTO_8S);
}

void loop()
{
  // Increment every cycle modbus is not active
  if(Mb.Active)
  {
    noReadCount = 0;
  } else 
  {
    noReadCount++;
  }

  // Reset watchdog 
  if(noReadCount < 200)
  {
    wdt_reset();
  }
  
  Mb.Run();
  
  // Read sensor data
  Sen1.read();
  Sen2.requestTemperatures();
  
  // Holding registers, R[0] = 40001 and onwards
  Mb.R[0]++;
  Mb.R[1] = Sen1.getHumidity();
  Mb.R[2] = Sen1.getTemperature();
  Mb.R[3] = dewPoint(Sen1.getTemperature(), Sen1.getHumidity());
  Mb.R[4] = Sen2.getTempCByIndex(0)*100; // Two infered decimals
  Mb.R[5] = dewPoint(Sen2.getTempCByIndex(0), Sen1.getHumidity());;
  Mb.R[6] = noReadCount;
  Mb.R[7] = 40008;
  Mb.R[8] = 40009;
  Mb.R[9] = 40010;
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


