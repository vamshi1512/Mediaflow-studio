#include "mediaflow/config.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace mediaflow {

namespace {

MediaType parse_media_type(const std::string& value) {
  if (value == "audio") {
    return MediaType::Audio;
  }
  if (value == "video") {
    return MediaType::Video;
  }
  throw std::runtime_error("Unsupported media type: " + value);
}

void validate_stream_config(
    const StreamConfig& config,
    const std::string& label,
    std::vector<std::string>& errors) {
  if (!config.enabled) {
    return;
  }

  if (config.stream_id.empty()) {
    errors.push_back(label + " stream id must not be empty");
  }
  if (config.frame_interval_ms <= 0) {
    errors.push_back(label + " frame interval must be positive");
  }
  if (config.frame_payload_bytes == 0) {
    errors.push_back(label + " frame payload must be greater than zero");
  }
  if (config.packets_per_frame == 0) {
    errors.push_back(label + " packets per frame must be greater than zero");
  }
}

void validate_probability(
    double value,
    const std::string& label,
    std::vector<std::string>& errors) {
  if (value < 0.0 || value > 1.0) {
    errors.push_back(label + " must be between 0.0 and 1.0");
  }
}

}  // namespace

void to_json(nlohmann::json& json, const StreamConfig& config) {
  json = nlohmann::json{
      {"enabled", config.enabled},
      {"streamId", config.stream_id},
      {"mediaType", to_string(config.media_type)},
      {"frameIntervalMs", config.frame_interval_ms},
      {"framePayloadBytes", config.frame_payload_bytes},
      {"packetsPerFrame", config.packets_per_frame},
  };
}

void from_json(const nlohmann::json& json, StreamConfig& config) {
  config.enabled = json.value("enabled", config.enabled);
  config.stream_id = json.value("streamId", config.stream_id);
  config.media_type = parse_media_type(json.value("mediaType", to_string(config.media_type)));
  config.frame_interval_ms = json.value("frameIntervalMs", config.frame_interval_ms);
  config.frame_payload_bytes = json.value("framePayloadBytes", config.frame_payload_bytes);
  config.packets_per_frame = json.value("packetsPerFrame", config.packets_per_frame);
}

void to_json(nlohmann::json& json, const NetworkConfig& config) {
  json = nlohmann::json{
      {"baseLatencyMs", config.base_latency_ms},
      {"jitterMs", config.jitter_ms},
      {"packetLossRate", config.packet_loss_rate},
      {"burstLossTriggerRate", config.burst_loss_trigger_rate},
      {"burstLossLength", config.burst_loss_length},
      {"reorderRate", config.reorder_rate},
      {"reorderHoldMs", config.reorder_hold_ms},
      {"bandwidthKbps", config.bandwidth_kbps},
      {"queueCapacityPackets", config.queue_capacity_packets},
      {"queuePressureDropThresholdMs", config.queue_pressure_drop_threshold_ms},
  };
}

void from_json(const nlohmann::json& json, NetworkConfig& config) {
  config.base_latency_ms = json.value("baseLatencyMs", config.base_latency_ms);
  config.jitter_ms = json.value("jitterMs", config.jitter_ms);
  config.packet_loss_rate = json.value("packetLossRate", config.packet_loss_rate);
  config.burst_loss_trigger_rate = json.value("burstLossTriggerRate", config.burst_loss_trigger_rate);
  config.burst_loss_length = json.value("burstLossLength", config.burst_loss_length);
  config.reorder_rate = json.value("reorderRate", config.reorder_rate);
  config.reorder_hold_ms = json.value("reorderHoldMs", config.reorder_hold_ms);
  config.bandwidth_kbps = json.value("bandwidthKbps", config.bandwidth_kbps);
  config.queue_capacity_packets = json.value("queueCapacityPackets", config.queue_capacity_packets);
  config.queue_pressure_drop_threshold_ms =
      json.value("queuePressureDropThresholdMs", config.queue_pressure_drop_threshold_ms);
}

void to_json(nlohmann::json& json, const JitterBufferConfig& config) {
  json = nlohmann::json{
      {"targetDelayMs", config.target_delay_ms},
      {"minDelayMs", config.min_delay_ms},
      {"maxDelayMs", config.max_delay_ms},
      {"maxPackets", config.max_packets},
      {"adaptive", config.adaptive},
      {"startBufferFrames", config.start_buffer_frames},
  };
}

void from_json(const nlohmann::json& json, JitterBufferConfig& config) {
  config.target_delay_ms = json.value("targetDelayMs", config.target_delay_ms);
  config.min_delay_ms = json.value("minDelayMs", config.min_delay_ms);
  config.max_delay_ms = json.value("maxDelayMs", config.max_delay_ms);
  config.max_packets = json.value("maxPackets", config.max_packets);
  config.adaptive = json.value("adaptive", config.adaptive);
  config.start_buffer_frames = json.value("startBufferFrames", config.start_buffer_frames);
}

