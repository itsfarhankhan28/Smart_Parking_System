#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <NewPing.h>
#include "FS.h" // Include the SPIFFS library
#include <string> // Add this line to include the string library
#include <ThingSpeak.h>

ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

const unsigned long myChannelNumber = 2318487;
const char* myWriteAPIKey = "50DXU7SSVPQP34BG";

WiFiClient  client;

const int triggerPin = D1;
const int echoPin = D2;
const int triggerPin2 = D3;
const int echoPin2 = D4;  

NewPing sonar1(triggerPin, echoPin, 50);
NewPing sonar2(triggerPin2, echoPin2, 50);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        webSocket.sendTXT(num, "sensor1:0");
        webSocket.sendTXT(num, "sensor2:0");
      }
      break;

    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      break;
      
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\n", num, length);
      hexdump(payload, length);
      break;
  }
}

void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void handleStyle() {
  File file = SPIFFS.open("/style.css", "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "text/css");
  file.close();
}

void handleScript() {
  File file = SPIFFS.open("/script.js", "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "application/javascript");
  file.close();
}

void setup() {
  Serial.begin(115200);
  ThingSpeak.begin(client);
  delay(1000);

  WiFiMulti.addAP("Nish", "123456789");

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }

  Serial.println(WiFi.localIP());

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/style.css", HTTP_GET, handleStyle);
  server.on("/script.js", HTTP_GET, handleScript);

  server.begin();
}

void loop() {
  webSocket.loop();
  server.handleClient();

  // Distance measurement and WebSocket broadcasting
  int distance1 = sonar1.ping_cm();
  int distance2 = sonar2.ping_cm();

  Serial.println("Sensor 1" + distance1);
  Serial.println("Sensor 2" + distance2);

  int parkingStatus1 = (distance1 > 0 && distance1 < 10) ? 1 : 0;
  int parkingStatus2 = (distance2 > 0 && distance2 < 10) ? 1 : 0;

  // set the field with parking availability
  ThingSpeak.setField(1, parkingStatus1 ? 1 : 0);
  ThingSpeak.setField(1, parkingStatus2 ? 1 : 0);

  // write to the ThingSpeak channel
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (x == 200) {
    Serial.println("Channel update successful.");
  } else {
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }

  char broadcastData[50]; // Adjust the size as needed

  // Create a formatted string with sensor identifiers
  snprintf(broadcastData, sizeof(broadcastData), "sensor1:%d sensor2:%d", parkingStatus1, parkingStatus2);

  // Send the data to all connected clients
  webSocket.broadcastTXT(broadcastData);

delay(1000);
}    