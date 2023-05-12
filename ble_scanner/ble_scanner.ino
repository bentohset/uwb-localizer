#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>

#define TARGET_NAME "ESP32-TAG"

int scanTime = 5; //In seconds
BLEScan* pBLEScan;
bool target_found = false;
String address = ""; 

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("Received BLE data: \n");
    Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    String deviceName = advertisedDevice.getName().c_str();
    if (advertisedDevice.haveName() && deviceName.equals(TARGET_NAME)) {
      Serial.println("target found!");
      Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
      Serial.println(deviceName);
      address = advertisedDevice.getAddress().toString().c_str();
      target_found = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  BLEDevice::init("");

  // Create a BLE Scan
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  
  while (!target_found) {
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
    delay(2000);
  }
  if (target_found) {
    Serial.println("Scan done!");
    Serial.print("Device address: ");
    Serial.println(address);
    pBLEScan->clearResults();   // delete results to free memory
    delay(2000);
  }
}

void loop() {
  
}
