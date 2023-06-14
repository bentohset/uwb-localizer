#include "dw3000.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include "link.h"

#define UDP_ENABLED true

#define APP_NAME "SS TWR INIT v1.0"
#define PIN_RST 27
#define PIN_IRQ 34
#define PIN_SS 4

#define RNG_DELAY_MS 1000
#define TX_ANT_DLY 16365
#define RX_ANT_DLY 16365
#define ALL_MSG_COMMON_LEN 10
#define ALL_MSG_SN_IDX 2
#define RESP_MSG_POLL_RX_TS_IDX 10
#define RESP_MSG_RESP_TX_TS_IDX 14
#define RESP_MSG_TS_LEN 4

#define POLL_TX_TO_RESP_RX_DLY_UUS 1720
#define RESP_RX_TIMEOUT_UUS 250

// Wifi details
const char *ssid = "TP-Link_3logytech";
const char *password = "3logytech1928";
const char *host = "192.168.0.135";   //ip address of host aka laptop for visualisation
WiFiClient client;
WiFiUDP udp;

const int numReadings = 4;

// filters for filtering distance
double readings[numReadings];
int readIndex = 0;
double total = 0;
double average = 0;


// UDP transfer
struct MyLink *uwb_data;
int index_num = 0;
long runtime = 0;
String all_json = "";

/* Default communication configuration. We use default non-STS DW mode. */
static dwt_config_t config = {
  5,                /* Channel number. */
  DWT_PLEN_128,     /* Preamble length. Used in TX only. */
  DWT_PAC8,         /* Preamble acquisition chunk size. Used in RX only. */
  9,                /* TX preamble code. Used in TX only. */
  9,                /* RX preamble code. Used in RX only. */
  1,                /* 0 to use standard 8 symbol SFD, 1 to use non-standard 8 symbol, 2 for non-standard 16 symbol SFD and 3 for 4z 8 symbol SDF type */
  DWT_BR_6M8,       /* Data rate. */
  DWT_PHRMODE_STD,  /* PHY header mode. */
  DWT_PHRRATE_STD,  /* PHY header rate. */
  (129 + 8 - 8),    /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
  DWT_STS_MODE_OFF, /* STS disabled */
  DWT_STS_LEN_64,   /* STS length see allowed values in Enum dwt_sts_lengths_e */
  DWT_PDOA_M0       /* PDOA mode off */
};

// message for handshake ACK
static uint8_t tx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'S', 'I', 'J', 'A', 0xE0, 0, 0}; //id of anchor1
static uint8_t rx_resp_msg[] = {0x42, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// waiting to receive from VEWA - id of tag

#define RX_BUF_LEN 20
static uint8_t frame_seq_nb = 0;
static uint8_t rx_buffer[RX_BUF_LEN];
static uint32_t status_reg = 0;

/* Hold copies of computed time of flight and distance here for reference so that it can be examined at a debug breakpoint. */
static double tof;
static double distance;

/* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth and power of the spectrum at the current
 * temperature. These values can be calibrated prior to taking reference measurements. */
extern dwt_txconfig_t txconfig_options;

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
  udp.begin(1234);

  // if (client.connect(host, 80)) {
  //   Serial.println("Success");
  //   client.print(
  //     String("GET /") + " HTTP/1.1\r\n" +
  //     "Host: " + host + "\r\n" +
  //     "Connection: close\r\n" +
  //     "\r\n"
  //   );
  // }
}

void setupUWB() {
  spiBegin(PIN_IRQ, PIN_RST);
  spiSelect(PIN_SS);

  delay(2); // Time needed for DW3000 to start up (transition from INIT_RC to IDLE_RC, or could wait for SPIRDY event)

  // Need to make sure DW IC is in IDLE_RC before proceeding
  while (!dwt_checkidlerc()) {
    UART_puts("IDLE FAILED\r\n");
    while (1);
  }

  if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) {
    UART_puts("INIT FAILED\r\n");
    while (1);
  }

  // Enabling LEDs here for debug so that for each TX the D1 LED will flash on DW3000 red eval-shield boards.
  dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);

  /* Configure DW IC. See NOTE 6 below. */
  // if the dwt_configure returns DWT_ERROR either the PLL or RX calibration has failed the host should reset the device
  if (dwt_configure(&config)) {
    UART_puts("CONFIG FAILED\r\n");
    while (1);
  }

  /* Configure the TX spectrum parameters (power, PG delay and PG count) */
  dwt_configuretxrf(&txconfig_options);

  /* Apply default antenna delay value. See NOTE 2 below. */
  dwt_setrxantennadelay(RX_ANT_DLY);
  dwt_settxantennadelay(TX_ANT_DLY);

  /* Set expected response's delay and timeout. See NOTE 1 and 5 below.
   * As this example only handles one incoming frame with always the same delay and timeout, those values can be set here once for all. */
  dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
  dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);

  /* Next can enable TX/RX states output on GPIOs 5 and 6 to help debug, and also TX/RX LEDs
   * Note, in real low power applications the LEDs should not be used. */
  dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);

  Serial.println("Range RX");
}

void setupUDP() {
  uwb_data = init_link();
}

void send_udp(String *msg_json) {
  udp.beginPacket(host, 80);
  // Serial.println(msg_json);
  udp.write(reinterpret_cast<const uint8_t*>(msg_json->c_str()), msg_json->length());
  udp.endPacket();
  Serial.println("UDP send");
}

