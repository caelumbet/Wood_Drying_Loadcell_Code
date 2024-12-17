#include "defines.h"

// Setup ---------------------------------------------------------------------|

void setup() {
  setupComm();
  setupPins();
  setupAnalogue();
  setupOnboardSensor();
  setupLoRaWAN();
  
}

// Global Variables ----------------------------------------------------------|

payload_data_t receivedData;
volatile bool loraPayloadReadyFlag = false;

// Loop ----------------------------------------------------------------------|

void loop() {
  os_runloop_once();
  if (txCounter >= 10) LMIC_setBatteryLevel(setBattLevel());
}

// ISR callbacks -------------------------------------------------------------|

void onEvent(ev_t ev) {
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      if (debug) Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      if (debug) Serial.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      if (debug) Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      if (debug) Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
      if (debug) Serial.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
      if (debug) Serial.println(F("EV_JOINED"));
      {
        u4_t netid = 0;
        devaddr_t devaddr = 0;
        u1_t nwkKey[16];
        u1_t artKey[16];
        LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
        if (debug) {
          Serial.print("netid: ");
          Serial.println(netid, DEC);
          Serial.print("devaddr: ");
          Serial.println(devaddr, HEX);
          Serial.print("AppSKey: ");
          for (size_t i = 0; i < sizeof(artKey); ++i) {
            if (i != 0)
              Serial.print("-");
            printHex2(artKey[i]);
          }
          Serial.println("");
          Serial.print("NwkSKey: ");
          for (size_t i = 0; i < sizeof(nwkKey); ++i) {
            if (i != 0)
              Serial.print("-");
            printHex2(nwkKey[i]);
          }
          Serial.println();
        }
      }
      break;
    case EV_RFU1:
      if (debug) Serial.println(F("EV_RFU1"));
      break;
    case EV_JOIN_FAILED:
      if (debug) Serial.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
      if (debug) Serial.println(F("EV_REJOIN_FAILED"));
      break;
    case EV_TXCOMPLETE:
      if (debug) Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
        if (debug) Serial.println(F("Received ack"));
      if (LMIC.dataLen) {
        if (debug) {
          Serial.println(F("Received "));
          Serial.println(LMIC.dataLen);
          Serial.println(F(" bytes of payload"));
        }
      }
      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), LoRaWANsend); // Enable to regularly send payload every TX_INTERVAL seconds
      break;
    case EV_LOST_TSYNC:
      if (debug) Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      if (debug) Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      if (debug) Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      if (debug) Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      if (debug) Serial.println(F("EV_LINK_ALIVE"));
      break;
    case EV_SCAN_FOUND:
      if (debug) Serial.println(F("EV_SCAN_FOUND"));
      break;
    case EV_TXSTART:
      if (debug) Serial.println(F("EV_TXSTART"));
      break;
    case EV_TXCANCELED:
      if (debug) Serial.println(F("EV_TXCANCELED"));
      break;
    case EV_RXSTART:
      /* do not print anything -- it wrecks timing */
      break;
    case EV_JOIN_TXCOMPLETE:
      if (debug) Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
      break;

    default:
      if (debug) Serial.print(F("Unknown event: "));
      if (debug) Serial.println((unsigned)ev);
      break;
  }
}

