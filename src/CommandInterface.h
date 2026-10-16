/**
 * @file CommandInterface.h
 * @brief Command-line interface system for ESP32 with password protection and typed arguments
 * @author Felipe Dadison
 * @version 1.1.1
 * @date 2026-10-16
 *
 * Provides a flexible command registration system with support for untyped and strongly-typed
 * command arguments, password protection, and multi-stream input/output handling.
 */

#pragma once

#if __cplusplus < 201703L
#error "CommandInterface requires C++17 or newer (ESP32 Arduino core 3.x, or build_unflags = -std=gnu++11 and build_flags = -std=gnu++17)"
#endif

#include <Arduino.h>
#include <functional>
#include <map>
#include <vector>
#include <tuple>
#include <utility>
#include <algorithm>
#include <cerrno>
#include <climits>

/**
 * @struct CommandArgSpec
 * @brief Specification for a typed command argument
 */
struct CommandArgSpec {
    String name;   ///< Argument name for display
    String type;   ///< Argument type name (e.g., "int", "float", "String")
};

/**
 * @class CommandInterface
 * @brief Command-line interface handler with support for untyped and typed commands
 *
 * Manages command registration, execution, and multi-stream I/O. Supports optional
 * password protection with per-stream sessions and formatted help display.
 *
 * Inherits from Print: print(), println() and printf() reply to the stream that sent
 * the command being executed, or broadcast to all output streams outside a command.
 */
class CommandInterface : public Print {
    public:
        /// @brief Callback function type for untyped commands
        using CommandCallback = std::function<void(const String& args)>;

        /// @brief Callback function type for failed login attempts
        using LoginFailedCallback = std::function<void(Stream& stream, uint8_t attempts)>;

        /**
         * @brief Construct a new CommandInterface instance
         */
        CommandInterface();

        /**
         * @brief Add an input stream for command input
         * @param stream Reference to input Stream (Serial, NetSerial, etc.)
         */
        void addInputStream(Stream& stream);

        /**
         * @brief Add an output stream for broadcast output
         * @param stream Reference to output Stream
         */
        void addOutputStream(Stream& stream);

        /**
         * @brief Remove an input stream
         * @param stream Reference to input Stream to remove
         */
        void removeInputStream(Stream& stream);

        /**
         * @brief Remove an output stream
         * @param stream Reference to output Stream to remove
         */
        void removeOutputStream(Stream& stream);

        /**
         * @brief Process available input from all input streams
         * Call this regularly in your main loop to handle commands
         */
        void update();

        /**
         * @brief Set the maximum command length, including arguments
         * Longer commands are discarded with "Command too long.". Default is 256.
         * @param length Maximum number of characters per command
         */
        void setMaxCommandLength(size_t length);

        using Print::write;

        /**
         * @brief Write a single byte to the current stream, or to all output streams
         * @param b Byte to write
         * @return Number of bytes written
         */
        size_t write(uint8_t b) override;

        /**
         * @brief Write a buffer to the current stream, or to all output streams
         * @param buffer Pointer to data buffer
         * @param size Number of bytes to write
         * @return Number of bytes written
         */
        size_t write(const uint8_t* buffer, size_t size) override;

        /**
         * @brief Print message to all output streams (no newline)
         * @param msg Message to print
         */
        void printToAll(const String& msg);

        /**
         * @brief Print message to all output streams (with newline)
         * @param msg Message to print
         */
        void printlnToAll(const String& msg = "");

        /**
         * @brief Print formatted message to all output streams
         * @param format printf-style format string
         */
        void printfToAll(const char* format, ...) __attribute__((format(printf, 2, 3)));

        /**
         * @brief Get the stream that sent the command being executed
         * @return Pointer to the stream, or nullptr outside a command
         */
        Stream* currentStream() const;

        /**
         * @brief Set the access password for command interface
         * Locks all input streams. An empty string disables protection.
         * @param password Password string
         */
        void setPassword(const String& password);

        /**
         * @brief Lock all input streams (requires password to unlock)
         * Also ends the effect of unlock() on streams added later.
         */
        void lock();

        /**
         * @brief Lock a single input stream
         * @param stream Input stream to lock
         */
        void lock(Stream& stream);

        /**
         * @brief Unlock all input streams, including streams added later
         * Streams unlocked from code are not affected by auto-lock until lock() is called.
         */
        void unlock();

        /**
         * @brief Unlock a single input stream
         * Streams unlocked from code are not affected by auto-lock until locked again.
         * @param stream Input stream to unlock
         */
        void unlock(Stream& stream);

        /**
         * @brief Check if any input stream is locked
         * @return true if at least one input stream is locked
         */
        bool isLocked() const;

        /**
         * @brief Check if an input stream is locked
         * @param stream Input stream to check
         * @return true if the stream is locked
         */
        bool isLocked(Stream& stream) const;

