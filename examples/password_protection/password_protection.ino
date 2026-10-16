/**
 * @example password_protection.ino
 * @brief CommandInterface example with password protection
 *
 * Features:
 * - Password protected CLI access, unlocked per stream
 * - Brute-force protection: 3 free attempts, then an increasing wait (up to 5 min)
 * - Automatic lock after 5 minutes without commands
 * - Failed attempts reported through a callback
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

    // Brute-force protection (these are the defaults)
    Cli.setLoginLimits(3, 5 * 60 * 1000);

    // Lock again after 5 minutes without commands
    Cli.setAutoLock(5 * 60 * 1000);

    // Report failed attempts
    Cli.onLoginFailed([](Stream& stream, uint8_t attempts) {
        Serial.printf("[Security] Wrong password, attempt %u\n", attempts);
    });

    // Register commands (won't be accessible until password is entered)
    Cli.registerCommand("hello", "Print greeting", [](const String& args) {
        Cli.println("Hello! Access granted!");
    });

    Cli.registerCommand("led", "Toggle LED", [](const String& args) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        bool state = digitalRead(LED_PIN);
        Cli.println(state ? "LED: ON" : "LED: OFF");
    });

    Cli.registerCommand("lock", "Lock this session again", [](const String& args) {
        Cli.println("Session locked!");
        Cli.lock(*Cli.currentStream());
    });

    Cli.registerCommand("lockall", "Lock all sessions", [](const String& args) {
        Cli.println("All sessions locked!");
        Cli.lock();
    });

    Cli.printlnToAll("\n[LOCKED] Enter PASSWORD to access");
    Cli.printlnToAll("Example: PASSWORD\\admin\n");
}

void loop() {
    Cli.update();
    delay(10);
}