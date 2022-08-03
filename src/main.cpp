/*    _              __  _   
     | |            / _|| |  
 ___ | |__    __ _ | |_ | |_ 
/ __|| '_ \  / _` ||  _|| __|
\__ \| | | || (_| || |  | |_ 
|___/|_| |_| \__,_||_|   \__|
                          
 *   DESENVOLVIDO POR VITOR SHAFT EM 22-JUL-2022
 * https://github.com/vitorshaft/cyberCortina.git
 * https://instagram.com/shaftrobotica
 * shaftrobotica@gmail.com
 */

 /* 1. Baixar e instalar libs WiFiManager https://github.com/tzapu/WiFiManager
    e Arduino JSON https://github.com/bblanchon/ArduinoJson
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>

#include <ESPAsyncWiFiManager.h>

//#include <WiFiManager.h>

//Define os GPIO do motor
#define IN1 14  //PINO D5
#define IN2 12  //PINO D6

// Replace with your network credentials
const char* ssid = "DUARTE DOMINGUES";
const char* password = "recife2014";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
DNSServer dns;
// Create a WebSocket object

AsyncWebSocket ws("/ws");
// Set LED GPIO
const int ledPin1 = 2;

String message = "";
String sliderValue1 = "0";

int dutyCycle1;

int posicao = 0;

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40];
char mqtt_port[6] = "8080";
char blynk_token[34] = "YOUR_BLYNK_TOKEN";

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

//Json Variable to Hold Slider Values
JSONVar sliderValues;

//Get Slider Values
String getSliderValues(){
  sliderValues["sliderValue1"] = String(sliderValue1);

  String jsonString = JSON.stringify(sliderValues);
  return jsonString;
}

// Initialize LittleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
   Serial.println("LittleFS mounted successfully");
  }
}

// Initialize WiFi

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}


void notifyClients(String sliderValues) {
  ws.textAll(sliderValues);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char*)data;
    if (message.indexOf("1s") >= 0) {
      sliderValue1 = message.substring(2);
      dutyCycle1 = map(sliderValue1.toInt(), 0, 100, 0, 1023);
      Serial.println(dutyCycle1);
      Serial.print(getSliderValues());
      notifyClients(getSliderValues());
    }
    if (strcmp((char*)data, "getValues") == 0) {
      notifyClients(getSliderValues());
    }
  }
}
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

//Funcoes de controle do motor
void positivo(int passos){
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    delay(passos*1000);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
}

void negativo(int passos){
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  delay(passos*1000);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin1, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  //pinMode(ledPin2, OUTPUT);
  //pinMode(ledPin3, OUTPUT);
  initFS();
  //initWiFi();

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  AsyncWiFiManager wifiManager(&server,&dns);

  wifiManager.autoConnect("AutoConnectAP");
  Serial.println("connected...yeey :)");

  /*
  WiFiManager wifiManager;

  wifiManager.autoConnect("CortinaBot");
  Serial.println("Conectado.");
  */
  initWebSocket();
  
  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });
  
  server.serveStatic("/", LittleFS, "/");

  // Start server
  server.begin();
}

void loop() {
  
  int diferenca = dutyCycle1-posicao;
  if(diferenca != 0){
    Serial.print("dutyCycle: ");
    Serial.println(dutyCycle1);
    Serial.print("diferenca: ");
    Serial.println(diferenca);
    if(diferenca > 0){
      positivo(diferenca/100);
      posicao = dutyCycle1;
    }
    else if(diferenca < 0){
      negativo(-diferenca/100);
      posicao = dutyCycle1;
    }
  }

  ws.cleanupClients();
}