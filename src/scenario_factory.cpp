#include "mediaflow/scenario_factory.hpp"

#include <stdexcept>

namespace mediaflow {

namespace {

ScenarioConfig base_av(const std::string& name, const std::string& description) {
  ScenarioConfig config;
  config.name = name;
  config.description = description;
  config.duration_ms = 10'000;
  config.sample_interval_ms = 100;
  config.seed = 42;
  config.audio_stream = StreamConfig {true, "audio-main", MediaType::Audio, 20, 240, 1};
  config.video_stream = StreamConfig {true, "video-main", MediaType::Video, 33, 8'400, 7};
  config.network = NetworkConfig {38, 2, 0.0, 0.0, 0, 0.0, 0, 2'500, 300, 360};
  config.jitter_buffer = JitterBufferConfig {90, 50, 260, 640, false, 2};
  return config;
}

ScenarioConfig stable() {
  auto config = base_av(
      "stable",
      "Balanced audio/video path with low impairment and predictable playout.");
  return config;
}

ScenarioConfig moderate_jitter() {
  auto config = base_av(
      "moderate-jitter",
      "Adds visible jitter while keeping loss near zero so the buffer absorbs timing variation.");
  config.network.jitter_ms = 24;
  config.network.base_latency_ms = 55;
  return config;
}

ScenarioConfig high_jitter() {
  auto config = base_av(
      "high-jitter",
      "Stress case with heavy arrival variance and frequent jitter buffer target adjustments.");
  config.network.jitter_ms = 52;
  config.network.base_latency_ms = 62;
  config.jitter_buffer.adaptive = true;
  config.jitter_buffer.target_delay_ms = 110;
  config.jitter_buffer.max_delay_ms = 320;
  return config;
}

ScenarioConfig mild_loss() {
  auto config = base_av(
      "mild-loss",
      "Injects small random loss that should dent quality without collapsing playout.");
  config.network.packet_loss_rate = 0.025;
  return config;
}

ScenarioConfig burst_loss() {
  auto config = base_av(
      "burst-loss",
      "Uses burst loss to mimic transient Wi-Fi impairment and force visible underflows.");
  config.network.packet_loss_rate = 0.01;
  config.network.burst_loss_trigger_rate = 0.015;
  config.network.burst_loss_length = 5;
  return config;
}

ScenarioConfig reorder_heavy() {
  auto config = base_av(
      "reorder-heavy",
      "Delays selected packets to create receiver-visible reordering and recovery behavior.");
  config.network.reorder_rate = 0.12;
  config.network.reorder_hold_ms = 40;
  config.network.jitter_ms = 18;
  return config;
}

ScenarioConfig bandwidth_constrained() {
  auto config = base_av(
      "bandwidth-constrained",
      "Pushes sender load above the link budget so queue pressure and drops appear.");
  config.network.bandwidth_kbps = 1'200;
  config.network.queue_capacity_packets = 180;
  config.network.queue_pressure_drop_threshold_ms = 240;
  return config;
}

ScenarioConfig low_buffer_failure() {
  auto config = base_av(
      "low-buffer-failure",
      "Small fixed playout delay intentionally fails under jitter to show buffer tradeoffs.");
  config.network.jitter_ms = 35;
  config.jitter_buffer.target_delay_ms = 45;
  config.jitter_buffer.min_delay_ms = 40;
  config.jitter_buffer.max_delay_ms = 120;
  config.jitter_buffer.start_buffer_frames = 1;
  return config;
}

ScenarioConfig adaptive_buffer_comparison() {
  auto config = base_av(
      "adaptive-buffer-comparison",
      "Interview demo preset tuned for comparing fixed and adaptive delay policies under jitter.");
  config.network.base_latency_ms = 58;
  config.network.jitter_ms = 40;
  config.network.packet_loss_rate = 0.01;
  config.jitter_buffer.adaptive = true;
  config.jitter_buffer.target_delay_ms = 95;
  config.jitter_buffer.max_delay_ms = 320;
  return config;
}

ScenarioConfig audio_only() {
  auto config = base_av(
      "audio-only-stable",
      "Audio-only transport run with tight packetization and smaller bandwidth footprint.");
  config.video_stream.enabled = false;
  config.audio_stream.frame_payload_bytes = 180;
  config.network.bandwidth_kbps = 320;
  config.jitter_buffer.target_delay_ms = 60;
  return config;
}

}  // namespace

std::vector<ScenarioConfig> predefined_scenarios() {
  return {
      stable(),
      moderate_jitter(),
      high_jitter(),
      mild_loss(),
      burst_loss(),
      reorder_heavy(),
      bandwidth_constrained(),
      low_buffer_failure(),
      adaptive_buffer_comparison(),
      audio_only(),
  };
}

ScenarioConfig make_scenario(const std::string& name) {
  for (const auto& scenario : predefined_scenarios()) {
    if (scenario.name == name) {
      return scenario;
    }
  }
  throw std::runtime_error("Unknown scenario: " + name);
}

nlohmann::json predefined_scenarios_json() {
  nlohmann::json scenarios = nlohmann::json::array();
  for (const auto& scenario : predefined_scenarios()) {
    scenarios.push_back(nlohmann::json(scenario));
  }
  return scenarios;
}

}  // namespace mediaflow
