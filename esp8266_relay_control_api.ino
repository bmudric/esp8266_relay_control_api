#include "config.h"
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>

#ifdef DEBUG_ESP_PORT
#define debugf(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#else
#define debugf(...)
#endif

// defined in config.h
const char* ssid = YOUR_SSID;
const char* password = YOUR_PASSWORD;

const char* deviceType = "relay";
const char* serviceType = "relayAPI";
const char* serviceProtocol = "tcp";
const int relayPin = D1; // select ESP8266 pin you want to use to control the relay
const int ledPin = D0; // onboard LED
const int port = 80;

ESP8266WebServer server(port);

void handleOn();
void handleOff();
void handleStatus();
void handleNotFound();

const bool printDebug = false;

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(relayPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  digitalWrite(ledPin, HIGH); // turn onboard LED off to signify setup process started

  String hostName = "relay_";
  hostName += ESP.getChipId();
  WiFi.hostname(hostName);
  debugf("Hostname: %s\n", hostName.c_str());

  debugf("\nConnecting to: %s\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    debugf("*");
  }
  debugf("Connected to: %s\n", WiFi.SSID().c_str());
  debugf("IP address: %s\n", WiFi.localIP().toString().c_str());

  if (MDNS.begin(hostName)) {
    MDNS.addService(serviceType, serviceProtocol, port);
    debugf("mDNS responder started\n");
  } else {
    debugf("Error setting up MDNS responder!\n");
  }

  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/status", HTTP_GET, handleStatus);
  server.onNotFound(handleNotFound);

  server.begin();
  debugf("Server started\n");

  digitalWrite(ledPin, LOW); // turn onboard LED on to signify setup process completed
}

void loop() {
  MDNS.update();
  server.handleClient();
}

void handleOn() {
  debugf("Client requesting relay ON\n");
  digitalWrite(relayPin, HIGH);
  digitalWrite(ledPin, HIGH);
  sendStatusResponse();
}

void handleOff() {
  debugf("Client requesting relay OFF\n");
  digitalWrite(relayPin, LOW);
  digitalWrite(ledPin, LOW);
  sendStatusResponse();
}

void handleStatus() {
  debugf("Client requesting relay STATUS\n");
  sendStatusResponse();
}

bool isRelayOn() {
  int relayPinState = digitalRead(relayPin);
  debugf("State of the relayPin: %d\n", relayPinState);
  return relayPinState == HIGH;
}

void sendStatusResponse() {
  DynamicJsonDocument doc(JSON_OBJECT_SIZE(1));
  doc["relayIsOn"] = isRelayOn();
  String response;
  serializeJson(doc, response);

  server.send(200, "application/json", response);
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not found");
}
