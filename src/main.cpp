#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#define LED_R 17
#define LED_G 16
#define LED_B 4

#define WIFI_SSID "WIFI_SSID..."
#define WIFI_PASS "WIFI_PASS..."

void webSocketEvent(byte num, WStype_t type, uint8_t *payload, size_t length);
void send_update(String type, int value);
String hex_to_String(int hex);
int String_to_hex(char *hex_str);
void writetoRGB();

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
IPAddress ServerIP;

StaticJsonDocument<200> doc_rx;
StaticJsonDocument<200> doc_tx;

int color = 0x000000;
int RGB[] = {0, 0, 0};
int intensity = 50;

// for LED analog write SETUP
const int freq = 5000;
const int led_channels[] = {1, 2, 3};
const int resolution = 8;

void setup()
{
  // LED analog Write Setup
  ledcSetup(led_channels[0], freq, resolution);
  ledcSetup(led_channels[1], freq, resolution);
  ledcSetup(led_channels[2], freq, resolution);

  ledcAttachPin(LED_R, led_channels[0]);
  ledcAttachPin(LED_G, led_channels[1]);
  ledcAttachPin(LED_B, led_channels[2]);

  // LED pinMode setup
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);

  Serial.print("starting Server");

  LittleFS.begin();

  // Setup INTRANET Acces
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
  ServerIP = WiFi.localIP();
  Serial.print("\nIP-Adress: ");
  Serial.println(ServerIP);

  // URL/
  File webpage = LittleFS.open("/index.html", "r");
  String webpage_str = webpage.readStringUntil(EOF);
  webpage.close();

  server.on("/", [webpage_str]()
            { server.send(200, "text/html", webpage_str); });

  // URL/script.js
  File script = LittleFS.open("/script.js", "r");
  String script_str = script.readStringUntil(EOF);
  script.close();

  server.on("/script.js", [script_str]()
            { server.send(200, "text/javascript", script_str); });

  // URL/fade
  File webpage_fade = LittleFS.open("/fade/fade.html", "r");
  String webpage_fade_str = webpage_fade.readStringUntil(EOF);
  webpage_fade.close();

  server.on("/fade", [webpage_fade_str]()
            { server.send(200, "text/html", webpage_fade_str); });

  // URL/fade/script_fade.js
  File script_fade = LittleFS.open("/fade/script_fade.js", "r");
  String script_fade_str = script_fade.readStringUntil(EOF);
  script_fade.close();

  server.on("/fade/script_fade.js", [script_fade_str]
            { server.send(200, "text/javascript", script_fade_str); });

  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

bool connected = false;

void loop()
{
  webSocket.loop();
  // handle Client Request
  server.handleClient();
  // keep status up to date
  if (WiFi.status() == WL_CONNECTED && !connected)
  {
    // Serial.println("connected");
    digitalWrite(LED_BUILTIN, HIGH);
    connected = true;
  }

  else if (WiFi.status() != WL_CONNECTED)
  {
    // Serial.print(".");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    connected = false;
    delay(1000);
  }
}

void webSocketEvent(byte num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.println("Client Disconnected");
    break;
  case WStype_CONNECTED:
    Serial.println("Client Connected");
    send_update("LED_COLOR", color);
    send_update("LED_INTENSITY", intensity);
    break;
  case WStype_TEXT:
    DeserializationError error = deserializeJson(doc_rx, payload);
    if (error)
    {
      Serial.println("deserialization failed.");
      return;
    }
    const char *l_type = doc_rx["type"];

    if (String(l_type) == "LED_INTENSITY")
    {
      const int value = doc_rx["value"];
      intensity = int(value);
      send_update("LED_INTENSITY", intensity);
      if (color != 0) // every LED off -> skip
      {
        writetoRGB();
      }
    }
    else if (String(l_type) == "LED_COLOR")
    {
      String value = doc_rx["value"];
      char *value_c = new char[value.length() + 1];
      strcpy(value_c, value.c_str());

      color = String_to_hex(value_c);

      delete[] value_c;

      send_update("LED_COLOR", color);

      // set new RGB values
      RGB[0] = ((color >> 16) & 0xFF);
      RGB[1] = ((color >> 8) & 0xFF);
      RGB[2] = ((color)&0xFF);

      Serial.printf("\nColor:\n\tr: %i\n\tg: %i\n\tb: %i\n", RGB[0], RGB[1], RGB[2]);

      writetoRGB();
    }
    else
    {
      Serial.println("type not found!");
    }

    break;
  }
}

// sends Msg with included Values to Clients connected to WebSocket  (Types: LED_COLOR, LED_INTENSITY)
void send_update(String type, int value)
{
  String jsonString = "";
  JsonObject object = doc_tx.to<JsonObject>();
  // get cur status
  object["type"] = type;
  object["value"] = value;
  // for Color Hex String needed in Format: #RRGGBB
  if (type == "LED_COLOR")
  {
    object["value"] = hex_to_String(value);
  }
  serializeJson(doc_tx, jsonString);

  Serial.println(jsonString);
  // send JsonString to Websocket
  webSocket.broadcastTXT(jsonString);
}

String hex_to_String(int hex)
{
  // Serial.printf("hex_to_String:\n\tint_hex: %x\n\t", hex);
  char hex_str[] = "#000000";
  sprintf(hex_str, "#%.6x", hex);
  // Serial.printf("str_hex: %s\n", hex_str);
  return String(hex_str);
}

int String_to_hex(char *hex_str)
{
  // Serial.printf("String_to_hex:\n\tstr_hex: %s\n\t", hex_str);
  if (hex_str[0] == '#')
    hex_str++;
  int hex = (int)strtol(hex_str, NULL, 16);
  // Serial.printf("int_hex: %x\n", hex);
  return hex;
}

// writes RGB[] values combined with intensity(+PWM) to Output
void writetoRGB()
{
  for (int i = 0; i < 3; i++)
  {
    if (RGB[i] != 0) // single LED off -> skip
    {
      // Serial.printf("old Color: %i\n", RGB[i]);
      int dimmed_color = (int)(RGB[i] * (intensity / 100.0f));
      // Serial.printf("nes Color: %i", dimmed_color);
      ledcWrite(led_channels[i], dimmed_color);
    }
    else
    {
      ledcWrite(led_channels[i], 0);
    }
  }
}