/************************************************
 * ESP32 BLE-2-MQTT-Scanner 
 * 2020 by Dario Carluccio 
 ************************************************
 * based on Idea from 
 * https://github.com/ct-Open-Source/esp32-bt-mqtt-scanner
 ************************************************
 * - Scan for BLE Advertisement Beacons
 * - Report them as JSON-String
 *   - via Serial Port (115200 Baud) and
 *   - via MQTT
 ************************************************
 * Hardware:
 * - any ESP32 Board
 ************************************************
 * Arduino Settings:
 * - Board: ESP32 Dev Module
 * - Partition Scheme: No OTA (2MB APP/2MB FATFS)
 *************************************************
 * Libraries needed:
 * - PubSubClient 
 *   Nick O’Leary   
 *   version: 2.8 - released 2020-05-20    
 *   https://github.com/knolleary/pubsubclient
 * - ESP32 BLE for Arduino  
 *   Neil Kolban  
 *   version: 1.0.1 - released 2020-05-20  
 *   https://github.com/nkolban/ESP32_BLE_Arduino
 *************************************************/ 

/* Includes */ 
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAddress.h>
 
// Activate MQTT
// defined:     MQTT active - Publish Beacons via MQTT and send tehm over Serial Port
// not defined: No MQTT - Report Beacons only over Serial Port
#define MQTT_ACTIVE

/* MQTT-Server-Settings */ 
#define MQTT_SERVER   "192.168.31.31"            // IP of MQTT-Server
#define MQTT_PORT     1883                       // Port of MQTT-Server  
#define MQTT_NAME     "espbtlescanner"           // MQTT-Clientname
#define MQTT_USER     ""                         // MQTT-Username
#define MQTT_PASS     ""                         // MQTT-Password
#define MQTT_BUFSIZE  2048                       // MQTT-Buffersize (may be augmented, when Scan returns many BLE-Devices
#define TOPIC_SCAN    "btscanner/scan"           // Topic for BLE-Scanresults 
#define TOPIC_STATUS  "btscanner/status"         // Topic for Statusinformation

// JSON_SEND_EMPTY_STRING  
//   Send Fieldname, if Beacon does not contain a value 
//   valid for fileds: "Name", "UUID" and "ManufacturerData" 
//#define JSON_SEND_EMPTY_STRING

// How long to wait for BLE-Beacons [s]
#define BLE_SCANTIME              10

// WiFi-Settings 
#define WIFI_Ssid "YOUR_SSID_HERE"                // WiFi-SSID
#define WIFI_Key  "YOUR_PASS_HERE"                // WiFi-Password

// Globals
BLEScan* pBLEScan;
WiFiClient myWiFiClient;
PubSubClient mqtt(MQTT_SERVER, MQTT_PORT, myWiFiClient);


 /************************ 
  *  Setup
  ************************ 
  * - init Serial
  * - Init Wifi
  * - Init MQTT
  * - Init BLE-Scanner
  ************************/
void setup() {
    // int i;
    int timeout;
    
    Serial.begin(115200);
    Serial.println("\n");
    Serial.println("ESP32 BLE Scanner");
    Serial.println("=================");

#ifdef MQTT_ACTIVE
    // Connect to WiFi
    Serial.print("Connecting to WiFi ... ");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_Ssid, WIFI_Key);
    timeout = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        timeout++;
        if (timeout > 60) {
            Serial.println("\r\nWiFi-Connection failed - Rebooting.");
            delay(5000);
            ESP.restart(); 
        }
    }
    Serial.println("connected.");  
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    // Connect to MQTT-Server
    Serial.print("Connecting to MQTT-Server ... ");
    if (mqtt.connect(MQTT_NAME, MQTT_USER, MQTT_PASS))  { 
        mqtt.setBufferSize(MQTT_BUFSIZE);
        mqtt.publish(TOPIC_STATUS,"BLE-Scanner connected");
        Serial.println("connected.");
    } else {
        Serial.println("\r\nMQTT-Connection failed - Rebooting.");
        delay(5000);
        ESP.restart(); 
    }   
    mqtt.loop();
