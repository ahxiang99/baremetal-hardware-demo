#pragma once
#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>

#include "crc_calculation.hpp"


enum class PacketType { VERSION_1 = 0x01U, VERSION_2 = 0x02U, INVALID = 0xFFU };

struct SensorData {
    float_t temperature;
    float_t humidity;
};

template <typename T>
class Packet {
    std::array<uint8_t, sizeof(T) + 4> m_bytes;

   public:
    explicit Packet(const T& data) {
        m_bytes[0] = 0xAAU;
        m_bytes[1] = 0x55U;
        m_bytes[2] = sizeof(T);
        std::memcpy(&m_bytes[3], &data, sizeof(T));
        m_bytes[3 + sizeof(T)] = crc_calculate(&m_bytes[3], sizeof(T));
    }

    const uint8_t* raw() const {
        return m_bytes.data();
    }

    size_t size() const {
        return m_bytes.size();
    }
};

template <typename T, PacketType pType>
class PacketV2 {
    std::array<uint8_t, sizeof(T) + 5> m_bytes;

   public:
    explicit PacketV2(const T& data) {
        m_bytes[0] = 0xAAU;
        m_bytes[1] = 0x55U;
        m_bytes[2] = static_cast<uint8_t>(pType);  // 0x01U for Version 1 and 0x02U for Version 2
        m_bytes[3] = sizeof(T);
        std::memcpy(&m_bytes[4], &data, sizeof(T));
        m_bytes[4 + sizeof(T)] = crc_calculate(&m_bytes[4], sizeof(T));
    }

    const uint8_t* raw() const {
        return m_bytes.data();
    }

    size_t size() const {
        return m_bytes.size();
    }
};

struct SensorPacketV1 {
    SensorData         m_data;

    static std::string header() {
        return "temperature,humidity\n";
    }

    std::string toCsv() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << m_data.temperature << ',' << m_data.humidity << '\n';
        return oss.str();
    }
};

struct SensorPacketV2 {
    float_t            temperature;

    static std::string header() {
        return "temperature\n";
    }

    std::string toCsv() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << temperature << ',' << '\n';
        return oss.str();
    }
};