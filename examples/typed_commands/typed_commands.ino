/**
 * @example typed_commands.ino
 * @brief CommandInterface example with strongly-typed commands
 *
 * Commands available:
 * - setpin\gpio\state : Set GPIO pin state (int, bool: 1/0, on/off, true/false)
 * - delay_ms\milliseconds : Delay for N milliseconds (unsigned int)
 * - read_pin\gpio : Read GPIO state (int)
 * - scale\value\factor : Multiply two numbers (float, float)
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

    // Typed command: setpin (int pin, bool state)
    Cli.registerTypedCommand<int, bool>(
        "setpin",
        "Set GPIO pin state (pin, state)",
        {
            {"pin", "int"},
            {"state", "bool"}
        },
        [](int pin, bool state) {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, state);
            Cli.printf("GPIO %d -> %s\n", pin, state ? "HIGH" : "LOW");
        }
    );

    // Typed command: delay_ms (unsigned int milliseconds)
    Cli.registerTypedCommand<unsigned int>(
        "delay_ms",
        "Delay for N milliseconds (ms)",
        {
            {"milliseconds", "unsigned int"}
        },
        [](unsigned int ms) {
            Cli.printf("Delaying %u ms...\n", ms);
            delay(ms);
            Cli.println("Done!");
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
            Cli.printf("GPIO %d = %s\n", pin, state ? "HIGH" : "LOW");
        }
    );

    // Typed command: scale (float value, float factor)
    Cli.registerTypedCommand<float, float>(
        "scale",
        "Multiply a value by a factor (value, factor)",
        {
            {"value", "float"},
            {"factor", "float"}
        },
        [](float value, float factor) {
            Cli.printf("%.3f x %.3f = %.3f\n", value, factor, value * factor);
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
            Cli.print("Echo: ");
            Cli.println(msg);
        }
    );

    Cli.printlnToAll("\n[Typed Commands Ready] Type '?' for help\n");
}

void loop() {
    Cli.update();
    delay(10);
}