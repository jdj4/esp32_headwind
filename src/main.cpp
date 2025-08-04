#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <LittleFS.h>

#include "HeadwindController.h"

#define DEVICE_NAME "Headwind_1"
#define HTTP_PORT 80

// Conditional compilation flag, set to `true` to erase the contents of the ESP filesystem
#define FORMAT_FS false

//const String ssid = "js25";
//const String password  = "xhj6yyhn3q3vp8u";

WiFiUDP connection;
IPAddress coverIP((uint32_t) 0);
const int connectionPort = 31319;
const int pluginPort = 31322;

// WiFi Manager Setup
AsyncWebServer server(HTTP_PORT);

String deviceName = "Headwind";

const char* NAME_PARAM = "device_name";
const char* SSID_PARAM = "ssid";
const char* PASS_PARAM = "pass";
const char* IP_PARAM = "ip";
const char* GATEWAY_PARAM = "gateway";

const char* deviceNamePath = "/device_name.txt";
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

String ssid;
String password;
String ip;
String gateway;

IPAddress localIP; // (192, 168, 1, 200)
IPAddress localGateway; // (192, 168, 1, 1)
IPAddress subnet(255, 255, 0, 0);

unsigned long prevMillis = 0;
const long interval = 10000; // Interval to wait for wifi connection (millis)

HeadwindController* headwind;
bool initialConnection;
int speed;

void initLittleFS() {
    if (!LittleFS.begin(true, "")) {
        Serial.println("An error has occurred while mounting LittleFS");
    } else {
        Serial.println("LittleFS mounted successfully");
    }
}

String readWiFiConfig(fs::FS &fs, const char* path) {
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory()) {
        Serial.println("Failed to open file for reading, does not exist or is directory");
        return String();
    }

    String content;
    while (file.available()) {
        content = file.readStringUntil('\n');
        break;
    }
    return content;
}

void writeWiFiConfig(fs::FS &fs, const char* path, const char* message) {
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    if (file.print(message))
        Serial.println("File written");
    else
        Serial.println("Write failed");
}

bool connectToWiFi() {
    if (ssid == "" || ip == "") {
        Serial.println("Undefined ssid or IP address");
        return false;
    }
    const char* ssid_c = ssid.c_str();
    const char* pass_c = password.c_str();
    const char* ip_c = ip.c_str();
    const char* gateway_c = gateway.c_str();

    WiFi.mode(WIFI_STA);
    localIP.fromString(ip_c);
    localGateway.fromString(gateway_c);

    WiFi.begin(ssid_c, pass_c);

    unsigned long currentMillis = millis();
    prevMillis = currentMillis;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);

        if (initialConnection)
            Serial.println("Connecting to WiFi...");
        else
            Serial.println("Reconnecting to WiFi...");

        currentMillis = millis();
        if (currentMillis - prevMillis >= interval) {
            Serial.println("Failed to connect to WiFi");
            return false;
        }
    }
    Serial.println("Connected to WiFi");
    Serial.print("IP Address: "); 
    Serial.println(WiFi.localIP());
    return true;
}

