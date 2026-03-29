#include <Wire.h> // I2C Library
#include <Adafruit_ADS1X15.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// -----------------------------
// Pin definitions
// -----------------------------
#define tempPin 2
#define relayPin 11

// -----------------------------
// OLED settings
// -----------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// -----------------------------
// Voltage divider values
// -----------------------------
#define R1 1000000.0
#define R2 100000.0

// -----------------------------
// Limits
// -----------------------------
#define packVoltageHigh 25.2
#define packVoltageLow 20.4
#define displayVoltageMin 18.0
#define voltageHys 0.6

#define packTempHigh 60.0
#define packTempLow -20.0

// Create objects
Adafruit_ADS1115 adc;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
OneWire oneWire(tempPin);
DallasTemperature tempSensor(&oneWire);

// Define variables
long adc_ain0_pack = 0;
float volts_adc = 0.0;
float volts_pack = 0.0;
float tempC = 0.0;

bool voltageOK = false;
bool tempOK = false;
bool relayState = false;

float batteryPercent = 0.0;
float batteryPercentDisplay = 0.0;

// -----------------------------
// Function prototypes
// -----------------------------
float adcToPackVolts(float adcVolts);
void monitorVoltage(void);
void checkVoltageWithHysteresis(void);
void monitorTemperature(void);
void controlRelay(void);
void displayData(void);
void drawCenteredText(const char *text, int y, int textSize);
void drawBatteryBar(void);
void sendBluetoothData(void);
float voltageToPercent(float voltage);
void updateBatteryAnimation(void);

void setup(void)
{
  // HC-06 is on Nano RX0/TX1, so Serial is the Bluetooth output
  Serial.begin(9600);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    while (1);
  }

  tempSensor.begin();

  adc.setGain(GAIN_TWOTHIRDS);

  if (!adc.begin(0x48))
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 28);
    display.println(F("ADC Init Failed"));
    display.display();
    while (1);
  }

  // Draw startup screen
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  drawCenteredText("BATTERY MONITOR", 0, 1);
  display.drawRoundRect(14, 11, 100, 12, 2, SSD1306_WHITE);
  display.fillRect(114, 14, 4, 6, SSD1306_WHITE);
  display.display();

  delay(500);
}

void loop(void)
{
  Serial.println(freeMemory());

  monitorVoltage();
  monitorTemperature();
  controlRelay();
  updateBatteryAnimation();
  displayData();
  sendBluetoothData();

  delay(300);
}

float adcToPackVolts(float adcVolts)
{
  return adcVolts * ((R1 + R2) / R2);
}

void monitorVoltage(void)
{
  adc_ain0_pack = 0;

  for (int i = 0; i < 10; i++)
  {
    adc_ain0_pack += adc.readADC_SingleEnded(0);
    delay(3);
  }

  adc_ain0_pack /= 10;
  volts_adc = adc.computeVolts(adc_ain0_pack);
  volts_pack = adcToPackVolts(volts_adc);

  checkVoltageWithHysteresis();
}

void checkVoltageWithHysteresis(void)
{
  if (voltageOK)
  {
    if (volts_pack > (packVoltageHigh + voltageHys) || volts_pack < (packVoltageLow - voltageHys))
    {
      voltageOK = false;
    }
  }
  else
  {
    if (volts_pack <= packVoltageHigh && volts_pack >= packVoltageLow)
    {
      voltageOK = true;
    }
  }
}

void monitorTemperature(void)
{
  tempSensor.requestTemperatures();
  tempC = tempSensor.getTempCByIndex(0);

  if (tempC == DEVICE_DISCONNECTED_C)
  {
    tempOK = false;
    return;
  }

  if (tempC <= packTempHigh && tempC >= packTempLow)
  {
    tempOK = true;
  }
  else
  {
    tempOK = false;
  }
}

void controlRelay(void)
{
  if (voltageOK && tempOK)
  {
    relayState = true;
    digitalWrite(relayPin, HIGH);
  }
  else
  {
    relayState = false;
    digitalWrite(relayPin, LOW);
  }
}

