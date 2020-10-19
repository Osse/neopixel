#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

// PIN constants
const int OUTPIN0 = 12; // D6   On LoLin NodeMCU V3

// LED color values, names and signal
int LED_RED_0, LED_GREEN_0, LED_BLUE_0;
const char LED_RED_NAME_0[] = "red_0";
const char LED_GREEN_NAME_0[] = "green_0";
const char LED_BLUE_NAME_0[] = "blue_0";

int LED_RED_1, LED_GREEN_1, LED_BLUE_1;
const char LED_RED_NAME_1[] = "red_1";
const char LED_GREEN_NAME_1[] = "green_1";
const char LED_BLUE_NAME_1[] = "blue_1";

bool LED_NEW_VALUES = false;
bool DO_BME_READING = false;

// HTML
extern const char INDEX_HTML[];
extern const char SETTINGS_HTML[];
extern const char STYLE_CSS[];
extern const char SCRIPT_JS[];

// WiFi details
#include "wifi_settings.h"

MDNSResponder mdns;
ESP8266WiFiMulti WiFiMulti;
Adafruit_NeoPixel pixels(2, OUTPIN0);
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);
Adafruit_BME680 bme;

void sendCurrentLedColors(uint8_t num)
{
    char payload[100];
    int length = sprintf(payload, "%s:%d,%s:%d,%s:%d,%s:%d,%s:%d,%s:%d",
        LED_RED_NAME_0, LED_RED_0,
        LED_GREEN_NAME_0, LED_GREEN_0,
        LED_BLUE_NAME_0, LED_BLUE_0,
        LED_RED_NAME_1, LED_RED_1,
        LED_GREEN_NAME_1, LED_GREEN_1,
        LED_BLUE_NAME_1, LED_BLUE_1
    );
    webSocket.sendTXT(num, payload, length);
}

void handleLEDUpdate(uint8_t* payload, size_t length)
{
    const char* payload_char = (const char*)payload;
    char* colon = (char*)memchr(payload_char, ':', length);
    if (colon != NULL)
    {
        size_t pos = colon - payload_char;

        int* led_ptr = NULL;
        if(!strncmp(payload_char, LED_RED_NAME_0, pos))
            led_ptr = &LED_RED_0;
        else if(!strncmp(payload_char, LED_GREEN_NAME_0, pos))
            led_ptr = &LED_GREEN_0;
        else if(!strncmp(payload_char, LED_BLUE_NAME_0, pos))
            led_ptr = &LED_BLUE_0;

        if(!strncmp(payload_char, LED_RED_NAME_1, pos))
            led_ptr = &LED_RED_1;
        else if(!strncmp(payload_char, LED_GREEN_NAME_1, pos))
            led_ptr = &LED_GREEN_1;
        else if(!strncmp(payload_char, LED_BLUE_NAME_1, pos))
            led_ptr = &LED_BLUE_1;

        if (led_ptr != NULL)
        {
            int value = atoi(colon + 1);
            value = max(0, min(255, value));
            *led_ptr = value;
            LED_NEW_VALUES = true;
        }
    }
}

void updateLEDs()
{
    // LED_RED and LED_GREEN are intentionally in the wrong order.
    // Not entirely sure why that is the case.
    pixels.setPixelColor(0, pixels.Color(LED_GREEN_0, LED_RED_0, LED_BLUE_0));
    pixels.setPixelColor(1, pixels.Color(LED_GREEN_1, LED_RED_1, LED_BLUE_1));
    pixels.show();
}

void doBMEReading()
{
    bme.performReading();
    Serial.print("Temperature = "); Serial.print(bme.temperature); Serial.println(" *C");
    Serial.print("Pressure = "); Serial.print(bme.pressure / 100.0); Serial.println(" hPa");
    Serial.print("Humidity = "); Serial.print(bme.humidity); Serial.println(" %");
    Serial.print("Gas = "); Serial.print(bme.gas_resistance / 1000.0); Serial.println(" KOhms");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
    Serial.println();
    Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\r\n", num);
            break;
        case WStype_CONNECTED: {
            sendCurrentLedColors(num);
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
            break;
        }
        case WStype_TEXT:
            Serial.printf("[%u] get Text: %s\r\n", num, payload);
            handleLEDUpdate(payload, length);
            break;
        case WStype_BIN:
            Serial.printf("[%u] get binary length: %u\r\n", num, length);
            hexdump(payload, length);
            // echo data back to browser
            webSocket.sendBIN(num, payload, length);
            break;
        default:
            Serial.printf("Invalid WStype [%d]\r\n", type);
            break;
    }
}

void setup()
{
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
#endif

    pixels.begin();

    bme.begin();
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320*C for 150 ms

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

    server.on("/", []() { server.send_P(200, "text/html", INDEX_HTML); } );
    server.on("/settings.html", []() { server.send_P(200, "text/html", SETTINGS_HTML); } );
    server.on("/style.css", []() { server.send_P(200, "text/css", STYLE_CSS); } );
    server.on("/script.js", []() { server.send_P(200, "text/javascript", SCRIPT_JS); } );
    server.begin();

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    // LED is off by default
    auto black = pixels.Color(0, 0, 0);
    pixels.setPixelColor(0, black);
    pixels.setPixelColor(1, black);
    pixels.show();
}

void loop()
{
    webSocket.loop();
    server.handleClient();

    if (LED_NEW_VALUES)
    {
        updateLEDs();
        LED_NEW_VALUES = false;
    }

    if (DO_BME_READING)
    {
        doBMEReading();
        DO_BME_READING = false;
    }

    delay(100);
}
