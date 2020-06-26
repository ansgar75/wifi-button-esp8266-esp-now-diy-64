/****************************************************************************************************************************************************
 *  TITLE: An Efficient WiFi Button Using ESP-NOW
 *
 *  By Frenoy Osburn
 *  YouTube Video: https://youtu.be/iEkGRI8_txE
 ****************************************************************************************************************************************************/

 /********************************************************************************************************************
  * Please make sure that you install the board support package for the ESP8266 boards.
  * You will need to add the following URL to your Arduino preferences.
  * Boards Manager URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json
 ********************************************************************************************************************/
 
 /********************************************************************************************************************
 *  Board Settings:
 *  Board: "WeMos D1 R1 or Mini"
 *  Upload Speed: "921600"
 *  CPU Frequency: "80MHz"
 *  Flash Size: "4MB (FS:@MB OTA:~1019KB)"
 *  Debug Port: "Disabled"
 *  Debug Level: "None"
 *  VTables: "Flash"
 *  IwIP Variant: "v2 Lower Memory"
 *  Exception: "Legacy (new can return nullptr)"
 *  Erase Flash: "Only Sketch"
 *  SSL Support: "All SSL ciphers (most compatible)"
 *  COM Port: Depends *On Your System*
 *********************************************************************************************************************/

#include<ESP8266WiFi.h>
#include<espnow.h>

#define MY_NAME   "ESP-NOW_TO_MQTT_BRIDGE"

enum dataPacketType {
  BUTTON_PACKET,
  UNKNOWN_PACKET,
};

uint8_t buttonOneMac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };          //store the MAC address of the registered sender node

struct __attribute__((packed)) buttonPacket {
  int buttonId;            //used to determine which button was pressed
  int buttonValue;         //could be useful in the future
  int batteryVoltage;      //range is 00 to 42. Divide by 10 to get the actual battery voltage
};

bool compareMacAddresses(uint8_t *macAddressOne, uint8_t *macAddressTwo) {
  char macAddressOneString[18];
  char macAddressTwoString[18];
  
  snprintf(macAddressOneString, sizeof(macAddressOneString), "%02x:%02x:%02x:%02x:%02x:%02x", macAddressOne[0], macAddressOne[1], macAddressOne[2], macAddressOne[3], macAddressOne[4], macAddressOne[5]);
  snprintf(macAddressTwoString, sizeof(macAddressTwoString), "%02x:%02x:%02x:%02x:%02x:%02x", macAddressTwo[0], macAddressTwo[1], macAddressTwo[2], macAddressTwo[3], macAddressTwo[4], macAddressTwo[5]);
  
  if(strcmp(macAddressOneString, macAddressTwoString) == 0) {         //strcmp returns 0 if strings are equal
    return true;
  } else {
    return false;
  }
}

dataPacketType getDataPacketType(uint8_t *senderMac) {
  if(compareMacAddresses(senderMac, buttonOneMac)) {          //start comparing the most frequent packet first depending on your setup
    return BUTTON_PACKET;
  } else {
    return UNKNOWN_PACKET;
  }
}

void dataReceived(uint8_t *senderMac, uint8_t *data, uint8_t dataLength) {
  dataPacketType receivedPacketType;
  receivedPacketType = getDataPacketType(senderMac);

  if(receivedPacketType == BUTTON_PACKET) {
    buttonPacket packetData;
    memcpy(&packetData, data, sizeof(packetData));
    
    Serial.print("buttonId: ");
    Serial.println(packetData.buttonId);
    Serial.print("buttonValue: ");
    Serial.println(packetData.buttonValue);
    Serial.print("batteryVoltage: ");
    Serial.println(packetData.batteryVoltage);
  } else if(receivedPacketType == UNKNOWN_PACKET) {
    char macAddressUnknown[18];
    snprintf(macAddressUnknown, sizeof(macAddressUnknown), "%02x:%02x:%02x:%02x:%02x:%02x", senderMac[0], senderMac[1], senderMac[2], senderMac[3], senderMac[4], senderMac[5]);
    Serial.print("Received unknown data from: ");
    Serial.println(macAddressUnknown);
  }
}
 
void setup() {
  Serial.begin(115200);     // initialize serial port

  Serial.println();
  Serial.println();
  Serial.println();
  Serial.print("Initializing...");
  Serial.println(MY_NAME);
  Serial.print("My MAC address is: ");
  Serial.println(WiFi.macAddress());

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();        // we do not want to connect to a WiFi network

  if(esp_now_init() != 0) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_register_recv_cb(dataReceived);   // this function will get called once all data is sent

  Serial.println("Initialized.");
}

void loop() {

}
