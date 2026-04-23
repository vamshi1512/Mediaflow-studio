#include "mediaflow/metrics.hpp"

#include <algorithm>
#include <cmath>

namespace mediaflow {

namespace {

constexpr std::size_t kMaxRecentEvents = 180;

}  // namespace

MetricsCollector::MetricsCollector(EventBus<SimulationEvent>& bus) : bus_(bus) {
  bus_.subscribe([this](const SimulationEvent& event) { record_event(event); });
}

void MetricsCollector::reset() {
  summary_ = SummaryMetrics {};
  series_ = MetricSeries {};
  recent_events_.clear();
  playout_latencies_.clear();
  previous_transit_ms_.reset();
  received_bytes_ = 0;
  received_bytes_at_last_sample_ = 0;
  last_sample_time_ms_ = 0;
}

void MetricsCollector::on_frame_generated(const Frame&) { ++summary_.sent_frames; }

void MetricsCollector::on_packet_sent(const Packet&) { ++summary_.sent_packets; }

void MetricsCollector::on_packet_received(const Packet& packet, std::uint64_t arrival_time_ms) {
  ++summary_.received_packets;
  received_bytes_ += packet.payload_size_bytes;

  const double transit_ms =
      static_cast<double>(arrival_time_ms) - static_cast<double>(packet.send_time_ms);
  if (previous_transit_ms_.has_value()) {
    const double variation = std::fabs(transit_ms - previous_transit_ms_.value());
    summary_.jitter_estimate_ms += (variation - summary_.jitter_estimate_ms) / 16.0;
  }
  previous_transit_ms_ = transit_ms;
}

void MetricsCollector::on_frame_played(const PlayedFrame& frame, std::size_t packets_played) {
  ++summary_.played_frames;
  summary_.played_packets += packets_played;

  const double latency_ms =
      static_cast<double>(frame.playout_time_ms) - static_cast<double>(frame.generation_time_ms);
  playout_latencies_.push_back(latency_ms);

  const double previous_average = summary_.avg_latency_ms;
  const double sample_count = static_cast<double>(playout_latencies_.size());
  summary_.avg_latency_ms = previous_average + ((latency_ms - previous_average) / sample_count);
}

void MetricsCollector::on_frame_missed(
    const std::string& stream_id, std::uint64_t frame_id, std::uint64_t time_ms) {
  ++summary_.underflow_count;
  bus_.publish(SimulationEvent {
      time_ms,
      "underflow",
      Severity::Warning,
      "Playout underflow on " + stream_id,
      stream_id,
      std::nullopt,
      frame_id,
  });
}

void MetricsCollector::on_network_drop(
    const Packet& packet, std::uint64_t time_ms, const std::string& reason) {
  ++summary_.dropped_packets;
  ++summary_.network_drop_count;
  bus_.publish(SimulationEvent {
      time_ms,
      "network-drop",
      Severity::Warning,
      "Dropped packet " + std::to_string(packet.packet_id) + " due to " + reason,
      packet.stream_id,
      packet.packet_id,
      packet.frame_id,
  });
}

void MetricsCollector::on_late_packet(const Packet& packet, std::uint64_t time_ms) {
  ++summary_.dropped_packets;
  ++summary_.late_packets;
  bus_.publish(SimulationEvent {
      time_ms,
      "late-packet",
      Severity::Warning,
      "Late packet arrived after playout deadline",
      packet.stream_id,
      packet.packet_id,
      packet.frame_id,
  });
}

void MetricsCollector::on_buffer_overflow(const Packet& packet, std::uint64_t time_ms) {
  ++summary_.dropped_packets;
  ++summary_.overflow_count;
  bus_.publish(SimulationEvent {
      time_ms,
      "buffer-overflow",
      Severity::Warning,
      "Jitter buffer overflow rejected packet",
      packet.stream_id,
      packet.packet_id,
      packet.frame_id,
  });
}

void MetricsCollector::on_reorder(const Packet& packet, std::uint64_t time_ms) {
  ++summary_.reorder_count;
  bus_.publish(SimulationEvent {
      time_ms,
      "reorder",
      Severity::Info,
      "Receiver observed out-of-order packet delivery",
      packet.stream_id,
      packet.packet_id,
      packet.frame_id,
  });
}

void MetricsCollector::sample(std::uint64_t time_ms, std::size_t buffer_occupancy_packets) {
  series_.latency_ms.push_back(TimeSeriesPoint {time_ms, summary_.avg_latency_ms});
  series_.jitter_ms.push_back(TimeSeriesPoint {time_ms, summary_.jitter_estimate_ms});
  series_.buffer_occupancy_packets.push_back(
      TimeSeriesPoint {time_ms, static_cast<double>(buffer_occupancy_packets)});

  const double packet_loss_rate_pct =
      summary_.sent_packets == 0
          ? 0.0
          : (static_cast<double>(summary_.dropped_packets) / static_cast<double>(summary_.sent_packets)) *
                100.0;
  series_.packet_loss_pct.push_back(TimeSeriesPoint {time_ms, packet_loss_rate_pct});

  const std::uint64_t elapsed_ms = time_ms - last_sample_time_ms_;
  const std::uint64_t delta_bytes = received_bytes_ - received_bytes_at_last_sample_;
  double throughput_kbps = 0.0;
  if (elapsed_ms > 0) {
    throughput_kbps =
        (static_cast<double>(delta_bytes) * 8.0) / static_cast<double>(elapsed_ms);
  }
  series_.throughput_kbps.push_back(TimeSeriesPoint {time_ms, throughput_kbps});

  received_bytes_at_last_sample_ = received_bytes_;
  last_sample_time_ms_ = time_ms;
}

void MetricsCollector::finalize(std::uint64_t duration_ms) {
  summary_.duration_ms = duration_ms;
  summary_.packet_loss_rate_pct =
      summary_.sent_packets == 0
          ? 0.0
          : (static_cast<double>(summary_.dropped_packets) / static_cast<double>(summary_.sent_packets)) *
                100.0;
  summary_.p95_latency_ms = percentile95(playout_latencies_);
  summary_.playout_smoothness_pct =
      summary_.sent_frames == 0
          ? 100.0
          : (static_cast<double>(summary_.played_frames) / static_cast<double>(summary_.sent_frames)) *
                100.0;

  if (!series_.throughput_kbps.empty()) {
    double accumulated_throughput = 0.0;
    for (const auto& sample : series_.throughput_kbps) {
      accumulated_throughput += sample.value;
    }
    summary_.throughput_kbps =
        accumulated_throughput / static_cast<double>(series_.throughput_kbps.size());
  }
}

SummaryMetrics MetricsCollector::summary() const { return summary_; }

const MetricSeries& MetricsCollector::series() const { return series_; }

std::vector<SimulationEvent> MetricsCollector::recent_events() const {
  return std::vector<SimulationEvent>(recent_events_.begin(), recent_events_.end());
}

nlohmann::json MetricsCollector::series_json() const {
  const auto convert = [](const std::vector<TimeSeriesPoint>& points) {
    nlohmann::json values = nlohmann::json::array();
    for (const auto& point : points) {
      values.push_back({{"timeMs", point.time_ms}, {"value", point.value}});
    }
    return values;
  };

  return nlohmann::json{
      {"latencyMs", convert(series_.latency_ms)},
      {"jitterMs", convert(series_.jitter_ms)},
      {"packetLossPct", convert(series_.packet_loss_pct)},
      {"bufferOccupancyPackets", convert(series_.buffer_occupancy_packets)},
      {"throughputKbps", convert(series_.throughput_kbps)},
  };
}

void MetricsCollector::record_event(const SimulationEvent& event) {
  recent_events_.push_back(event);
  while (recent_events_.size() > kMaxRecentEvents) {
    recent_events_.pop_front();
  }
}

double MetricsCollector::percentile95(std::vector<double> samples) {
  if (samples.empty()) {
    return 0.0;
  }
  std::sort(samples.begin(), samples.end());
  const std::size_t index = static_cast<std::size_t>(
      std::ceil(static_cast<double>(samples.size()) * 0.95)) -
      1;
  return samples.at(index);
}

void to_json(nlohmann::json& json, const SummaryMetrics& metrics) {
  json = nlohmann::json{
      {"sentPackets", metrics.sent_packets},
      {"receivedPackets", metrics.received_packets},
      {"playedPackets", metrics.played_packets},
      {"sentFrames", metrics.sent_frames},
      {"playedFrames", metrics.played_frames},
      {"droppedPackets", metrics.dropped_packets},
      {"latePackets", metrics.late_packets},
      {"reorderCount", metrics.reorder_count},
      {"underflowCount", metrics.underflow_count},
      {"overflowCount", metrics.overflow_count},
      {"networkDropCount", metrics.network_drop_count},
      {"avgLatencyMs", metrics.avg_latency_ms},
      {"p95LatencyMs", metrics.p95_latency_ms},
      {"jitterEstimateMs", metrics.jitter_estimate_ms},
      {"packetLossRatePct", metrics.packet_loss_rate_pct},
      {"throughputKbps", metrics.throughput_kbps},
      {"playoutSmoothnessPct", metrics.playout_smoothness_pct},
      {"durationMs", metrics.duration_ms},
  };
}

}  // namespace mediaflow
