#include <numeric>

#include <gtest/gtest.h>

#include "mediaflow/sender.hpp"

namespace {

TEST(SenderTest, PacketizeSplitsPayloadAndMaintainsSequenceNumbers) {
  mediaflow::Sender sender;

  const mediaflow::Frame frame {
      "video-main",
      7,
      99,
      99,
      1'000,
      3,
      mediaflow::MediaType::Video,
  };

  const auto packets = sender.packetize(frame, 100);
  ASSERT_EQ(packets.size(), 3U);
  EXPECT_EQ(packets[0].packet_id, 0U);
  EXPECT_EQ(packets[1].packet_id, 1U);
  EXPECT_EQ(packets[2].packet_id, 2U);
  EXPECT_EQ(packets[0].sequence_number, 0U);
  EXPECT_EQ(packets[1].sequence_number, 1U);
  EXPECT_EQ(packets[2].sequence_number, 2U);
  EXPECT_EQ(packets[0].packet_index, 0U);
  EXPECT_EQ(packets[2].packet_index, 2U);
  EXPECT_EQ(packets[2].packets_in_frame, 3U);

  const auto total_payload = std::accumulate(
      packets.begin(),
      packets.end(),
      std::size_t {0},
      [](std::size_t total, const mediaflow::Packet& packet) {
        return total + packet.payload_size_bytes;
      });
  EXPECT_EQ(total_payload, 1'000U);

  const auto next_packets = sender.packetize(frame, 120);
  ASSERT_EQ(next_packets.size(), 3U);
  EXPECT_EQ(next_packets[0].sequence_number, 3U);
  EXPECT_EQ(next_packets[2].sequence_number, 5U);
}

}  // namespace
