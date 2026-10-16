/**
 * @file CommandInterface.cpp
 * @brief Implementation of command-line interface system for ESP32
 * @author Felipe Dadison
 * @version 1.1.0
 * @date 2026-10-16
 *
 * Implements command parsing, execution, password protection, and formatted output.
 */

#include "CommandInterface.h"
#include <cstdarg>

namespace {

    constexpr char ARGS_SEPARATOR = '\\';

    String makePadding(size_t count) {
        String padding;
        padding.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            padding += ' ';
        }
        return padding;
    }

    String formatString(const char* format, va_list args) {
        va_list copy;
        va_copy(copy, args);
        int len = vsnprintf(nullptr, 0, format, copy);
        va_end(copy);
        if (len <= 0) return String();

        std::vector<char> buffer(len + 1);
        vsnprintf(buffer.data(), buffer.size(), format, args);
        return String(buffer.data());
    }
}

CommandInterface::CommandInterface() {}

void CommandInterface::addInputStream(Stream& stream) {
    if (findInput(stream)) return;

    InputState input;
    input.stream = &stream;
    input.locked = !_password.isEmpty() && !_unlockedAll;
    input.unlockedByCode = _unlockedAll;
    input.lastActivity = millis();
    _inputs.push_back(input);
}

void CommandInterface::addOutputStream(Stream& stream) {
    if (std::find(_outputStreams.begin(), _outputStreams.end(), &stream) != _outputStreams.end()) return;
    _outputStreams.push_back(&stream);
}

void CommandInterface::removeInputStream(Stream& stream) {
    for (auto it = _inputs.begin(); it != _inputs.end(); ++it) {
        if (it->stream == &stream) {
            _inputs.erase(it);
            break;
        }
    }
}

void CommandInterface::removeOutputStream(Stream& stream) {
    for (auto it = _outputStreams.begin(); it != _outputStreams.end(); ++it) {
        if (*it == &stream) {
            _outputStreams.erase(it);
            break;
        }
    }
}

void CommandInterface::update() {
    for (size_t i = 0; i < _inputs.size(); ++i) {
        Stream* s = _inputs[i].stream;

        if (_autoLockMs > 0 && !_password.isEmpty() && !_inputs[i].locked && !_inputs[i].unlockedByCode
            && millis() - _inputs[i].lastActivity >= _autoLockMs) {
            _inputs[i].locked = true;
            s->println("Session locked due to inactivity.");
        }

        while (i < _inputs.size() && _inputs[i].stream == s && s->available()) {
            int c = s->read();
            if (c < 0) break;

            InputState& input = _inputs[i];

            if (c == '\n' || c == '\r') {
                String cmd = input.buffer;
                bool overflow = input.overflow;
                input.buffer = "";
                input.overflow = false;

                if (overflow) {
                    s->println("Command too long.");
                } else {
                    cmd.trim();
                    if (cmd.length() > 0) {
                        handleCommand(i, cmd);
                    }
                }
            } else if (input.buffer.length() < MAX_COMMAND_LENGTH) {
                input.buffer += (char)c;
            } else {
                input.overflow = true;
            }
        }
    }
}

size_t CommandInterface::write(uint8_t b) {
    return write(&b, 1);
}

size_t CommandInterface::write(const uint8_t* buffer, size_t size) {
    if (_currentStream) {
        return _currentStream->write(buffer, size);
    }

    for (auto s : _outputStreams) s->write(buffer, size);
    return size;
}

void CommandInterface::printToAll(const String& msg) {
    for (auto s : _outputStreams) s->print(msg);
}

void CommandInterface::printlnToAll(const String& msg) {
    for (auto s : _outputStreams) s->println(msg);
}

void CommandInterface::printfToAll(const char* format, ...) {
    va_list args;
    va_start(args, format);
    String msg = formatString(format, args);
    va_end(args);
    printToAll(msg);
}

Stream* CommandInterface::currentStream() const {
    return _currentStream;
}

