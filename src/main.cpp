#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <LittleFS.h>

#include "HeadwindController.h"

#define LED_PIN 8
#define DEVICE_NAME "Headwind_1"
#define CONFIG_FILENAME "/wifi_cred.dat"
#define HTTP_PORT 80

//const String ssid = "js25";
//const String password  = "xhj6yyhn3q3vp8u";

WiFiUDP connection;
IPAddress coverIP((uint32_t) 0);
const int connectionPort = 31319;
const int pluginPort = 31322;

// WiFi Manager Setup
AsyncWebServer server(HTTP_PORT);

const char* SSID_PARAM = "ssid";
const char* PASS_PARAM = "pass";
const char* IP_PARAM = "ip";
const char* GATEWAY_PARAM = "gateway";

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

bool initialConnection;

HeadwindController* headwind;
//AsyncWebServer server(HTTP_PORT);

//IPAddress localIP = IPAddress(10, 192, 98, 175);
//IPAddress localIP = IPAddress(0, 0, 0, 0);
//IPAddress gateway = IPAddress(192, 168, 2, 1);
//IPAddress subnet = IPAddress(255, 255, 255, 0);

String ledState;
int speed;

void initLittleFS() {
    if (!LittleFS.begin(true, "")) {
        Serial.println("An error has occurred while mounting LittleFS");
    } else {
        Serial.println("LittleFS mounted successfully");
    }
}

// TODO: Rename to readWiFiConfig
String readFile(fs::FS &fs, const char* path) {
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

    if (file.print(message)) {
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
}

// TODO: Rename to connectToWiFi
bool initWiFi() {
    if (ssid == "" || ip == "") {
        Serial.println("Undefined ssid or IP address");
        return false;
    }
    Serial.printf("ssid: %s\n", ssid);
    Serial.printf("password: %s\n", password);
    Serial.printf("ip: %s\n", ip);
    Serial.printf("gateway: %s\n", gateway);

    WiFi.mode(WIFI_STA);
    localIP.fromString(ip.c_str());
    localGateway.fromString(gateway.c_str());

    if (!WiFi.config(localIP, localGateway, subnet)) {
        Serial.println("STA failed to configure");
        return false;
    }
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long currentMillis = millis();
    prevMillis = currentMillis;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);

        /*if (initialConnection)
            Serial.println("Connecting to WiFi...");
        else
            Serial.println("Reconnecting to WiFi...");*/

        currentMillis = millis();
        if (currentMillis - prevMillis <= interval) {
            Serial.println("Failed to connect to WiFi");
            return false;
        }
    }
    Serial.println("Connected to WiFi");
    Serial.printf("IP Address: %s\n", WiFi.localIP());
    return true;
}

/*String processor(const String& var) {
    if (var == "STATE") {
        if (digitalRead(LED_PIN)) {
            ledState = "ON";
        } else {
            ledState = "OFF";
        }
        return ledState;
    }
    return String();
}

void sendDeviceInfo(IPAddress destinationAddress) {
    char buffer[100];
    connection.beginPacket(destinationAddress, connectionPort);
    strcpy(buffer, "devInfo " DEVICE_NAME);
    Serial.println(buffer);
    connection.write((const uint8_t *)&buffer, strlen(buffer) + 1);
    connection.endPacket();
}

typedef struct {
    String ssid;
    String password;
} WiFi_Config;

WiFi_Config loadConfigData() {
    WiFi_Config config;

    File file = SPIFFS.open(CONFIG_FILENAME, "r");
    if (!file) {
        Serial.println("Failed to open config file");
        return config;
    }
    config.ssid = file.readStringUntil('\n');
    config.password = file.readStringUntil('\n');

    config.ssid.trim();
    config.password.trim();

    file.close();
    return config;
}

void connectToWiFi(String ssid, String password, bool initialConnection) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if (initialConnection)
            Serial.println("Connecting to WiFi...");
        else
            Serial.println("Reconnecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}*/

