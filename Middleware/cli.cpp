#include "cli.hpp"

#include <cstring>

void Cli::print_help() {
    LOG_PRINT("Available Commands:");
    LOG_PRINT("help     : Show command list");
    LOG_PRINT("get-temp : Get Temperature");
    setState(CliState::WaitingForInput);
}

void Cli::onUartData(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        char c = static_cast<char>(data[i]);
        switch (c) {
            case '\r':
            case '\n':
                if (lineBuffer.size() > 0) {
                    lineBuffer.push('\0');
                    uart_->send(reinterpret_cast<const uint8_t*>(buf_newline.data()), buf_newline.size());
                    executeCommand();
                }
                return;
            case '\b':
            case 0x7F:
                if (lineBuffer.size() > 0) {
                    lineBuffer.remove_last();
                    uart_->send(reinterpret_cast<const uint8_t*>(buf_backspace.data()), buf_backspace.size());
                }
                break;
            default:
                lineBuffer.push(c);                                    // Push to buffer
                uart_->send(reinterpret_cast<const uint8_t*>(&c), 1);  // Echo to the console
        }
    }
}

void Cli::executeCommand() {
    std::array<char, 256> buffer;
    size_t                idx = 0;
    while (lineBuffer.size() > 0 && idx < 256) {
        buffer[idx++] = lineBuffer.pop().value();
    }

    state_     = CliState::Executing;

    bool found = false;
    for (const auto& entry : cmd_table) {
        if (entry.cmd_name && std::strcmp(entry.cmd_name, buffer.data()) == 0) {
            entry.fn(entry.ctx);
            found = true;
            break;
        }
    }

    if (!found) {
        LOG_PRINT("Error: Command not found.");
        state_ = CliState::WaitingForInput;
    }
}

void Cli::get_input() {
    uart_->send(reinterpret_cast<const uint8_t*>(buf_input.data()), buf_input.size());
}

void Cli::echo() {
    // Echo to Console
    uint32_t                  transfer_length = lineBuffer.size();
    std::array<uint8_t, 1024> transferBuffer;
    for (size_t i = 0; i < transfer_length; ++i) {
        auto temp = lineBuffer.pop();
        if (temp.has_value()) {
            transferBuffer.at(i) = temp.value();
        }
    }
    uart_->send(transferBuffer.data(), transfer_length);
    uart_->send(reinterpret_cast<const uint8_t*>(buf_newline.data()), buf_newline.size());
    transferBuffer.fill(0);
    // Print >
    get_input();
}

CliState Cli::getState() const {
    return state_;
}
void Cli::setState(CliState&& state) {
    state_ = state;
}