void CommandInterface::setPassword(const String& password) {
    _password = password;
    _unlockedAll = false;
    bool locked = !password.isEmpty();
    for (auto& input : _inputs) {
        input.locked = locked;
        input.unlockedByCode = false;
        input.failedAttempts = 0;
    }
}

void CommandInterface::lock() {
    _unlockedAll = false;
    if (_password.isEmpty()) return;
    for (auto& input : _inputs) {
        input.locked = true;
        input.unlockedByCode = false;
    }
}

void CommandInterface::lock(Stream& stream) {
    if (_password.isEmpty()) return;
    InputState* input = findInput(stream);
    if (input) {
        input->locked = true;
        input->unlockedByCode = false;
    }
}

void CommandInterface::unlock() {
    _unlockedAll = true;
    for (auto& input : _inputs) {
        input.locked = false;
        input.unlockedByCode = true;
    }
}

void CommandInterface::unlock(Stream& stream) {
    InputState* input = findInput(stream);
    if (input) {
        input->locked = false;
        input->unlockedByCode = true;
    }
}

bool CommandInterface::isLocked() const {
    for (const auto& input : _inputs) {
        if (input.locked) return true;
    }
    return false;
}

bool CommandInterface::isLocked(Stream& stream) const {
    const InputState* input = findInput(stream);
    return input ? input->locked : false;
}

void CommandInterface::setLoginLimits(uint8_t freeAttempts, unsigned long maxDelayMs) {
    _freeAttempts = freeAttempts;
    _maxLoginDelayMs = maxDelayMs;
}

void CommandInterface::setAutoLock(unsigned long timeoutMs) {
    _autoLockMs = timeoutMs;
    unsigned long now = millis();
    for (auto& input : _inputs) input.lastActivity = now;
}

void CommandInterface::onLoginFailed(LoginFailedCallback callback) {
    _loginFailedCallback = callback;
}

void CommandInterface::setProjectInfo(const String& name, const String& version, const String& hostName) {
    _projectName = name;
    _projectVersion = version;
    if (hostName.length() != 0) {
        _projectHostName = hostName;
    }
}

void CommandInterface::registerCommand(const String& name, const String& description, CommandCallback callback) {
    CommandEntry entry;
    entry.description = description;
    entry.callback = callback;
    entry.isTyped = false;
    _commands[name] = entry;
}

std::vector<String> CommandInterface::splitArgs(const String& input) {
    std::vector<String> args;
    if (input.length() == 0) return args;

    int start = 0;
    int end = input.indexOf(ARGS_SEPARATOR);

    while (end != -1) {
        args.push_back(input.substring(start, end));
        start = end + 1;
        end = input.indexOf(ARGS_SEPARATOR, start);
    }

    args.push_back(input.substring(start));

    return args;
}

CommandInterface::InputState* CommandInterface::findInput(Stream& stream) {
    for (auto& input : _inputs) {
        if (input.stream == &stream) return &input;
    }
    return nullptr;
}

const CommandInterface::InputState* CommandInterface::findInput(Stream& stream) const {
    for (const auto& input : _inputs) {
        if (input.stream == &stream) return &input;
    }
    return nullptr;
}

void CommandInterface::handleCommand(size_t index, const String& cmd) {
    Stream* previous = _currentStream;
    _currentStream = _inputs[index].stream;

    int separator = cmd.indexOf(ARGS_SEPARATOR);
    String name = (separator == -1) ? cmd : cmd.substring(0, separator);
    String args = (separator == -1) ? "" : cmd.substring(separator + 1);

    if (_inputs[index].locked) {
        if (name == "PASSWORD") {
            handlePassword(index, args);
        } else if (name == "?") {
            println("CLI access denied. Enter PASSWORD" + String(ARGS_SEPARATOR) + "your_password to access.");
        }

        _currentStream = previous;
        return;
    }

    _inputs[index].lastActivity = millis();

    if (name == "?") {
        printHelp();
    } else {
        auto it = _commands.find(name);
        if (it != _commands.end()) {
            CommandCallback callback = it->second.callback;
            callback(args);
        } else {
            println("Unknown command. Type '?' for a list.");
        }
    }

    _currentStream = previous;
}

