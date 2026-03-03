#include <Wire.h>               // I2C Library
#include <Adafruit_ADS1X15.h>

#define batCapacity 32400.0       // As = 9 Ah * 60 min/h * 60 s/min; Should be adjusted after testing

// Create ADC objects
Adafruit_ADS1115 adc2;  // ADDR to VCC; Batteries & Panels
Adafruit_ADS1115 adc1;  // ADDR to GND; Motors

// Define variables
float batCapRemaining = batCapacity;    // As

void setup(void)
{
  // Initialize serial
  Serial.begin(9600);
  Serial.println("Serial Initialized.");

  // Set ADC's gain to 1x gain; +/- 6.144V; 1 bit = 0.1875mV (default)
  adc2.setGain(GAIN_TWOTHIRDS);
  adc1.setGain(GAIN_TWOTHIRDS);

  // Initialize ADC's
  if (!adc1.begin(0x48))
  {
    Serial.println("Failed to initialize ADS 1.");
    // while (1);
  }

  if (!adc2.begin(0x49))
  {
    Serial.println("Failed to initialize ADS 2.");
    // while (1);
  } 
}

void loop(void)
{
  // Define variables
  long adc2_ain0_bat = 0;
  long adc2_ain1_sol = 0;
  long adc2_ain3_div = 0;
  long adc1_ain0_65lb = 0;
  long adc1_ain1_80lb = 0;
  long adc1_ain3_div = 0;
  float volts_bat = 0.0;
  float volts_sol = 0.0;
  float volts_div1 = 0.0;
  float volts_65lb = 0.0;
  float volts_80lb = 0.0;
  float volts_div2 = 0.0;
  float amps_bat = 0.0;
  float amps_sol = 0.0;
  float amps_div1 = 0.0;
  float amps_65lb = 0.0;
  float amps_80lb = 0.0;
  float amps_div2 = 0.0;
  float batSoC = 0.0;
  float batEff = 0.0;
  float solEff = 0.0;

  // Read each analog input pin single-ended (10 times in a second)
  for (int i = 0; i < 10; i++)
  {
    adc2_ain0_bat += adc2.readADC_SingleEnded(0);
    adc2_ain1_sol += adc2.readADC_SingleEnded(1);
    adc2_ain3_div += adc2.readADC_SingleEnded(3);
    adc1_ain0_65lb += adc1.readADC_SingleEnded(0);
    adc1_ain1_80lb += adc1.readADC_SingleEnded(1);
    adc1_ain3_div += adc1.readADC_SingleEnded(3);

    // Wait 1/10 s
    delay(100);
  }
  
  // Average 10 readings in a second
  adc2_ain0_bat /= 10;
  adc2_ain1_sol /= 10;
  adc2_ain3_div /= 10;
  adc1_ain0_65lb /= 10;
  adc1_ain1_80lb /= 10;
  adc1_ain3_div /= 10;

  // Convert analog read to volts
  volts_bat = adc2.computeVolts(adc2_ain0_bat);
  volts_sol = adc2.computeVolts(adc2_ain1_sol);
  volts_div1 = adc2.computeVolts(adc2_ain3_div);
  volts_65lb = adc1.computeVolts(adc1_ain0_65lb);
  volts_80lb = adc1.computeVolts(adc1_ain1_80lb);
  volts_div2 = adc1.computeVolts(adc1_ain3_div);

  // Convert voltage to current
  amps_bat = voltsToAmps(volts_bat, volts_div1);
  amps_sol = voltsToAmps(volts_sol, volts_div1);
  amps_65lb = voltsToAmps(volts_65lb, volts_div2);
  amps_80lb = voltsToAmps(volts_80lb, volts_div2);

  // Track battery SoC
  batCapRemaining -= amps_bat * 1;   // Since loop runs every second, time is 1s
  batSoC = batCapRemaining / batCapacity * 100.0;   // Percentage

  // Calculate efficiencies
  if (!(amps_bat == 0.0))
  {
    batEff = (amps_65lb + amps_80lb) / amps_bat * 100.0;
  }
  else
  {
    batEff = 0.0;
  }

  if (!(amps_sol == 0.0))
  {
    solEff = (amps_65lb + amps_80lb) / amps_sol * 100.0;
  }
  else
  {
    solEff = 0.0;
  }

  // Print analog inputs and calculated voltages to serial
  Serial.println("-----------------------------------------------------------");
  Serial.print("Battery: "); Serial.print(adc2_ain0_bat); Serial.print("  "); Serial.print(volts_bat); Serial.print("V  "); Serial.print(amps_bat); Serial.print("A  Eff: "); Serial.print(batEff); Serial.print("%  SoC: "); Serial.print(batSoC); Serial.println("%");
  Serial.print("Solar: "); Serial.print(adc2_ain1_sol); Serial.print("  "); Serial.print(volts_sol); Serial.print("V  "); Serial.print(amps_sol); Serial.print("A  Eff: "); Serial.print(solEff); Serial.println("%");
  Serial.print("65lb Motor: "); Serial.print(adc1_ain0_65lb); Serial.print("  "); Serial.print(volts_65lb); Serial.print("V  "); Serial.print(amps_65lb); Serial.println("A");
  Serial.print("80lb Motor: "); Serial.print(adc1_ain1_80lb); Serial.print("  "); Serial.print(volts_80lb); Serial.print("V  "); Serial.print(amps_80lb); Serial.println("A");
}

float voltsToAmps(float currentVolts, float zeroVolts)
{
  return ((currentVolts - zeroVolts) * 1000) / 40;      // difference in measured and resting voltage times 1000 to get mV and divided by 40 mV/A
}






















