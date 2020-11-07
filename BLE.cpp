#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "BLE.h"
#include <cstring>
#include <algorithm>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

MyServerCallbacks::MyServerCallbacks() {
  deviceConnected = false;
}
void MyServerCallbacks::onConnect(BLEServer* pServer) {
  deviceConnected = true;
}
void MyServerCallbacks::onDisconnect(BLEServer* pServer) {
  deviceConnected = false;
}
bool MyServerCallbacks::isConnected() {
  return deviceConnected;
}

MyCallbacks::MyCallbacks() {
  rxValue = "";
  addCR = false;
}
void MyCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
  rxValue += pCharacteristic->getValue();
}
bool MyCallbacks::available() {
  return rxValue.length() > 0;
}
std::string MyCallbacks::readStr() {
  std::string s = rxValue;
  rxValue = "";
  return s;
}
char MyCallbacks::read() {
  char c = 0;
  if (addCR) {
    c = '\n';
    addCR = false;
  } else if (rxValue.length() == 1) {
    c = rxValue.front();
    if (c != '\n') {
      addCR = true;
    }
    rxValue.clear();
  } else if (rxValue.length() > 0) {
    c = rxValue.front();
    rxValue.erase(0, 1);
  }
  return c;
}
int MyCallbacks::read(unsigned char *bfr, int maxLen) {
  if (rxValue.length() == 0) return 0;
  if (rxValue.length() < (maxLen - 1)) maxLen = rxValue.length();
  memcpy(bfr, rxValue.data(), maxLen - 1);
  bfr[maxLen] = '\0';
  return maxLen;
}

BLE::BLE() {
  oldDeviceConnected = false;
  rxLen = 0;
  txLen = 0;
}

bool BLE::begin(const char *s) {
  BLEDevice::init(s);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(serverCb = new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE
                                         );
  pRxCharacteristic->setCallbacks(myCb = new MyCallbacks());
  pService->start();
  pServer->getAdvertising()->start();
  return true;
}

void BLE::send(const char *s) {
  pTxCharacteristic->setValue((byte *)s, strlen(s));
  pTxCharacteristic->notify();
}

void BLE::send(unsigned char s) {
  pTxCharacteristic->setValue(&s, 1);
  pTxCharacteristic->notify();
}

void BLE::send(word *msg, byte bfrLen) {
  char bfr[100];
  byte idx;

  sprintf(bfr, "%c%3x %2x %2x", '%', msg[0], msg[1], msg[2]);
  for (idx = 3; idx < bfrLen; idx++) {
    sprintf(bfr + strlen(bfr), " %2x", msg[idx]);
  }
  sprintf(bfr + strlen(bfr), "\n");
  pTxCharacteristic->setValue((byte *)bfr, strlen(bfr));
  pTxCharacteristic->notify();
}

bool BLE::available() {
  myCb->available();
}

unsigned char BLE::read(void) {
  return myCb->read();
}

int BLE::read(unsigned char *bfr, int maxLen) {
  return myCb->read(bfr, maxLen);
}

bool BLE::connected() {
  return serverCb->isConnected();
}

void BLE::check() {
  if (serverCb->isConnected()) {
    // can transmit
  }
  // disconnecting
  if (!serverCb->isConnected() && oldDeviceConnected) {
    //    delay(500); // give the bluetooth stack the chance to get things ready
    Serial.println("BLE disconnecting");
    pServer->startAdvertising(); // restart advertising
    oldDeviceConnected = serverCb->isConnected();
  }
  // connecting
  if (serverCb->isConnected() && !oldDeviceConnected) {
    // do stuff here on connecting
    Serial.println("BLE connecting");
    oldDeviceConnected = serverCb->isConnected();
  }
}
