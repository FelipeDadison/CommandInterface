/**
 * @example basic_commands.cpp
 * @brief Basic CommandInterface example with simple untyped commands
 * 
 * Commands available:
 * - hello : Print greeting
 * - led : Toggle LED
 * - reset : Software reset
 * - ? : Show help
 */

#include <Arduino.h>
#include "CommandInterface.h"

const int LED_PIN = 2;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Configure GPIO
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Setup CLI
    Cli.addInputStream(Serial);
    Cli.addOutputStream(Serial);
    
    Cli.setProjectInfo("LED Controller", "1.0.0", "ESP32-Device");
    
    // Register simple commands
    Cli.registerCommand("hello", "Print greeting", [](const String& args) {
        Cli.printlnToAll("Hello from ESP32!");
    });
    
    Cli.registerCommand("led", "Toggle LED on/off", [](const String& args) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        bool state = digitalRead(LED_PIN);
        Cli.printlnToAll(state ? "LED: ON" : "LED: OFF");
    });
    
    Cli.registerCommand("reset", "Perform software reset", [](const String& args) {
        Cli.printlnToAll("Resetting...");
        delay(500);
        ESP.restart();
    });
    
    Cli.printlnToAll("\n[CLI Ready] Type '?' for help\n");
}

void loop() {
    Cli.update();
    delay(10);
}
