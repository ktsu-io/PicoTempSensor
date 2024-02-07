#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_TMP117.h>
#include <Adafruit_Sensor.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>

JsonDocument config;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3D
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_TMP117  tmp117;
float cachedTemp = 0;

WiFiMulti wifiMulti;
WiFiUDP udp;

const char* zone;
const char* wifiName;
const char* wifiPassword;
const char* serverHostname;
int serverPort;

void setup() {
  Serial.begin();
  //while (!Serial) delay(10);

  if (!tmp117.begin(0x48)) {
    Serial.println("Failed to find sensor");
    while (1) { delay(10); }
  }

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Failed to find display"));
    while (1) { delay(10); }
  }

  display.clearDisplay();
  display.display();
  display.setCursor(0,0);
  display.setTextColor(SSD1306_WHITE);

  LittleFS.begin();

  char configFileName[32] = "config.json";
  auto f = LittleFS.open(configFileName, "r");
  if (!f) {
    Serial.println("Failed to find config");
    while (1) { delay(10); }
  }

  std::string jsonString;
  char buffer[16];
  while(f.available())
  {
    int bytesRead = f.readBytes(buffer, 16);
    jsonString.append(buffer, bytesRead);
  }

  auto jsonError = deserializeJson(config, jsonString.c_str());
  if (jsonError) {
    Serial.println("Failed to load config");
    Serial.println(jsonError.f_str());
    while (1) { delay(10); }
  }

  zone = config["zone"];
  wifiName = config["wifiName"];
  wifiPassword = config["wifiPassword"];
  serverHostname = config["serverHostname"];
  serverPort = config["serverPort"];

  wifiMulti.addAP(wifiName, wifiPassword);
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Failed to connect to wifi, rebooting");
    delay(10000);
    rp2040.reboot();
  }
}

void loop() {
  sensors_event_t temp; // create an empty event to be filled
  tmp117.getEvent(&temp); //fill the empty event object with the current measurements

  auto newTemp = roundf(temp.temperature*10.f)/10.f;
  if(newTemp != cachedTemp)
  {
    cachedTemp = newTemp;

    char buffer[32];
    udp.beginPacket(serverHostname, serverPort);
    sprintf(buffer, "%.2f", temp.temperature);
    std::string msg = std::string(zone) + " " + std::string(buffer);
    udp.write(msg.c_str(), msg.length());
    udp.endPacket();

    display.clearDisplay();
    display.setCursor(2, 2);
    display.setTextSize(3);
    display.print(temp.temperature, 2);
    display.print(" C");
    display.setTextSize(2);
    display.setCursor(2, 28);
    display.print(zone);
    display.setTextSize(1);
    display.setCursor(2, 48);
    if(WiFi.isConnected())
    {
      display.print(WiFi.localIP());
    }
    else
    {
      display.print("Disconnected");
    }
    display.display();
  }
}
