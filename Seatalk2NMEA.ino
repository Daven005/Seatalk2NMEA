#include <Signal.h>
#include <Timer.h>

#include "Arduino.h"
#include "seatalkOut.h"
#include "seatalkIn.h"
#include "flashLED.h"
#include "defs.h"
#include "ByteBuffer.h"
#include "CharBuffer.h"
#include "PCD8544.h"
#include <WiFi.h>
#include "BLE.h"
#include "dataStore.h"
#include "nmea.h"
#include <SimpleCLI.h>


PCD8544 lcd(LCD_CLK, LCD_DIN, LCD_DC, LCD_RST_, LCD_CE_);
SeaTalkIn stInput;
SeaTalkOut stOutput;
FlashLED flash; // Reqd for seatalkOut.cpp
FlashLED flashYellow;
FlashLED flashGreen;
Timer t;
unsigned long lastMsgTime = millis();
bool sendSeaTalk_BLE = true;

BLE ble;
DataStore ds;
Nmea nmea;

SimpleCLI cli;
Command set;

void cliSetup();
void cmdError(cmd_error* e);
void setCommand(cmd* c);

char version[] = "SeaTalk NMEA & BLE - V0.13";

void setup() {
  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);
  nmea.begin();
  lcd.begin(); // Use defaults
  Serial.println(version);
  ble.begin("ST-NMEA");
  ble.send(version);
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(version);
  lcd.setCursor(0, 5);
  if (nmea.txMode()) {
    nmea.println(version);
  }
  lcd.printf("NMEA %cx Mode", nmea.txMode() ? 'T' : 'R');
  pinMode(STIN, INPUT);
  pinMode(STOUT, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(NMEA_TX, OUTPUT);
  pinMode(LCD_LT_, OUTPUT);
  digitalWrite(LCD_LT_, 1);
  stInput.begin(STIN);
  stOutput.begin(STOUT, STIN);
  flash.begin(RED_LED);
  flashYellow.begin(YELLOW_LED);
  flashGreen.begin(GREEN_LED);
  flash.start(5, 200);
  flashYellow.start(5, 100);
  flashGreen.start(5, 210);
  Serial.println("Ready");
  t.every(2000, checkDataStore);
  digitalWrite(LCD_LT_, 0); // ON

  cliSetup();
}

bool isHex(const char c, word *b) {
  if (isDigit(c)) {
    *b |= c - '0';
    return true;
  }
  if ('A' <= c && c <= 'F') {
    *b |= c - 'A' + 10;
    return true;
  }
  if ('a' <= c && c <= 'f') {
    *b |= c - 'a' + 10;
    return true;
  }
  return false;
}

byte decodeMsg(const char *bfr, ByteBuffer *msg) {
  word b = 0;
  while (*bfr) {
    while (isHex(*bfr, &b)) {
      b = b << 4;
      bfr++;
    }
    if (!msg->add((byte)(b >> 4)))
      return false;
    while (*bfr && !isHex(*bfr, &b)) {
      bfr++;
    }
  }
  return true;
}

void decodeSeaTalkMsg(word cmd) {
  static word msg[20];
  static byte idx = 0;
  static byte expectedLength = 10;

  if (cmd & 0x100) {
    Serial.println();
    idx = 0;
  }
  if (idx == 1)
    expectedLength = (cmd & 0xf) + 3;
  msg[idx++] = cmd;
  Serial.print(cmd, HEX);
  Serial.print(" ");
  if (idx >= expectedLength) {
    idx = 0;

    if (ble.connected()) {
      flashGreen.turnOn();
      if (sendSeaTalk_BLE) ble.send(msg, expectedLength);
      flashYellow.start(1, 100);
    } else {
      flashGreen.turnOff();
      flash.start(1, 100);
    }
    ds.decodeSeaTalk(msg, expectedLength);
  }
}

byte checkSum(char *bfr) {
  byte sum = 0;
  while (*bfr) {
    sum ^= *bfr;
    bfr++;
  }
  return sum;
}

void checkDataStore() {
  char bfr[100];
  int lineNo = 0;

  if (ds.isValidPosition()) {
    char latitude[40];
    char longitude[40];
    ds.printLongitude(longitude);
    ds.printLatitude(latitude);
    lcd.setCursor(0, lineNo++); lcd.clearLine();
    lcd.print("Lat:  "); lcd.print(latitude);
    lcd.setCursor(0, lineNo++); lcd.clearLine();
    lcd.print("Long: "); lcd.print(longitude);
    sprintf(bfr, "$GPGGA,");
    sprintf(ds.printTime(bfr + strlen(bfr)), ",");
    sprintf(bfr + strlen(bfr), "%s,", latitude);
    sprintf(bfr + strlen(bfr), "%s,1,,,,", longitude);
    sprintf(bfr + strlen(bfr), "*%02X\r\n", checkSum(bfr));
    nmea.print(bfr);
    Serial.print(bfr);
  }
  if (ds.isValidDepth()) {
    char depth[40];
    ds.printDepth(depth);
    lcd.setCursor(0, lineNo++); lcd.clearLine();
    lcd.print("Depth: "); lcd.print(depth);
    sprintf(bfr, "$GPDBT,,f,%s,M,,F", depth);
    sprintf(bfr + strlen(bfr), "*%02X\r\n", checkSum(bfr));
    nmea.print(bfr);
    Serial.print(bfr);
  }
  if (ds.isValidApparentWind()) {
    char angle[40];
    char speed[40];
    ds.printApparentWindAngle(angle);
    ds.printApparentWindSpeed(speed);
    lcd.setCursor(0, lineNo++); lcd.clearLine();
    lcd.print("Wind: ");
    lcd.print(angle);
    lcd.print("/");
    lcd.print(speed);
    sprintf(bfr, "$GPMWV,%s,R,%s,K,A", angle, speed);
    sprintf(bfr + strlen(bfr), "*%02X\r\n", checkSum(bfr));
    nmea.print(bfr);
    Serial.print(bfr);
  }
}

