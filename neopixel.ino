#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#include <ESP8266WiFi.h>
// #include <WiFiClient.h> // Already included in ESP8266WiFi.h
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266mDNS.h>
#include <Wire.h>

// PIN constants
const int OUTPIN0 = 12; // D6   On LoLin NodeMCU V3

// LED color values, names and signal
int LED_RED, LED_GREEN, LED_BLUE;
const char LED_RED_NAME[] = "red";
const char LED_GREEN_NAME[] = "green";
const char LED_BLUE_NAME[] = "blue";
bool LED_NEW_VALUES = false;

// HTML
extern const char INDEX_HTML[];

// WiFi details
static const char ssid[] = WIFI_SSID;
static const char password[] = WIFI_PASSWORD;

MDNSResponder mdns;
ESP8266WiFiMulti WiFiMulti;
Adafruit_NeoPixel pixels(2, OUTPIN0);
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

void handleRoot()
{
    for (uint8_t i =  0; i < server.args(); i++)
    {
        int* led_color = NULL;
        if (server.argName(i) == LED_RED_NAME)
            led_color = &LED_RED;
        if (server.argName(i) == LED_GREEN_NAME)
            led_color = &LED_GREEN;
        if (server.argName(i) == LED_BLUE_NAME)
            led_color = &LED_BLUE;

        if (led_color != NULL)
        {
            auto value = server.arg(i).toInt();

            if (value < 0)
                value = 0;
            if (value > 255)
                value = 255;

            LED_NEW_VALUES = true;
            *led_color = value;
        }
    }

    server.send_P(200, "text/html", INDEX_HTML);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  Serial.println();
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  
  switch(type) {
    case WStype_DISCONNECTED: {
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    }
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      break;
    }
    case WStype_TEXT: {
      Serial.printf("[%u] get Text: %s\r\n", num, payload);
      const char* payload_char = (const char*)payload;
      char* colon = (char*)memchr(payload_char, ':', length);
      if (colon != NULL)
      {
          size_t pos = colon - payload_char;

          int* led_ptr = NULL;
          if(!strncmp(payload_char, LED_RED_NAME, pos))
              led_ptr = &LED_RED;
          else if(!strncmp(payload_char, LED_GREEN_NAME, pos))
              led_ptr = &LED_GREEN;
          else if(!strncmp(payload_char, LED_BLUE_NAME, pos))
              led_ptr = &LED_BLUE;
          
          if (led_ptr != NULL)
          {
              int value = atoi(colon + 1);
              value = max(0, min(255, value));
              *led_ptr = value;
              LED_NEW_VALUES = true;
          }
      }
      break;
    }
    case WStype_BIN: {
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      hexdump(payload, length);
      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
    }
    default: {
      Serial.printf("Invalid WStype [%d]\r\n", type);
      break;
    }
  }
}

void setup()
{
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
#endif

    pixels.begin();

    Serial.begin(115200);
    // Serial.setDebugOutput(true);

    WiFiMulti.addAP(ssid, password);

    while(WiFiMulti.run() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (mdns.begin("espWebSock", WiFi.localIP())) {
        Serial.println("MDNS responder started");
        mdns.addService("http", "tcp", 80);
        mdns.addService("ws", "tcp", 81);
    } else {
        Serial.println("MDNS.begin failed");
    }
    Serial.print("Connect to http://espWebSock.local or http://");
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.begin();

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    // LED is off by default
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
}

void loop()
{
    webSocket.loop();
    server.handleClient();

    if (LED_NEW_VALUES)
    {
        // LED_RED and LED_GREEN are intentionally in the wrong order.
        // Not entirely sure why that is the case.
        auto color = pixels.Color(LED_GREEN, LED_RED, LED_BLUE);
        pixels.setPixelColor(0, color);
        pixels.setPixelColor(1, color);
        pixels.show();
        LED_NEW_VALUES = false;
    }

    delay(100);
}