float voltageToPercent(float voltage)
{
  float percent;

  if (voltage <= displayVoltageMin)
  {
    percent = 0.0;
  }
  else if (voltage >= packVoltageHigh)
  {
    percent = 100.0;
  }
  else
  {
    percent = ((voltage - displayVoltageMin) / (packVoltageHigh - displayVoltageMin)) * 100.0;
  }

  return percent;
}

void updateBatteryAnimation(void)
{
  batteryPercent = voltageToPercent(volts_pack);

  // Smoothly move displayed value toward actual value
  batteryPercentDisplay = batteryPercentDisplay + (batteryPercent - batteryPercentDisplay) * 0.18;

  // Snap when very close so it does not drift forever
  if (fabs(batteryPercent - batteryPercentDisplay) < 0.2)
  {
    batteryPercentDisplay = batteryPercent;
  }
}

void displayData(void)
{
  char lineBuffer[24];
  char tempText[24];

  // Clear only changing areas to reduce flicker
  display.fillRect(15, 12, 98, 10, SSD1306_BLACK);  // battery fill area
  display.fillRect(0, 24, 128, 40, SSD1306_BLACK);  // lower text area

  // Redraw battery outline
  display.drawRoundRect(14, 11, 100, 12, 2, SSD1306_WHITE);
  display.fillRect(114, 14, 4, 6, SSD1306_WHITE);

  // Battery graphic with animated fill and percent text
  drawBatteryBar();

  // Voltage text
  dtostrf(volts_pack, 4, 2, lineBuffer);
  strcat(lineBuffer, "V");
  drawCenteredText(lineBuffer, 26, 2);

  // Temperature text with degree symbol
  dtostrf(tempC, 4, 1, tempText);
  int len = strlen(tempText);
  tempText[len] = char(248);  // degree symbol in default font
  tempText[len + 1] = 'C';
  tempText[len + 2] = '\0';
  drawCenteredText(tempText, 41, 2);

  // Relay status
  drawCenteredText(relayState ? "RELAY ON" : "RELAY OFF", 56, 1);

  display.display();
}

void drawCenteredText(const char *text, int y, int textSize)
{
  int16_t x1, y1;
  uint16_t w, h;

  display.setTextSize(textSize);
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);

  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, y);
  display.print(text);
}

void drawBatteryBar(void)
{
  int fillWidth;
  char percentText[8];
  int16_t x1, y1;
  uint16_t w, h;
  int textX;
  int textY = 13;

  fillWidth = (int)((batteryPercentDisplay / 100.0) * 96.0);

  if (fillWidth < 0)
  {
    fillWidth = 0;
  }

  if (fillWidth > 96)
  {
    fillWidth = 96;
  }

  // Fill bar
  display.fillRect(16, 13, fillWidth, 8, SSD1306_WHITE);

  // Build percent text
  snprintf(percentText, sizeof(percentText), "%d%%", (int)(batteryPercentDisplay + 0.5));

  // Center text inside battery bar
  display.setTextSize(1);
  display.getTextBounds(percentText, 0, 0, &x1, &y1, &w, &h);
  textX = 14 + (100 - w) / 2;

  // Draw text in contrasting color depending on fill
  if (fillWidth > 48)
  {
    display.setTextColor(SSD1306_BLACK);
  }
  else
  {
    display.setTextColor(SSD1306_WHITE);
  }

  display.setCursor(textX, textY);
  display.print(percentText);

  display.setTextColor(SSD1306_WHITE);
}

void sendBluetoothData(void)
{
  Serial.print(F("Voltage: "));
  Serial.print(volts_pack, 2);
  Serial.print(F(" V, Temp: "));
  Serial.print(tempC, 1);
  Serial.print(F(" C, Relay: "));
  Serial.print(relayState ? F("ON") : F("OFF"));
  Serial.print(F(", VoltageOK: "));
  Serial.print(voltageOK ? F("YES") : F("NO"));
  Serial.print(F(", TempOK: "));
  Serial.println(tempOK ? F("YES") : F("NO"));
}

int freeMemory() {
  char top;
  extern int __heap_start, *__brkval;
  // We cast &top to (int) so the subtraction returns a number, not a pointer
  return (int) &top - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}