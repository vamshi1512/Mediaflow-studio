#include "mediaflow/receiver.hpp"

#include <string>

namespace mediaflow {

Receiver::Receiver(
    const ScenarioConfig& config,
    MetricsCollector& metrics,
    EventBus<SimulationEvent>& bus)
    : config_(config), metrics_(metrics), bus_(bus), jitter_buffer_(config, bus) {
  reset();
}

void Receiver::reset() {
  sequence_state_.clear();
  jitter_buffer_.reset();
}

void Receiver::receive(const Packet& packet, std::uint64_t arrival_time_ms) {
  auto& state = sequence_state_[packet.stream_id];
  if (!state.initialized) {
    state.initialized = true;
    state.highest_sequence = packet.sequence_number;
  } else {
    if (packet.sequence_number < state.highest_sequence) {
      metrics_.on_reorder(packet, arrival_time_ms);
    } else if (packet.sequence_number > state.highest_sequence + 1) {
      bus_.publish(SimulationEvent {
          arrival_time_ms,
          "gap-observed",
          Severity::Info,
          "Receiver observed sequence gap before packet " + std::to_string(packet.sequence_number),
          packet.stream_id,
          packet.packet_id,
          packet.frame_id,
      });
    }
    state.highest_sequence = std::max(state.highest_sequence, packet.sequence_number);
  }

  metrics_.on_packet_received(packet, arrival_time_ms);
  const double jitter_estimate_ms = metrics_.summary().jitter_estimate_ms;
  const auto ingest_result = jitter_buffer_.ingest_packet(packet, arrival_time_ms, jitter_estimate_ms);
  switch (ingest_result.disposition) {
    case IngestDisposition::Buffered:
      return;
    case IngestDisposition::LateDrop:
      metrics_.on_late_packet(packet, arrival_time_ms);
      return;
    case IngestDisposition::OverflowDrop:
      metrics_.on_buffer_overflow(packet, arrival_time_ms);
      return;
  }
}

void Receiver::advance(std::uint64_t now_ms) {
  for (const auto& result : jitter_buffer_.advance(now_ms)) {
    if (result.played && result.frame.has_value()) {
      metrics_.on_frame_played(result.frame.value(), result.packets_played);
    } else {
      metrics_.on_frame_missed(result.stream_id, result.frame_id, now_ms);
    }
  }
}

void Receiver::flush(std::uint64_t now_ms) {
  for (const auto& result : jitter_buffer_.flush_remaining(now_ms)) {
    if (result.played && result.frame.has_value()) {
      metrics_.on_frame_played(result.frame.value(), result.packets_played);
    } else {
      metrics_.on_frame_missed(result.stream_id, result.frame_id, now_ms);
    }
  }
}

std::size_t Receiver::buffer_occupancy_packets() const { return jitter_buffer_.occupancy_packets(); }

std::string Receiver::buffer_strategy_name() const { return jitter_buffer_.strategy_name(); }

}  // namespace mediaflow