void CommandInterface::handlePassword(size_t index, const String& input) {
    InputState& state = _inputs[index];
    unsigned long now = millis();

    unsigned long wait = loginDelay(state.failedAttempts);
    if (wait > 0 && now - state.lastFailure < wait) {
        unsigned long remaining = (wait - (now - state.lastFailure) + 999) / 1000;
        println("Too many attempts. Try again in " + String(remaining) + "s.");
        return;
    }

    if (checkPassword(input)) {
        state.failedAttempts = 0;
        state.locked = false;
        state.unlockedByCode = false;
        state.lastActivity = now;
        println("Access granted.");
        return;
    }

    if (state.failedAttempts < 255) state.failedAttempts++;
    state.lastFailure = now;

    Stream* stream = state.stream;
    uint8_t attempts = state.failedAttempts;

    println("Wrong password.");

    if (_loginFailedCallback) {
        _loginFailedCallback(*stream, attempts);
    }
}

unsigned long CommandInterface::loginDelay(uint8_t failures) const {
    if (_maxLoginDelayMs == 0 || failures <= _freeAttempts) return 0;

    uint8_t exponent = failures - _freeAttempts - 1;
    if (exponent >= 20) return _maxLoginDelayMs;

    return std::min(_maxLoginDelayMs, LOGIN_BASE_DELAY_MS << exponent);
}

void CommandInterface::printHelp() {
    println("");
    println("========= " + _projectName + " v" + _projectVersion + " =========");
    println("");
    println("Host name: " + _projectHostName);
    println("");

    println("Available commands:");

    size_t maxNameLen = 0;
    size_t maxArgsLen = 0;

    struct Row {
        String name;
        String args;
        String description;
        bool hasArgs;
    };

    std::vector<Row> rows;

    for (const auto& pair : _commands) {
        const String& name = pair.first;
        const CommandEntry& entry = pair.second;

        String args = "";
        bool hasArgs = entry.isTyped && !entry.argSpecs.empty();
        if (hasArgs) {
            args = "( ";
            for (size_t i = 0; i < entry.argSpecs.size(); ++i) {
                const auto& spec = entry.argSpecs[i];
                args += spec.type + " " + spec.name;
                if (i < entry.argSpecs.size() - 1)
                    args += ", ";
            }
            args += " )";
        }

        maxNameLen = std::max(maxNameLen, (size_t)name.length());
        if (hasArgs)
            maxArgsLen = std::max(maxArgsLen, (size_t)args.length());

        rows.push_back({name, args, entry.description, hasArgs});
    }

    for (const auto& row : rows) {
        String line = " | ";
        line += row.name;
        line += makePadding(maxNameLen - row.name.length());
        line += "   ";

        if (row.hasArgs) {
            line += row.args;
            line += makePadding(maxArgsLen - row.args.length());
        } else {
            line += makePadding(maxArgsLen);
        }

        line += " : \"" + row.description + "\"";
        println(line);
    }

    println("");
    println("Use '" + String(ARGS_SEPARATOR) + "' to separate the arguments.");
    println("");
}

bool CommandInterface::checkPassword(const String& input) const {
    size_t inputLen = input.length();
    size_t passwordLen = _password.length();
    size_t len = std::max(inputLen, passwordLen);

    uint8_t diff = (inputLen != passwordLen) ? 1 : 0;
    for (size_t i = 0; i < len; ++i) {
        uint8_t a = (i < inputLen) ? (uint8_t)input[i] : 0;
        uint8_t b = (i < passwordLen) ? (uint8_t)_password[i] : 0;
        diff |= a ^ b;
    }

    return diff == 0 && passwordLen > 0;
}

CommandInterface Cli;