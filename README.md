# CommandInterface

Command-line interface system for ESP32 with support for untyped and strongly-typed commands, password protection, and multi-stream I/O.

Works with any Arduino `Stream`: `Serial`, `Serial2`, Bluetooth serial, or network streams such as [NetSerial](https://github.com/FelipeDadison/NetSerial).

## Installation

### PlatformIO

Add to your `platformio.ini`:

```ini
lib_deps =
    https://github.com/FelipeDadison/CommandInterface.git
```

Requires C++17, the default on the ESP32 Arduino core 3.x. On core 2.x add:

```ini
build_unflags = -std=gnu++11
build_flags = -std=gnu++17
```

### Arduino IDE

Download the ZIP from the [latest release](https://github.com/FelipeDadison/CommandInterface/releases/latest), then use *Sketch → Include Library → Add .ZIP Library...*.

Requires the **esp32** board package 3.x or newer (*Tools → Board → Boards Manager*), which builds with C++17.

## Usage

### Basic Setup
```cpp
#include "CommandInterface.h"

const int LED_PIN = 2;

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    
    // Add streams for input/output
    Cli.addInputStream(Serial);
    Cli.addOutputStream(Serial);
    
    // Set project info (shown by the '?' help command)
    Cli.setProjectInfo("MyProject", "1.0.0", "ESP32-Device");
    
    // Register commands
    Cli.registerCommand("hello", "Say hello", [](const String& args) {
        Cli.println("Hello from ESP32!");
    });
    
    Cli.registerCommand("led", "Toggle LED", [](const String& args) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        Cli.println("LED toggled");
    });
}

void loop() {
    Cli.update();  // Process incoming commands
    delay(10);
}
```

Commands end with a newline (`\n` or `\r\n`). Leading and trailing spaces are ignored.

> **Arduino IDE:** set the Serial Monitor line ending to **New Line** or **Both NL & CR**. With **No Line Ending**, commands are never executed.

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
        Cli.printf("Set %s to %d\n", name.c_str(), value);
    }
);

// Usage from CLI: setvalue\42\myvar
```

Supported argument types: `int`, `long`, `unsigned int`, `unsigned long`, `float`, `double`, `bool` (`1`/`0`, `true`/`false`, `on`/`off`, `yes`/`no`) and `String`. Any other type is a compile-time error. Numbers out of range for their type are rejected.

### Replies and Broadcasts

`Cli` is a `Print`, so `print()`, `println()` and `printf()` work like on `Serial`:

- Inside a command, `Cli.print()`, `Cli.println()` and `Cli.printf()` reply only to the stream that sent the command.
- `Cli.printToAll()`, `Cli.printlnToAll()` and `Cli.printfToAll()` broadcast to every output stream.
- Outside a command (e.g. in `loop()`), `Cli.print()` also broadcasts.

### Password Protection
```cpp
void setup() {
    // Set password for CLI access
    Cli.setPassword("1234");
    
    // Commands won't execute until password is entered
    // User must type: PASSWORD\1234
}
```

- Each input stream is unlocked separately: entering the password on `Serial` doesn't unlock the WebSocket.
- `lock()` / `unlock()` act on all streams, `lock(stream)` / `unlock(stream)` on one.
- `unlock()` also applies to streams added later, until `lock()` is called. Useful to open the CLI from your firmware, e.g. on a trusted network.
- Inside a command, `Cli.currentStream()` returns the stream that sent it, e.g. `Cli.lock(*Cli.currentStream())`.

**Brute-force protection** is enabled by default: after 3 wrong passwords on a stream, each new failure doubles the wait before the next attempt (1 s, 2 s, 4 s...), up to 5 minutes. During the wait, attempts are refused without being checked. A correct password resets the counter. Each stream has its own counter, so attempts over the network don't block `Serial`.

```cpp
Cli.setLoginLimits(3, 5 * 60 * 1000);  // free attempts, max wait (ms); (0, 0) disables
Cli.setAutoLock(5 * 60 * 1000);        // lock a password session after 5 min without commands (default: off)
Cli.onLoginFailed([](Stream& stream, uint8_t attempts) {
    Serial.printf("Wrong password, attempt %u\n", attempts);
});
```

Auto-lock only applies to streams unlocked by typing the password. Streams unlocked from code with `unlock()` / `unlock(stream)` stay open until your code locks them.

### Multi-Stream I/O
```cpp
#include "NetSerial.h"

NetSerial wsSerial(server, "/ws");

void setup() {
    // Add multiple input/output streams
    Cli.addInputStream(Serial);
    Cli.addInputStream(wsSerial);
    
    Cli.addOutputStream(Serial);
    Cli.addOutputStream(wsSerial);
    
    // All commands now work on both Serial and WebSocket!
}
```

Each input stream has its own command buffer, so commands typed on different streams at the same time never mix.

Complete sketches are in [`examples/`](examples). The `multi_stream` example requires [NetSerial](https://github.com/FelipeDadison/NetSerial); the library itself has no dependencies.

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
- `static std::vector<String> splitArgs(input)` - Split arguments by separator (empty arguments are kept)
- `Stream* currentStream()` - Stream that sent the command being executed (`nullptr` outside a command)

### Output
- `print()`, `println()`, `printf()` - Reply to the current stream (broadcast outside a command)
- `void printToAll(msg)` - Print to all output streams (no newline)
- `void printlnToAll(msg)` - Print to all output streams (with newline)
- `void printfToAll(format, ...)` - Formatted print to all output streams

### Security
- `void setPassword(password)` - Set access password and lock all streams (empty string disables protection)
- `void lock()` / `void lock(stream)` - Lock all streams / one stream
- `void unlock()` / `void unlock(stream)` - Unlock all streams (including streams added later) / one stream; not affected by auto-lock
- `bool isLocked()` - `true` if any stream is locked
- `bool isLocked(stream)` - Lock status of one stream
- `void setLoginLimits(freeAttempts, maxDelayMs)` - Brute-force protection (default: 3, 300000)
- `void setAutoLock(timeoutMs)` - Lock password sessions after inactivity (default: 0, disabled)
- `void onLoginFailed(callback)` - Called on every wrong password with the stream and attempt count

### Configuration
- `void setProjectInfo(name, version, hostName)` - Set info for help display

## Separator Character

Commands and arguments are separated by `\` character:

```
command\arg1\arg2\arg3
```

Two consecutive separators produce an empty argument: `command\a\\c` has the arguments `a`, an empty string and `c`.

## Limits

- Commands longer than 256 characters are discarded with `Command too long.`
- The help command is `?`; while a stream is locked, only `?` and `PASSWORD` are accepted.

## Security

The password is sent in plain text, like everything else on the stream. Over a network transport anyone who can capture the traffic can read it, so only use network streams on trusted networks, and choose a password longer than 4 digits.

A network stream such as NetSerial is a single `Stream` shared by all its clients: once one client unlocks it, every client connected through it is unlocked. Use `setAutoLock()` to limit how long a session stays open.

## Dependencies

- Arduino Framework
- C++17 standard library (functional, map, vector, tuple)

## License

MIT
