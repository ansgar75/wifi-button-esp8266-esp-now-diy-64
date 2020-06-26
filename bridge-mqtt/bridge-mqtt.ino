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
  * 
  * You will also need to install the PubSubClient library by Nick O'Leary and the Arduino_JSON library by Arduino. 
  * Use Tools->Manage Libraries to navigate to the library manager. 
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

//START: MQTT related statements
#include <PubSubClient.h>
#include <Arduino_JSON.h>

const char ssid[] = "Network";
const char password[] = "password";

const char mqttServer[] = "192.168.1.37";
const char mqttUsername[] = "mqtt";
const char mqttPassword[] = "mqtt";

char pubButtonTopic[] = "mqtt/button";     

WiFiClient espClient;
PubSubClient client(espClient);
//END: MQTT related statements

#define MY_NAME   "ESP-NOW_TO_MQTT_BRIDGE"

enum dataPacketType {
  BUTTON_PACKET,
  UNKNOWN_PACKET,
};

uint8_t buttonOneMac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };          //store the MAC address of the registered node

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
    char buttonPayLoad[6];   
    buttonPacket packetData;
    
    memcpy(&packetData, data, sizeof(packetData));

    JSONVar buttonPayload;
    buttonPayload["id"] = packetData.buttonId;
    buttonPayload["value"] = packetData.buttonValue;
    buttonPayload["voltage"] = packetData.batteryVoltage;
    String jsonString = JSON.stringify(buttonPayload);
    
    client.publish(pubButtonTopic, jsonString.c_str());
    Serial.print("Sent MQTT payload: ");
    Serial.println(jsonString.c_str());
  } else if(receivedPacketType == UNKNOWN_PACKET) {
    char macAddressUnknown[18];
    snprintf(macAddressUnknown, sizeof(macAddressUnknown), "%02x:%02x:%02x:%02x:%02x:%02x", senderMac[0], senderMac[1], senderMac[2], senderMac[3], senderMac[4], senderMac[5]);
    Serial.print("Received unknown data from: ");
    Serial.println(macAddressUnknown);
  }
}

void setup() {
  uint8_t bootCounter = 0; 

  Serial.begin(115200);     //initialize serial port

  Serial.println();
  Serial.println();
  Serial.println();
  Serial.print("Initializing...");
  Serial.println(MY_NAME);
  Serial.print("My MAC address is: ");
  Serial.println(WiFi.macAddress());
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    bootCounter++;
    if(bootCounter == 40)
    {
      ESP.reset();
    }
    delay(500);
  }
  randomSeed(micros());       //do this as we will use random client ID
  client.setServer(mqttServer, 1883);
  
  if(esp_now_init() != 0) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_register_recv_cb(dataReceived);   //this function will get called once all data is sent

  Serial.println("Initialized.");
}

void loop() {
  while (!client.connected())                       //loop until we are connected
  {
    String clientId = "ESP8266Client-";           
    clientId += String(random(0xffff), HEX);        //create a random client ID
    if (!client.connect(clientId.c_str(), mqttUsername, mqttPassword))    //attempt to connect to MQTT broker
    {
      delay(1000);        //wait 1 second before retrying to connect
    }
  }
  client.loop();
}
