// DHT22 Sensor with 7 segments display - ESP8266
// features : WiFi, DHT22 Sensor, Seven segments display

const char version[6] = "1.5.2";

#include <ESP8266WiFi.h>
#include <DHT.h>
#include "LedControl.h"
#include <ArduinoJson.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <PubSubClient.h>

const char *ssid     = "BZ_IOT";
const char *password = "Password";

const byte DHTPIN = D4;

IPAddress local_IP(192, 168, 1, 201);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 1, 1);
IPAddress secondaryDNS(192, 168, 1, 13);

#define MQTT_HOST IPAddress(192, 168, 1, 3)

unsigned long updateSegments;
unsigned long publishMQTT;
unsigned long mqttESPinfo;
unsigned long reconnTime;
float temperature;
float humidity;
int rainValue;
byte temp1 = 0;
byte temp2 = 0;
byte temp3 = 0;
byte hum1 = 0;
byte hum2 = 0;
byte hum3 = 0;

WiFiClient espClient;
PubSubClient client(espClient);
AsyncWebServer server(80);

DHT dht(DHTPIN, DHT22);
LedControl lc = LedControl(D0, D1, D2, 1);

void reconnect() {
    if (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("ESP8266Client")) {
            Serial.println("connected");
            client.subscribe("esp/weather/get");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
        }
    }
}

void connectToWifi() {
	Serial.println("Connecting to Wi-Fi...");
	if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
		Serial.println("STA Failed to configure");
	}
	WiFi.begin(ssid, password);
	WiFi.setAutoReconnect(true);
	WiFi.persistent(true);
	Serial.print("Connected to ");
	Serial.println(WiFi.SSID());
}

void setup() {
    Serial.begin(115200);
    Serial.println("Booting");
    lc.shutdown(0, false);
    lc.setIntensity(0, 1);
    lc.clearDisplay(0);
    connectToWifi();
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP8266. ESP-Weather\nVersion: " + String(version));
	});
	
	AsyncElegantOTA.begin(&server);         // Start ElegantOTA
    server.begin();                         // ElegantOTA
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    client.setServer(MQTT_HOST, 1883);      // MQTT
    dht.begin();
    delay(500);
}

void loop() {
    client.loop();

    if ((millis() - updateSegments) > 10000) {
        temperature = dht.readTemperature();
        humidity = dht.readHumidity();
        rainValue = 1024 - analogRead(A0);
        temp1 = int(temperature) / 10;
        temp2 = int(temperature) % 10;
        temp3 = int(temperature * 10) % 10;
        hum1 = int(humidity) / 10;
        hum2 = int(humidity) % 10;
        hum3 = int(humidity * 10) % 10;
        updateSegments = millis();
    }

    if (!client.connected()) {
		if (millis() - reconnTime > 1000) {
			reconnect();
			reconnTime = millis();
		}
	} else {
        if (millis() - publishMQTT > 60000) {
            DynamicJsonDocument docInfo(64);
            String MQTT_STR;
            docInfo["temp"] = String(temperature, 1);
            docInfo["hum"] = String(humidity, 1);
            docInfo["rain"] = String(rainValue);
            serializeJson(docInfo, MQTT_STR);
            client.publish("esp/weather/chart", MQTT_STR.c_str());
            publishMQTT = millis();
        }
        if (millis() - mqttESPinfo > 1000) {
            DynamicJsonDocument docInfo(128);
            String MQTT_STR;
            docInfo["temp"] = String(temperature, 1);
            docInfo["hum"] = String(humidity, 1);
            docInfo["rain"] = String(rainValue);
            docInfo["rssi"] = String(WiFi.RSSI());
            docInfo["uptime"] = String(millis()/1000);
            serializeJson(docInfo, MQTT_STR);
            client.publish("esp/weather/info", MQTT_STR.c_str());
            mqttESPinfo = millis();
        }
    }
    if (isnan(temperature) || isnan(humidity) || (humidity == 7 && temperature == 7)) {
        lc.setChar(0, 7, 'E', false);
        lc.setChar(0, 6, 'r', false);
        lc.setChar(0, 5, 'r', false);
        lc.setChar(0, 3, 'E', false);
        lc.setChar(0, 2, 'r', false);
        lc.setChar(0, 1, 'r', false);
    } else {
        lc.setChar(0, 7, temp1, false);
        lc.setChar(0, 6, temp2, true);
        lc.setChar(0, 5, temp3, false);
        lc.setChar(0, 4, 'C',   false);
        lc.setChar(0, 3, hum1,  false);
        lc.setChar(0, 2, hum2,  true);
        lc.setChar(0, 1, hum3,  false);
        lc.setChar(0, 0, 'H',   false);
    }
}