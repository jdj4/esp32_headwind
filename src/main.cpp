#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <LittleFS.h>

#include "HeadwindController.h"

// Pinout definitions
#define FAN 0
#define SW1 1
#define SW2 3
#define SW3 4
#define LED0 5
#define LED3 7
#define LED5 10

// Regular constants
#define MAX_SPEED 100
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
String ssid;
String password;

const char* NAME_PARAM = "device_name";
const char* SSID_PARAM = "ssid";
const char* PASS_PARAM = "pass";

const char* deviceNamePath = "/device_name.txt";
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";

IPAddress localIP; // (192, 168, 1, 200)
IPAddress localGateway; // (192, 168, 1, 1)
IPAddress subnet(255, 255, 0, 0);

// Global variables
unsigned long prevMillisWiFi = 0;
const long interval = 10000; // Interval to wait for wifi connection (millis)

unsigned long prevMillisFan = 0;
bool fanState = false;

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
    if (ssid == "") {
        Serial.println("Undefined ssid or IP address");
        return false;
    }
    const char* ssid_c = ssid.c_str();
    const char* pass_c = password.c_str();

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid_c, pass_c);

    unsigned long currentMillis = millis();
    prevMillisWiFi = currentMillis;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);

        if (initialConnection)
            Serial.println("Connecting to WiFi...");
        else
            Serial.println("Reconnecting to WiFi...");

        currentMillis = millis();
        if (currentMillis - prevMillisWiFi >= interval) {
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

    pinMode(FAN, OUTPUT);
    pinMode(SW1, INPUT);
    pinMode(SW2, INPUT);
    pinMode(SW3, INPUT);
    pinMode(LED0, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(LED5, OUTPUT);

    initLittleFS();
#if (FORMAT_FS)
    LittleFS.format(); // Formats the filesystem for testing purposes
#endif
    deviceName = readWiFiConfig(LittleFS, deviceNamePath);
    ssid = readWiFiConfig(LittleFS, ssidPath);
    password = readWiFiConfig(LittleFS, passPath);

    initialConnection = true;
    if (!connectToWiFi()) {
        WiFi.softAP("ESP-WIFI-MANAGER", NULL);

        IPAddress ip = WiFi.softAPIP();
        Serial.print("Access Point IP address, set ssid and password of your network here: ");
        Serial.println(ip);

        // Web interface for getting WiFi credentials
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
        
        // Records device name, ssid, and password from user input
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
                    if (p->name() == SSID_PARAM) {
                        ssid = p->value();
                        Serial.print("SSID set to: ");
                        Serial.println(ssid);
                        writeWiFiConfig(LittleFS, ssidPath, ssid.c_str());
                    }
                    if (p->name() == PASS_PARAM) {
                        password = p->value();
                        Serial.print("Password set to: ");
                        Serial.println(password);
                        writeWiFiConfig(LittleFS, passPath, password.c_str());
                    }
                }
            }
#if (FORMAT_FS)
            request->send(200, "text/plain", "WiFi config written.");
#else
            request->send(200, "text/plain", "WiFi config written. ESP will restart, connect to your router and go to IP address: " + WiFi.localIP());
            delay(3000);
            ESP.restart();
#endif
        });
        server.begin();
    }

    // Set up ArduinoOTA for over the air updates
    ArduinoOTA.setHostname(deviceName.c_str());
    Serial.printf("Hostname set to: %s\n", deviceName.c_str());
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

    // Web interface to remotely set fan speed
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
            speed = (speed < 0) ? 0 : (speed > 100 ? 100 : speed); // Ensure speed is between 0-100 inclusive
            Serial.printf("Setting fan speed to: %d\n", speed);
            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Bad Request: Missing value parameter");
        }
    });

    server.begin();
}

void loop() {
    ArduinoOTA.handle();

    digitalWrite(LED0, !digitalRead(SW1));
    digitalWrite(LED3, !digitalRead(SW2));
    digitalWrite(LED5, !digitalRead(SW3));

    // Software simulation of PWM to regulate fan speed
    unsigned long currentMillis = millis();
    if (currentMillis - prevMillisFan >= MAX_SPEED) {
        prevMillisFan = currentMillis;
        fanState = false;
        digitalWrite(FAN, false);
    }

    if (currentMillis - prevMillisFan < speed) {
        if (!fanState) {
            fanState = true;
            digitalWrite(FAN, true);
        }
    } else {
        if (fanState) {
            fanState = false;
            digitalWrite(FAN, false);
        }
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