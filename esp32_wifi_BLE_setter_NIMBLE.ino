#include "EEPROM.h"
#define EEPROM_SIZE 128

#include <WiFi.h>
#include <ESP32Ping.h>
const char* remote_host = "www.google.com";

#include <NimBLEDevice.h> //nimbleBLE로 스케치 용량확보
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
//#include <BLE2902.h>
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

NimBLEServer* pServer = NULL;
NimBLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

const int ledPin = 22;
const int modeAddr = 0;
const int wifiAddr = 10;

int modeIdx;

class MyServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
      deviceConnected = true;
      NimBLEDevice::startAdvertising();
    };

    void onDisconnect(NimBLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
        Serial.print("Value : ");
        Serial.println(value.c_str());
        writeString(wifiAddr, value.c_str());
      }
    }

    void writeString(int add, String data) {
      int _size = data.length();
      for (int i = 0; i < _size; i++) {
        EEPROM.write(add + i, data[i]);
      }
      EEPROM.write(add + _size, '\0');
      EEPROM.commit();
    }
};


void setup() {
  
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  if (!EEPROM.begin(EEPROM_SIZE)) {
    delay(1000);
  }

  modeIdx = EEPROM.read(modeAddr);
  Serial.print("modeIdx : ");
  Serial.println(modeIdx);

  EEPROM.write(modeAddr, modeIdx != 0 ? 0 : 1);
  EEPROM.commit();

  if (modeIdx != 0) {
    
    digitalWrite(ledPin, true);
    Serial.println("BLE MODE");
    bleTask();
  } else {
    
    digitalWrite(ledPin, false);
    Serial.println("WIFI MODE");
    wifiTask();
  }

}

void bleTask() {
  
  NimBLEDevice::init("ESP32 THAT PROJECT");

  
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  
  NimBLEService *pService = pServer->createService(SERVICE_UUID);

  
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      NIMBLE_PROPERTY::READ   |
                      NIMBLE_PROPERTY::WRITE  |
                      NIMBLE_PROPERTY::NOTIFY |
                      NIMBLE_PROPERTY::INDICATE
                    );

  pCharacteristic->setCallbacks(new MyCallbacks());

  //NimBLECharacteristic::createDescriptor();
  
  //pCharacteristic->addDescriptor(new BLE2902());

  
  pService->start();

  
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  
  NimBLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void wifiTask() {
  String receivedData;
  receivedData = read_String(wifiAddr);

  if (receivedData.length() > 0) {
    String wifiName = getValue(receivedData, ',', 0);
    String wifiPassword = getValue(receivedData, ',', 1);

    if (wifiName.length() > 0 && wifiPassword.length() > 0) {
      Serial.print("WifiName : ");
      Serial.println(wifiName);

      Serial.print("wifiPassword : ");
      Serial.println(wifiPassword);

      WiFi.begin(wifiName.c_str(), wifiPassword.c_str());
      Serial.print("Connecting to Wifi");
      while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
      }
      Serial.println();
      Serial.print("Connected with IP: ");
      Serial.println(WiFi.localIP());


      Serial.print("Ping Host: ");
      Serial.println(remote_host);

      if (Ping.ping(remote_host)) {
        Serial.println("Success!!");
      } else {
        Serial.println("ERROR!!");
      }

    }
  }
}

String read_String(int add) {
  char data[100];
  int len = 0;
  unsigned char k;
  k = EEPROM.read(add);
  while (k != '\0' && len < 500) {
    k = EEPROM.read(add + len);
    data[len] = k;
    len++;
  }
  data[len] = '\0';
  return String(data);
}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void loop() {
  
}
