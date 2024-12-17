/************************************************************************************************************
Unit 1 keys

LoRaWAN random keys (MSB first)
DevEUI  aca52aff35178802
JoinEUI 922fc2dcc721bcb8
AppKey  04c3dfcc37abe779dd64024437b40c82

LSB first, byte arrays for LMIC lib
DevEUI (LSB):  { 0x2, 0x88, 0x17, 0x35, 0xff, 0x2a, 0xa5, 0xac, }
JoinEUI (LSB): { 0xb8, 0xbc, 0x21, 0xc7, 0xdc, 0xc2, 0x2f, 0x92, }
AppKey (MSB):  { 0x4, 0xc3, 0xdf, 0xcc, 0x37, 0xab, 0xe7, 0x79, 0xdd, 0x64, 0x2, 0x44, 0x37, 0xb4, 0xc, 0x82, }

************************************************************************************************************/

// DEVEUI -> This should also be in little endian format.
static const u1_t PROGMEM DEVEUI[8] = { 0x2, 0x88, 0x17, 0x35, 0xff, 0x2a, 0xa5, 0xac, };

// APPEUI (or JOINEUI) -> This must be in little-endian format, so least-significant-byte first.
static const u1_t PROGMEM APPEUI[8] = { 0xb8, 0xbc, 0x21, 0xc7, 0xdc, 0xc2, 0x2f, 0x92, };

// APPKEY -> This key should be in big endian format.
static const u1_t PROGMEM APPKEY[16] = { 0x4, 0xc3, 0xdf, 0xcc, 0x37, 0xab, 0xe7, 0x79, 0xdd, 0x64, 0x2, 0x44, 0x37, 0xb4, 0xc, 0x82, };

void os_getDevEui(uint8_t* buf) {
  memcpy_P(buf, DEVEUI, 8);
}

void os_getArtEui(uint8_t* buf) {
  memcpy_P(buf, APPEUI, 8);
}

void os_getDevKey(uint8_t* buf) {
  memcpy_P(buf, APPKEY, 16);
}