#endif // MQTT_ACTIVE

    // Init BLE-Scanning
    Serial.print("Starting BLE Scan-Task ...");
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    Serial.println("done.");
}

/********************************
 * Main-Loop  
 ********************************
 * - Get Results from BLE-Scanner
 * - Poll MQTT-Client
 * - JSON
 *   - Compose 
 *   - Send to Serial
 *   - MQTT Publish 
 *******************************/
void loop() {
  int cnt;
  int devNum;
  String myJSON;
  char* myMFdata;
  BLEScanResults myDevices = pBLEScan->start(BLE_SCANTIME);
  
  // Start composing JSON
  myJSON = "{\"Results\":{"; 
  devNum = myDevices.getCount();
    for (cnt = 0; cnt < devNum; cnt++) {
    // BLE-Address
    myJSON = myJSON + "\"" + myDevices.getDevice(cnt).getAddress().toString().c_str() + "\":{";
    // Devicename
    if (myDevices.getDevice(cnt).haveName()) {
        myJSON = myJSON + "\"name\":\"" + myDevices.getDevice(cnt).getName().c_str() + "\",";
    } else {
        #ifdef JSON_SEND_EMPTY_STRING
            myJSON = myJSON + "\"name\":\"\",";            
        #endif    
    }
    // UUID
    if (myDevices.getDevice(cnt).haveServiceUUID()) {
        myJSON = myJSON + "\"serviceuuid\":\"" + myDevices.getDevice(cnt).getServiceUUID().toString().c_str() + "\","; 
    } else {
        #ifdef JSON_SEND_EMPTY_STRING
            myJSON = myJSON + "\"serviceuuid\":\"\",";
        #endif    
    }
    // ManufacturerData coded as HEX-String
    if (myDevices.getDevice(cnt).haveManufacturerData()) {       
       myMFdata = BLEUtils::buildHexData(NULL, (uint8_t*)myDevices.getDevice(cnt).getManufacturerData().data(), myDevices.getDevice(cnt).getManufacturerData().length());                                                         
       myJSON = myJSON + "\"servicemfdata\":\"" + myMFdata + "\","; 
    } else {
        #ifdef JSON_SEND_EMPTY_STRING
            myJSON = myJSON + "\"servicemfdata\":\"\",";
        #endif    
    }
    // RSSI
    myJSON = myJSON +  "\"rssi\":\"" + String(myDevices.getDevice(cnt).getRSSI()) + "\"}";
    if (cnt < devNum - 1) {
        myJSON = myJSON + ",";
    }       
  }
  // Complete JSON String
  myJSON = myJSON + "}}";
  
  // Free Memory
  pBLEScan->clearResults();                                 

#ifdef MQTT_ACTIVE
  // Publish Result to MQTT
  mqtt.publish(TOPIC_SCAN, (char*) myJSON.c_str());      
  
  // Reboot if WIFI-Connection lost
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\r\nLost WiFi-Connection - Rebooting.");
      delay(5000);
      ESP.restart(); 
    }
    
  // Try to Reconnect, otherwise Reboot if MQTT-Connection lost
  if (! mqtt.connected()) {  
    Serial.println("\r\nLost MQTT Connection - trying ro reconnect ... ");
    mqtt.connect(MQTT_NAME, MQTT_USER, MQTT_PASS);
    int timeout = 0;
    while (!mqtt.connected()) {
       delay(500);
       Serial.print(".");
       timeout++;
        if  (timeout > 60) {
            Serial.println("failed - Reboot.");
            ESP.restart();
        }
    }
    Serial.println("success.");
    mqtt.publish(TOPIC_STATUS,"BLE-Scanner lost Connection and reconnected");
  } 
  mqtt.loop();
#endif // MQTT_ACTIVE

  // Send Result to Serial
  Serial.println(myJSON);
  delay(100);
}