void LoRaWANsend(osjob_t* j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    if (debug) Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
    getSensorData();
    setBattLevel();
    memcpy(payload, &payload_data, sizeof(payload_data));

    // prepare upstream data transmission at the next possible time.
    // transmit on port 1 (the first parameter); you can use any value from 1 to 223 (others are reserved).
    // don't request an ack (the last parameter, if not zero, requests an ack from the network).
    // Remember, acks consume a lot of network resources; don't ask for an ack unless you really need it.
    txCounter++;
    LMIC_setTxData2(1, payload, sizeof(payload), 1);
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

// Main loop functions -------------------------------------------------------|

void goToSleep(uint32_t sleepMillis) {
  if (USBDevice.connected()) {
    debug = true;
    delay(sleepMillis);
    return;
  }
  txMillis = millis() - currentMillis;  // Set awake time by subracting time woken from sleep from time now
  LowPower.deepSleep(sleepMillis);      // Go to sleep
  /*            Z              /
  /              z             /
  /           z                /
  /            z              */
  currentMillis = millis() + sleepMillis;  // Update millis value by adding sleep time to current value
  setMillis(currentMillis);                // Push corrected value to millis()
}

uint8_t setBattLevel(void) {
  txCounter = 0;
  digitalWrite(PIN_PER_PWR, HIGH);
  delay(1);
  uint32_t vBatRaw = analogRead(PIN_V_BATT);
  payload_data.mVBatt = (float)vBatRaw * mV_PER_LSB;
  digitalWrite(PIN_PER_PWR, LOW);
  if (debug) Serial.printf("Batt mV: %imV\n", (uint32_t)payload_data.mVBatt);
  if ((payload_data.mVBatt > 4500) || (payload_data.mVBatt < 100)) return MCMD_DEVS_EXT_POWER;  // Assume plugged in to external supply
  if (payload_data.mVBatt < 3000) return 1;                                                     // Battery dead
  uint32_t Vrange = payload_data.mVBatt - 3000;
  if (Vrange > 199) Vrange = 199;
  if (debug) Serial.printf("V range: %imV\n", Vrange);
  uint8_t bLevel = (uint8_t)(Vrange * 1.27) + 1;
  if (debug) Serial.printf("LoRa batt level: %i\n", bLevel);
  return bLevel;
}

void getSensorData(void) {
  sensors_event_t temp_event;
  
  dps_temp->getEvent(&temp_event);
  payload_data.temperature = temp_event.temperature;
  if (debug) {
    Serial.print("Temperature: ");
    Serial.print(payload_data.temperature);
    Serial.println("°C");
  }
}
// Utility functions ---------------------------------------------------------|

// Debug functions -----------------------------------------------------------|

void printHex2(unsigned v) {
  v &= 0xff;
  if (v < 16)
    Serial.print('0');
  Serial.print(v, HEX);
}

// Setup functions -----------------------------------------------------------|

void setupPins(void) {
  pinMode(PIN_PER_PWR, OUTPUT);
  digitalWrite(PIN_PER_PWR, LOW);
}

void setupComm(void) {
  if (debug) {
    Serial.begin(115200);
    int serialChecks = 10;
    while (!Serial) {
      delay(1000);
      serialChecks--;
      if (!serialChecks) {
        debug = false;
        break;
      }
    }
    if (debug) {
      Serial.println(F("Starting"));
      Serial.println("DPS310 sensor start");
    }
  }
}

void setupAnalogue(void) {
  analogReference(AR_INTERNAL1V65);
  analogReadResolution(12);
}

void setupOnboardSensor(void) {
  if (!dps.begin_I2C()) {
    if (debug) Serial.println("Failed to find DPS310 sensor");
  }
  if (debug) Serial.println("DPS310 temperature sensor OK");

  // Setup highest precision
  dps.configureTemperature(DPS310_128HZ, DPS310_8SAMPLES);

  if (debug) {
    dps_temp->printSensorDetails();
  }
}

void setupLoRaWAN(void) {
  // LMIC init
  os_init_ex(&lorawan_pins);
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  // Disable link-check mode and ADR, because ADR tends to complicate testing.
  LMIC_setLinkCheckMode(1);
  // Set the data rate to Spreading Factor 7.  This is the fastest supported rate for 125 kHz channels, and it
  // minimizes air time and battery power. Set the transmission power to 14 dBi (25 mW).
  LMIC_setDrTxpow(DR_SF7, 14);

  LMIC_setBatteryLevel(setBattLevel());

  LoRaWANsend(&sendjob); // Enable to send data immediately on startup
}
