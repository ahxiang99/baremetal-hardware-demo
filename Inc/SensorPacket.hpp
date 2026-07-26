#pragma once
#include "Sht40ad1b.hpp"
#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>

#include "crc_calculation.hpp"

enum class PacketType {
	VERSION_0 = 0xB0U,
	VERSION_1 = 0x01U,
	VERSION_2 = 0x02U,
	INVALID = 0xFFU
};

struct __attribute__((packed)) Env_Sensor_Data {
	uint16_t last_rx_tick;
	uint16_t seq;
	int16_t temp_x100;
	int16_t rh_x100;
};

template <typename T, PacketType pType> class Packet
{
	std::array<uint8_t, sizeof(T) + 5> m_bytes;

      public:
	explicit Packet(const T &data)
	{
		m_bytes[0] = 0xAAU;
		m_bytes[1] = 0x55U;
		m_bytes[2] =
			static_cast<uint8_t>(pType); // 0x01U for Version 1 and 0x02U for Version 2
		m_bytes[3] = sizeof(T);
		std::memcpy(&m_bytes[4], &data, sizeof(T));
		m_bytes[4 + sizeof(T)] = crc_calculate(&m_bytes[4], sizeof(T));
	}

	const uint8_t *raw() const
	{
		return m_bytes.data();
	}

	size_t size() const
	{
		return m_bytes.size();
	}
};

struct SensorPacketV1 {
	Sht40ad1b::SensorData m_data;

	static std::string header()
	{
		return "temperature,humidity\n";
	}
};

struct SensorPacketV2 {
	float_t temperature;

	static std::string header()
	{
		return "temperature\n";
	}

	std::string toCsv() const
	{
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(2) << temperature << ',' << '\n';
		return oss.str();
	}
};
