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
#include <TimeLib.h>
#include <DS1307RTC.h>

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

char *formatAsCsv(char *text1, char *text2)
{
    char *csvValue = (char *)malloc(sizeof(char) * 25); //csv data buffer
    strcpy(csvValue, text1);
    strcat(csvValue, ",");
    strcat(csvValue, text2);
    return csvValue;
}

char *convertFloatToString(float float1)
{
    char *stringValue = (char *)malloc(sizeof(char) * 12);
    dtostrf(float1, 6, 3, stringValue); // float_val, min_width, digits_after_decimal, char_buffer
    return stringValue;
}

char *timeNowAsString()
{
    char *stringTime = (char *)malloc(sizeof(char) * 12);
    time_t timeNow = now();
    sprintf(stringTime, "%ld", timeNow);
    return stringTime;
}

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
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_INDICATE);
    pCharacteristicrx = pService->createCharacteristic(
        RX_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |   
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
    
    //give a default value to time characteristic
    int timeFallback = 0;
    pCharacteristicrx->setValue(timeFallback);
    Serial.println("Waiting a client connection to notify...");
}

void loop()
{
    // Reading sensor value
    potValue = analogRead(potPin) * (1.1 / 4095) * 1000; //convert to mvolts
    // Notify changed value
    if (deviceConnected)
    {
        Serial.println("connected");
        // If time is not set by the client
        if (timeStatus() != timeSet)
        {
            uint8_t *rxValue = pCharacteristicrx->getData();
            signed long timeValue = (rxValue[3] << 24) | (rxValue[2] << 16) | (rxValue[1] << 8) | rxValue[0];
            Serial.print("Received Value: ");
            Serial.println(timeValue);
            time_t t = timeValue;

            // set time if the client send time data
            if (t != 0)
            {
                RTC.set(t); // set the RTC and the system time to the received value
                setTime(t);
            }
        }
        // Else, time is set by client
        else
        {
            char *potValue_s = convertFloatToString(potValue);

            char *timeNow_s = timeNowAsString();

            char *txValue = formatAsCsv(timeNow_s, potValue_s);

            pCharacteristictx->setValue(txValue);
            pCharacteristictx->notify();
            Serial.print("transmited value: ");
            Serial.println(txValue);

            free(potValue_s);
            free(timeNow_s);
            free(txValue);
        }

        delay(1000); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected)
    {
        delay(1000);                 // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("disconnecting, start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected)
    {
        // do stuff here on connecting
        Serial.println("connecting");
        oldDeviceConnected = deviceConnected;
        delay(1000);
    }
    if (!deviceConnected)
    {
        Serial.println("disconnected");

        // If time is set by the client
        if (timeStatus() == timeSet)
        {
            Serial.println("storing data");
        }
        delay(1000);
    }
}