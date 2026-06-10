#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>

#include "RingBuffer.hpp"
#include "Sht40ad1b.hpp"
#include "cpp/DmaI2C.hpp"
#include "cpp/II2C.hpp"
#include "cpp/IUart.hpp"
#include "cpp/Stm32I2C.hpp"
#include "cpp/systick.hpp"
#include "logger.hpp"

constexpr std::array<char, 3> buf_newline{"\r\n"};
constexpr std::array<char, 4> buf_backspace{"\b \b"};
constexpr std::array<char, 5> buf_input{"CLI>"};

using CmdFn = void (*)(void*);

struct cmd {
    const char* cmd_name;
    CmdFn       fn;
    void*       ctx;
};

enum class CliState { WaitingForInput, Processing, Executing };

class Cli {
   public:
    Cli() {
        cmd_table = {
            {
             {"help", [](void* ctx) { static_cast<Cli*>(ctx)->print_help(); }, this},
             {"get-temp", [](void* ctx) { static_cast<Cli*>(ctx)->cmd_get_temp(); }, this},
             }
        };
    }
    void setUart(IUart* uart) {
        uart_ = uart;
    }
    void setSensor(Sht40ad1b* sensor) {
        sensor_ = sensor;
    }
    void     onUartData(const uint8_t* data, size_t len);
    void     executeCommand();
    void     get_input();

    CliState getState() const;
    void     setState(CliState&& state);

   private:
    IUart*                    uart_   = nullptr;
    Sht40ad1b*                sensor_ = nullptr;
    RingBuffer<uint8_t, 1024> lineBuffer;
    CliState                  state_ = CliState::WaitingForInput;
    std::array<cmd, 4>        cmd_table;
    void                      echo();
    /* List of Command Available */
    void print_help();
    void cmd_get_temp();
};