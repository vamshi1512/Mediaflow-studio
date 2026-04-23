#include <gtest/gtest.h>

#include "mediaflow/event_bus.hpp"
#include "mediaflow/metrics.hpp"
#include "mediaflow/receiver.hpp"
#include "mediaflow/scenario_factory.hpp"

namespace {

mediaflow::Packet make_packet(
    std::uint64_t packet_id,
    std::uint32_t sequence_number,
    std::uint64_t timestamp_ms,
    std::uint64_t frame_id,
    std::size_t packet_index,
    std::size_t packets_in_frame) {
  return mediaflow::Packet {
      "audio-main",
      packet_id,
      sequence_number,
      timestamp_ms,
      frame_id,
      120,
      packet_index,
      packets_in_frame,
      mediaflow::MediaType::Audio,
      timestamp_ms,
      timestamp_ms,
  };
}

TEST(ReceiverTest, MissingPacketCausesUnderflowWhenDeadlinePasses) {
  auto config = mediaflow::make_scenario("audio-only-stable");
  config.duration_ms = 200;
  config.audio_stream.packets_per_frame = 2;
  config.jitter_buffer.target_delay_ms = 40;
  config.jitter_buffer.start_buffer_frames = 1;

  mediaflow::EventBus<mediaflow::SimulationEvent> bus;
  mediaflow::MetricsCollector metrics(bus);
  mediaflow::Receiver receiver(config, metrics, bus);

  receiver.receive(make_packet(1, 0, 0, 0, 0, 2), 0);
  receiver.advance(40);

  EXPECT_EQ(metrics.summary().underflow_count, 1U);
}

TEST(ReceiverTest, LatePacketIsDroppedBeforeBuffering) {
  auto config = mediaflow::make_scenario("audio-only-stable");
  config.duration_ms = 200;
  config.jitter_buffer.target_delay_ms = 40;
  config.jitter_buffer.start_buffer_frames = 1;

  mediaflow::EventBus<mediaflow::SimulationEvent> bus;
  mediaflow::MetricsCollector metrics(bus);
  mediaflow::Receiver receiver(config, metrics, bus);

  receiver.receive(make_packet(1, 0, 0, 0, 0, 1), 0);
  receiver.receive(make_packet(2, 1, 20, 1, 0, 1), 70);

  EXPECT_EQ(metrics.summary().late_packets, 1U);
}

TEST(ReceiverTest, DetectsOutOfOrderPacketArrival) {
  auto config = mediaflow::make_scenario("audio-only-stable");
  config.duration_ms = 200;
  config.audio_stream.packets_per_frame = 2;
  config.jitter_buffer.target_delay_ms = 40;
  config.jitter_buffer.start_buffer_frames = 1;

  mediaflow::EventBus<mediaflow::SimulationEvent> bus;
  mediaflow::MetricsCollector metrics(bus);
  mediaflow::Receiver receiver(config, metrics, bus);

  receiver.receive(make_packet(2, 1, 0, 0, 1, 2), 0);
  receiver.receive(make_packet(1, 0, 0, 0, 0, 2), 1);
  receiver.advance(40);

  EXPECT_EQ(metrics.summary().reorder_count, 1U);
  EXPECT_EQ(metrics.summary().played_frames, 1U);
}

}  // namespace
