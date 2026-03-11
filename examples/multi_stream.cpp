/**
 * @example multi_stream.cpp
 * @brief CommandInterface with multiple input/output streams
 * 
 * Features:
 * - Commands work on both Serial and WebSocket simultaneously
 * - Same CLI accessible from Serial monitor or web browser
 * 
 * Requires: WebSerial library
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "CommandInterface.h"
#include "WebSerial.h"

AsyncWebServer server(80);
WebSerial wsSerial(server, "/ws");

// WiFi credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

const int LED_PIN = 2;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Connect to WiFi
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nFailed to connect to WiFi");
        while (true) {}
    }
    
    // Start web server and WebSerial
    server.begin();
    wsSerial.begin();
    
    // Setup CLI with multiple streams
    Cli.addInputStream(Serial);
    Cli.addInputStream(wsSerial);
    
    Cli.addOutputStream(Serial);
    Cli.addOutputStream(wsSerial);
    
    Cli.setProjectInfo("Multi-Stream CLI", "1.0.0", "ESP32-WebSocket");
    
    // Register commands (work on both Serial and WebSocket)
    Cli.registerCommand("hello", "Print greeting", [](const String& args) {
        Cli.printlnToAll("Hello from all streams!");
    });
    
    Cli.registerCommand("led", "Toggle LED", [](const String& args) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        bool state = digitalRead(LED_PIN);
        Cli.printlnToAll(state ? "LED: ON" : "LED: OFF");
    });
    
    Cli.registerCommand("status", "Show system status", [](const String& args) {
        Cli.printlnToAll("=== System Status ===");
        Cli.printToAll("Uptime: ");
        Cli.printlnToAll(String(millis() / 1000) + "s");
        Cli.printToAll("Free Heap: ");
        Cli.printlnToAll(String(ESP.getFreeHeap()) + " bytes");
        Cli.printToAll("WiFi RSSI: ");
        Cli.printlnToAll(String(WiFi.RSSI()) + " dBm");
        Cli.printToAll("Connected Streams: ");
        Cli.printToAll(String(Cli.getClientCount() + 1));  // +1 for Serial
        Cli.printlnToAll("");
    });
    
    Cli.printlnToAll("\n[Multi-Stream CLI Ready]");
    Cli.printlnToAll("Use '?' for help");
    Cli.printlnToAll("Access from Serial OR WebSocket!\n");
}

void loop() {
    Cli.update();
    delay(10);
}
