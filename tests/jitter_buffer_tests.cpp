#include <gtest/gtest.h>

#include "mediaflow/event_bus.hpp"
#include "mediaflow/jitter_buffer.hpp"
#include "mediaflow/scenario_factory.hpp"

namespace {

mediaflow::Packet make_audio_packet(
    std::uint64_t packet_id,
    std::uint32_t sequence_number,
    std::uint64_t timestamp_ms,
    std::uint64_t frame_id,
    std::size_t packet_index = 0,
    std::size_t packets_in_frame = 1) {
  return mediaflow::Packet {
      "audio-main",
      packet_id,
      sequence_number,
      timestamp_ms,
      frame_id,
      180,
      packet_index,
      packets_in_frame,
      mediaflow::MediaType::Audio,
      timestamp_ms,
      timestamp_ms,
  };
}

TEST(JitterBufferTest, PlaysFrameAfterConfiguredDelay) {
  auto config = mediaflow::make_scenario("audio-only-stable");
  config.duration_ms = 200;
  config.jitter_buffer.target_delay_ms = 60;
  config.jitter_buffer.start_buffer_frames = 1;

  mediaflow::EventBus<mediaflow::SimulationEvent> bus;
  mediaflow::JitterBuffer buffer(config, bus);

  EXPECT_EQ(
      buffer.ingest_packet(make_audio_packet(1, 0, 0, 0), 10, 0.0).disposition,
      mediaflow::IngestDisposition::Buffered);
  EXPECT_TRUE(buffer.advance(69).empty());

  const auto results = buffer.advance(70);
  ASSERT_EQ(results.size(), 1U);
  ASSERT_TRUE(results.front().played);
  ASSERT_TRUE(results.front().frame.has_value());
  EXPECT_EQ(results.front().frame->playout_time_ms, 70U);
  EXPECT_EQ(results.front().frame->target_delay_ms, 60);
}

TEST(JitterBufferTest, AdaptiveDelayIncreasesAfterLatePacket) {
  auto config = mediaflow::make_scenario("audio-only-stable");
  config.duration_ms = 200;
  config.jitter_buffer.adaptive = true;
  config.jitter_buffer.target_delay_ms = 60;
  config.jitter_buffer.max_delay_ms = 120;
  config.jitter_buffer.start_buffer_frames = 1;

  mediaflow::EventBus<mediaflow::SimulationEvent> bus;
  mediaflow::JitterBuffer buffer(config, bus);

  EXPECT_EQ(
      buffer.ingest_packet(make_audio_packet(1, 0, 0, 0), 0, 0.0).disposition,
      mediaflow::IngestDisposition::Buffered);
  EXPECT_EQ(buffer.current_target_delay_ms("audio-main"), 60);

  EXPECT_EQ(
      buffer.ingest_packet(make_audio_packet(2, 1, 20, 1), 90, 25.0).disposition,
      mediaflow::IngestDisposition::LateDrop);
  EXPECT_GT(buffer.current_target_delay_ms("audio-main"), 60);
}

}  // namespace