void setup() {
    Serial.begin(115200);

    // Connect to bluetooth and wifi
    BLEDevice::init("ESP32-BLE-CLIENT");
    Serial.println("BLE Device initialized");

    headwind = new HeadwindController();
    Serial.println("Headwind defined");

    bool connectionSuccess = headwind->connectToHeadwind();
    if (!connectionSuccess)
        Serial.println("Connection failed");

    // Setup SPIFFS for reading WiFi credentials
    /*if (!SPIFFS.begin(true)) {
        Serial.println("Faled to mount file system");
        return;
    }
    Serial.println("SPIFFS file system mounted successfully");
    WiFi_Config config = loadConfigData();
    if (config.ssid == nullptr || config.password == nullptr) {
        Serial.println("WiFi config file not found");
        return;
    }*/

    initLittleFS();
#if (FORMAT_FS)
    LittleFS.format(); // Formats the filesystem for testing purposes
#endif
    ssid = readWiFiConfig(LittleFS, ssidPath);
    password = readWiFiConfig(LittleFS, passPath);
    ip = readWiFiConfig(LittleFS, ipPath);
    gateway = readWiFiConfig(LittleFS, gatewayPath);

    initialConnection = true;
    if (!connectToWiFi()) {
        WiFi.softAP("ESP-WIFI-MANAGER", NULL);

        IPAddress IP = WiFi.softAPIP();
        Serial.print("Access Point IP address, set ssid and password of your network here: ");
        Serial.println(IP); 
        
        /*if (LittleFS.exists("/wifimanager.html"))
            Serial.println("Wifi manager exists!");
        else
            Serial.println("Wifi manager does not exist");*/

        // Web Server Root URL
        /*server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(LittleFS, "/wifimanager.html", "text/html");
        });*/

        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(200, "text/html", R"rawliteral(
                <!DOCTYPE html>
                <html>
                <head>
                <title>ESP Wi-Fi Manager</title>
                <meta name="viewport" content="width=device-width, initial-scale=1">
                </head>
                <body>
                <div class="topnav">
                    <h1>ESP Wi-Fi Manager</h1>
                </div>
                <div class="content">
                    <div class="card-grid">
                    <div class="card">
                        <form action="/" method="POST">
                        <p>
                            <label for="device_name">Device Name</label>
                            <input type="text" id="device_name" name="device_name" value="Headwind"><br>
                            <label for="ssid">SSID</label>
                            <input type="text" id="ssid" name="ssid"><br>
                            <label for="pass">Password</label>
                            <input type="text" id="pass" name="pass"><br>
                            <label for="ip">IP Address</label>
                            <input type="text" id="ip" name="ip" value="192.168.1.200"><br>
                            <label for="gateway">Gateway Address</label>
                            <input type="text" id="gateway" name="gateway" value="192.168.1.1"><br>
                            <input type="submit" value="Submit">
                        </p>
                        </form>
                    </div>
                    </div>
                </div>
                </body>
                </html>
            )rawliteral");
        });
        
        server.serveStatic("/", LittleFS, "/");
        
        server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
            int params = request->params();
            for(int i=0;i<params;i++){
                const AsyncWebParameter* p = request->getParam(i);
                if(p->isPost()) {
                    if (p->name() == NAME_PARAM) {
                        deviceName = p->value();
                        Serial.print("Device name set to: ");
                        Serial.println(deviceName);
                        writeWiFiConfig(LittleFS, deviceNamePath, deviceName.c_str());
                    }
                    // HTTP POST ssid value
                    if (p->name() == SSID_PARAM) {
                        ssid = p->value();
                        Serial.print("SSID set to: ");
                        Serial.println(ssid);
                        writeWiFiConfig(LittleFS, ssidPath, ssid.c_str());
                    }
                    // HTTP POST pass value
                    if (p->name() == PASS_PARAM) {
                        password = p->value();
                        Serial.print("Password set to: ");
                        Serial.println(password);
                        writeWiFiConfig(LittleFS, passPath, password.c_str());
                    }
                    // HTTP POST ip value
                    if (p->name() == IP_PARAM) {
                        ip = p->value();
                        Serial.print("IP Address set to: ");
                        Serial.println(ip);
                        writeWiFiConfig(LittleFS, ipPath, ip.c_str());
                    }
                    // HTTP POST gateway value
                    if (p->name() == GATEWAY_PARAM) {
                        gateway = p->value();
                        Serial.print("Gateway set to: ");
                        Serial.println(gateway);
                        writeWiFiConfig(LittleFS, gatewayPath, gateway.c_str());
                    }
                    //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
                }
            }
#if (FORMAT_FS)
            request->send(200, "text/plain", "WiFi config written.");
#else
            request->send(200, "text/plain", "WiFi config written. ESP will restart, connect to your router and go to IP address: " + ip);
            delay(3000);
            ESP.restart();
#endif
        });
        server.begin();
    }

    // Set up ArduinoOTA for over the air updates
    ArduinoOTA.setHostname(DEVICE_NAME);
    ArduinoOTA.begin();
    //connection.begin(connectionPort);

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) type = "sketch";
        else type = "filesystem";
        Serial.print("Start updating ");
        Serial.println(type);
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        switch (error) {
            case OTA_AUTH_ERROR:
                Serial.println("Authentication Failed");
                break;
            case OTA_BEGIN_ERROR:
                Serial.println("Begin Failed");
                break;
            case OTA_CONNECT_ERROR:
                Serial.println("Connection Failed");
                break;
            case OTA_RECEIVE_ERROR:
                Serial.println("Receive Failed");
                break;
            case OTA_END_ERROR:
                Serial.println("End Failed");
        }
    });

    // Setup async webserver to set fan speed remotely
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", R"rawliteral(
          <!DOCTYPE HTML><html>
          <head>
            <title>ESP32 Fan Control</title>
            <script>
              function setSpeed() {
                var speed = document.getElementById("speed").value;
                var xhr = new XMLHttpRequest();
                xhr.open("GET", "/set_speed?value=" + speed, true);
                xhr.send();
              }
            </script>
          </head>
          <body>
            <h1>Fan Speed Control</h1>
            <label for="speed">Set speed (0-100):</label>
            <input type="number" id="speed" name="speed" min="0" max="100">
            <button onclick="setSpeed()">Set Speed</button>
          </body>
          </html>
        )rawliteral");
    });

    server.on("/set_speed", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("value")) {
            speed = request->getParam("value")->value().toInt();
            if (headwind->isConnected()) {
                bool result = headwind->setPower(speed);
                Serial.printf("Set power result for headwind: %s\n", result ? "Success" : "Failure");
            }
            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Bad Request: Missing value parameter");
        }
    });

    server.begin();
}

void loop() {
    ArduinoOTA.handle();

    if (!headwind->isConnected()) {
        Serial.println("Reconnecting to Headwind...");
        headwind->reconnect();
        delay(1000);
    }
#if (!FORMAT_FS)
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconnecting to WiFi...");
        initialConnection = false;
        connectToWiFi();
        delay(1000);
    }
#endif
}