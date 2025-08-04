#include "HeadwindController.h"

/**
 * Sets the power (fan speed) of the headwind device. The variable `power` can only be
 * between 0 and 100 inclusive. Setting power to 0 will turn the fan off, and setting
 * to a higher value will turn the fan on again.
 */
bool HeadwindController::setPower(int power) {
    power = (power < 0) ? 0 : (power > 100 ? 100 : power);
    if (isConnected()) {
        uint8_t buffer[] = { 2, power };
        Serial.print("\nSetting power to: ");
        Serial.println(power);
        powerCharacteristics->writeValue(buffer, 2);
        return true;
    }
    return false;
}

/**
 * Connects to the headwind device using the MAC Address defined in `HeadwindController.h`.
 * Connets to the device without scanning for bluetooth devices using the address. Returns
 * a boolean based on whether the device connects.
*/
bool HeadwindController::connectToHeadwind() {
    BLEUUID powerServiceUUID("a026ee0c-0a7d-4ab3-97fa-f1500f9feb8b");
    BLEUUID powerCharacteristicUUID("a026e038-0a7d-4ab3-97fa-f1500f9feb8b");
    BLEUUID initCharacteristicUUID("a026e038-0a7d-4ab3-97fa-f1500f9feb8b");

    Serial.print("Connecting to Headwind with MAC Address: ");
    Serial.println(headwindAddress.toString().c_str());

    pClientHeadwind = BLEDevice::createClient();
    pClientHeadwind->setClientCallbacks(this);

    // TODO: This works, change this to hardcoding the MAC Address
    /*Serial.println("Scanning for BLE devices...");
    BLEScan *pBLE_Scan = BLEDevice::getScan();
    BLEScanResults foundDevices = pBLE_Scan->start(5);
    for (int i = 0; i < foundDevices.getCount(); i++) {
        BLEAdvertisedDevice foundDevice = foundDevices.getDevice(i);
        Serial.print("Found: ");
        Serial.println(foundDevice.toString().c_str());

        if (foundDevice.getAddress().equals(headwindAddress)) {
            Serial.println("Found target device.\n");
            device = new BLEAdvertisedDevice(foundDevice);
            break;
        }
    }*/

    /*if (device == nullptr) {
        Serial.println("Target device not found.");
        return false;
    }*/

    if (pClientHeadwind->connect(headwindAddress, BLE_ADDR_TYPE_RANDOM)) { //if (pClientHeadwind->connect(device)) {
        // Services durchsuchen und ausgeben
        /*std::map<std::string, BLERemoteService*>* services = pClientHeadwind->getServices();
        for (auto const& pair : *services) {
            Serial.printf("Service UUID ist folgende: %s\n", pair.second->getUUID().toString().c_str());

            // Characteristics durchsuchen und ausgeben
            std::map<std::string, BLERemoteCharacteristic*>* characteristics = pair.second->getCharacteristics();
            for (auto const& charPair : *characteristics) {
                Serial.printf("  Characteristic UUID ist folgende: %s\n", charPair.second->getUUID().toString().c_str());
            }
        }*/

        // Wrap `notifyPowerCallback` in a lambda to to pass into `registerToNotify`
        auto callback = std::function<void(BLERemoteCharacteristic*, uint8_t*, size_t, bool)>(
            [this](BLERemoteCharacteristic* pRC, uint8_t* pData, size_t len, bool isN) {
                notifyPowerCallback(pRC, pData, len, isN);
            }
        );

        BLERemoteService* pRemoteService = pClientHeadwind->getService(powerServiceUUID);
        if (pRemoteService != nullptr) {
            powerCharacteristics = pRemoteService->getCharacteristic(powerCharacteristicUUID);
            if (powerCharacteristics) {
                if (powerCharacteristics->canNotify()) {
                    powerCharacteristics->registerForNotify(callback);
                }
                connectedToHeadwind = true;
            }
        } else {
            Serial.println("Service not found.");
            return false;
        }
        return true;
    } else {
        Serial.println("Failed to connect to headwind.");
        return false;
    }
};

/**
 * Attempts to reconnect to the headwind device in the case of a failed connection.
*/
bool HeadwindController::reconnect() {
    if (!isConnected()) {
        doConnect = true;
        delete pClientHeadwind;
        pClientHeadwind = nullptr;
        delete powerCharacteristics;
        powerCharacteristics = nullptr;
    }
    if (doConnect) {
        if (connectToHeadwind()) {
            Serial.println("Successfully connected to Headwind.");
        } else {
            Serial.println("Failed to connect to Headwind.");
            return false;
        }
        doConnect = false;
        return true;
    }
    return false;
}

/*bool HeadwindController::init(BLEAdvertisedDevice &advertisedDevice) {
    //Serial.println("init");
    //Serial.println(advertisedDevice.getAddress().toString().c_str());
    //Serial.println(advertisedDevice.toString().c_str());
    if (device == nullptr) {// && advertisedDevice.getAddress()==*address) {

        Serial.println("found ");
        device = new BLEAdvertisedDevice(advertisedDevice);
        Serial.println(device->toString().c_str());
        doConnect = true;
        pClientHeadwind = nullptr;
        powerCharacteristics = nullptr;
        return true;
    }
    return false;
}*/

void HeadwindController::onDisconnect(BLEClient *pclient)
{
    Serial.println("Headwind disconnected");
}

void HeadwindController::onConnect(BLEClient *pclient)
{
    Serial.println("Connected to Headwind");
}

bool HeadwindController::isConnected()
{
    if (pClientHeadwind == nullptr)
        return false;
    return pClientHeadwind->isConnected();
}

void HeadwindController::notifyPowerCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                                             uint8_t *pData, size_t length, bool isNotify)
{
    /*Serial.print("Notify callback for power characteristic: ");
    for (int i = 0; i < length; i++) {
        Serial.print(pData[i]);
        Serial.print(" ");
    }
    Serial.println();*/
}