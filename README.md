# CommandInterface

Command-line interface system for ESP32 with support for untyped and strongly-typed commands, password protection, and multi-stream I/O.

## Installation

Add to your `platformio.ini`:

```ini
lib_deps =
    CommandInterface
```

## Usage

### Basic Setup
```cpp
#include "CommandInterface.h"
#include "WebSerial.h"

void setup() {
    Serial.begin(115200);
    
    // Add streams for input/output
    Cli.addInputStream(Serial);
    Cli.addOutputStream(Serial);
    
    // Set project info
    Cli.setProjectInfo("MyProject", "1.0.0", "ESP32-Device");
    
    // Register commands
    Cli.registerCommand("hello", "Say hello", [](const String& args) {
        Cli.printlnToAll("Hello from ESP32!");
    });
    
    Cli.registerCommand("led", "Toggle LED", [](const String& args) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        Cli.printlnToAll("LED toggled");
    });
}

void loop() {
    Cli.update();  // Process incoming commands
    delay(10);
}
```

### Typed Commands with Arguments
```cpp
Cli.registerTypedCommand<int, String>(
    "setvalue",
    "Set a value (int value, String name)",
    {
        {"value", "int"},
        {"name", "String"}
    },
    [](int value, String name) {
        Cli.printToAll("Set ");
        Cli.printToAll(name);
        Cli.print(" to ");
        Cli.printlnToAll(String(value));
    }
);

// Usage from CLI: setvalue\42\myvar
```

### Password Protection
```cpp
void setup() {
    // Set password for CLI access
    Cli.setPassword("1234");
    
    // Commands won't execute until password is entered
    // User must type: PASSWORD\1234
}
```

### Multi-Stream I/O
```cpp
#include "WebSerial.h"

WebSerial wsSerial(server, "/ws");

void setup() {
    // Add multiple input/output streams
    Cli.addInputStream(Serial);
    Cli.addInputStream(wsSerial);
    
    Cli.addOutputStream(Serial);
    Cli.addOutputStream(wsSerial);
    
    // All commands now work on both Serial and WebSocket!
}
```

## API

### Constructors
- `CommandInterface()` - Create new interface instance (or use global `Cli`)

### Stream Management
- `void addInputStream(Stream& stream)` - Add input stream
- `void addOutputStream(Stream& stream)` - Add output stream
- `void removeInputStream(Stream& stream)` - Remove input stream
- `void removeOutputStream(Stream& stream)` - Remove output stream

### Core Methods
- `void update()` - Process incoming commands (call in main loop)
- `void registerCommand(name, description, callback)` - Register untyped command
- `void registerTypedCommand<Args...>(name, description, argSpecs, callback)` - Register typed command
- `static std::vector<String> splitArgs(input)` - Split arguments by separator

### Output
- `void printToAll(msg)` - Print to all output streams (no newline)
- `void printlnToAll(msg)` - Print to all output streams (with newline)

### Security
- `void setPassword(password)` - Set access password
- `void lock()` - Lock interface
- `void unlock()` - Unlock interface
- `bool isLocked()` - Check lock status

### Configuration
- `void setProjectInfo(name, version, hostName)` - Set info for help display

## Separator Character

Commands and arguments are separated by `\` character:

```
command\arg1\arg2\arg3
```

## Dependencies

- Arduino Framework
- Standard C++ library (functional, map, vector, tuple)

## License

MIT
