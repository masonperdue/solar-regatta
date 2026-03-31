#include <Wire.h> // I2C Library
#include <Adafruit_ADS1X15.h>

#define batCapacity 34200.0 // As = 9.5 Ah * 60 min/h * 60 s/min; Should be adjusted after testing

// Create ADC objects
Adafruit_ADS1115 adc; // ADDR to GND

// Define variables
float batCapRemaining = batCapacity;  // As
long adc_ain0_div = 0;
long adc_ain1_bat = 0;
long adc_ain2_sol = 0;
float volts_div = 0.0;
float volts_bat = 0.0;
float volts_sol = 0.0;
float amps_div = 0.0;
float amps_bat = 0.0;
float amps_sol = 0.0;
float batSoC = 100.0;

void setup(void)
{
  // Initialize serial
  Serial.begin(9600);
  Serial.println("Serial Initialized.");

  // Set ADC gain to 2/3x gain; +/- 6.144V; 1 bit = 0.1875mV (default)
  adc.setGain(GAIN_TWOTHIRDS);

  // Initialize ADC
  while (!adc.begin(0x48))
  {
    Serial.println("Failed to initialize ADC.");
  }
}

void loop(void)
{
  // Reset input variables
  adc_ain0_div = 0;
  adc_ain1_bat = 0;
  adc_ain2_sol = 0;

  // Read each analog input pin single-ended (10 times in a second)
  for (int i = 0; i < 10; i++)
  {
    adc_ain0_div += adc.readADC_SingleEnded(0);
    adc_ain1_bat += adc.readADC_SingleEnded(1);
    adc_ain2_sol += adc.readADC_SingleEnded(2);

    // Wait 1/10s
    delay(100);
  }
  
  // Average 10 readings in a second
  adc_ain0_div /= 10;
  adc_ain1_bat /= 10;
  adc_ain2_sol /= 10;

  // Convert analog read to volts
  volts_div = adc.computeVolts(adc_ain0_div);
  volts_bat = adc.computeVolts(adc_ain1_bat);
  volts_sol = adc.computeVolts(adc_ain2_sol);

  // Convert voltage to current
  amps_bat = voltsToAmps(volts_bat, volts_div);
  amps_sol = voltsToAmps(volts_sol, volts_div);

  // Track battery SoC
  if (amps_bat < -0.2 || amps_bat > 0.2)
  {
    batCapRemaining -= amps_bat * 1;  // Since loop runs every second, time is 1s
    batSoC = batCapRemaining / batCapacity * 100.0; // Percentage
  }

  // Print analog inputs and calculated voltages to serial
  Serial.println("---------");
  Serial.print("Battery:  "); Serial.print(amps_bat); Serial.print("A  SoC: "); Serial.print(batSoC); Serial.println("%");
  Serial.print("Solar:  "); Serial.print(amps_sol); Serial.println("A");
}

float voltsToAmps(float currentVolts, float zeroVolts)
{
  return ((currentVolts - zeroVolts) * 1000) / 40;  // difference in measured and resting voltage times 1000 to get mV and divided by 40 mV/A
}


 











