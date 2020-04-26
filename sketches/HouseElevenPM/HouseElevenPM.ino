#include <avr/wdt.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Mudbus.h>
#include <EmonLib.h>

EnergyMonitor emon1;                    // Create an instance
EnergyMonitor emon2;                    // Create an instance

// Number of loops not modbus read has not been requested
int noReadCount;

//Function codes 1(read coils), 3(read registers), 5(write coil), 6(write register)
//signed int Mb.R[0 to 125] and bool Mb.C[0 to 128] MB_N_R MB_N_C
//Port 502 (defined in Mudbus.h) MB_PORT
Mudbus Mb;

void setup()
{
  uint8_t mac[]     = { 0x90, 0xA2, 0xDA, 0x00, 0x51, 0x07 };
  uint8_t ip[]      = { 192, 168, 1, 71 };
  uint8_t gateway[] = { 192, 168, 1, 6 };
  uint8_t subnet[]  = { 255, 255, 255, 0 };

  // Disable watchdog for startup
  wdt_disable();
  
  // Avoid pins 4,10,11,12,13 when using ethernet shield
  Ethernet.begin(mac, ip, gateway, subnet);

  emon1.current(0, 29.0);               // Current: input pin, calibration.
  emon2.current(1, 29.0);               // Current: input pin, calibration.
  
  // First values are rubbish, don't use
  emon1.calcIrms(5000);
  emon2.calcIrms(5000);

  // Start with count at zero
  noReadCount = 0;

  // Enable watchdog with 8s timeout
  wdt_enable(WDTO_8S);
}

void loop()
{
  double Irms1 = emon1.calcIrms(2000);  // Calculate Irms only
  double Irms2 = emon2.calcIrms(2000);  // Calculate Irms only

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
  
  // Holding registers, R[0] = 40001 and onwards
  Mb.R[0]++;
  Mb.R[1] = Irms1*1000;
  Mb.R[2] = Irms2*1000;
  Mb.R[3] = noReadCount;

  Mb.Run();
}

