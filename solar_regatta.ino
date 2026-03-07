#include <Wire.h>               // I2C Library
#include <Adafruit_ADS1X15.h>

#define batCapacity 32400.0       // As = 9 Ah * 60 min/h * 60 s/min; Should be adjusted after testing

// Create ADC objects
Adafruit_ADS1115 adc;  // ADDR to GND; Motors

// Define variables
float batCapRemaining = batCapacity;    // As

void setup(void)
{
  // Initialize serial
  Serial.begin(9600);
  Serial.println("Serial Initialized.");

  // Set ADC's gain to 1x gain; +/- 6.144V; 1 bit = 0.1875mV (default)
  adc.setGain(GAIN_TWOTHIRDS);

  // Initialize ADC's
  if (!adc.begin(0x48))
  {
    Serial.println("Failed to initialize ADC.");
  }
}

void loop(void)
{
  // Define variables
  long adc_ain0_div = 0;
  long adc_ain1_mot = 0;
  long adc_ain2_bat = 0;
  // long adc_ain3_sol = 0;
  float volts_div = 0.0;
  float volts_mot = 0.0;
  float volts_bat = 0.0;
  // float volts_sol = 0.0;
  float amps_div = 0.0;
  float amps_mot = 0.0;
  float amps_bat = 0.0;
  // float amps_sol = 0.0;
  float batSoC = 0.0;
  float batEff = 0.0;
  float solEff = 0.0;

  // Read each analog input pin single-ended (10 times in a second)
  for (int i = 0; i < 10; i++)
  {
    adc_ain0_div += adc.readADC_SingleEnded(0);
    adc_ain1_mot += adc.readADC_SingleEnded(1);
    adc_ain2_bat += adc.readADC_SingleEnded(2);
    // adc_ain3_sol += adc.readADC_SingleEnded(3);

    // Wait 1/10 s
    delay(100);
  }
  
  // Average 10 readings in a second
  adc_ain0_div /= 10;
  adc_ain1_mot /= 10;
  adc_ain2_bat /= 10;
  // adc_ain3_sol /= 10;

  // Convert analog read to volts
  volts_div = adc.computeVolts(adc_ain0_div);
  volts_mot = adc.computeVolts(adc_ain1_mot);
  volts_bat = adc.computeVolts(adc_ain2_bat);
  // volts_sol = adc.computeVolts(adc_ain3_sol);

  // Convert voltage to current
  amps_mot = voltsToAmps(volts_mot, volts_div);
  amps_bat = voltsToAmps(volts_bat, volts_div);
  // amps_sol = voltsToAmps(volts_sol, volts_div);

  // Track battery SoC
  batCapRemaining -= amps_bat * 1;   // Since loop runs every second, time is 1s
  batSoC = batCapRemaining / batCapacity * 100.0;   // Percentage

  // Calculate efficiencies
  if (amps_bat < 0.1 && amps_bat > -0.1)
  {
    batEff = amps_mot / amps_bat * 100.0;
  }
  else
  {
    batEff = 0.0;
  }

  // if (!(amps_sol == 0.0))
  // {
  //   solEff = amps_mot / amps_sol * 100.0;
  // }
  // else
  // {
  //   solEff = 0.0;
  // }

  // Print analog inputs and calculated voltages to serial
  Serial.println("-----------------------------------------------------------");
  Serial.print("Battery: "); /* Serial.print(adc_ain2_bat); Serial.print("  "); Serial.print(volts_bat); Serial.print("V  "); */ Serial.print(amps_bat); Serial.print("A  Eff: "); Serial.print(batEff); Serial.print("%  SoC: "); Serial.print(batSoC); Serial.println("%");
  // Serial.print("Solar: "); Serial.print(adc_ain2_sol); Serial.print("  "); Serial.print(volts_sol); Serial.print("V  "); Serial.print(amps_sol); Serial.print("A  Eff: "); Serial.print(solEff); Serial.println("%");
  Serial.print("Motor: "); /* Serial.print(adc_ain1_mot); Serial.print("  "); Serial.print(volts_mot); Serial.print("V  "); */ Serial.print(amps_mot); Serial.println("A");
}

float voltsToAmps(float currentVolts, float zeroVolts)
{
  return ((currentVolts - zeroVolts) * 1000) / 40;      // difference in measured and resting voltage times 1000 to get mV and divided by 40 mV/A
}






