void setup() {
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

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
    //LittleFS.format(); // Needed for testing

    ssid = readFile(LittleFS, ssidPath);
    password = readFile(LittleFS, passPath);
    ip = readFile(LittleFS, ipPath);
    gateway = readFile(LittleFS, gatewayPath);
    /*Serial.printf("ssid: %s\n", ssid); // TODO: Remove these
    Serial.printf("password: %s\n", password);
    Serial.printf("ip: %s\n", ip);
    Serial.printf("gateway: %s\n", gateway);*/

    // Connect to WiFi using WiFi credentials
    initialConnection = true;
    //connectToWiFi(config.ssid, config.password, initialConnection);

    if (initWiFi()) {
        String index = R"rawliteral(
        <!DOCTYPE html>
        <html>
        <head>
            <title>ESP WEB SERVER</title>
            <meta name="viewport" content="width=device-width, initial-scale=1">
            <link rel="stylesheet" type="text/css" href="style.css">
            <link rel="icon" type="image/png" href="favicon.png">
            <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
        </head>
        <body>
            <div class="topnav">
            <h1>ESP WEB SERVER</h1>
            </div>
            <div class="content">
            <div class="card-grid">
                <div class="card">
                <p class="card-title"><i class="fas fa-lightbulb"></i> GPIO 2</p>
                <p>
                    <a href="on"><button class="button-on">ON</button></a>
                    <a href="off"><button class="button-off">OFF</button></a>
                </p>
                <p class="state">State: %STATE%</p>
                </div>
            </div>
            </div>
        </body>
        </html>
        )rawliteral";

        // Route for root / web page
        /*server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(LittleFS, "/index.html", "text/html", false, processor);
        });
        server.serveStatic("/", LittleFS, "/");
        
        // Route to set GPIO state to HIGH
        server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
            digitalWrite(LED_PIN, HIGH);
            request->send(LittleFS, "/index.html", "text/html", false, processor);
        });

        // Route to set GPIO state to LOW
        server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
            digitalWrite(LED_PIN, LOW);
            request->send(LittleFS, "/index.html", "text/html", false, processor);
        });*/

        server.on("/", HTTP_GET, [index](AsyncWebServerRequest *request) {
            request->send(200, "text/html", index);
        });
        server.serveStatic("/", LittleFS, "/");
        
        // Route to set GPIO state to HIGH
        server.on("/on", HTTP_GET, [index](AsyncWebServerRequest *request) {
            digitalWrite(LED_PIN, HIGH);
            request->send(200, "text/html", index);
        });

        // Route to set GPIO state to LOW
        server.on("/off", HTTP_GET, [index](AsyncWebServerRequest *request) {
            digitalWrite(LED_PIN, LOW);
            request->send(200, "text/html", index);
        });

        server.begin();
    } else {
        // Connect to Wi-Fi network with SSID and password
        Serial.println("Setting AP (Access Point)");
        // NULL sets an open Access Point
        WiFi.softAP("ESP-WIFI-MANAGER", NULL);

        IPAddress IP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(IP); 
        
        /*if (LittleFS.exists("/wifimanager.html"))
            Serial.println("Wifi manager exists!");
        else
            Serial.println("Wifi manager does not exist");*/

        // Web Server Root URL
        /*server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(LittleFS, "/wifimanager.html", "text/html");
        });*/

        // Web Server Root URL
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(200, "text/html", R"rawliteral(
                <!DOCTYPE html>
                <html>
                <head>
                <title>ESP Wi-Fi Manager</title>
                <meta name="viewport" content="width=device-width, initial-scale=1">
                <link rel="icon" href="data:,">
                <link rel="stylesheet" type="text/css" href="style.css">
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
                if(p->isPost()){
                // HTTP POST ssid value
                if (p->name() == SSID_PARAM) {
                    ssid = p->value().c_str();
                    Serial.print("SSID set to: ");
                    Serial.println(ssid);
                    // Write file to save value
                    writeWiFiConfig(LittleFS, ssidPath, ssid.c_str());
                }
                // HTTP POST pass value
                if (p->name() == PASS_PARAM) {
                    password = p->value().c_str();
                    Serial.print("Password set to: ");
                    Serial.println(password);
                    // Write file to save value
                    writeWiFiConfig(LittleFS, passPath, password.c_str());
                }
                // HTTP POST ip value
                if (p->name() == IP_PARAM) {
                    ip = p->value().c_str();
                    Serial.print("IP Address set to: ");
                    Serial.println(ip);
                    // Write file to save value
                    writeWiFiConfig(LittleFS, ipPath, ip.c_str());
                }
                // HTTP POST gateway value
                if (p->name() == GATEWAY_PARAM) {
                    gateway = p->value().c_str();
                    Serial.print("Gateway set to: ");
                    Serial.println(gateway);
                    // Write file to save value
                    writeWiFiConfig(LittleFS, gatewayPath, gateway.c_str());
                }
                //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
                }
            }
            request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
            delay(3000);
            ESP.restart();
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

    /*if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconnecting to WiFi...");
        WiFi_Config config = loadConfigData();
        initialConnection = false;
        connectToWiFi(config.ssid, config.password, initialConnection);
        delay(1000);
    }*/
}