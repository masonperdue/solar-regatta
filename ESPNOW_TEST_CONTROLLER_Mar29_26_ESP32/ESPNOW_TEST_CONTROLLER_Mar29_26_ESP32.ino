// Includes
#include <esp_now.h>
#include <esp_wifi.h>

// Name defenitions
#define MY_NAME        "CONTROLLER_NODE"
#define WIFI_CHANNEL   1

uint8_t receiverAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
int t = 0;

struct __attribute__((packed)) dataPacket {
  int state;

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
  esp_wifi_set_protocol( WIFI_IF_STA, WIFI_PROTOCOL_LR);
  esp_wifi_disconnect();

  // Check if ESP-NOW initialize correctly, prematurely end setup if not
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  // Peer info
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, receiverAddress, 6);
  peer.channel = 0;  
  peer.encrypt = false;

  // Add peer and register a callback function
  esp_now_register_send_cb(transmissionComplete); // this function is called once all data is sent
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("Initialized.");
}

void loop() {
  // Repeatedly send data for testing
  dataPacket packet;

  packet.state = t;

  esp_now_send(receiverAddress, (uint8_t *) &packet, sizeof(packet));

  t++;
  delay(1000);
}
