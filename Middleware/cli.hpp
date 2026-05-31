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
#include "cpp/II2C.hpp"
#include "cpp/IUart.hpp"
#include "cpp/InterruptI2C.hpp"
#include "cpp/systick.hpp"
#include "logger.hpp"

extern MySysTick              my_systick;

extern InterruptI2C           i2c1;

extern Sht40ad1b              temp_sensor;

constexpr std::array<char, 3> buf_newline{"\r\n"};
constexpr std::array<char, 4> buf_backspace{"\b \b"};
constexpr std::array<char, 5> buf_input{"CLI>"};

struct cmd {
    std::array<char, 64>  cmd_name;
    std::function<void()> callBack;
};

void get_temp();

enum class CliState { WaitingForInput, Processing, Executing };

class Cli {
   public:
    Cli(IUart& uart) : uart_(uart) {
        cmd_table = {
            {{{"help"}, [this]() { this->print_help(); }}, {{"get-temp"}, [&]() { temp_sensor.read(); }}}
        };
    }

    void     onUartData(const uint8_t* data, size_t len);
    void     executeCommand();
    void     get_input();

    CliState getState() const;
    void     setState(CliState&& state);

   private:
    IUart&                    uart_;
    RingBuffer<uint8_t, 1024> lineBuffer;
    CliState                  state_ = CliState::WaitingForInput;
    std::array<cmd, 4>        cmd_table;
    void                      echo();
    /* List of Command Available */
    void print_help();
};