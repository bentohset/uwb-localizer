/*

For ESP32 UWB or ESP32 UWB Pro

*/
#include <SPI.h>
#include <DW1000.h>
#include <DW1000CompileOptions.h>
#include <DW1000Constants.h>
#include <DW1000Device.h>
#include <DW1000Mac.h>
#include <DW1000Ranging.h>
#include <DW1000Time.h>
#include <deprecated.h>
#include <require_cpp11.h>

#include <WiFi.h>
#include "link.h"

// ensure anchors have different MAC Add
#define ANCHOR_ADD "81:00:5B:D5:A9:9A:E2:9C"

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4

// connection pins
const uint8_t PIN_RST = 27; // reset pin
const uint8_t PIN_IRQ = 34; // irq pin
const uint8_t PIN_SS = 4;   // spi select pin

const char *ssid = "TP-Link_3logytech";
const char *password = "3logytech1928";
const char *host = "192.168.0.143";
WiFiClient client;

struct MyLink *uwb_data;
int index_num = 0;
long runtime = 0;
String all_json = "";

uint16_t Adelay = 16636;
float dist_m = 0.8; //meters

void setup() {
  Serial.begin(115200);

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

  delay(1000);
  //init the configuration
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); //Reset, CS, IRQ pin
  DW1000.setAntennaDelay(Adelay);
  Serial.println("Initialising configuation");

  //define the sketch as anchor. It will be great to dynamically change the type of module
  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachBlinkDevice(newBlink);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);
  //Enable the filter to smooth the distance
  //DW1000Ranging.useRangeFilter(true);

  //we start the module as an anchor
  // DW1000Ranging.startAsAnchor("82:17:5B:D5:A9:9A:E2:9C", DW1000.MODE_LONGDATA_RANGE_ACCURACY);

  // DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_LONGDATA_RANGE_LOWPOWER, false);
  // DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_SHORTDATA_FAST_LOWPOWER);
  DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_LONGDATA_FAST_LOWPOWER, false);
  // DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_SHORTDATA_FAST_ACCURACY);
  // DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_LONGDATA_FAST_ACCURACY, false);
  // DW1000Ranging.startAsAnchor(ANCHOR_ADD, DW1000.MODE_LONGDATA_RANGE_ACCURACY);
  uwb_data = init_link();
}

void loop() {
  DW1000Ranging.loop();

  if ((millis() - runtime) > 1000) {
    make_link_json(uwb_data, &all_json);
    send_udp(&all_json);
    runtime = millis();
  }
}

void newRange() {
  Serial.print("from: ");
  Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);

  // calculate range
  Serial.print("\t Range: ");
  int iteration_count = 10;
  float dist = 0.0;
  for (int i = 0; i < iteration_count; i++) {
    dist += DW1000Ranging.getDistantDevice()->getRange();
  }
  dist = dist/iteration_count;
  Serial.print(dist);
  Serial.print(" m");

  Serial.print("\t RX power: ");
  Serial.print(DW1000Ranging.getDistantDevice()->getRXPower());
  Serial.println(" dBm");

  fresh_link(
    uwb_data, 
    DW1000Ranging.getDistantDevice()->getShortAddress(), 
    DW1000Ranging.getDistantDevice()->getRange(), 
    DW1000Ranging.getDistantDevice()->getRXPower()
  );
}

void newBlink(DW1000Device *device) {
  Serial.print("blink; 1 device added ! -> ");
  Serial.print(" short:");
  Serial.println(device->getShortAddress(), HEX);

  add_link(uwb_data, device->getShortAddress());
}

void inactiveDevice(DW1000Device *device) {
  Serial.print("delete inactive device: ");
  Serial.println(device->getShortAddress(), HEX);

  delete_link(uwb_data, device->getShortAddress());
}

void send_udp(String *msg_json) {
  if (client.connected()) {
    client.print(*msg_json);
    Serial.println("UDP send");
  }
}
