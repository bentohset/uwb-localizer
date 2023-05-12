/*

For ESP32 UWB or ESP32 UWB Pro

*/
#include "DW1000Ranging.h"
#include "DW1000.h"
#include <SPI.h>
#include <WiFi.h>
#include <math.h>
#include <float.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include "link.h"

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4
#define PIN_RST 27
#define PIN_IRQ 34

//hardware configs
#define N_ANCHORS 2
#define ANCHOR_EXPIRE_TIME 5000   //measurements older than this are ignore (milliseconds)
#define TAG_ADDRESS "7D:00:22:EA:82:60:3B:9C"
#define ANCHOR_DISTANCE 1.0 // distance between 2 anchors in metres
#define ANCHOR1_SHORT_ADD "81"
#define ANCHOR2_SHORT_ADD "82"
#define A1_RANGE_OFFSET -1.6
#define A2_RANGE_OFFSET 0.5 //minus 


//BLE configs
#define BLE_ENABLED false   //enable all ble methods
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

//wifi + udp configs
#define UDP_ENABLED true
const char *ssid = "TP-Link_3logytech";
const char *password = "3logytech1928";
const char *host = "192.168.0.139";   //ip address of host aka laptop for visualisation
WiFiClient client;
struct MyLink *uwb_data;
int index_num = 0;
long runtime = 0;
String all_json = "";

//variables for coordinate calculation
// #define DEBUG_DIST
#define CALC_ENABLED false
volatile double x_coordinate = DBL_MIN;
volatile double y_coordinate = DBL_MIN;
double a1_range;
double a2_range;
uint32_t last_anchor_update[N_ANCHORS] = {0}; //millis() value last time anchor was seen
double last_anchor_distance[N_ANCHORS] = {0.0}; //most recent distance reports
double anchor_matrix[N_ANCHORS][2] = { //list of anchor coordinates, relative to chosen origin.
  {0.0, 0.0},  //Anchor #1
  {ANCHOR_DISTANCE, 0.0},  //Anchor #2
};
double current_tag_position[2] = {0.0, 0.0}; //global current position (meters with respect to anchor origin)


void setup() {
  Serial.begin(115200);
  if (UDP_ENABLED) {
    setupWifi();
    delay(1000);
  }
  setupUWB();
  if (BLE_ENABLED) {
    setupBLE();
  }
  //creating a linkedlist for uwb data
  if (UDP_ENABLED) {
    uwb_data = init_link();
  }
  
}

// setup esp32 as a broadcasting ble server
void setupBLE() {
  BLEDevice::init("ESP32-TAG");
  BLEAddress currentAddress = BLEDevice::getAddress();
  Serial.println(currentAddress.toString().c_str());
  pServer = BLEDevice::createServer();
  pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE
  );
}

// setup esp32 as a uwb tag
void setupUWB() {
  //init the configuration
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  DW1000Ranging.initCommunication(PIN_RST, DW_CS, PIN_IRQ);
  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachNewDevice(newDevice);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);

  //start the module as a tag
  DW1000Ranging.startAsTag(TAG_ADDRESS, DW1000.MODE_LONGDATA_RANGE_LOWPOWER, false);
  Serial.println("Started as a tag");
}

// connect esp32 to wifi and client (laptop) thru IPv4
void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected");
  Serial.print("IP Address:");
  Serial.println(WiFi.localIP());

  if (client.connect(host, 80)) {
    Serial.println("Success");
    client.print(
      String("GET /") + " HTTP/1.1\r\n" +
      "Host: " + host + "\r\n" +
      "Connection: close\r\n" +
      "\r\n"
    );
  }
}

void loop() {
  DW1000Ranging.loop();

  //send data to host by udp
  if (UDP_ENABLED) {
    if ((millis() - runtime) > 1000) {
      make_link_json(uwb_data, &all_json);
      send_udp(&all_json);
      runtime = millis();
    }
  }
  
  //if tag is within range, broadcast ble
  if (y_coordinate <= 0 && x_coordinate > 0 && x_coordinate < ANCHOR_DISTANCE) {
    Serial.println("Within zone");
    if (BLE_ENABLED) {
      sendBLE();
    }
  }
}

//ble broadcasting of tag's MAC address
void sendBLE() {
  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("Advertising BLE...");

  pCharacteristic->setValue(TAG_ADDRESS);
  pCharacteristic->notify();
  delay(2000);
}

void newRange() {
  int i; //indices for iteration of anchros, expecting values 0 to 1
  //index of this anchor, expecting values 1/2
  int index = DW1000Ranging.getDistantDevice()->getShortAddress() & 0x03;
  Serial.printf("index detected %d\n", index);

  if (index > 0) {
    last_anchor_update[index - 1] = millis();  //decrement index for array index
    double range = DW1000Ranging.getDistantDevice()->getRange();
    Serial.printf("distance %f\n", range);
    last_anchor_distance[index - 1] = range;
    if (range < 0.0 || range > 30.0) {
      last_anchor_update[index - 1] = 0.01;  //error or out of bounds, ignore this measurement
    }   
  }

  int detected = 0;
  for (i = 0; i < N_ANCHORS; i++) {
    if (millis() - last_anchor_update[i] > ANCHOR_EXPIRE_TIME) {
      last_anchor_update[i] = 0; //not from this one
    }
    if (last_anchor_update[i] > 0) {
      detected++;
    }
  }
  #ifdef DEBUG_DIST
    // print distance and age of measurement
    uint32_t current_time = millis();
    for (i = 0; i < N_ANCHORS; i++) {
      Serial.print(i+1); //ID
      Serial.print("> ");
      Serial.print(last_anchor_distance[i]);
      Serial.print("\t");
      Serial.println(current_time - last_anchor_update[i]); //age in millis
    }
  #endif
  Serial.printf("detected %d\n", detected);
  if (detected == 2) {
    
    getCoordinates();
    Serial.print("Position = ");
    Serial.print(current_tag_position[0]);
    Serial.write(',');
    Serial.println(current_tag_position[1]);
  }

  if (UDP_ENABLED) {
    fresh_link(
      uwb_data, 
      DW1000Ranging.getDistantDevice()->getShortAddress(), 
      DW1000Ranging.getDistantDevice()->getRange(), 
      DW1000Ranging.getDistantDevice()->getRXPower()
    );
  }
}

//updates location of tag x and y (0,0 based from Anchor 1) using triangulation
void getCoordinates() {
  double dist[N_ANCHORS]; //distances from anchors
  int i;
  // copy distances to local storage
  for (i = 0; i < N_ANCHORS; i++) {
    dist[i] = last_anchor_distance[i];
  }
  double b = dist[0];
  double a = dist[1];
  double c = ANCHOR_DISTANCE;

  double cosA = (b * b + c * c - a * a) / (2 * b * c);
  current_tag_position[0] = b * cosA;
  current_tag_position[1] = b * sqrt(1 - cosA * cosA);
}

void newDevice(DW1000Device *device) {
  Serial.print("ranging init; Device added ! -> ");
  Serial.print(" short:");
  Serial.println(device->getShortAddress(), HEX);

  if (UDP_ENABLED) {
    add_link(uwb_data, device->getShortAddress());    
  }
}

void inactiveDevice(DW1000Device *device) {
  Serial.print("delete inactive device: ");
  Serial.println(device->getShortAddress(), HEX);

  if (UDP_ENABLED) {
    delete_link(uwb_data, device->getShortAddress());
  }
}

void send_udp(String *msg_json) {
  if (client.connected()) {
    client.print(*msg_json);
    Serial.println("UDP send");
  }
}
