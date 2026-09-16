#include <SPI.h>
#include <RF24.h>
#include <Preferences.h>

static const uint8_t CE_PIN  = 4;
static const uint8_t CSN_PIN = 5;

RF24 radio(CE_PIN, CSN_PIN);
Preferences prefs;

static const uint8_t ADDR[5]     = {0x55, 0x55, 0x55, 0x55, 0x55};
static const uint8_t PREAMBLE[8] = {0x53, 0x39, 0x14, 0xDD, 0x1C, 0x49, 0x34, 0x12};

uint32_t remoteId    = 0xABCDEF;
uint8_t  counter     = 0;
uint8_t  repetitions = 20;
uint16_t repeatDelay = 10;

uint16_t crc16(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFE;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t b = 0; b < 8; b++) {
      if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
      else              crc = (crc << 1);
    }
  }
  return crc;
}

void buildPacket(uint8_t *pkt, uint16_t command, uint8_t cnt) {
  memcpy(pkt, PREAMBLE, 8);
  pkt[8]  = (remoteId >> 16) & 0xFF;
  pkt[9]  = (remoteId >> 8) & 0xFF;
  pkt[10] = remoteId & 0xFF;
  pkt[11] = 0xFF;
  pkt[12] = cnt;
  pkt[13] = (command >> 8) & 0xFF;
  pkt[14] = command & 0xFF;
  uint16_t c = crc16(pkt, 15);
  pkt[15] = (c >> 8) & 0xFF;
  pkt[16] = c & 0xFF;
}

void sendCode(uint16_t command) {
  uint8_t pkt[17];
  buildPacket(pkt, command, counter);
  counter = (counter + 1) & 0xFF;
  for (uint8_t i = 0; i < repetitions; i++) {
    radio.write(pkt, 17);
    delay(repeatDelay);
  }
}

int clamp15(long v) {
  if (v < 0)  return 0;
  if (v > 15) return 15;
  return (int)v;
}

void setBrightness(long v) {
  sendCode(0x0500 - 16);
  sendCode(0x0400 + clamp15(v));
}

void setColorTemp(long v) {
  sendCode(0x0300 - 16);
  sendCode(0x0200 + clamp15(v));
}

rf24_crclength_e savedCrcLength = RF24_CRC_16;

void configureTx() {
  radio.setChannel(6);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_2MBPS);
  radio.setRetries(0, 0);
  radio.setAutoAck(false);
  radio.setCRCLength(savedCrcLength);
  radio.disableDynamicPayloads();
  radio.setPayloadSize(17);
  radio.openWritingPipe(ADDR);
  radio.stopListening();
}

bool decodeScan(const uint8_t *raw, int nbytes, uint32_t &idOut) {
  if (nbytes < 12) return false;
  const uint8_t *data = raw + 3;

  uint8_t sep     = data[3];
  uint8_t counter = data[4];
  uint16_t crc    = ((uint16_t)data[7] << 8) | data[8];

  uint8_t hdr[15];
  memcpy(hdr, PREAMBLE, 8);
  hdr[8] = data[0]; hdr[9] = data[1]; hdr[10] = data[2];
  hdr[11] = sep; hdr[12] = counter; hdr[13] = data[5]; hdr[14] = data[6];

  if (crc16(hdr, 15) != crc) return false;

  idOut = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
  return true;
}

void runScan(unsigned long durationMs) {
  Serial.println("SCAN START");

  radio.stopListening();
  radio.setCRCLength(RF24_CRC_DISABLED);
  radio.setAutoAck(false);
  radio.disableDynamicPayloads();
  radio.setPayloadSize(12);
  radio.setAddressWidth(5);
  radio.openReadingPipe(1, 0x533914DD1CULL);
  radio.startListening();

  const unsigned long start = millis();
  while (millis() - start < durationMs) {
    if (radio.available()) {
      uint8_t raw[12];
      radio.read(raw, 12);
      uint32_t id = 0;
      if (decodeScan(raw, 12, id)) {
        Serial.printf("SCAN id=%06X\n", id);
        configureTx();
        return;
      }
    }
    delay(5);
  }

  configureTx();
  Serial.println("SCAN NONE");
}

void setup() {
  Serial.begin(115200);
  delay(200);

  prefs.begin("lightbar", false);
  remoteId = prefs.getUInt("id", remoteId);

  bool ok = false;
  for (uint8_t i = 0; i < 10 && !ok; i++) {
    ok = radio.begin();
    if (!ok) delay(100);
  }

  savedCrcLength = radio.getCRCLength();
  configureTx();

  if (!ok) Serial.println("WARN nRF24 begin() returned false, trying anyway");
  Serial.printf("READY id=%06X\n", remoteId);
}

void loop() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  String cmd = line;
  long arg = 0;
  bool hasArg = false;
  int colon = line.indexOf(':');
  if (colon >= 0) {
    cmd = line.substring(0, colon);
    String a = line.substring(colon + 1);
    a.trim();
    if (a.startsWith("0x") || a.startsWith("0X")) arg = strtol(a.c_str() + 2, nullptr, 16);
    else                                          arg = strtol(a.c_str(), nullptr, 10);
    hasArg = true;
  }
  cmd.toUpperCase();

  if (cmd == "PING") {
    Serial.println("OK PONG");
  } else if (cmd == "ID" && hasArg) {
    String hex = line.substring(colon + 1);
    hex.trim();
    remoteId = strtoul(hex.c_str(), nullptr, 16) & 0xFFFFFF;
    prefs.putUInt("id", remoteId);
    Serial.printf("OK ID=%06X\n", remoteId);
  } else if (cmd == "ONOFF") {
    sendCode(0x0100);
    Serial.println("OK ONOFF");
  } else if (cmd == "RESET") {
    sendCode(0x0600);
    Serial.println("OK RESET");
  } else if (cmd == "COOLER") {
    sendCode(0x0200 + clamp15(hasArg ? arg : 1));
    Serial.println("OK COOLER");
  } else if (cmd == "WARMER") {
    sendCode(0x0300 - clamp15(hasArg ? arg : 1));
    Serial.println("OK WARMER");
  } else if (cmd == "HIGHER") {
    sendCode(0x0400 + clamp15(hasArg ? arg : 1));
    Serial.println("OK HIGHER");
  } else if (cmd == "LOWER") {
    sendCode(0x0500 - clamp15(hasArg ? arg : 1));
    Serial.println("OK LOWER");
  } else if (cmd == "BRIGHT" && hasArg) {
    setBrightness(arg);
    Serial.println("OK BRIGHT");
  } else if (cmd == "TEMP" && hasArg) {
    setColorTemp(arg);
    Serial.println("OK TEMP");
  } else if (cmd == "RAW" && hasArg) {
    sendCode((uint16_t)arg);
    Serial.println("OK RAW");
  } else if (cmd == "STATUS") {
    Serial.printf("OK STATUS id=%06X counter=%u\n", remoteId, counter);
  } else if (cmd == "SCAN") {
    runScan(15000);
    Serial.println("OK SCAN");
  } else {
    Serial.printf("ERR unknown command: %s\n", line.c_str());
  }
}
