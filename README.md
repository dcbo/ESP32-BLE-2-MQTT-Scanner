# ESP32-BLE-2-MQTT-Scanner
ESP32 scans for BLE Advertisement Beacons and reports them as JSON via Serial-Port and MQTT

- Scan for BLE Advertisement Beacons
- Report them as JSON-String
- via Serial Port (115200 Baud) and
- via MQTT

# Hardware
Any ESP32 Board

# Arduino Settings
- Board: ESP32 Dev Module
- Partition Scheme: No OTA (2MB APP/2MB FATFS)

# Libraries needed
- PubSubClient 
  Nick O’Leary   
  version: 2.8 - released 2020-05-20    
  https://github.com/knolleary/pubsubclient
- ESP32 BLE for Arduino  
  Neil Kolban  
  version: 1.0.1 - released 2020-05-20  
  https://github.com/nkolban/ESP32_BLE_Arduino
 
