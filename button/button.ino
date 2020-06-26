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

#define DEBUG_MODE      0                               //set this to 1 to enable serial print statements

#define MOSFET_PIN      D1   

#define MY_NAME         "BUTTON_ONE_NODE"
#define MY_ID           1                               //this will be sent to the bridge and HA so that they can identify which button was pressed
#define MY_ROLE         ESP_NOW_ROLE_CONTROLLER         //set the role of this device: CONTROLLER, SLAVE, COMBO or MAX
#define BRIDGE_ROLE     ESP_NOW_ROLE_SLAVE              //set the role of the receiver. In our case, the ESP-NOW to MQTT bridge
#define WIFI_CHANNEL    1

#define BUTTON_ID       1

#define NODE_TIMEOUT    5000                            //after this time, the node will go back to sleep. This is useful if something goes wrong
#define SECONDS_TO_US   1000000

uint8_t bridgeAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };   //please update this with the MAC address of your ESP-NOW TO MQTT brigde
volatile bool attemptedTransmission = false;                        //keeps track the call back function. We need to be sure that the function was called before we go to sleep

struct __attribute__((packed)) buttonPacket {
  int buttonId;            //used to determine which button was pressed
  int buttonValue;         //could be useful in the future
  int batteryVoltage;      //range is 00 to 42. Divide by 10 to get the actual battery voltage
};

void transmissionComplete(uint8_t *receiver_mac, uint8_t transmissionStatus) {
  if(transmissionStatus == 0) {
    if(DEBUG_MODE == 1) {
      Serial.println("Data sent successfully");
    }
  } else {
    if(DEBUG_MODE == 1) {
      Serial.print("Error code: ");
      Serial.println(transmissionStatus);
    }
  }
  attemptedTransmission = true;
}

void setup() {    
  pinMode(MOSFET_PIN, OUTPUT); 
  digitalWrite(MOSFET_PIN, LOW);    
  
  if(DEBUG_MODE == 1) {
    Serial.begin(115200);     //initialize serial port
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.print("Initializing...");
    Serial.println(MY_NAME);
    Serial.print("My MAC address is: ");
    Serial.println(WiFi.macAddress());
  }
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();        //we do not want to connect to a WiFi network
  
  if(esp_now_init() != 0) {
    if(DEBUG_MODE == 1) {
      Serial.println("ESP-NOW initialization failed");
    }
    return;
  }

  esp_now_set_self_role(MY_ROLE);   
  esp_now_register_send_cb(transmissionComplete);   //this function will get called once all data is sent
  esp_now_add_peer(bridgeAddress, BRIDGE_ROLE, WIFI_CHANNEL, NULL, 0);

  if(DEBUG_MODE == 1) {
    Serial.println("Initialized.");
  }
  
  buttonPacket temporaryDataPacket;                 //get instance of buttonPacket so that we can add the data and send this out
  temporaryDataPacket.buttonId = MY_ID;
  temporaryDataPacket.buttonValue = 1;
  temporaryDataPacket.batteryVoltage = analogRead(A0)*4.2*10/1023;

  attemptedTransmission = false;
  esp_now_send(bridgeAddress, (uint8_t *) &temporaryDataPacket, sizeof(temporaryDataPacket));
}

void loop() {
  if(attemptedTransmission || (millis() > NODE_TIMEOUT)) {
    digitalWrite(MOSFET_PIN, HIGH);               //GPIO state is undefined in deep sleep with a low drive. This ensures that the MOSFET is switched OFF
    ESP.deepSleep(0 * SECONDS_TO_US);             //the board should already be powered OFF due to the line above, so we will most likley never execute this statement
  }
}
