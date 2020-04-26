#include "EmonLib.h"                    // Include Emon Library
EnergyMonitor emon1;                    // Create an instance
EnergyMonitor emon2;                    // Create an instance

void setup()
{  
  Serial.begin(9600);
  
  emon1.current(0, 29.0);               // Current: input pin, calibration.
  emon2.current(1, 29.0);               // Current: input pin, calibration.
  
  // First values are rubbish, don't use
  emon1.calcIrms(5000);
  emon2.calcIrms(5000);
}

void loop()
{
  double Irms1 = emon1.calcIrms(10000);  // Calculate Irms only
  double Irms2 = emon2.calcIrms(10000);  // Calculate Irms only
  
  // Clip the values when less than 50mA
  if(Irms1 < 0.05) Irms1 = 0.0;
  if(Irms2 < 0.05) Irms2 = 0.0;
  
  Serial.print(Irms1*240.0);	        // Apparent power
  Serial.print("W ");
  Serial.print(Irms1);		        // Irms
  Serial.print("A, ");
  Serial.print(Irms2*240.0);	        // Apparent power
  Serial.print("W ");
  Serial.print(Irms2);		        // Irms
  Serial.println("A");
}
