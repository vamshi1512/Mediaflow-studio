#include "mediaflow/simulation_engine.hpp"

#include "mediaflow/logger.hpp"

namespace mediaflow {

namespace {

nlohmann::json event_to_json(const SimulationEvent& event) {
  nlohmann::json json = {
      {"timeMs", event.time_ms},
      {"kind", event.kind},
      {"severity", to_string(event.severity)},
      {"message", event.message},
      {"streamId", event.stream_id},
  };
  json["packetId"] = event.packet_id.has_value() ? nlohmann::json(event.packet_id.value()) : nlohmann::json(nullptr);
  json["frameId"] = event.frame_id.has_value() ? nlohmann::json(event.frame_id.value()) : nlohmann::json(nullptr);
  return json;
}

}  // namespace

SimulationEngine::SimulationEngine()
    : metrics_(bus_) {
  rebuild_components();
}

void SimulationEngine::load(const ScenarioConfig& config, std::uint64_t seed) {
  validate_config(config);
  config_ = config;
  seed_ = seed;
  config_.seed = seed_;
  rebuild_components();
  status_ = SimulationStatus::Idle;
}

void SimulationEngine::start() {
  if (status_ == SimulationStatus::Running) {
    return;
  }
  if (status_ == SimulationStatus::Paused) {
    resume();
    return;
  }
  if (status_ == SimulationStatus::Completed) {
    reset();
  }
  status_ = SimulationStatus::Running;
  publish_event(SimulationEvent {
      current_time_ms_,
      "simulation-started",
      Severity::Info,
      "Simulation started",
      "",
      std::nullopt,
      std::nullopt,
  });
}

void SimulationEngine::pause() {
  if (status_ == SimulationStatus::Running) {
    status_ = SimulationStatus::Paused;
  }
}

void SimulationEngine::resume() {
  if (status_ == SimulationStatus::Paused) {
    status_ = SimulationStatus::Running;
  }
}

void SimulationEngine::reset() {
  rebuild_components();
  status_ = SimulationStatus::Idle;
}

void SimulationEngine::step(std::uint64_t delta_ms) {
  if (status_ != SimulationStatus::Running) {
    return;
  }

  for (std::uint64_t step_index = 0; step_index < delta_ms; ++step_index) {
    if (current_time_ms_ < static_cast<std::uint64_t>(config_.duration_ms)) {
      for (const auto& frame : frame_generator_->generate_due_frames(
               current_time_ms_, static_cast<std::uint64_t>(config_.duration_ms))) {
        metrics_.on_frame_generated(frame);
        for (const auto& packet : sender_.packetize(frame, current_time_ms_)) {
          metrics_.on_packet_sent(packet);
          const auto result = network_->enqueue_packet(packet, current_time_ms_);
          if (!result.accepted) {
            metrics_.on_network_drop(packet, current_time_ms_, result.drop_reason.value_or("unknown"));
          } else if (result.reordered_hold_applied) {
            publish_event(SimulationEvent {
                current_time_ms_,
                "network-reorder-hold",
                Severity::Info,
                "Applied extra hold to increase reorder likelihood",
                packet.stream_id,
                packet.packet_id,
                packet.frame_id,
            });
          }
        }
      }
    }

    for (const auto& packet : network_->poll_arrivals(current_time_ms_)) {
      receiver_->receive(packet, current_time_ms_);
    }
    receiver_->advance(current_time_ms_);

    if (current_time_ms_ % static_cast<std::uint64_t>(config_.sample_interval_ms) == 0) {
      metrics_.sample(current_time_ms_, receiver_->buffer_occupancy_packets());
    }

    if (current_time_ms_ >= final_tick_ms()) {
      complete();
      break;
    }

    ++current_time_ms_;
  }
}

void SimulationEngine::run_to_completion() {
  start();
  while (status_ == SimulationStatus::Running) {
    step(1);
  }
}

SimulationStatus SimulationEngine::status() const { return status_; }

std::uint64_t SimulationEngine::seed() const { return seed_; }

std::uint64_t SimulationEngine::current_time_ms() const { return current_time_ms_; }

const ScenarioConfig& SimulationEngine::config() const { return config_; }

SummaryMetrics SimulationEngine::summary() const { return metrics_.summary(); }

std::string SimulationEngine::printable_summary() const {
  return format_summary(config_.name, seed_, metrics_.summary());
}

nlohmann::json SimulationEngine::snapshot_json() const {
  nlohmann::json events = nlohmann::json::array();
  for (const auto& event : metrics_.recent_events()) {
    events.push_back(event_to_json(event));
  }

  return nlohmann::json{
      {"status", to_string(status_)},
      {"scenarioName", config_.name},
      {"scenarioDescription", config_.description},
      {"seed", seed_},
      {"simulationTimeMs", current_time_ms_},
      {"durationMs", config_.duration_ms},
      {"bufferStrategy", receiver_->buffer_strategy_name()},
      {"summary", metrics_.summary()},
      {"series", metrics_.series_json()},
      {"recentEvents", events},
      {"config", config_},
  };
}

nlohmann::json SimulationEngine::export_json() const {
  auto snapshot = snapshot_json();
  snapshot["exportedAtMs"] = current_time_ms_;
  snapshot["final"] = status_ == SimulationStatus::Completed;
  return snapshot;
}

void SimulationEngine::rebuild_components() {
  current_time_ms_ = 0;
  rng_.reseed(seed_);
  metrics_.reset();
  frame_generator_ = std::make_unique<FrameGenerator>(config_);
  sender_.reset();
  network_ = std::make_unique<NetworkSimulator>(config_.network, rng_);
  receiver_ = std::make_unique<Receiver>(config_, metrics_, bus_);
}

void SimulationEngine::complete() {
  current_time_ms_ = final_tick_ms();
  receiver_->advance(current_time_ms_);
  receiver_->flush(current_time_ms_);
  metrics_.sample(current_time_ms_, receiver_->buffer_occupancy_packets());
  metrics_.finalize(static_cast<std::uint64_t>(config_.duration_ms));
  status_ = SimulationStatus::Completed;
  publish_event(SimulationEvent {
      current_time_ms_,
      "simulation-complete",
      Severity::Info,
      "Simulation completed",
      "",
      std::nullopt,
      std::nullopt,
  });
}

void SimulationEngine::publish_event(const SimulationEvent& event) { bus_.publish(event); }

std::uint64_t SimulationEngine::final_tick_ms() const {
  const std::uint64_t network_margin =
      static_cast<std::uint64_t>(config_.network.base_latency_ms + config_.network.jitter_ms +
                                 config_.network.reorder_hold_ms + config_.network.queue_pressure_drop_threshold_ms);
  const std::uint64_t buffer_margin =
      static_cast<std::uint64_t>(config_.jitter_buffer.max_delay_ms + 1'000);
  return static_cast<std::uint64_t>(config_.duration_ms) + network_margin + buffer_margin;
}

}  // namespace mediaflow
