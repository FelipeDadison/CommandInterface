/**
 * @example multi_stream.ino
 * @brief CommandInterface with multiple input/output streams
 *
 * Features:
 * - Commands work on both Serial and WebSocket simultaneously
 * - Replies go back to the stream that sent the command
 * - Broadcast messages reach every stream
 *
 * Requires: NetSerial library (https://github.com/FelipeDadison/NetSerial)
 * Connect with: websocat ws://<esp32_ip>/ws
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "CommandInterface.h"
#include "NetSerial.h"

AsyncWebServer server(80);
NetSerial wsSerial(server, "/ws");

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
        Serial.print("Connect to: ws://");
        Serial.print(WiFi.localIP());
        Serial.println("/ws");
    } else {
        Serial.println("\nFailed to connect to WiFi, restarting...");
        delay(2000);
        ESP.restart();
    }

    // Start web server and NetSerial
    server.begin();
    wsSerial.begin();

    // Setup CLI with multiple streams
    Cli.addInputStream(Serial);
    Cli.addInputStream(wsSerial);

    Cli.addOutputStream(Serial);
    Cli.addOutputStream(wsSerial);

    Cli.setProjectInfo("Multi-Stream CLI", "1.0.0", "ESP32-WebSocket");

    // Register commands (work on both Serial and WebSocket)
    Cli.registerCommand("hello", "Reply to the sender only", [](const String& args) {
        Cli.println("Hello!");
    });

    Cli.registerCommand("announce", "Broadcast a message to every stream", [](const String& args) {
        Cli.printlnToAll("Announcement: " + args);
    });

    Cli.registerCommand("led", "Toggle LED", [](const String& args) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        bool state = digitalRead(LED_PIN);
        Cli.printlnToAll(state ? "LED: ON" : "LED: OFF");
    });

    Cli.registerCommand("status", "Show system status", [](const String& args) {
        Cli.println("=== System Status ===");
        Cli.printf("Uptime: %lus\n", millis() / 1000);
        Cli.printf("Free Heap: %lu bytes\n", (unsigned long)ESP.getFreeHeap());
        Cli.printf("WiFi RSSI: %d dBm\n", WiFi.RSSI());
        Cli.printf("WebSocket clients: %u\n", wsSerial.getClientCount());
    });

    Cli.printlnToAll("\n[Multi-Stream CLI Ready]");
    Cli.printlnToAll("Use '?' for help");
    Cli.printlnToAll("Access from Serial OR WebSocket!\n");
}

void loop() {
    Cli.update();
    delay(10);
}