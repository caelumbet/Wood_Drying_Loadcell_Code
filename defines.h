// Lib includes --------------------------->
#include <stdint.h>
#include <ArduinoLowPower.h>
#include <delay.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Adafruit_DPS310.h>
#include <Wire.h>

#include "loraKeys.h"
 
// Pin defines ---------------------------->
#define PIN_XIN32 0
#define PIN_XOUT32 1
#define PIN_V_BATT 2
#define PIN_IO_0 3
#define PIN_IO_1 4
#define PIN_IO_2 5
#define PIN_IO_3 6
#define PIN_RFM_RESET 7
#define PIN_RFM_MOSI 8
#define PIN_RFM_SCK 9
#define PIN_RFM_MISO 10
#define PIN_RFM_CS 11
#define PIN_RS485_TX 14
#define PIN_RS485_RX 15
#define PIN_SHT_SDA 16
#define PIN_SHT_SCL 14
#define PIN_PER_PWR 18
#define PIN_RFM_DIO0 19
#define PIN_RFM_DIO1 22
#define PIN_RFM_DIO2 23
#define PIN_USB_D_N 24
#define PIN_USB_D_P 25
#define PIN_IO_4 27
#define PIN_IO_5 28
#define PIN_LED_DATA 28
#define PIN_SWCLK 30
#define PIN_SWDIO 31

#define mV_PER_LSB  1.188

// LED colours ---------------------------->
#define RED     0xFF0000
#define GREEN   0x00FF00
#define BLUE    0x0000FF
#define YELLOW  0xFFFF00
#define CYAN    0x00FFFF
#define MAGENTA 0xFF00FF
#define AMBER   0xFFAA00
#define WHITE   0xFFFFFF
#define BLACK   0

// Objects -------------------------------->

Adafruit_DPS310 dps;
Adafruit_Sensor* dps_temp = dps.getTemperatureSensor();

// Data structures ------------------------>

// Payload data struct (edit this with your data)
struct payload_data_t {
  float weight_1;  // 4 bytes
  float weight_2;  // 4 bytes
  float weight_3;  // 4 bytes
  float weight_4;  // 4 bytes
  float temperature; // 4 bytes
  float mVBatt; // 4 bytes
};

// Globals -------------------------------->

// Boolean flags
bool debug = true;

// Counters and time stamps
uint32_t currentMillis = 0;
uint32_t txMillis = 0;
uint32_t txCounter = 0;

// Schedule TX every this many seconds (might become longer due to duty cycle limitations).what is
const unsigned TX_INTERVAL = 30; //900 15 minutes

// LoRaWAN Basic Module
const lmic_pinmap lorawan_pins = {
  .nss = PIN_RFM_CS,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = PIN_RFM_RESET,
  .dio = { PIN_RFM_DIO0, PIN_RFM_DIO1, PIN_RFM_DIO2 },
  .rxtx_rx_active = 0,
  .rssi_cal = 8,  // LBT cal for the Adafruit Feather M0 LoRa, in dB
  .spi_freq = 8000000,
};

// payload to send to gateway
payload_data_t payload_data;
static uint8_t payload[sizeof(payload_data)];
static osjob_t sendjob;
