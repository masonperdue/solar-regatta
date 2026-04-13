// Includes
#include <esp_now.h> // used for: wireless data transmission
#include <esp_wifi.h> // used for: wireless data transmission

#include <BLEDevice.h> // Includes BLEServer.h
#include <BLE2901.h> // Characteristic description descriptor
#include <BLE2902.h> // Characteristic notifications descriptor
#include <BLE2904.h> // Characteristic data type descriptor

// Definitions
#define DEVICE_NAME "Solar Regatta - SC"
#define SERVICE_UUID "ee052fcc-a927-4537-9570-c4ee8f66e6f4"
#define TEMPERATURE_CHARACTERISTIC_UUID "71da89ed-d11e-461a-bc69-c5c2fc4c1a84"
#define VOLTAGE_CHARACTERISTIC_UUID "503d43ce-71c8-47b1-9c2d-cdb3a9386a0e"
#define CHARGE_CHARACTERISTIC_UUID "92a0134d-f04d-40c2-8bc1-167c5f772c09"
#define BATTERY_CURRENT_CHARACTERISTIC_UUID "494cd727-9581-4ead-b59f-ce63f2e1e2ef"
#define SOLAR_CURRENT_CHARACTERISTIC_UUID "2dcc0469-6384-4c52-ad33-2483cb315f38"
#define RELAY_STATE_CHARACTERISTIC_UUID "8c42e7b3-8445-4cc4-b2a6-9e188c7af6a0"
#define ERROR_CHARACTERISTIC_UUID "4e3f133d-e80c-4879-8fe5-9a06b56bebd1"

#define MAX_CONNECTIONS 3

// Variable declarations/initializations
bool deviceConnected;
int devicesConnected;
BLECharacteristic *pTempCharacteristic = nullptr;
BLECharacteristic *pVoltageCharacteristic = nullptr;
BLECharacteristic *pChargeCharacteristic = nullptr;
BLECharacteristic *pBatCurrentCharacteristic = nullptr;
BLECharacteristic *pSolCurrentCharacteristic = nullptr;
BLECharacteristic *pRelayStateCharacteristic = nullptr;
BLECharacteristic *pErrorCharacteristic = nullptr;

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

// Create a datapacket to store information for bluetooth retrieval
dataPacket currPacket;

// executes upon receiving data
void dataReceived(const esp_now_recv_info *info, const uint8_t *data, int datalength) {

  // Converts the sender's mac address into something printable
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", info->src_addr[0], info->src_addr[1], info->src_addr[2], info->src_addr[3], info->src_addr[4], info->src_addr[5]);

  // Copy the transferred data into out packet
  memcpy(&currPacket, data, sizeof(currPacket));

  // displays data to Serial
  displayData(currPacket, macStr);

}

// -----------------------
//    BLE Server CONFIG
// -----------------------

// Client connection callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {

      // Update connection variables
      deviceConnected = true;
      devicesConnected++;

      // Display how many devices are connected
      Serial.print("Client Disconnected. Total Connections: ");
      Serial.println(devicesConnected);

      // start advertising as long as there is space
      if (devicesConnected < MAX_CONNECTIONS) {
        BLEDevice::startAdvertising();
      }

    };

    void onDisconnect(BLEServer* pServer) {

      // update connection variables
      devicesConnected--;
      if (devicesConnected == 0) {
        deviceConnected = false;
      }

      // Display how many devices are now connected
      Serial.print("Client Disconnected. Total Connections: ");
      Serial.println(devicesConnected);

      BLEDevice::startAdvertising(); // Restart advertising so a new device can connect
    }
};

// Battery Temperature characteristc callbacks
class TempCharacteristicCallbacks : public BLECharacteristicCallbacks {

  // Updates temp variable on an attempted read
  void onRead(BLECharacteristic *pTempCharacteristic) {
    char temp[10]; 
    sprintf(temp, "%.2f C", currPacket.batTempC); // Creates a UTF-8 version of float for display
    pTempCharacteristic->setValue(temp);
  }

};

// Battery Voltage characteristc callbacks
class VoltageCharacteristicCallbacks : public BLECharacteristicCallbacks {

  // Updates voltage variable on an attempted read
  void onRead(BLECharacteristic *pVoltageCharacteristic) {
    char temp[10]; 
    sprintf(temp, "%.2f V", currPacket.batVolt); // Creates a UTF-8 version of float for display
    pVoltageCharacteristic->setValue(temp);
  }

};

// Battery Charge characteristc callbacks
class ChargeCharacteristicCallbacks : public BLECharacteristicCallbacks {

  // Updates charge variable on an attempted read
  void onRead(BLECharacteristic *pChargeCharacteristic) {
    char temp[10]; 
    sprintf(temp, "%.2f%%", currPacket.batSoC); // Creates a UTF-8 version of float for display
    pChargeCharacteristic->setValue(temp);
  }

};

// Battery Current characteristc callbacks
class BatCurrentCharacteristicCallbacks : public BLECharacteristicCallbacks {

