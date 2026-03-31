// Includes
#include <esp_now.h> // used for: wireless data transmission
#include <esp_wifi.h> // used for: wireless data transmission

// --------------------
//    ESP-NOW CONFIG
// --------------------

// Data Packet to be received
struct __attribute__((packed)) dataPacket {
  float ampsBat, ampsSol, batSoC; // Battery Current draw and charge
  float batVolt, batTempC; // Battery voltage and temperature
  bool relayState; // Status of the relay (True = Closed, False = Open)
  int err = 0; // Error codes: 1 = Temp Sensor Disconnect
};

// executes upon receiving data
void dataReceived(const esp_now_recv_info *info, const uint8_t *data, int datalength) {

  // Converts the sender's mac address into something printable
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", info->src_addr[0], info->src_addr[1], info->src_addr[2], info->src_addr[3], info->src_addr[4], info->src_addr[5]);

  // Copy the transferred data into out packet
  dataPacket packet;
  memcpy(&packet, data, sizeof(packet));

  // displays data to Serial
  displayData(packet, macStr);

}

// --------------------------------
//           SETUP & LOOP
// --------------------------------

void setup() {
  // Open Serial for debugging
  Serial.begin(921600);

  // Print the Initialization and Mac address
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.print("Initializing...");

  // Start and disconnect from WiFi
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_disconnect();

  // Check if ESP-NOW initialize correctly, prematurely end setup if not
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  // Register function to be called upon receiving data
  esp_now_register_recv_cb(dataReceived);

  Serial.println("Initialized.");
}

void loop() {
  // Note: all execution occurs upon receiving data
  delay(100);
}

// --------------------------------
//           Display Data
// --------------------------------

// Displays the data contained in a packet
void displayData(dataPacket packet, char *macAddr) {

  // Upper Serial Divider
  Serial.println();
  Serial.println("----------------------------------------------------------------------");

  Serial.printf("Receiving Data from: %s\n\n", macAddr);
  
  Serial.printf("Battery Life: %-9.2f/% | Voltage: %.2f V, Temp: %.1f C\n", packet.batSoC, packet.batVolt, packet.batTempC);
  Serial.printf("Battery Current: %-7.2f A | Solar Current: %-7.2f A\n", packet.ampsBat, packet.ampsSol);
  Serial.printf("Relay is: %s\n\n", packet.relayState ? "Closed" : "Open");

  // Displays Errors according to Error Code
  if (packet.err) 
  {
    Serial.print("ERROR: ");
    
    if (packet.err == 1) {
      Serial.println("Temperature Sensor disconnected, please reconnect.");
    }

  }

  // Lower Serial Divider
  Serial.println();
  Serial.println("----------------------------------------------------------------------");

}