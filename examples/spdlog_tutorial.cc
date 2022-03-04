
#include "spdlog/cfg/env.h"  // support for loading levels from the environment variable
#include "spdlog/fmt/ostr.h" // support for user defined types
#include "spdlog/spdlog.h"
#include <chrono>
#include <cstdio>

// stopwatch example
#include "spdlog/stopwatch.h"
#include <thread>
void stopwatch_example() {
    spdlog::stopwatch sw;
    std::this_thread::sleep_for(std::chrono::milliseconds(123));
    spdlog::info("Stopwatch: {} seconds", sw);
}

#include "spdlog/fmt/bin_to_hex.h"

void binary_example() {
    std::vector<char> buf(80);
    for (int i = 0; i < 80; i++) {
        buf[i] = i;
    }
    char  buffer[16] = {0x16};
    spdlog::info("Binary example: {}", spdlog::to_hex(buf));
    spdlog::info("Binary example1: {}", spdlog::to_hex(std::begin(buffer), std::end(buffer)));
    spdlog::info("Another binary example:{:n}", spdlog::to_hex(std::begin(buf), std::begin(buf) + 10));
    // more examples:
    // logger->info("uppercase: {:X}", spdlog::to_hex(buf));
    // logger->info("uppercase, no delimiters: {:Xs}", spdlog::to_hex(buf));
    // logger->info("uppercase, no delimiters, no position info: {:Xsp}", spdlog::to_hex(buf));
}

#include "spdlog/sinks/stdout_color_sinks.h"
// or #include "spdlog/sinks/stdout_sinks.h" if no colors needed.
void stdout_logger_example() {
    // Create color multi threaded logger.
    auto console = spdlog::stdout_color_mt("console");
    // or for stderr:
    // auto console = spdlog::stderr_color_mt("error-logger");
}

#include "spdlog/sinks/basic_file_sink.h"
void basic_example() {
    // Create basic file logger (not rotated).
    auto my_logger = spdlog::basic_logger_mt("file_logger", "logs/basic-log.txt", true);
    my_logger->set_level(spdlog::level::debug);
    my_logger->info("asdfljasdf");
}


#include "spdlog/sinks/udp_sink.h"
void udp_example()
{
    spdlog::sinks::udp_sink_config cfg("127.0.0.1", 11091);
    auto my_logger = spdlog::udp_logger_mt("udplog", cfg);
    my_logger->set_level(spdlog::level::debug);
    my_logger->info("hello world");
}

// A logger with multiple sinks (stdout and file) - each with a different format and log level.
void multi_sink_example()
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::warn);
    console_sink->set_pattern("[multi_sink_example] [%^%l%$] %v");

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/multisink.txt", true);
    file_sink->set_level(spdlog::level::trace);

    spdlog::logger logger("multi_sink", {console_sink, file_sink});
    logger.set_level(spdlog::level::debug);
    logger.warn("this should appear in both console and file");
    logger.info("this message should not appear in the console, only in the file");
}
void trace_example()
{
    // trace from default logger
    SPDLOG_TRACE("Some trace message.. {} ,{}", 1, 3.23);
    // debug from default logger
    SPDLOG_DEBUG("Some debug message.. {} ,{}", 1, 3.23);

    // trace from logger object
    auto logger = spdlog::get("file_logger");
    SPDLOG_LOGGER_TRACE(logger, "another trace message");
}
// User defined types logging
struct my_type
{
    int i = 0;
    explicit my_type(int i)
        : i(i){};
};

namespace fmt_lib = spdlog::fmt_lib;
template<>
struct fmt_lib::formatter<my_type> : fmt_lib::formatter<std::string>
{
    auto format(my_type my, format_context &ctx) -> decltype(ctx.out())
    {
        return fmt_lib::format_to(ctx.out(), "[my_type i={}]", my.i);
    }
};

void user_defined_example()
{
    spdlog::info("user defined type: {}", my_type(14));
}

void file_events_example()
{
    // pass the spdlog::file_event_handlers to file sinks for open/close log file notifications
    spdlog::file_event_handlers handlers;
    handlers.before_open = [](spdlog::filename_t filename) { spdlog::info("Before opening {}", filename); };
    handlers.after_open = [](spdlog::filename_t filename, std::FILE *fstream) {
        spdlog::info("After opening {}", filename);
        fputs("After opening\n", fstream);
    };
    handlers.before_close = [](spdlog::filename_t filename, std::FILE *fstream) {
        spdlog::info("Before closing {}", filename);
        fputs("Before closing\n", fstream);
    };
    handlers.after_close = [](spdlog::filename_t filename) { spdlog::info("After closing {}", filename); };
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/events-sample.txt", true, handlers);
    spdlog::logger my_logger("some_logger", file_sink);
    my_logger.info("Some log line");
    my_logger.set_error_handler([](const std::string &msg) { printf("*** Custom log error handler: %s ***\n", msg.c_str()); });
}

int main(int, char *[]) {

    basic_example();
    // spdlog::set_pattern("[%H:%M:%S %z] [%^%L%$] [tid %t] %v");
    spdlog::set_level(spdlog::level::trace);
    spdlog::info("Welcome to spdlog version {}.{}.{}  !", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);

    spdlog::warn("Easy padding in numbers like {:08d}", 12);
    spdlog::critical("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
    spdlog::info("Support for floats {:03.2f}", 1.23456);
    spdlog::info("Positional args are {1} {0}..", "too", "supported");
    spdlog::info("{:>8} aligned, {:<8} aligned", "right", "left");

    // Runtime log levels
    spdlog::set_level(spdlog::level::info); // Set global log level to info
    spdlog::debug("This message should not be displayed!");
    spdlog::set_level(spdlog::level::trace); // Set specific logger's log level
    spdlog::debug("This message should be displayed..");

    stdout_logger_example();

    // Customize msg format for all loggers
    spdlog::set_pattern("[%H:%M:%S %z] [%^%L%$] [tid %t] %v");
    spdlog::info("This an info message with custom format");
    spdlog::set_pattern("%+"); // back to default format
    spdlog::set_level(spdlog::level::info);

    spdlog::stopwatch sw;
    spdlog::info("Stopwatch: {} seconds", sw);
    spdlog::dump_backtrace(); // log them now!
    binary_example();
    udp_example();
    multi_sink_example();
    trace_example();
    file_events_example();
    user_defined_example();
}