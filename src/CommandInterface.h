/**
 * @file CommandInterface.h
 * @brief Command-line interface system for ESP32 with password protection and typed arguments
 * @author Felipe Dadison
 * @version 1.0.1
 * @date 2026-05-12
 * 
 * Provides a flexible command registration system with support for untyped and strongly-typed
 * command arguments, password protection, and multi-stream input/output handling.
 */

#pragma once

#include <Arduino.h>
#include <functional>
#include <map>
#include <vector>
#include <tuple>
#include <utility>

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
 * password protection and formatted help display.
 */
class CommandInterface {
    public:
        /// @brief Callback function type for untyped commands
        using CommandCallback = std::function<void(const String& args)>;

        /**
         * @brief Construct a new CommandInterface instance
         */
        CommandInterface();

        /**
         * @brief Add an input stream for command input
         * @param stream Reference to input Stream (Serial, WebSerial, etc.)
         */
        void addInputStream(Stream& stream);

        /**
         * @brief Add an output stream for command output
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
         * @brief Print message to all output streams (no newline)
         * @param msg Message to print
         */
        void printToAll(const String& msg);

        /**
         * @brief Print message to all output streams (with newline)
         * @param msg Message to print
         */
        void printlnToAll(const String& msg);

        /**
         * @brief Set the access password for command interface
         * @param password Password string (empty string to disable protection)
         */
        void setPassword(const String& password);

        /**
         * @brief Lock the command interface (requires password to unlock)
         */
        void lock();

        /**
         * @brief Unlock the command interface
         */
        void unlock();

        /**
         * @brief Check if interface is currently locked
         * @return true if locked, false otherwise
         */
        bool isLocked() const;

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
         * @param input String containing arguments separated by the ARGS_SEPARATOR character
         * @return Vector of parsed argument strings
         */
        static std::vector<String> splitArgs(const String& input);

        /**
         * @brief Register a strongly-typed command with compile-time argument checking
         * @tparam Args Argument types (int, float, String) to expect
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

        std::vector<Stream*> _inputStreams;
        std::vector<Stream*> _outputStreams;

        String _commandBuffer;

        String _projectName = "UnnamedProject";
        String _projectVersion = "0.0.0";
        String _projectHostName = "Undefined";

        String _password = "";
        bool _locked = false;

        std::map<String, CommandEntry> _commands;

        /// @brief Handle and execute a command string
        /// @param cmd Command string to process
        void handleCommand(const String& cmd);

        /// @brief Print formatted help text with all registered commands
        void printHelp();

        /// @brief Check if input password matches the set password
        /// @param input Password string to verify
        /// @return true if password matches, false otherwise
        bool checkPassword(const String& input);

        /// @brief Parse a string argument to a specific type
        /// @tparam T Target type (int, float, String)
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

/// @brief Generic argument parser (returns false for unsupported types)
template<typename T>
bool CommandInterface::parseArg(const String& s, T& out) {
    return false;
}

/// @brief Specialization: parse integer argument
template<>
inline bool CommandInterface::parseArg<int>(const String& s, int& out) {
    char* endptr = nullptr;
    long val = strtol(s.c_str(), &endptr, 10);
    if (*endptr != '\0') return false;
    out = static_cast<int>(val);
    return true;
}

/// @brief Specialization: parse floating-point argument
template<>
inline bool CommandInterface::parseArg<float>(const String& s, float& out) {
    char* endptr = nullptr;
    float val = strtof(s.c_str(), &endptr);
    if (*endptr != '\0') return false;
    out = val;
    return true;
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
            printlnToAll("Invalid argument count.");
            return;
        }
        std::tuple<Args...> parsed;
        if (!parseArgsTuple(args, parsed, std::index_sequence_for<Args...>{})) {
            printlnToAll("Argument type error.");
            return;
        }
        std::apply(cb, parsed);
    };
    _commands[name] = entry;
}

extern CommandInterface Cli;