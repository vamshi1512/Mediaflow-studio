#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "mediaflow/config.hpp"
#include "mediaflow/event_bus.hpp"
#include "mediaflow/jitter_buffer.hpp"
#include "mediaflow/metrics.hpp"

namespace mediaflow {

class Receiver {
 public:
  Receiver(
      const ScenarioConfig& config,
      MetricsCollector& metrics,
      EventBus<SimulationEvent>& bus);

  void reset();
  void receive(const Packet& packet, std::uint64_t arrival_time_ms);
  void advance(std::uint64_t now_ms);
  void flush(std::uint64_t now_ms);

  [[nodiscard]] std::size_t buffer_occupancy_packets() const;
  [[nodiscard]] std::string buffer_strategy_name() const;

 private:
  struct SequenceState {
    bool initialized {false};
    std::uint32_t highest_sequence {};
  };

  ScenarioConfig config_;
  MetricsCollector& metrics_;
  EventBus<SimulationEvent>& bus_;
  JitterBuffer jitter_buffer_;
  std::map<std::string, SequenceState> sequence_state_;
};

}  // namespace mediaflow