        /**
         * @brief Configure brute-force protection for password attempts
         * After freeAttempts wrong passwords, each new failure doubles the wait before the
         * next attempt (1 s, 2 s, 4 s...), up to maxDelayMs. A correct password resets it.
         * Default: 3 attempts, 300000 ms. Pass maxDelayMs = 0 to disable.
         * @param freeAttempts Wrong attempts allowed without waiting
         * @param maxDelayMs Maximum wait between attempts in milliseconds
         */
        void setLoginLimits(uint8_t freeAttempts, unsigned long maxDelayMs);

        /**
         * @brief Lock a stream unlocked by password after a period without commands
         * Streams unlocked from code with unlock() are not affected.
         * @param timeoutMs Inactivity timeout in milliseconds (0 disables, default)
         */
        void setAutoLock(unsigned long timeoutMs);

        /**
         * @brief Set a callback called on every wrong password
         * @param callback Function receiving the stream and its failed attempt count
         */
        void onLoginFailed(LoginFailedCallback callback);

        /**
         * @brief Set project information displayed in help
         * @param name Project name
         * @param version Project version
         * @param hostName Device hostname (optional)
         */
        void setProjectInfo(const String& name, const String& version, const String& hostName = "");

        /**
         * @brief Register an untyped command
         * @param name Command name
         * @param description Brief description for help text
         * @param callback Function to execute when command is invoked
         */
        void registerCommand(const String& name, const String& description, CommandCallback callback);

        /**
         * @brief Split argument string by separator character
         * Empty arguments are kept: two consecutive separators produce an empty argument
         * @param input String containing arguments separated by the ARGS_SEPARATOR character
         * @return Vector of parsed argument strings
         */
        static std::vector<String> splitArgs(const String& input);

        /**
         * @brief Register a strongly-typed command with compile-time argument checking
         * @tparam Args Argument types (int, long, unsigned int, unsigned long, float, double, bool, String)
         * @tparam Callback Callable type matching the function signature
         * @param name Command name
         * @param description Brief description for help text
         * @param argSpecs Vector specifying argument names and types
         * @param callback Function to execute on successful argument parsing
         */
        template<typename... Args, typename Callback>
        void registerTypedCommand(
            const String& name,
            const String& description,
            const std::vector<CommandArgSpec>& argSpecs,
            Callback&& callback
        );

    private:

        struct CommandEntry {
            String description;
            CommandCallback callback;
            std::vector<CommandArgSpec> argSpecs;
            bool isTyped = false;
        };

        struct InputState {
            Stream* stream = nullptr;
            String buffer;
            bool overflow = false;
            bool locked = false;
            bool unlockedByCode = false;
            uint8_t failedAttempts = 0;
            unsigned long lastFailure = 0;
            unsigned long lastActivity = 0;
        };

        std::vector<InputState> _inputs;
        std::vector<Stream*> _outputStreams;
        Stream* _currentStream = nullptr;

        size_t _maxCommandLength = 256;
        const unsigned long LOGIN_BASE_DELAY_MS = 1000;

        String _projectName = "UnnamedProject";
        String _projectVersion = "0.0.0";
        String _projectHostName = "Undefined";

        String _password = "";
        bool _unlockedAll = false;
        uint8_t _freeAttempts = 3;
        unsigned long _maxLoginDelayMs = 300000;
        unsigned long _autoLockMs = 0;
        LoginFailedCallback _loginFailedCallback;

        std::map<String, CommandEntry> _commands;

        /// @brief Find the state of an input stream
        /// @param stream Input stream
        /// @return Pointer to the state, or nullptr if not registered
        InputState* findInput(Stream& stream);

        /// @brief Find the state of an input stream
        /// @param stream Input stream
        /// @return Pointer to the state, or nullptr if not registered
        const InputState* findInput(Stream& stream) const;

        /// @brief Handle and execute a command string
        /// @param index Index of the input stream that sent the command
        /// @param cmd Command string to process
        void handleCommand(size_t index, const String& cmd);

        /// @brief Handle a password attempt with brute-force protection
        /// @param index Index of the input stream that sent the attempt
        /// @param input Password entered
        void handlePassword(size_t index, const String& input);

        /// @brief Wait required before the next password attempt
        /// @param failures Number of consecutive failed attempts
        /// @return Wait in milliseconds
        unsigned long loginDelay(uint8_t failures) const;

        /// @brief Print formatted help text with all registered commands
        void printHelp();

        /// @brief Check if input password matches the set password (constant time)
        /// @param input Password string to verify
        /// @return true if password matches, false otherwise
        bool checkPassword(const String& input) const;

        /// @brief Parse a string argument to a specific type
        /// @tparam T Target type (int, long, unsigned int, unsigned long, float, double, bool, String)
        /// @param s String to parse
        /// @param out Reference to store parsed value
        /// @return true if parsing successful, false otherwise
        template<typename T>
        bool parseArg(const String& s, T& out);