void to_json(nlohmann::json& json, const ScenarioConfig& config) {
  json = nlohmann::json{
      {"name", config.name},
      {"description", config.description},
      {"durationMs", config.duration_ms},
      {"seed", config.seed},
      {"sampleIntervalMs", config.sample_interval_ms},
      {"audioStream", config.audio_stream},
      {"videoStream", config.video_stream},
      {"network", config.network},
      {"jitterBuffer", config.jitter_buffer},
  };
}

void from_json(const nlohmann::json& json, ScenarioConfig& config) {
  config.name = json.value("name", config.name);
  config.description = json.value("description", config.description);
  config.duration_ms = json.value("durationMs", config.duration_ms);
  config.seed = json.value("seed", config.seed);
  config.sample_interval_ms = json.value("sampleIntervalMs", config.sample_interval_ms);
  config.audio_stream = json.value("audioStream", config.audio_stream);
  config.video_stream = json.value("videoStream", config.video_stream);
  config.network = json.value("network", config.network);
  config.jitter_buffer = json.value("jitterBuffer", config.jitter_buffer);
}

ScenarioConfig load_config_from_file(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input.is_open()) {
    throw std::runtime_error("Unable to open config file: " + path.string());
  }

  nlohmann::json json;
  input >> json;
  auto config = json.get<ScenarioConfig>();
  validate_config(config);
  return config;
}

void save_config_to_file(const ScenarioConfig& config, const std::filesystem::path& path) {
  validate_config(config);
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream output(path);
  if (!output.is_open()) {
    throw std::runtime_error("Unable to write config file: " + path.string());
  }
  output << nlohmann::json(config).dump(2) << '\n';
}

std::vector<std::string> validate_config_errors(const ScenarioConfig& config) {
  std::vector<std::string> errors;

  if (config.name.empty()) {
    errors.push_back("scenario name must not be empty");
  }
  if (config.description.empty()) {
    errors.push_back("scenario description must not be empty");
  }
  if (config.duration_ms <= 0) {
    errors.push_back("duration must be positive");
  }
  if (config.sample_interval_ms <= 0) {
    errors.push_back("sample interval must be positive");
  }
  if (!config.audio_stream.enabled && !config.video_stream.enabled) {
    errors.push_back("at least one stream must be enabled");
  }

  validate_stream_config(config.audio_stream, "audio", errors);
  validate_stream_config(config.video_stream, "video", errors);

  if (config.audio_stream.enabled && config.audio_stream.media_type != MediaType::Audio) {
    errors.push_back("audio stream must use mediaType=audio");
  }
  if (config.video_stream.enabled && config.video_stream.media_type != MediaType::Video) {
    errors.push_back("video stream must use mediaType=video");
  }
  if (config.audio_stream.enabled && config.video_stream.enabled &&
      config.audio_stream.stream_id == config.video_stream.stream_id) {
    errors.push_back("audio and video streams must use distinct stream ids");
  }

  if (config.network.base_latency_ms < 0) {
    errors.push_back("base latency must be non-negative");
  }
  if (config.network.jitter_ms < 0) {
    errors.push_back("jitter must be non-negative");
  }
  validate_probability(config.network.packet_loss_rate, "packet loss rate", errors);
  validate_probability(config.network.burst_loss_trigger_rate, "burst loss trigger rate", errors);
  validate_probability(config.network.reorder_rate, "reorder rate", errors);
  if (config.network.burst_loss_length < 0) {
    errors.push_back("burst loss length must be non-negative");
  }
  if (config.network.reorder_hold_ms < 0) {
    errors.push_back("reorder hold must be non-negative");
  }
  if (config.network.bandwidth_kbps <= 0) {
    errors.push_back("bandwidth cap must be positive");
  }
  if (config.network.queue_capacity_packets <= 0) {
    errors.push_back("queue capacity must be positive");
  }
  if (config.network.queue_pressure_drop_threshold_ms < 0) {
    errors.push_back("queue pressure drop threshold must be non-negative");
  }

  if (config.jitter_buffer.target_delay_ms <= 0) {
    errors.push_back("jitter buffer target delay must be positive");
  }
  if (config.jitter_buffer.min_delay_ms <= 0) {
    errors.push_back("jitter buffer minimum delay must be positive");
  }
  if (config.jitter_buffer.max_delay_ms < config.jitter_buffer.min_delay_ms) {
    errors.push_back("jitter buffer max delay must be >= min delay");
  }
  if (config.jitter_buffer.target_delay_ms < config.jitter_buffer.min_delay_ms ||
      config.jitter_buffer.target_delay_ms > config.jitter_buffer.max_delay_ms) {
    errors.push_back("jitter buffer target delay must be between min and max delay");
  }
  if (config.jitter_buffer.max_packets <= 0) {
    errors.push_back("jitter buffer max packets must be positive");
  }
  if (config.jitter_buffer.start_buffer_frames <= 0) {
    errors.push_back("jitter buffer start buffer frames must be positive");
  }

  return errors;
}

void validate_config(const ScenarioConfig& config) {
  const auto errors = validate_config_errors(config);
  if (errors.empty()) {
    return;
  }

  std::ostringstream message;
  message << "Invalid scenario config:";
  for (const auto& error : errors) {
    message << "\n- " << error;
  }
  throw std::runtime_error(message.str());
}

}  // namespace mediaflow