void cliSetup() {
  cli.setOnError(cmdError);
  set = cli.addCommand("set", setCommand);
  set.addArgument("n/mea", "x");
  set.addArgument("s/eatalk", "x");
  set.setCaseSensetive(false);
}

void cmdError(cmd_error* e) {
  CommandError cmdError(e);
  Serial.print("Cmd err: ");
  Serial.println(cmdError.toString());
}

void setCommand(cmd* c) {
  Command cmd(c);
  if (cmd == set) {
    Argument n = cmd.getArgument("nmea");
    if (n) {
      String v = n.getValue();
      std::transform(v.begin(), v.end(), v.begin(),
      [](unsigned char c) {
        return std::toupper(c);
      });
      if (v == "RX") nmea.setMode(Nmea::Mode_t::RX);
      if (v == "TX") nmea.setMode(Nmea::Mode_t::TX);
      lcd.setCursor(0, 5);
      lcd.printf("NMEA %cx Mode", nmea.txMode() ? 'T' : 'R');
    }
    Argument s = cmd.getArgument("seatalk");
    if (s) {
      String v = s.getValue();
      std::transform(v.begin(), v.end(), v.begin(),
        [](unsigned char c) { return std::toupper(c); });
      if (v == "ON") sendSeaTalk_BLE = true;
      if (v == "OFF") sendSeaTalk_BLE = false;
      lcd.setCursor(0, 5);
      lcd.printf("ST-BLE %s   ", sendSeaTalk_BLE ? "ON" : "OFF");
    }
  }
}

void checkNmeaInput(char c, ProcessNmeaInput_t respond) {
  static std::string bfr;
  int lineLen;

  bfr.push_back(c);
  if ((lineLen = bfr.find('\n')) != std::string::npos) {
    std::string line (bfr, 0, lineLen);
    bfr.erase(0, lineLen + 1);
    if (line.find(">", 0) == 0) {
      line.erase(0, 1);
      respond(COMMAND_ACTION, line.c_str());
    } else if (line.find("$GP", 0) == 0) {
      respond(PRINT_ACTION, line.c_str());
    } else if (line.find("%", 0) == 0) {
      respond(SEATALK_ACTION, line.c_str()+1);
    } else {
      flash.start(2, 300);
      Serial.println(line.c_str());
    }
  }
}

void processCommand(const char *cmd) {
  cli.parse(cmd);
}

void processNmeaInput(NmeaAction_t action, const char *line) {
  char bfr[200];
  switch (action) {
    case PRINT_ACTION:
      Serial.println(line);
      sprintf(bfr, "%s\n", line);
      ble.send(bfr);
      break;
    case COMMAND_ACTION:
      processCommand(line);
      break;
    case SEATALK_ACTION:
      ByteBuffer msg;
      if (decodeMsg(line, &msg)) {
        if (msg.length() >= 3) { // Minimum SeaTalk msg length
          stOutput.send(msg.string(), msg.length());
        }
        msg.clear();
      }
      break;
  }
}

void loop() {
  const byte goPort[] = {0x86, 0x11, 0x05, 0xFA};
  const byte goStbd[] = {0x86, 0x11, 0x07, 0xf8};
  static CharBuffer inputBfrSer;
  static CharBuffer inputBfrBle;

  stOutput.check(lastMsgTime);
  if (Serial.available()) {
    if (inputBfrSer.add(Serial.read())) {
      processCommand(inputBfrSer.string());
      inputBfrSer.clear();
    }
  }
  if (stInput.available()) {
    decodeSeaTalkMsg(stInput.get());
    lastMsgTime = millis();
  }
  if (ble.available()) {
    if (inputBfrBle.add(ble.read())) {
      // Complete line terminated with \n
      switch (inputBfrBle.string()[0]) {
        case '>': processCommand(inputBfrBle.string() + 1); break;
        case '%': {
            ByteBuffer msg;
            if (decodeMsg(inputBfrBle.string()+1, &msg)) {
              if (msg.length() >= 3) { // Minimum SeaTalk msg length
                stOutput.send(msg.string(), msg.length());
              }
              msg.clear();
            }
        }
          break;
        case '$': nmea.println(inputBfrBle.string()); break;
      }
      inputBfrBle.clear();
    }
  }
  if (nmea.available()) {
    flashYellow.start(2, 50);
    checkNmeaInput(nmea.read(), processNmeaInput);
  }
  flash.check();
  flashYellow.check();
  flashGreen.check();
  ble.check();
  t.update();
}
