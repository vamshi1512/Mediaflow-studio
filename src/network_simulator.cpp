#include "mediaflow/network_simulator.hpp"

#include <cmath>
#include <cstdint>
#include <utility>

namespace mediaflow {

NetworkSimulator::NetworkSimulator(const NetworkConfig& config, DeterministicRng& rng)
    : config_(config), rng_(rng) {}

void NetworkSimulator::reset() {
  pending_ = {};
  next_link_free_time_ms_ = 0;
  burst_drop_remaining_ = 0;
  insertion_counter_ = 0;
  max_arrival_time_ms_ = 0;
}

NetworkSubmitResult NetworkSimulator::enqueue_packet(const Packet& packet, std::uint64_t now_ms) {
  if (burst_drop_remaining_ > 0) {
    --burst_drop_remaining_;
    return NetworkSubmitResult {false, false, "burst-loss"};
  }

  if (config_.burst_loss_length > 0 && rng_.bernoulli(config_.burst_loss_trigger_rate)) {
    burst_drop_remaining_ = config_.burst_loss_length - 1;
    return NetworkSubmitResult {false, false, "burst-loss"};
  }

  if (rng_.bernoulli(config_.packet_loss_rate)) {
    return NetworkSubmitResult {false, false, "random-loss"};
  }

  const std::uint64_t transmission_time = transmission_time_ms(packet.payload_size_bytes);
  const std::uint64_t tx_start_time_ms = std::max(now_ms, next_link_free_time_ms_);
  const std::uint64_t queue_delay_ms = tx_start_time_ms - now_ms;

  if (pending_.size() >= static_cast<std::size_t>(config_.queue_capacity_packets)) {
    return NetworkSubmitResult {false, false, "queue-capacity"};
  }
  if (queue_delay_ms > static_cast<std::uint64_t>(config_.queue_pressure_drop_threshold_ms)) {
    return NetworkSubmitResult {false, false, "queue-pressure"};
  }

  next_link_free_time_ms_ = tx_start_time_ms + transmission_time;

  const int jitter_offset_ms =
      config_.jitter_ms > 0 ? rng_.uniform_int(-config_.jitter_ms, config_.jitter_ms) : 0;
  const int transit_time_ms = std::max(1, config_.base_latency_ms + jitter_offset_ms);

  bool reordered_hold_applied = false;
  std::uint64_t reorder_hold_ms = 0;
  if (config_.reorder_hold_ms > 0 && rng_.bernoulli(config_.reorder_rate)) {
    reorder_hold_ms = static_cast<std::uint64_t>(rng_.uniform_int(1, config_.reorder_hold_ms));
    reordered_hold_applied = true;
  }

  const std::uint64_t arrival_time_ms =
      next_link_free_time_ms_ + static_cast<std::uint64_t>(transit_time_ms) + reorder_hold_ms;

  pending_.push(PendingPacket {
      arrival_time_ms,
      insertion_counter_++,
      packet,
  });
  max_arrival_time_ms_ = std::max(max_arrival_time_ms_, arrival_time_ms);

  return NetworkSubmitResult {true, reordered_hold_applied, std::nullopt};
}

std::vector<Packet> NetworkSimulator::poll_arrivals(std::uint64_t now_ms) {
  std::vector<Packet> arrivals;
  while (!pending_.empty() && pending_.top().arrival_time_ms <= now_ms) {
    arrivals.push_back(pending_.top().packet);
    pending_.pop();
  }
  return arrivals;
}

std::size_t NetworkSimulator::queue_depth() const { return pending_.size(); }

std::uint64_t NetworkSimulator::drain_horizon_ms() const { return max_arrival_time_ms_; }

std::uint64_t NetworkSimulator::transmission_time_ms(std::size_t payload_size_bytes) const {
  const double bandwidth_kbps = static_cast<double>(std::max(1, config_.bandwidth_kbps));
  const double bits = static_cast<double>(payload_size_bytes) * 8.0;
  return static_cast<std::uint64_t>(std::max(1.0, std::ceil(bits / bandwidth_kbps)));
}

}  // namespace mediaflow
