#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <nlohmann/json.hpp>

#include "mediaflow/config.hpp"
#include "mediaflow/event_bus.hpp"
#include "mediaflow/frame_generator.hpp"
#include "mediaflow/metrics.hpp"
#include "mediaflow/network_simulator.hpp"
#include "mediaflow/random.hpp"
#include "mediaflow/receiver.hpp"
#include "mediaflow/sender.hpp"
#include "mediaflow/types.hpp"

namespace mediaflow {

class SimulationEngine {
 public:
  SimulationEngine();

  void load(const ScenarioConfig& config, std::uint64_t seed);
  void start();
  void pause();
  void resume();
  void reset();
  void step(std::uint64_t delta_ms);
  void run_to_completion();

  [[nodiscard]] SimulationStatus status() const;
  [[nodiscard]] std::uint64_t seed() const;
  [[nodiscard]] std::uint64_t current_time_ms() const;
  [[nodiscard]] const ScenarioConfig& config() const;
  [[nodiscard]] SummaryMetrics summary() const;
  [[nodiscard]] std::string printable_summary() const;
  [[nodiscard]] nlohmann::json snapshot_json() const;
  [[nodiscard]] nlohmann::json export_json() const;

 private:
  void rebuild_components();
  void complete();
  void publish_event(const SimulationEvent& event);
  [[nodiscard]] std::uint64_t final_tick_ms() const;

  EventBus<SimulationEvent> bus_ {};
  MetricsCollector metrics_;
  ScenarioConfig config_ {};
  std::uint64_t seed_ {42};
  DeterministicRng rng_ {42};
  std::unique_ptr<FrameGenerator> frame_generator_;
  Sender sender_ {};
  std::unique_ptr<NetworkSimulator> network_;
  std::unique_ptr<Receiver> receiver_;
  SimulationStatus status_ {SimulationStatus::Idle};
  std::uint64_t current_time_ms_ {0};
};

}  // namespace mediaflow
