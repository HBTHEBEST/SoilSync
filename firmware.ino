#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>  // Add this for notifications

HardwareSerial MySerial(1);  // Serial1 for NPK sensor (UART)

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *npkCharacteristic;
BLEServer *pServer;
bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE device connected");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BLE device disconnected, restarting advertising...");
    // The correct way to restart advertising
    BLEDevice::startAdvertising();
  }
};

void setup() {
  Serial.begin(115200);
  MySerial.begin(9600, SERIAL_8N1, 16, -1);  // GPIO16 for RX, no TX
  
  Serial.println("Starting BLE setup...");

  // Initialize BLE
  BLEDevice::init("ESP32-NPK");
  
  // Create server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Create characteristic with proper descriptor for notifications
  npkCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  
  // Add descriptor for notifications - THIS WAS MISSING
  npkCharacteristic->addDescriptor(new BLE2902());

  // Set initial value
  npkCharacteristic->setValue("Waiting...");
  
  // Start the service
  pService->start();

  // Configure advertising properly
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);  // THIS WAS MISSING
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  
  // Start advertising
  BLEDevice::startAdvertising();  // This is the correct way

  Serial.println("BLE advertising started with SERVICE_UUID");
}

void loop() {
  // Check for NPK data
  if (MySerial.available()) {
    String npkData = MySerial.readStringUntil('\n');
    npkData.trim();
    Serial.println("NPK from Arduino: " + npkData);
    
    // Only update and notify if connected
    if (deviceConnected) {
      npkCharacteristic->setValue(npkData.c_str());
      npkCharacteristic->notify();
      Serial.println("Notified BLE client: " + npkData);
    }
    delay(100);  // Limit BLE spam
  }
  
  delay(20); // Small delay to prevent CPU hogging
}