        /// @brief Parse multiple arguments into a tuple
        /// @tparam Tuple Tuple type containing argument types
        /// @tparam Is Index sequence for tuple iteration
        /// @param args Vector of argument strings
        /// @param tuple Reference to store parsed arguments
        /// @return true if all arguments parsed successfully, false otherwise
        template<typename Tuple, size_t... Is>
        bool parseArgsTuple(const std::vector<String>& args, Tuple& tuple, std::index_sequence<Is...>);
};

/// @brief Generic argument parser (unsupported types fail at compile time)
template<typename T>
bool CommandInterface::parseArg(const String& s, T& out) {
    static_assert(sizeof(T) == 0, "Unsupported argument type: use int, long, unsigned int, unsigned long, float, double, bool or String");
    return false;
}

/// @brief Specialization: parse long argument
template<>
inline bool CommandInterface::parseArg<long>(const String& s, long& out) {
    if (s.isEmpty()) return false;
    char* endptr = nullptr;
    errno = 0;
    long val = strtol(s.c_str(), &endptr, 10);
    if (*endptr != '\0' || errno == ERANGE) return false;
    out = val;
    return true;
}

/// @brief Specialization: parse integer argument
template<>
inline bool CommandInterface::parseArg<int>(const String& s, int& out) {
    long val;
    if (!parseArg<long>(s, val) || val < INT_MIN || val > INT_MAX) return false;
    out = static_cast<int>(val);
    return true;
}

/// @brief Specialization: parse unsigned long argument
template<>
inline bool CommandInterface::parseArg<unsigned long>(const String& s, unsigned long& out) {
    if (s.isEmpty() || s.indexOf('-') != -1) return false;
    char* endptr = nullptr;
    errno = 0;
    unsigned long val = strtoul(s.c_str(), &endptr, 10);
    if (*endptr != '\0' || errno == ERANGE) return false;
    out = val;
    return true;
}

/// @brief Specialization: parse unsigned integer argument
template<>
inline bool CommandInterface::parseArg<unsigned int>(const String& s, unsigned int& out) {
    unsigned long val;
    if (!parseArg<unsigned long>(s, val) || val > UINT_MAX) return false;
    out = static_cast<unsigned int>(val);
    return true;
}

/// @brief Specialization: parse floating-point argument
template<>
inline bool CommandInterface::parseArg<float>(const String& s, float& out) {
    if (s.isEmpty()) return false;
    char* endptr = nullptr;
    float val = strtof(s.c_str(), &endptr);
    if (*endptr != '\0') return false;
    out = val;
    return true;
}

/// @brief Specialization: parse double-precision argument
template<>
inline bool CommandInterface::parseArg<double>(const String& s, double& out) {
    if (s.isEmpty()) return false;
    char* endptr = nullptr;
    double val = strtod(s.c_str(), &endptr);
    if (*endptr != '\0') return false;
    out = val;
    return true;
}

/// @brief Specialization: parse boolean argument (1/0, true/false, on/off, yes/no)
template<>
inline bool CommandInterface::parseArg<bool>(const String& s, bool& out) {
    String v = s;
    v.toLowerCase();
    if (v == "1" || v == "true" || v == "on" || v == "yes") {
        out = true;
        return true;
    }
    if (v == "0" || v == "false" || v == "off" || v == "no") {
        out = false;
        return true;
    }
    return false;
}

/// @brief Specialization: parse String argument (always succeeds)
template<>
inline bool CommandInterface::parseArg<String>(const String& s, String& out) {
    out = s;
    return true;
}

/// @brief Implementation of tuple argument parsing using fold expression
template<typename Tuple, size_t... Is>
bool CommandInterface::parseArgsTuple(const std::vector<String>& args, Tuple& tuple, std::index_sequence<Is...>) {
    bool ok = true;
    (void)std::initializer_list<int>{
        (ok = ok && parseArg<typename std::tuple_element<Is, Tuple>::type>(args[Is], std::get<Is>(tuple)), 0)...
    };
    return ok;
}

/// @brief Implementation of typed command registration with argument parsing
template<typename... Args, typename Callback>
void CommandInterface::registerTypedCommand(
    const String& name,
    const String& description,
    const std::vector<CommandArgSpec>& argSpecs,
    Callback&& callback
) {
    CommandEntry entry;
    entry.description = description;
    entry.argSpecs = argSpecs;
    entry.isTyped = true;
    auto cb = std::function<void(Args...)>(std::forward<Callback>(callback));
    entry.callback = [cb, this](const String& argsStr) {
        std::vector<String> args = splitArgs(argsStr);
        if (args.size() != sizeof...(Args)) {
            println("Invalid argument count.");
            return;
        }
        std::tuple<Args...> parsed;
        if (!parseArgsTuple(args, parsed, std::index_sequence_for<Args...>{})) {
            println("Argument type error.");
            return;
        }
        std::apply(cb, parsed);
    };
    _commands[name] = entry;
}

extern CommandInterface Cli;