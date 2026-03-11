/**
 * @example password_protection.cpp
 * @brief CommandInterface example with password protection
 * 
 * Features:
 * - Password protected CLI access
 * - Commands only available after authentication
 * - Attempt messages for locked interface
 * 
 * Default password: admin
 *
 * Try: PASSWORD\admin
 */

#include <Arduino.h>
#include "CommandInterface.h"

const int LED_PIN = 2;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Setup CLI with password protection
    Cli.addInputStream(Serial);
    Cli.addOutputStream(Serial);
    
    Cli.setProjectInfo("Secure Device", "1.0.0", "ESP32-Secured");
    
    // Set access password
    Cli.setPassword("admin");
    
    // Register commands (won't be accessible until password is entered)
    Cli.registerCommand("hello", "Print greeting", [](const String& args) {
        Cli.printlnToAll("Hello! Access granted!");
    });
    
    Cli.registerCommand("led", "Toggle LED", [](const String& args) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        bool state = digitalRead(LED_PIN);
        Cli.printlnToAll(state ? "LED: ON" : "LED: OFF");
    });
    
    Cli.registerCommand("unlock", "Unlock permanently", [](const String& args) {
        Cli.unlock();
        Cli.printlnToAll("Interface unlocked!");
    });
    
    Cli.registerCommand("lock", "Lock interface again", [](const String& args) {
        Cli.lock();
        Cli.printlnToAll("Interface locked!");
    });
    
    Cli.printlnToAll("\n[LOCKED] Enter PASSWORD to access");
    Cli.printlnToAll("Example: PASSWORD\\admin\n");
}

void loop() {
    Cli.update();
    delay(10);
}
