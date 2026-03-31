// Includes
#include <esp_now.h>
#include <esp_wifi.h>

// Name defenitions
#define MY_NAME   "SLAVE_NODE"

struct __attribute__((packed)) dataPacket {
  int state;
};

void dataReceived(const esp_now_recv_info *info, const uint8_t *data, int datalength) {
  char macStr[18];
  dataPacket packet;

  // Converts the sender's mac address into something printable
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", info->src_addr[0], info->src_addr[1], info->src_addr[2], info->src_addr[3], info->src_addr[4], info->src_addr[5]);
  
  Serial.println();
  Serial.print("Received data from: ");
  Serial.println(macStr);

  // Copy the transferred data into out packet
  memcpy(&packet, data, sizeof(packet));

  Serial.print("Time elapsed: ");
  Serial.println(packet.state);
  // Can add additional functionality upon receiving data 

}

void setup() {
  // Open Serial for debugging
  Serial.begin(921600);

  // Print the Initialization and Mac address
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.print("Initializing...");
  Serial.println(MY_NAME);
  
  // Retrieve Mac Address
  uint8_t myAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  char macStr[18];
  esp_wifi_get_mac(WIFI_IF_STA, myAddress);
  // Converts the my mac address into printable
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", myAddress[0], myAddress[1], myAddress[2], myAddress[3], myAddress[4], myAddress[5]);
  Serial.print("Mac Address is: ");
  Serial.println(macStr);

  // Start and disconnect from WiFi
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_disconnect();

  // Check if ESP-NOW initialize correctly, prematurely end setup if not
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_register_recv_cb(dataReceived);

  Serial.println("Initialized.");
}

void loop() {
  delay(3000);
  Serial.println("------------------");
}
