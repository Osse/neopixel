#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#include <ESP8266WiFi.h>
// #include <WiFiClient.h> // Already included in ESP8266WiFi.h
#include <ESP8266WiFiMulti.h>
#include <Hash.h>
#include <ESP8266mDNS.h>
#include <Wire.h>

// PIN constants 
const int OUTPIN0 = 12; // D6   On LoLin NodeMCU V3

// WiFi details
static const char ssid[] = WIFI_SSID;
static const char password[] = WIFI_PASSWORD;

MDNSResponder mdns;
ESP8266WiFiMulti WiFiMulti;
Adafruit_NeoPixel pixels(1, OUTPIN0);

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
#endif

    // PIN setup
    pinMode(OUTPIN0, OUTPUT);
    digitalWrite(OUTPIN0, 0);

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

}

void loop() {
    pixels.setPixelColor(0, pixels.Color(150, 0, 0));
    pixels.show();
    delay(1000);
    Serial.println("tick");
}