  // Updates current variable on an attempted read
  void onRead(BLECharacteristic *pBatCurrentCharacteristic) {
    char temp[10]; 
    sprintf(temp, "%.2f  A", currPacket.ampsBat); // Creates a UTF-8 version of float for display
    pBatCurrentCharacteristic->setValue(temp);
  }

};

// Solar Current characteristc callbacks
class SolCurrentCharacteristicCallbacks : public BLECharacteristicCallbacks {

  // Updates current variable on an attempted read
  void onRead(BLECharacteristic *pSolCurrentCharacteristic) {
    char temp[10];
    sprintf(temp, "%.2f  A", currPacket.ampsSol); // Creates a UTF-8 version of float for display
    pSolCurrentCharacteristic->setValue(temp);
  }

};

// Relay State characteristc callbacks
class RelayStateCharacteristicCallbacks : public BLECharacteristicCallbacks {

  // Updates state variable on an attempted read
  void onRead(BLECharacteristic *pRelayStateCharacteristic) {
    pRelayStateCharacteristic->setValue(currPacket.relayState ? "Closed" : "Open");
  }

};

class ErrorCharacteristicCallbacks : public BLECharacteristicCallbacks {

  // Updates current variable on an attempted read
  void onRead(BLECharacteristic *pErrorCharacteristic) {
    // Updates text to current error
    pErrorCharacteristic->setValue(retrieveError(currPacket.err));
  }

};

// --------------------------------
//           SETUP & LOOP
// --------------------------------

void setup() {
  // Open Serial for debugging
  Serial.begin(921600);

  // Print the Initialization for ESP-Now
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.print("Initializing ESP-Now...");

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

  // Check if ESP-NOW initialize correctly, prematurely end setup if not
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  // Register function to be called upon receiving data
  esp_now_register_recv_cb(dataReceived);

  Serial.println("Initialized.");

  // Print the Initialization for BLE Server
  Serial.print("Initializing BLE Service...");

  // Initialize the device using static function for BLE library, and create a server
  BLEDevice::init(DEVICE_NAME);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks()); // Adds server connection callbacks

  // Creates a Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create Characteristics (Viewable information for the client) ------------------

  // Create Temp Characteristic
  pTempCharacteristic = pService->createCharacteristic(
    TEMPERATURE_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );
  pTempCharacteristic->setCallbacks(new TempCharacteristicCallbacks());
  // Add Temp descriptiors (description, notifications, units, datatype)
  BLE2901 *pTemp_2901 = new BLE2901(); // Variable Description
  pTemp_2901->setDescription("Temperature");
  BLE2902 *pTemp_2902 = new BLE2902(); // Variable Notifications
  pTemp_2902->setNotifications(true);
  pTempCharacteristic->addDescriptor(pTemp_2901);
  pTempCharacteristic->addDescriptor(pTemp_2902);

  // Create Voltage Characteristic
  pVoltageCharacteristic = pService->createCharacteristic(
    VOLTAGE_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );
  pVoltageCharacteristic->setCallbacks(new VoltageCharacteristicCallbacks());
  // Add Voltage descriptiors (description, notifications, units, datatype)
  BLE2901 *pVoltage_2901 = new BLE2901(); // Variable Description
  pVoltage_2901->setDescription("Battery Voltage");
  BLE2902 *pVoltage_2902 = new BLE2902(); // Variable Notifications
  pVoltage_2902->setNotifications(true);
  pVoltageCharacteristic->addDescriptor(pVoltage_2901);
  pVoltageCharacteristic->addDescriptor(pVoltage_2902);

  // Create Charge Characteristic
  pChargeCharacteristic = pService->createCharacteristic(
    CHARGE_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );
  pChargeCharacteristic->setCallbacks(new ChargeCharacteristicCallbacks());
  // Add Charge descriptiors (description, notifications, units, datatype)
  BLE2901 *pCharge_2901 = new BLE2901(); // Variable Description
  pCharge_2901->setDescription("Battery Charge (SoC)");
  BLE2902 *pCharge_2902 = new BLE2902(); // Variable Notifications
  pCharge_2902->setNotifications(true);
  pChargeCharacteristic->addDescriptor(pCharge_2901);
  pChargeCharacteristic->addDescriptor(pCharge_2902);

  // Create Battery Current Characteristic
  pBatCurrentCharacteristic = pService->createCharacteristic(
    BATTERY_CURRENT_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );
  pBatCurrentCharacteristic->setCallbacks(new BatCurrentCharacteristicCallbacks());
  // Add Battery Current descriptiors (description, notifications, units, datatype)
  BLE2901 *pBatCurrent_2901 = new BLE2901(); // Variable Description
  pBatCurrent_2901->setDescription("Battery Current");
  BLE2902 *pBatCurrent_2902 = new BLE2902(); // Variable Notifications
  pBatCurrent_2902->setNotifications(true);
  pBatCurrentCharacteristic->addDescriptor(pBatCurrent_2901);
  pBatCurrentCharacteristic->addDescriptor(pBatCurrent_2902);

  // Create Solar Current Characteristic
  pSolCurrentCharacteristic = pService->createCharacteristic(
    SOLAR_CURRENT_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );
  pSolCurrentCharacteristic->setCallbacks(new SolCurrentCharacteristicCallbacks());
  // Add Solar Current descriptiors (description, notifications, units, datatype)
  BLE2901 *pSolCurrent_2901 = new BLE2901(); // Variable Description
  pSolCurrent_2901->setDescription("Solar Current");
  BLE2902 *pSolCurrent_2902 = new BLE2902(); // Variable Notifications
  pSolCurrent_2902->setNotifications(true);
  pSolCurrentCharacteristic->addDescriptor(pSolCurrent_2901);
  pSolCurrentCharacteristic->addDescriptor(pSolCurrent_2902);

  // Create Relay State Characteristic
  pRelayStateCharacteristic = pService->createCharacteristic(
    RELAY_STATE_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );
  pRelayStateCharacteristic->setCallbacks(new RelayStateCharacteristicCallbacks());
  // Add Relay State descriptiors (description, notifications, datatype)
  BLE2901 *pRelayState_2901 = new BLE2901(); // Variable Description
  pRelayState_2901->setDescription("Relay State");
  BLE2902 *pRelayState_2902 = new BLE2902(); // Variable Notifications
  pRelayState_2902->setNotifications(true);
  pRelayStateCharacteristic->addDescriptor(pRelayState_2901);
  pRelayStateCharacteristic->addDescriptor(pRelayState_2902);

  // Create Relay State Characteristic
  pErrorCharacteristic = pService->createCharacteristic(
    ERROR_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );
  pErrorCharacteristic->setCallbacks(new ErrorCharacteristicCallbacks());
  // Add Relay State descriptiors (description, datatype)
  BLE2901 *pError_2901 = new BLE2901(); // Variable Description
  pError_2901->setDescription("Errors");
  pErrorCharacteristic->addDescriptor(pError_2901);

  // End of Characteristics --------------------------------------------------------


  Serial.println("Initialized.");

  // Start server and device advertising once initialized
  pService->start();
  BLEDevice::startAdvertising();
}

