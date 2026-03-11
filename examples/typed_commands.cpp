/**
 * @example typed_commands.cpp
 * @brief CommandInterface example with strongly-typed commands
 * 
 * Commands available:
 * - setpin\gpio\state : Set GPIO pin state (int, int)
 * - delay_ms\milliseconds : Delay for N milliseconds (int)
 * - read_pin\gpio : Read GPIO state (int)
 * - echo\message : Echo back a message (String)
 * - ? : Show help
 */

#include <Arduino.h>
#include "CommandInterface.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Cli.addInputStream(Serial);
    Cli.addOutputStream(Serial);
    
    Cli.setProjectInfo("GPIO Controller", "1.0.0", "ESP32-GPIO");
    
    // Typed command: setpin (int pin, int state)
    Cli.registerTypedCommand<int, int>(
        "setpin",
        "Set GPIO pin state (pin, state)",
        {
            {"pin", "int"},
            {"state", "int"}
        },
        [](int pin, int state) {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, state);
            Cli.printToAll("GPIO ");
            Cli.printToAll(String(pin));
            Cli.printToAll(" -> ");
            Cli.printlnToAll(state ? "HIGH" : "LOW");
        }
    );
    
    // Typed command: delay_ms (int milliseconds)
    Cli.registerTypedCommand<int>(
        "delay_ms",
        "Delay for N milliseconds (ms)",
        {
            {"milliseconds", "int"}
        },
        [](int ms) {
            Cli.printToAll("Delaying ");
            Cli.printToAll(String(ms));
            Cli.printlnToAll(" ms...");
            delay(ms);
            Cli.printlnToAll("Done!");
        }
    );
    
    // Typed command: read_pin (int pin)
    Cli.registerTypedCommand<int>(
        "read_pin",
        "Read GPIO pin state (pin)",
        {
            {"pin", "int"}
        },
        [](int pin) {
            pinMode(pin, INPUT);
            int state = digitalRead(pin);
            Cli.printToAll("GPIO ");
            Cli.printToAll(String(pin));
            Cli.printToAll(" = ");
            Cli.printlnToAll(state ? "HIGH" : "LOW");
        }
    );
    
    // Typed command: echo (String message)
    Cli.registerTypedCommand<String>(
        "echo",
        "Echo back a message (message)",
        {
            {"message", "String"}
        },
        [](String msg) {
            Cli.printToAll("Echo: ");
            Cli.printlnToAll(msg);
        }
    );
    
    Cli.printlnToAll("\n[Typed Commands Ready] Type '?' for help\n");
}

void loop() {
    Cli.update();
    delay(10);
}
