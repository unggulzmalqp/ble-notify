/*
    The design of creating the BLE server is:
    1. Create a BLE Server
    2. Create a BLE Service
    3. Create a BLE Characteristic on the Service
    4. Create a BLE Descriptor on the characteristic
    5. Start the service.
    6. Start advertising.

    This code read an analoge value from a sensor then notify the client about the value.
    Credits:
    - https://github.com/nkolban/ESP32_BLE_Arduino/blob/master/examples/BLE_notify/BLE_notify.ino
    - https://www.instructables.com/id/ESP32-BLE-Android-App-Arduino-IDE-AWESOME/
    - https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLETests/Arduino/BLE_uart/BLE_uart.ino
*/

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristictx = NULL;
BLECharacteristic *pCharacteristicrx = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool dataRecieved = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "62f794e9-3ef6-4fdd-81dc-c1940c81a5e2"
#define TX_CHARACTERISTIC_UUID "705e55a6-3d23-4bfa-9a27-ebc31f301cf9"
#define RX_CHARACTERISTIC_UUID "7e8adb7e-b68b-48ce-aaad-d97a3399ef5a"

// Potentiometer is connected to GPIO 34
const int potPin = 34;

// variable for storing the potentiometer value
float potValue = 0;

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
    }
};

void setup()
{
    Serial.begin(115200);
    analogSetAttenuation(ADC_0db); //set fullscale to 1.1V

    // Create the BLE Device
    BLEDevice::init("ESP32");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    pCharacteristictx = pService->createCharacteristic(
        TX_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_INDICATE);
    pCharacteristicrx = pService->createCharacteristic(
        RX_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_INDICATE);

    // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
    // Create a BLE Descriptor
    pCharacteristictx->addDescriptor(new BLE2902());
    pCharacteristicrx->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();
    pCharacteristicrx->setValue("mambana");
    Serial.println("Waiting a client connection to notify...");
}

void loop()
{
    // Reading potentiometer value
    potValue = analogRead(potPin) * (1.1 / 4095) * 1000; //convert to mvolts
    // Serial.println(potValue);
    delay(1000);
    //notify changed value
    if (deviceConnected)
    {
        // Serial.println("connected");
        std::string rxValue = pCharacteristicrx->getValue();
        char txString[8];                  // transmitted data
        dtostrf(potValue, 1, 3, txString); // float_val, min_width, digits_after_decimal, char_bufferr
        pCharacteristictx->setValue(txString);
        pCharacteristictx->notify();

        if (rxValue.length() > 0)
        {
            Serial.println();
            Serial.print("Received Value: ");

            for (int i = 0; i < rxValue.length(); i++)
            {
                Serial.print(rxValue[i]);
            }
        }
        delay(1000); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected)
    {
        delay(1000);                 // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected)
    {
        // do stuff here on connecting
        Serial.println("connecting");
        delay(1000);
        oldDeviceConnected = deviceConnected;
    }
}