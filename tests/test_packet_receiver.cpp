#include <gtest/gtest.h>

#include "PacketReceiver.hpp"
#include "SensorPacket.hpp"

// Test 1: valid packet — feed all bytes, isComplete() returns true
TEST(PacketReceiver, ValidPacketCompletesSuccessfully) {
    SensorPacketV1                                  sensor{23.5f, 65.2f};
    PacketV2<SensorPacketV1, PacketType::VERSION_1> pkt(sensor);
    PacketReceiverVariant                           receiver;

    for (size_t i = 0; i < pkt.size(); ++i) {
        receiver.feed(pkt.raw()[i]);
    }

    EXPECT_TRUE(receiver.isComplete());
    auto p = std::get<SensorPacketV1>(receiver.packet());

    EXPECT_EQ(sensor.m_data.temperature, p.m_data.temperature);
    EXPECT_EQ(sensor.m_data.humidity, p.m_data.humidity);
}

// Test 2: wrong CRC — isComplete() returns false
TEST(PacketReceiver, WrongCrcNotComplete) {
    SensorPacketV1                                  sensor{23.5f, 65.2f};
    PacketV2<SensorPacketV1, PacketType::VERSION_1> pkt(sensor);
    PacketReceiverVariant                           receiver;

    for (size_t i = 0; i < pkt.size() - 1; ++i) {
        receiver.feed(pkt.raw()[i]);
    }
    receiver.feed(0xFF);  // wrong CRC
    EXPECT_FALSE(receiver.isComplete());
}

// Test 3: sync bytes only — incomplete, not complete
TEST(PacketReceiver, PartialPacketNotComplete) {
    PacketReceiverVariant receiver;
    receiver.feed(0xAA);
    receiver.feed(0x55);

    EXPECT_FALSE(receiver.isComplete());
}

// Test 4: 0xAA 0xAA 0x55 edge case — still syncs correctly
TEST(PacketReceiver, DoubleHeaderSyncsCorrectly) {
    SensorPacketV1                                  sensor{23.5f, 65.2f};
    PacketV2<SensorPacketV1, PacketType::VERSION_1> pkt(sensor);
    PacketReceiverVariant                           receiver;
    receiver.feed(0xAA);
    for (size_t i = 0; i < pkt.size(); ++i) {
        receiver.feed(pkt.raw()[i]);
    }
    EXPECT_TRUE(receiver.isComplete());
}