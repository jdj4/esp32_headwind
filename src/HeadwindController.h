#ifndef HEADWINDCONTROLLER_H
#define HEADWINDCONTROLLER_H

#include <Arduino.h>
#include <stdint.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>

/* Specify the Service UUID of Server */
static BLEUUID serviceUUID("00001826-0000-1000-8000-00805f9b34fb");
/* Specify the Characteristic UUID of Server */
static BLEUUID charSpeedUUID("00002ad2-0000-1000-8000-00805f9b34fb");
static BLEUUID charPowerUUID("00002ad9-0000-1000-8000-00805f9b34fb");
static BLEUUID charStatusUUID("00002ada-0000-1000-8000-00805f9b34fb");
//static BLEAddress headwindAddress("dd:d5:42:30:32:4f");

// Change this hardcoded MAC Address to correspond to the MAC Address of the correct headwind fan
// on a Linux CLI, the command `bluetoothctl` followed by `scan on` can be used to find the address
static BLEAddress headwindAddress("dc:f8:0d:3c:c7:b9");

class HeadwindController : BLEClientCallbacks {
    public:
        //HeadwindController(BLEAddress macAddress);
        bool setPower(int power);
        bool connectToHeadwind();
        bool reconnect();
        //bool init(BLEAdvertisedDevice &advertisedDevice);
        bool isConnected();
        bool connectedToHeadwind;
        bool doConnect;
        void onDisconnect(BLEClient *pclient);
        void onConnect(BLEClient *pclient);
        void notifyPowerCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                                             uint8_t *pData, size_t length, bool isNotify);

    private:
        BLEClient *pClientHeadwind;
        BLERemoteCharacteristic *powerCharacteristics;
        //BLEAddress address;
        //BLEAdvertisedDevice *device;
};

#endif