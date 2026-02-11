#include "stdin_reader.hpp"

#include "logic/CommandsInterpreter.hpp"
#include "coders/json.hpp"
#include "debug/Logger.hpp"

#include <thread>
#include <iostream>

static debug::Logger logger("stdin-reader");

static std::thread reader_thread;

void start_stdin_reader(cmd::CommandsInterpreter& interpreter) {
    reader_thread = std::thread([&interpreter]() {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) {
                continue;
            }
            try {
                auto result = interpreter.execute(line);
                if (result.isString()) {
                    logger.info() << result.asString();
                } else {
                    logger.info() << json::stringify(result, true);
                }
            } catch (const std::exception& err) {
                logger.error() << err.what();
            }
        }
    });
    reader_thread.detach();
}
