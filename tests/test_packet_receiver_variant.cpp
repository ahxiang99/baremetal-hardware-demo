#include <gtest/gtest.h>

#include "PacketReceiver.hpp"
#include "SensorPacket.hpp"

// Test 1: valid V1 packet — TYPE=0x01, correct data
TEST(PacketReceiverVariant, ValidV1PacketCompletesSuccessfully) {
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

// Test 2: valid V2 packet — TYPE=0x02, correct data
TEST(PacketReceiverVariant, ValidV2PacketCompletesSuccessfully) {
    SensorPacketV2                                  sensor{23.5f};
    PacketV2<SensorPacketV2, PacketType::VERSION_2> pkt(sensor);
    PacketReceiverVariant                           receiver;

    for (size_t i = 0; i < pkt.size(); ++i) {
        receiver.feed(pkt.raw()[i]);
    }
    EXPECT_TRUE(receiver.isComplete());
    auto p = std::get<SensorPacketV2>(receiver.packet());
    EXPECT_EQ(sensor.temperature, p.temperature);
}

// Test 3: invalid TYPE byte — receiver resets, not complete
TEST(PacketReceiverVariant, InvalidTypeResetsReceiver) {
    PacketReceiverVariant receiver;
    receiver.feed(0xAA);
    receiver.feed(0x55);
    receiver.feed(0xFF);
    EXPECT_FALSE(receiver.isComplete());
}

// Test 4: wrong CRC
TEST(PacketReceiverVariant, WrongCrcNotComplete) {
    SensorPacketV2                                  sensor{23.5f};
    PacketV2<SensorPacketV2, PacketType::VERSION_2> pkt(sensor);
    PacketReceiverVariant                           receiver;

    for (size_t i = 0; i < pkt.size() - 1; ++i) {
        receiver.feed(pkt.raw()[i]);
    }
    receiver.feed(0xFF);
    EXPECT_FALSE(receiver.isComplete());
}

// Test 5: double header edge case
TEST(PacketReceiverVariant, DoubleHeaderSyncsCorrectly) {
    SensorPacketV2                                  sensor{23.5f};
    PacketV2<SensorPacketV2, PacketType::VERSION_2> pkt(sensor);
    PacketReceiverVariant                           receiver;
    receiver.feed(0xAA);
    for (size_t i = 0; i < pkt.size(); ++i) {
        receiver.feed(pkt.raw()[i]);
    }
    EXPECT_TRUE(receiver.isComplete());
}