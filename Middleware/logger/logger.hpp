#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>

#include "cpp/uart.hpp"

#ifdef NDEBUG
#define LOG_DEBUG(fmt, ...) ((void)0)
#else
#define LOG_DEBUG(fmt, ...) Logger::Log(LogLevel::DBG, fmt, ##__VA_ARGS__)
#endif

#define LOG_INFO(fmt, ...) Logger::Log(LogLevel::INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) Logger::Log(LogLevel::WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) Logger::Log(LogLevel::ERROR, fmt, ##__VA_ARGS__)

extern UARTDevice UART2;

enum class LogLevel : uint8_t { DBG = 1, INFO, WARN, ERROR };

struct Hex {
    uint32_t value;
    explicit Hex(uint32_t val) : value(val) {}
};

class Logger {
   public:
    Logger() = delete;  // No Constructor -> Logger to run as a Singleton.

    static void set_level(LogLevel level) {
        get_current_level() = level;
    }

    template <typename... Args>
    static void Log(LogLevel level, std::string_view format, Args&&... args) {
        if (level < get_current_level()) {
            return;
        }
        // Print Prefix [DEBUG] :
        print_prefix(level);
        // Print Message
        process_format(format, std::forward<Args>(args)...);
        // Line Termination
        print_transport("\r\n");
    }

   private:
    static LogLevel& get_current_level() {
        static LogLevel current_level = LogLevel::INFO;
        return current_level;
    }

    static void print_prefix(LogLevel& level) {
        switch (level) {
            case LogLevel::DBG:
                print_transport("[DEBUG] ");
                break;
            case LogLevel::INFO:
                print_transport("[INFO]  ");
                break;
            case LogLevel::WARN:
                print_transport("[WARN]  ");
                break;
            case LogLevel::ERROR:
                print_transport("[ERROR] ");
                break;
            default:
                break;
        }
    }

    static void process_format(std::string_view format) {
        print_transport(format.data());
    }

    template <typename T, typename... Args>
    static void process_format(std::string_view format, T&& first, Args&&... rest) {
        size_t placeholder = format.find("{}");
        if (placeholder == std::string_view::npos) {
            print_transport(format.data());
            return;
        }

        print_transport(format.substr(0, placeholder));
        print_arguments(std::forward<T>(first));
        process_format(format.substr(placeholder + 2), std::forward<Args>(rest)...);
    }

    static void print_arguments(float_t val) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f", val);
        print_transport(buf);
    }

    static void print_arguments(uint32_t val) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%02u", (unsigned int)val);
        print_transport(buf);
    }

    static void print_arguments(uint16_t val) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%02u", (unsigned int)val);
        print_transport(buf);
    }

    static void print_arguments(Hex val) {
        char buf[16];
        snprintf(buf, sizeof(buf), "0x%08X", (unsigned int)val.value);
        print_transport(buf);
    }

    static void print_arguments(std::string_view val) {
        print_transport(val.data());
    }

    static void print_transport(const char* str) {
        UART2.Print(str, strlen(str));
    }

    static void print_transport(std::string_view str) {
        UART2.Print(str.data(), str.size());
    }
};

#endif