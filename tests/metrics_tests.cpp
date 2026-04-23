#include <gtest/gtest.h>

#include "mediaflow/event_bus.hpp"
#include "mediaflow/metrics.hpp"

namespace {

TEST(MetricsTest, ComputesLatencyJitterAndThroughput) {
  mediaflow::EventBus<mediaflow::SimulationEvent> bus;
  mediaflow::MetricsCollector metrics(bus);

  const mediaflow::Packet packet_a {
      "audio-main", 1, 0, 0, 0, 180, 0, 1, mediaflow::MediaType::Audio, 0, 0};
  const mediaflow::Packet packet_b {
      "audio-main", 2, 1, 20, 1, 180, 0, 1, mediaflow::MediaType::Audio, 20, 20};

  metrics.on_packet_sent(packet_a);
  metrics.on_packet_received(packet_a, 40);
  metrics.on_packet_sent(packet_b);
  metrics.on_packet_received(packet_b, 75);

  metrics.on_frame_generated(mediaflow::Frame {"audio-main", 0, 0, 0, 180, 1, mediaflow::MediaType::Audio});
  metrics.on_frame_played(
      mediaflow::PlayedFrame {"audio-main", 0, 0, 0, 180, mediaflow::MediaType::Audio, 90, 60},
      1);

  metrics.sample(100, 3);
  metrics.finalize(100);

  const auto summary = metrics.summary();
  EXPECT_GT(summary.jitter_estimate_ms, 0.0);
  EXPECT_GT(summary.avg_latency_ms, 0.0);
  EXPECT_GT(summary.throughput_kbps, 0.0);
  EXPECT_EQ(summary.played_frames, 1U);
}

}  // namespace
