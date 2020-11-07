#ifndef BLE_H
#define BLE_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "Arduino.h"

class MyServerCallbacks: public BLEServerCallbacks {
  public:
    MyServerCallbacks();
    void onConnect(BLEServer* pServer);
    void onDisconnect(BLEServer* pServer);
    bool isConnected();
  private:
    bool deviceConnected;
};

class MyCallbacks: public BLECharacteristicCallbacks {
    std::string rxValue;
    bool addCR;
  public:
    MyCallbacks();
    void onWrite(BLECharacteristic *pCharacteristic);
    bool available();
    std::string readStr();
    char read();
    int read(unsigned char *bfr, int maxLen);
};

class BLE {
  public:
    BLE();
    bool begin(const char *s);
    void send(const char *s);
    void send(unsigned char s);
    void send(word bfr[], byte bfrLen);
    bool available();
    unsigned char read();
    int read(unsigned char *bfr, int maxLen);
    bool connected();
    void check();

  private:
    BLEServer *pServer = NULL;
    BLECharacteristic *pTxCharacteristic;
    MyServerCallbacks *serverCb;
    MyCallbacks *myCb;
    bool oldDeviceConnected;
    char rxBuffer[100];
    int rxLen;
    char txBuffer[100];
    int txLen;
};
#endif
