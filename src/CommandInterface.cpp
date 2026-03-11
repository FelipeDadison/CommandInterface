/**
 * @file CommandInterface.cpp
 * @brief Implementation of command-line interface system for ESP32
 * @author Felipe Dadison
 * @version 1.0.0
 * @date 2026-03-10
 * 
 * Implements command parsing, execution, password protection, and formatted output.
 */

#include "CommandInterface.h"

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
}

CommandInterface::CommandInterface() : _commandBuffer("") {}

void CommandInterface::addInputStream(Stream& stream) {
    _inputStreams.push_back(&stream);
}

void CommandInterface::addOutputStream(Stream& stream) {
    _outputStreams.push_back(&stream);
}

void CommandInterface::removeInputStream(Stream& stream) {
    for (auto it = _inputStreams.begin(); it != _inputStreams.end(); ++it) {
        if (*it == &stream) {
            _inputStreams.erase(it);
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
    for (auto s : _inputStreams) {
        while (s->available()) {
            char c = s->read();
            if (c == '\n' || c == '\r') {
                if (_commandBuffer.length() > 0) {
                    handleCommand(_commandBuffer);
                    _commandBuffer = "";
                }
            } else {
                _commandBuffer += c;
            }
        }
    }
}

void CommandInterface::printToAll(const String& msg) {
    for (auto s : _outputStreams) s->print(msg);
}

void CommandInterface::printlnToAll(const String& msg) {
    for (auto s : _outputStreams) s->println(msg);
}

void CommandInterface::setPassword(const String& password) {
    _password = password;
    _locked = !password.isEmpty();
}

void CommandInterface::lock() {
    if (!_password.isEmpty()) _locked = true;
}

void CommandInterface::unlock() {
    _locked = false;
}

bool CommandInterface::isLocked() const {
    return _locked;
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
    int start = 0;
    int end = input.indexOf(ARGS_SEPARATOR);

    while (end != -1) {
        if (end > start) {
            args.push_back(input.substring(start, end));
        }
        start = end + 1;
        end = input.indexOf(ARGS_SEPARATOR, start);
    }

    if (start < input.length()) {
        args.push_back(input.substring(start));
    }

    return args;
}

void CommandInterface::handleCommand(const String& cmd) {
    if (_locked) {
        int separator = cmd.indexOf(ARGS_SEPARATOR);
        String name = (separator == -1) ? cmd : cmd.substring(0, separator);
        String args = (separator == -1) ? "" : cmd.substring(separator + 1);

        if (name == "PASSWORD") {
            if (checkPassword(args)) {
                unlock();
                printlnToAll("Access granted.");
            } else {
                printlnToAll("Wrong password.");
            }
        } else if (name == "?") {
            printlnToAll("CLI access denied. Enter PASSWORD" + String(ARGS_SEPARATOR) + "your_password to access.");
        }

        return;
    }
    
    int separator = cmd.indexOf(ARGS_SEPARATOR);
    String name = (separator == -1) ? cmd : cmd.substring(0, separator);
    String args = (separator == -1) ? "" : cmd.substring(separator + 1);

    if (name == "?") {
        printHelp();
        return;
    }

    auto it = _commands.find(name);
    if (it != _commands.end()) {
        it->second.callback(args);
    } else {
        printlnToAll("Unknown command. Type '?' for a list.");
    }
}

void CommandInterface::printHelp() {
    printlnToAll("");
    printlnToAll("========= " + _projectName + " v" + _projectVersion + " =========");
    printlnToAll("");
    printlnToAll("Host name: " + _projectHostName);
    printlnToAll("");

    printlnToAll("Available commands:");

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

        maxNameLen = std::max(maxNameLen, name.length());
        if (hasArgs)
            maxArgsLen = std::max(maxArgsLen, args.length());

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
        printlnToAll(line);
    }

    printlnToAll("");
    printlnToAll("Use '" + String(ARGS_SEPARATOR) + "' to separate the arguments.");
    printlnToAll("");
}

bool CommandInterface::checkPassword(const String& input) {
    return input == _password;
}

CommandInterface Cli;