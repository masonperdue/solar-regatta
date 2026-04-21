// Library Includes
#include <Wire.h> // I2C Library
#include <Adafruit_ADS1X15.h>
#include <esp_now.h> // used for: wireless data transmission
#include <esp_wifi.h> // used for: wireless data transmission

// -----------------
//    Definitions
// -----------------
#define VOLTAGE_AVERAGING_FACTOR 0.25
#define batCapacity 34200.0 // As = 9.5 Ah * 60 min/h * 60 s/min; Should be adjusted after testing
#define samplesPerSecond 10 

// Pin definition
#define SDA_PIN 8
#define SCL_PIN 9

// Voltage divider values
#define R1 1000000.0
#define R2 100000.0

// Limits
#define voltageHys 0.15
#define currDrainThreshold 0.75 // No Motor connection draws a max of 0.60A

// ---------------------
//    Object Creation
// ---------------------
Adafruit_ADS1115 adc; // ADDR to GND | 0 = div, 1 = bat, 2 = sol, 3 = volt div

// -----------------------------------------
//    Variable Declaration/Initialization
// -----------------------------------------
uint8_t receiverAddress[] = {0xac, 0xa7, 0x04, 0x12, 0x64, 0x8c}; // address of ESP to receive data 
float batCapRemaining = batCapacity;  
long adc_ain0_div = 0;
long adc_ain1_bat = 0;
long adc_ain2_sol = 0;
long adc_ain3_pack = 0;
int samples = 0; // Each loop will increase samples with data being sent each second
uint32_t lastComputeMs = 0;

// Variables (to be Sent) 
float ampsBat = 0.0, ampsSol = 0.0, batSoC = 100.0;
float batVolt = 0.0;

// --------------------
//    ESP-NOW CONFIG
// --------------------

// Data Packet to be sent
struct __attribute__((packed)) dataPacket {
  float ampsBat, ampsSol, batSoC; // Battery Current draw and charge
  float batVolt; // Battery voltage
};

// Outputs success/fail data to Serial regarding an attempted transmission
void transmissionComplete(const wifi_tx_info_t *info, esp_now_send_status_t transmissionStatus) {
  if (transmissionStatus == 0) {
    Serial.println("Data sent Successfully");
  }
  else {
    Serial.print("Error Code: ");
    Serial.println(transmissionStatus);
  }
}

// --------------------------------
//           SETUP & LOOP
// --------------------------------

// Begin Serial Output and Initialize components
void setup(void)
{
  // Initialize serial
  Serial.begin(921600);
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.print("Initializing...");

  // Set ADC gain to 2/3x gain; +/- 6.144V; 1 bit = 0.1875mV (default)
  adc.setGain(GAIN_TWOTHIRDS);

  // Configure Pins for operation
  Wire.begin(SDA_PIN, SCL_PIN); // Configures I2C

  // Initialize Wifi & Drivers
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  // Set the mode and Long range protocol
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);

  // Start/Disconnect wifi to finish initialization
  esp_wifi_start();
  esp_wifi_disconnect();

  while (!adc.begin(0x48))
  {
    Serial.println("Failed to initialize ADC.");
    delay(200);
  }
  while (esp_now_init()) 
  {
    Serial.println("ESP-NOW initialization failed");
    delay(200);
  }

  // Peer info
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, receiverAddress, 6);
  peer.channel = 0;  
  peer.encrypt = false;

  // Add peer and register a callback function
  esp_now_register_send_cb(transmissionComplete); // this function is called once all data is sent
  while (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("Failed to add peer");
    delay(200);
  }

  Serial.println("Initialized.");
}

// Runs Monitoring functions and sends data every second
void loop(void)
{

  // Run each Monitoring function
  monitorCurrent();
  monitorVoltage(); 

  samples++;
  if (samples == samplesPerSecond) {
    samples = 0;
    sendData();
  }
  delay(1000 / samplesPerSecond);  // Delays based on the samples per second 
}

// -------------------------------------
//           Monitor Functions
// -------------------------------------

// Reads voltage values from the ADC
void monitorCurrent() {
  
  // Read each analog input pin single-ended 
  adc_ain0_div += adc.readADC_SingleEnded(0);
  adc_ain1_bat += adc.readADC_SingleEnded(1);
  adc_ain2_sol += adc.readADC_SingleEnded(2);
  
}

// Checks the voltage of the battery, includes total delay of 20ms
void monitorVoltage()
{

  adc_ain3_pack = adc.readADC_SingleEnded(3);

  // Converts the adc reading to volts, then converts to battery voltage using volt divider values
  batVolt = (batVolt * VOLTAGE_AVERAGING_FACTOR) + ((1 - VOLTAGE_AVERAGING_FACTOR) * adc.computeVolts(adc_ain3_pack) * ((R1 + R2) / R2));

}

// -------------------------------------
//           Convert Functions
// -------------------------------------

// Converts a cycles voltage readings into corresponding amperage and SoC, resets vars for next cycle
void computeCurrent() {
  // Average readings according to sample rate
  adc_ain0_div /= samplesPerSecond;
  adc_ain1_bat /= samplesPerSecond;
  adc_ain2_sol /= samplesPerSecond;

  // Convert analog read to volts for the divider
  float voltsDiv = adc.computeVolts(adc_ain0_div);

  // Convert voltage to current for battery and solar
  ampsBat = voltsToAmps(adc.computeVolts(adc_ain1_bat), voltsDiv);
  ampsSol = voltsToAmps(adc.computeVolts(adc_ain2_sol), voltsDiv);

  // Track battery SoC
  if (ampsBat < -currDrainThreshold || ampsBat > currDrainThreshold)
  {
    batCapRemaining -= ampsBat * ((millis() - lastComputeMs) / 1000);  
    batSoC = (batCapRemaining / batCapacity) * 100.0; // Percentage
  }

  // update time of last computation
  lastComputeMs = millis();

  // Reset tracking variables to zero
  adc_ain0_div = 0;
  adc_ain1_bat = 0;
  adc_ain2_sol = 0;
}

// Converts ADC voltage readings to amperes
float voltsToAmps(float currentVolts, float zeroVolts)
{
  return ((currentVolts - zeroVolts) * 1000) / 40;  // difference in measured and resting voltage times 1000 to get mV and divided by 40 mV/A
}

// --------------------------------------
//           Data Send Function
// --------------------------------------

void sendData() {

  // Convert voltage readings to current values from bat & sol
  computeCurrent();

  // Declare and initialize a data packet to send
  dataPacket packet;
  packet.ampsBat = ampsBat;
  packet.ampsSol = ampsSol;
  packet.batSoC = batSoC;
  packet.batVolt = batVolt;

  // Send Data
  esp_now_send(receiverAddress, (uint8_t *) &packet, sizeof(packet));

}