void loop() {
  
  // update data if a user is connected
  if (deviceConnected) {
    updateUser();
  }

  delay(1000);
}

// ------------------------------------
//           Update User (BLE)
// ------------------------------------

// Issues notifcations to the user with data (excluding Error)
void updateUser() {

  // Update all variables (converts floats to UTF-8 character arrays)
  char temp[10]; 
  sprintf(temp, "%.2f C", currPacket.batTempC); 
  pTempCharacteristic->setValue(temp);
  sprintf(temp, "%.2f V", currPacket.batVolt); 
  pVoltageCharacteristic->setValue(temp);
  sprintf(temp, "%.2f%%", currPacket.batSoC); 
  pChargeCharacteristic->setValue(temp);
  sprintf(temp, "%.2f A", currPacket.ampsBat);
  pBatCurrentCharacteristic->setValue(temp);
  sprintf(temp, "%.2f A", currPacket.ampsSol);
  pSolCurrentCharacteristic->setValue(temp);
  
  pRelayStateCharacteristic->setValue(currPacket.relayState ? "Closed" : "Open");

  // notify all variables to user
  pTempCharacteristic->notify();
  pVoltageCharacteristic->notify();
  pChargeCharacteristic->notify();
  pBatCurrentCharacteristic->notify();
  pSolCurrentCharacteristic->notify();
  pRelayStateCharacteristic->notify();

}

// ----------------------------------------
//           Display Data (Serial)
// ----------------------------------------

// Displays the data contained in a packet
void displayData(dataPacket packet, char *macAddr) {

  // Upper Serial Divider
  Serial.println();
  Serial.println("-----------------------");

  Serial.printf("Receiving Data from: %s\n\n", macAddr);
  
  Serial.printf("Battery Life: %.2f%%\nVoltage: %.2f V\nTemp: %.1f C\n", packet.batSoC, packet.batVolt, packet.batTempC);
  Serial.printf("Battery Current: %.2f A\nSolar Current: %.2f A\n", packet.ampsBat, packet.ampsSol);
  Serial.printf("Relay is: %s\n", packet.relayState ? "Closed" : "Open");

  // Displays Errors according to Error Code
  if (packet.err) 
  {
    Serial.print("\nERROR: ");

    // Print the error corresponding to the error code
    Serial.println(retrieveError(packet.err));

  }

  // Lower Serial Divider
  Serial.println();
  Serial.println("-----------------------");

}

// ----------------------------------------------------
//           Retrieve Error (from error code)
// ----------------------------------------------------
const char* retrieveError(int err) {

  switch (err) {
    case 0:
      return "No Error";
    case 1: 
      return "Temperature Sensor disconnected.";
    default:
      return "Unknown Error";
  }

}