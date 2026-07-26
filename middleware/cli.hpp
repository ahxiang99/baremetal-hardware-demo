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
#include "cpp/InterruptI2C.hpp"
#include "cpp/Stm32I2C.hpp"
#include "cpp/UartRef.hpp"
#include "cpp/systick.hpp"
#include "logger.hpp"

constexpr std::string_view buf_newline{"\r\n"};
constexpr std::string_view buf_backspace{"\b \b"};
constexpr std::string_view buf_input{"CLI>"};

using CmdFn = void (*)(void *);

struct cmd {
	const char *cmd_name;
	CmdFn fn;
	void *ctx;
};

enum class CliState {
	WaitingForInput,
	Processing,
	Executing,
	Completed
};

class Cli
{
      public:
	Cli()
	{
		cmd_table = {
			{{"help", [](void *ctx) { static_cast<Cli *>(ctx)->print_help(); }, this},
			 {"get-temp", [](void *ctx) { static_cast<Cli *>(ctx)->cmd_get_temp(); },
			  this}}};
	}
	void setUart(UartRef uart)
	{
		uart_ = uart;
	}

	void setSensor(Sht40ad1b *sensor)
	{
		sensor_ = sensor;
	}

	void onUartData(const uint8_t *data, size_t len)
	{
		for (size_t i = 0; i < len; ++i) {
			char c = static_cast<char>(data[i]);
			switch (c) {
			case '\r':
			case '\n':
				if (lineBuffer.size() > 0) {
					lineBuffer.push('\0');
					uart_.send({reinterpret_cast<const uint8_t *>(
							    buf_newline.data()),
						    buf_newline.size()});
					executeCommand();
				}
				return;
			case '\b':
			case 0x7F:
				if (lineBuffer.size() > 0) {
					lineBuffer.remove_last();
					uart_.send({reinterpret_cast<const uint8_t *>(
							    buf_backspace.data()),
						    buf_backspace.size()});
				}
				break;
			default:
				lineBuffer.push(c); // Push to buffer
				uart_.send({reinterpret_cast<const uint8_t *>(&c),
					    1}); // Echo to the console
			}
		}
	}

	void executeCommand()
	{
		std::array<char, 256> buffer;
		size_t idx = 0;
		while (lineBuffer.size() > 0 && idx < 256) {
			buffer[idx++] = lineBuffer.pop().value();
		}

		state_ = CliState::Executing;

		bool found = false;
		for (const auto &entry : cmd_table) {
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
	void get_input()
	{
		uart_.send({reinterpret_cast<const uint8_t *>(buf_input.data()), buf_input.size()});
	}

	CliState getState() const
	{
		return state_;
	}
	void setState(CliState state)
	{
		state_ = state;
	}

	void onDataReceived()
	{
		setState(CliState::Completed);
	}

	void read()
	{
	}

      private:
	UartRef uart_;
	Sht40ad1b *sensor_ = nullptr;
	RingBuffer<uint8_t, 1024> lineBuffer;
	CliState state_ = CliState::WaitingForInput;
	std::array<cmd, 4> cmd_table;

	void echo()
	{
		char buf[1024];
		// Echo to Console
		for (size_t i = 0; i < lineBuffer.size(); ++i) {
			auto temp = lineBuffer.pop();
			if (temp.has_value()) {
				buf[i] = temp.value();
			}
		}
		LOG_PRINT(buf);
		LOG_PRINT(buf_newline);
		// Print >
		get_input();
	}

	/* List of Command Available */
	void print_help()
	{
		LOG_PRINT("Available Commands:");
		LOG_PRINT("help     : Show command list");
		LOG_PRINT("get-temp : Get Temperature");
		state_ = CliState::WaitingForInput;
	}

	void cmd_get_temp()
	{
		if (sensor_ == nullptr) {
			return;
		}
		sensor_->read();
	}
};
