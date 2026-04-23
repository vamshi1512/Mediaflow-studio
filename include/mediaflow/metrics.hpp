#pragma once

#include <deque>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "mediaflow/event_bus.hpp"
#include "mediaflow/types.hpp"

namespace mediaflow {

struct SummaryMetrics {
  std::uint64_t sent_packets {};
  std::uint64_t received_packets {};
  std::uint64_t played_packets {};
  std::uint64_t sent_frames {};
  std::uint64_t played_frames {};
  std::uint64_t dropped_packets {};
  std::uint64_t late_packets {};
  std::uint64_t reorder_count {};
  std::uint64_t underflow_count {};
  std::uint64_t overflow_count {};
  std::uint64_t network_drop_count {};
  double avg_latency_ms {};
  double p95_latency_ms {};
  double jitter_estimate_ms {};
  double packet_loss_rate_pct {};
  double throughput_kbps {};
  double playout_smoothness_pct {};
  std::uint64_t duration_ms {};
};

struct MetricSeries {
  std::vector<TimeSeriesPoint> latency_ms;
  std::vector<TimeSeriesPoint> jitter_ms;
  std::vector<TimeSeriesPoint> packet_loss_pct;
  std::vector<TimeSeriesPoint> buffer_occupancy_packets;
  std::vector<TimeSeriesPoint> throughput_kbps;
};

class MetricsCollector {
 public:
  explicit MetricsCollector(EventBus<SimulationEvent>& bus);

  void reset();

  void on_frame_generated(const Frame& frame);
  void on_packet_sent(const Packet& packet);
  void on_packet_received(const Packet& packet, std::uint64_t arrival_time_ms);
  void on_frame_played(const PlayedFrame& frame, std::size_t packets_played);
  void on_frame_missed(const std::string& stream_id, std::uint64_t frame_id, std::uint64_t time_ms);
  void on_network_drop(const Packet& packet, std::uint64_t time_ms, const std::string& reason);
  void on_late_packet(const Packet& packet, std::uint64_t time_ms);
  void on_buffer_overflow(const Packet& packet, std::uint64_t time_ms);
  void on_reorder(const Packet& packet, std::uint64_t time_ms);
  void sample(std::uint64_t time_ms, std::size_t buffer_occupancy_packets);
  void finalize(std::uint64_t duration_ms);

  [[nodiscard]] SummaryMetrics summary() const;
  [[nodiscard]] const MetricSeries& series() const;
  [[nodiscard]] std::vector<SimulationEvent> recent_events() const;
  [[nodiscard]] nlohmann::json series_json() const;

 private:
  EventBus<SimulationEvent>& bus_;

  SummaryMetrics summary_ {};
  MetricSeries series_ {};
  std::deque<SimulationEvent> recent_events_ {};
  std::vector<double> playout_latencies_ {};
  std::optional<double> previous_transit_ms_ {};
  std::uint64_t received_bytes_ {};
  std::uint64_t received_bytes_at_last_sample_ {};
  std::uint64_t last_sample_time_ms_ {};

  void record_event(const SimulationEvent& event);
  static double percentile95(std::vector<double> samples);
};

void to_json(nlohmann::json& json, const SummaryMetrics& metrics);

}  // namespace mediaflow