uint8_t convertToUint8(uint8_t* ptr) {
  return *ptr;
}

void setup() {
  UART_init();
  UART_puts("TAG SETUP\r\n");
  test_run_info((unsigned char *)APP_NAME);

  setupWifi();
  setupUDP();
  setupUWB();
  
  Serial.println("Setup over........");
}

void loop() {
  /* Write frame data to DW IC and prepare transmission. See NOTE 7 below. */
  // UART_puts("looping");
  // Serial.println(anchorNum);

  // Handshake, transmit and polls for 2 uwb devices which acts as the anchors
  tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
  dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
  dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0); /* Zero offset in TX buffer. */
  dwt_writetxfctrl(sizeof(tx_poll_msg), 0, 1);          /* Zero offset in TX buffer, ranging. */

  
  /* Start transmission, indicating that a response is expected so that reception is enabled automatically after the frame is sent and the delay
   * set by dwt_setrxaftertxdelay() has elapsed. */
  dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);
  // Serial.println(SYS_STATUS_ID);
  // Serial.println(dwt_read32bitreg(SYS_STATUS_ID));
  // Serial.println((SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR));

  /* We assume that the transmission is achieved correctly, poll for reception of a frame or error/timeout. See NOTE 8 below. */
  while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR))){ };
  // Serial.println("frame received");

  /* Increment frame sequence number after transmission of the poll message (modulo 256). */
  frame_seq_nb++;
  // Serial.println(status_reg);
  // Serial.println(SYS_STATUS_RXFCG_BIT_MASK);

  if (status_reg & SYS_STATUS_RXFCG_BIT_MASK) {
    uint32_t frame_len;

    /* Clear good RX frame event in the DW IC status register. */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);
    // Serial.println("clear frame event");

    /* A frame has been received, read it into the local buffer. */
    frame_len = dwt_read32bitreg(RX_FINFO_ID) & RXFLEN_MASK;
    // Serial.println("frame read into local buffer");
    // Serial.println(frame_len);
    // Serial.println(sizeof(rx_buffer));

    if (frame_len <= sizeof(rx_buffer)) {
      dwt_readrxdata(rx_buffer, frame_len, 0);

      /* Check that the frame is the expected response from the companion "SS TWR responder" example.
       * As the sequence number field of the frame is not relevant, it is cleared to simplify the validation of the frame. */
      rx_buffer[ALL_MSG_SN_IDX] = 0;
      for (int i = 0; i < sizeof(rx_buffer)/sizeof(rx_buffer[0]); i++) Serial.print(rx_buffer[i], HEX);
      // Serial.println();

      /* Add link for new device detected (may or may not be used)*/
      if (UDP_ENABLED) {
        add_link(uwb_data, convertToUint8(tx_poll_msg), convertToUint8(rx_buffer));
      }

      // if (memcmp(rx_buffer, rx_resp_msg, ALL_MSG_COMMON_LEN) == 0) {
        /* store address*/
        uint8_t device_address = convertToUint8(rx_buffer);
        
        // Serial.println("compare correct");
        uint32_t poll_tx_ts, resp_rx_ts, poll_rx_ts, resp_tx_ts;
        int32_t rtd_init, rtd_resp;
        float clockOffsetRatio;

        /* Retrieve poll transmission and response reception timestamps. See NOTE 9 below. */
        poll_tx_ts = dwt_readtxtimestamplo32();
        resp_rx_ts = dwt_readrxtimestamplo32();

        /* Read carrier integrator value and calculate clock offset ratio. See NOTE 11 below. */
        clockOffsetRatio = ((float)dwt_readclockoffset()) / (uint32_t)(1 << 26);

        /* Get timestamps embedded in response message. */
        resp_msg_get_ts(&rx_buffer[RESP_MSG_POLL_RX_TS_IDX], &poll_rx_ts);
        resp_msg_get_ts(&rx_buffer[RESP_MSG_RESP_TX_TS_IDX], &resp_tx_ts);

        /* Compute time of flight and distance, using clock offset ratio to correct for differing local and remote clock rates */
        rtd_init = resp_rx_ts - poll_tx_ts;
        rtd_resp = resp_tx_ts - poll_rx_ts;

        tof = ((rtd_init - rtd_resp * (1 - clockOffsetRatio)) / 2.0) * DWT_TIME_UNITS;
        distance = tof * SPEED_OF_LIGHT;

        Serial.println(distance);

        total = total - readings[readIndex];
        readings[readIndex] = distance;
        total = total + readings[readIndex];
        readIndex = readIndex + 1;
        if (readIndex >= numReadings) {
          readIndex = 0;
        }
        average = total / numReadings;
        distance = average;
        Serial.print("Anchor 1");
        Serial.print(" Distance: ");
        Serial.print(distance);

        if (UDP_ENABLED) {
          fresh_link(
            uwb_data, 
            device_address,
            distance
          );
          // if ((millis() - runtime) > 1000) {
            make_link_json(uwb_data, &all_json);
            send_udp(&all_json);
          //   runtime = millis();
          // }
          
        }
        /* Display computed distance on LCD. */
        // snprintf(dist_str, sizeof(dist_str), "DIST: %3.2f m", distance);
        test_run_info((unsigned char *)dist_str);
      // }
    }
  } else {
    /* Clear RX error/timeout events in the DW IC status register. */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
  }

  /* Execute a delay between ranging exchanges. */
  Sleep(RNG_DELAY_MS);
}